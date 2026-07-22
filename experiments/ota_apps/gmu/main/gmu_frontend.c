#include "gmufrontend.h"
#include "core.h"
#include "audio.h"
#include "fileplayer.h"
#include "pbstatus.h"
#include "trackinfo.h"
#include "mia_host_abi.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define GMU_MAX_ENTRIES 48
#define GMU_VISIBLE_LINES 8
#define GMU_HEADER_HEIGHT 18
#define GMU_DISPLAY_HEIGHT 34
#define GMU_TEXTAREA_Y 52
#define GMU_LINE_HEIGHT 18
#define GMU_FOOTER_Y (GMU_TEXTAREA_Y + GMU_VISIBLE_LINES * GMU_LINE_HEIGHT)

typedef enum {
    GMU_VIEW_FILE_BROWSER = 0,
    GMU_VIEW_PLAYLIST,
    GMU_VIEW_TRACK_INFO,
} GmuView;

typedef struct {
    char name[64];
    uint8_t is_dir;
    uint32_t size;
} GmuEntry;

static GmuEntry entries[GMU_MAX_ENTRIES];
static uint32_t entry_count;
static uint32_t file_selection;
static uint32_t file_offset;
static char current_path[128] = "/";

static uint32_t playlist_selection;
static uint32_t playlist_offset;
static uint32_t trackinfo_offset;
static GmuView view = GMU_VIEW_FILE_BROWSER;
static uint8_t spectrum_mode = 1;
static uint8_t ui_chinese;
static char notice[80] = "GMU 0.10.1";
static uint32_t notice_until;
static uint8_t dirty;
static uint32_t last_draw_ms;
static uint8_t launch_autoplay;
static uint8_t launch_autoplay_started;

static const char *ui_text(const char *english, const char *chinese) {
    return ui_chinese ? chinese : english;
}

static int text_equal_ci(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

static int is_audio_file(const char *name) {
    const char *dot = strrchr(name, '.');
    return dot != NULL && (text_equal_ci(dot, ".mp3") || text_equal_ci(dot, ".mp2") ||
                           text_equal_ci(dot, ".ogg") || text_equal_ci(dot, ".oga") ||
                           text_equal_ci(dot, ".flac"));
}

static void set_notice(const char *text) {
    snprintf(notice, sizeof(notice), "%s", text ? text : "");
    notice_until = mia_host_millis() + 3000;
    dirty = 1;
}

static void set_noticef(const char *format, const char *value) {
    snprintf(notice, sizeof(notice), format, value ? value : "");
    notice_until = mia_host_millis() + 3000;
    dirty = 1;
}

static void build_path(char *out, size_t size, const char *base, const char *name) {
    if (strcmp(base, "/") == 0)
        snprintf(out, size, "/sd/%s", name);
    else
        snprintf(out, size, "/sd%s/%s", base, name);
}

static void scan(void) {
    MiaHostDirEntry raw[GMU_MAX_ENTRIES];
    int32_t result = mia_host_sd_list_dir(current_path, raw, GMU_MAX_ENTRIES);

    entry_count = 0;
    file_selection = 0;
    file_offset = 0;
    if (result <= 0) {
        set_notice(ui_text("EMPTY DIRECTORY", "目录为空"));
        return;
    }

    for (int32_t i = 0; i < result && entry_count < GMU_MAX_ENTRIES; ++i) {
        if (!raw[i].is_dir && !is_audio_file(raw[i].name)) continue;
        snprintf(entries[entry_count].name, sizeof(entries[entry_count].name), "%s", raw[i].name);
        entries[entry_count].is_dir = raw[i].is_dir;
        entries[entry_count].size = raw[i].size;
        ++entry_count;
    }
    snprintf(notice, sizeof(notice), ui_text("%u ITEMS", "%u 项"), (unsigned)entry_count);
    notice_until = 0;
}

static void draw_text_line(int row, const char *text, uint8_t fg, uint8_t bg) {
    const int32_t y = GMU_TEXTAREA_Y + row * GMU_LINE_HEIGHT;
    mia_host_fill_rect(0, y, mia_host_screen_width(), GMU_LINE_HEIGHT, bg);
    mia_host_draw_text(4, mia_host_text_y_centered(y, GMU_LINE_HEIGHT), text, fg, bg);
}

static void format_time(int seconds, char *out, size_t size) {
    if (seconds < 0) {
        snprintf(out, size, "--:--");
        return;
    }
    snprintf(out, size, "%02d:%02d", seconds / 60, seconds % 60);
}

static void draw_player_display(void) {
    TrackInfo *ti = gmu_core_get_current_trackinfo_ref();
    char title[96];
    char elapsed[12];
    char total[12];
    char time_text[28];
    char line[128];
    int length = 0;
    int elapsed_seconds = (int)(file_player_playback_get_time() / 1000);
    PB_Status status = (PB_Status)gmu_core_get_status();

    snprintf(title, sizeof(title), "%s", ui_text("GMU MUSIC PLAYER", "GMU 音乐播放器"));

    if (trackinfo_acquire_lock(ti)) {
        if (ti->title[0] != '\0')
            snprintf(title, sizeof(title), "%s", ti->title);
        length = (int)ti->length;
        trackinfo_release_lock(ti);
    }

    format_time(elapsed_seconds, elapsed, sizeof(elapsed));
    format_time(length, total, sizeof(total));
    snprintf(time_text, sizeof(time_text), "%s / %s", elapsed, total);
    snprintf(line, sizeof(line), "%s %s", status == PLAYING ? ">" :
             status == PAUSED ? "||" : "[]", title);
    mia_host_fill_rect(0, 0, mia_host_screen_width(), GMU_DISPLAY_HEIGHT, MIA_HOST_BLACK);
    mia_host_draw_text(4, 3, line, MIA_HOST_YELLOW, MIA_HOST_BLACK);

    int32_t progress_width = mia_host_screen_width() - 112;
    int32_t filled = length > 0 ? (progress_width * elapsed_seconds) / length : 0;
    if (filled < 0) filled = 0;
    if (filled > progress_width) filled = progress_width;
    mia_host_fill_rect(4, 20, progress_width, 4, MIA_HOST_DARK_BLUE);
    if (filled > 0) mia_host_fill_rect(4, 20, filled, 4, MIA_HOST_CYAN);
    mia_host_draw_text(progress_width + 12, 16, time_text, MIA_HOST_WHITE, MIA_HOST_BLACK);
}

static void draw_header(const char *text) {
    mia_host_fill_rect(0, GMU_DISPLAY_HEIGHT, mia_host_screen_width(), GMU_HEADER_HEIGHT,
                       MIA_HOST_YELLOW);
    mia_host_draw_text(4, mia_host_text_y_centered(GMU_DISPLAY_HEIGHT, GMU_HEADER_HEIGHT),
                       text, MIA_HOST_BLACK, MIA_HOST_YELLOW);
}

static void draw_file_browser(void) {
    char header[64];
    char path[64];
    snprintf(path, sizeof(path), "%s", current_path);
    if (strlen(path) > 30) {
        memmove(path, path + strlen(path) - 30, 31);
        path[0] = '.';
        path[1] = '.';
        path[2] = '.';
    }
    snprintf(header, sizeof(header), "%s (%s)", ui_text("File browser", "文件浏览器"), path);
    draw_header(header);

    uint32_t end = entry_count < file_offset + GMU_VISIBLE_LINES ?
                   entry_count : file_offset + GMU_VISIBLE_LINES;
    for (uint32_t i = file_offset; i < end; ++i) {
        char line[80];
        const uint8_t selected = i == file_selection;
        if (entries[i].is_dir)
            snprintf(line, sizeof(line), "[DIR] %s/", entries[i].name);
        else
            snprintf(line, sizeof(line), "%5luK %s",
                     (unsigned long)(entries[i].size / 1024), entries[i].name);
        draw_text_line((int)(i - file_offset), line,
                       selected ? MIA_HOST_BLACK : MIA_HOST_WHITE,
                       selected ? MIA_HOST_CYAN : MIA_HOST_BLACK);
    }
    for (uint32_t i = end - file_offset; i < GMU_VISIBLE_LINES; ++i)
        draw_text_line((int)i, "", MIA_HOST_WHITE, MIA_HOST_BLACK);
}

static const char *play_mode_name(PlayMode mode) {
    switch (mode) {
        case PM_REPEAT_ALL: return ui_text("REPEAT ALL", "全部重复");
        case PM_REPEAT_1: return ui_text("REPEAT TRACK", "单曲重复");
        case PM_RANDOM: return ui_text("RANDOM", "随机");
        case PM_RANDOM_REPEAT: return ui_text("RANDOM+REPEAT", "随机重复");
        default: return ui_text("CONTINUE", "连续播放");
    }
}

static void draw_playlist(void) {
    char header[80];
    const size_t length = gmu_core_playlist_get_length();
    snprintf(header, sizeof(header), "%s (%u, %s)", ui_text("Playlist", "播放列表"),
             (unsigned)length,
             play_mode_name(gmu_core_playlist_get_play_mode()));
    draw_header(header);

    uint32_t end = length < playlist_offset + GMU_VISIBLE_LINES ?
                   (uint32_t)length : playlist_offset + GMU_VISIBLE_LINES;
    gmu_core_playlist_acquire_lock();
    for (uint32_t i = playlist_offset; i < end; ++i) {
        Entry *entry = gmu_core_playlist_get_entry(i);
        char name[PL_ENTRY_NAME_MAX_LENGTH];
        char line[80];
        uint8_t selected = i == playlist_selection;
        uint8_t current = entry == gmu_core_playlist_get_current();
        uint8_t played = entry != NULL && gmu_core_playlist_get_played(entry);
        if (entry != NULL)
            snprintf(name, sizeof(name), "%s", gmu_core_playlist_get_entry_name(entry));
        else
            snprintf(name, sizeof(name), "%s", ui_text("(missing)", "(缺失)"));
        snprintf(line, sizeof(line), "%c%3u %s", current ? '*' : played ? 'o' : ' ',
                 (unsigned)(i + 1), name);
        draw_text_line((int)(i - playlist_offset), line,
                       selected ? MIA_HOST_BLACK : MIA_HOST_WHITE,
                       selected ? MIA_HOST_CYAN : MIA_HOST_BLACK);
    }
    gmu_core_playlist_release_lock();
    for (uint32_t i = end - playlist_offset; i < GMU_VISIBLE_LINES; ++i)
        draw_text_line((int)i, "", MIA_HOST_WHITE, MIA_HOST_BLACK);
}

static void draw_track_info_text(void) {
    TrackInfo *ti = gmu_core_get_current_trackinfo_ref();
    char lines[12][80];
    int count = 0;
    int length = 0;
    int samplerate = 0;
    int channels = 0;
    long bitrate = 0;

    if (trackinfo_acquire_lock(ti)) {
        snprintf(lines[count++], sizeof(lines[0]), "%s: %s", ui_text("Title", "标题"), ti->title[0] ? ti->title : "-");
        snprintf(lines[count++], sizeof(lines[0]), "%s: %s", ui_text("Artist", "艺术家"), ti->artist[0] ? ti->artist : "-");
        snprintf(lines[count++], sizeof(lines[0]), "%s: %s", ui_text("Album", "专辑"), ti->album[0] ? ti->album : "-");
        snprintf(lines[count++], sizeof(lines[0]), "%s: %s   %s: %s",
                 ui_text("Track", "曲目"),
                 ti->tracknr[0] ? ti->tracknr : "-", ui_text("Date", "日期"),
                 ti->date[0] ? ti->date : "-");
        length = (int)ti->length;
        samplerate = ti->samplerate;
        channels = ti->channels;
        bitrate = ti->bitrate;
        snprintf(lines[count++], sizeof(lines[0]), "%s: %02d:%02d  %d Hz",
                 ui_text("Length", "时长"),
                 length / 60, length % 60, samplerate);
        snprintf(lines[count++], sizeof(lines[0]), "%s: %d channel  %ld kbit/s%s",
                 ui_text("Audio", "音频"),
                 channels, bitrate / 1000, ti->vbr ? " VBR" : "");
        snprintf(lines[count++], sizeof(lines[0]), "%s: %s", ui_text("Type", "类型"), ti->file_type[0] ? ti->file_type : "-");
        snprintf(lines[count++], sizeof(lines[0]), "%s: %s", ui_text("File", "文件"), ti->file_name[0] ? ti->file_name : "-");
        trackinfo_release_lock(ti);
    }

    draw_header(ui_text("Track info", "曲目信息"));
    for (uint32_t i = 0; i < GMU_VISIBLE_LINES; ++i) {
        uint32_t index = trackinfo_offset + i;
        draw_text_line((int)i, index < (uint32_t)count ? lines[index] : "",
                       MIA_HOST_WHITE, MIA_HOST_BLACK);
    }
}

static void draw_spectrum(void) {
    TrackInfo *ti = gmu_core_get_current_trackinfo_ref();
    char title[96];
    int16_t amplitudes[8] = {};

    snprintf(title, sizeof(title), "%s", ui_text("No track", "无歌曲"));

    if (trackinfo_acquire_lock(ti)) {
        if (ti->title[0] != '\0') snprintf(title, sizeof(title), "%s", ti->title);
        trackinfo_release_lock(ti);
    }
    if (audio_spectrum_read_lock()) {
        memcpy(amplitudes, audio_spectrum_get_current_amplitudes(), sizeof(amplitudes));
        audio_spectrum_read_unlock();
    }

    draw_header(ui_text("Spectrum analyzer", "频谱分析"));
    mia_host_fill_rect(0, GMU_TEXTAREA_Y, mia_host_screen_width(),
                       GMU_FOOTER_Y - GMU_TEXTAREA_Y, MIA_HOST_BLACK);
    mia_host_draw_text(4, mia_host_text_y_centered(GMU_TEXTAREA_Y, 18), title,
                       MIA_HOST_WHITE, MIA_HOST_BLACK);

    const int32_t baseline = GMU_FOOTER_Y - 8;
    const int32_t bar_width = 30;
    const int32_t bar_gap = 8;
    const int32_t max_height = baseline - GMU_TEXTAREA_Y - 24;
    for (int i = 0; i < 8; ++i) {
        int32_t height = 2 + amplitudes[i] / 400;
        if (height > max_height) height = max_height;

        const int32_t x = 4 + i * (bar_width + bar_gap);
        mia_host_fill_rect(x, baseline - height, bar_width, height, MIA_HOST_BLUE);
    }
}

static void draw_track_info(void) {
    if (spectrum_mode) draw_spectrum();
    else draw_track_info_text();
}

static void draw_footer(void) {
    const char *footer;
    const int32_t text_height = mia_host_text_height();
    const int32_t first_y = GMU_FOOTER_Y + 3;
    const int32_t second_y = first_y + text_height + 4;
    switch (view) {
        case GMU_VIEW_FILE_BROWSER:
            footer = ui_text("UP/DN move  A play/open  Y add  M views", "上/下移动 A播放/打开 Y添加 M视图");
            break;
        case GMU_VIEW_PLAYLIST:
            footer = ui_text("UP/DN move  A play  Y mode  X clear  M views", "上/下移动 A播放 Y模式 X清空 M视图");
            break;
        default:
            footer = ui_text("UP/DN scroll  Y text  M views  START pause", "上/下滚动 Y文字 M视图 START暂停");
            break;
    }
    mia_host_fill_rect(0, GMU_FOOTER_Y, mia_host_screen_width(),
                       mia_host_screen_height() - GMU_FOOTER_Y, MIA_HOST_BLACK);
    mia_host_fill_rect(0, GMU_FOOTER_Y, mia_host_screen_width(), 1, MIA_HOST_DARK_BLUE);
    mia_host_draw_text(4, first_y, footer, MIA_HOST_GRAY, MIA_HOST_BLACK);
    if (notice_until == 0 || (int32_t)(notice_until - mia_host_millis()) > 0)
        mia_host_draw_text(4, second_y, notice, MIA_HOST_GREEN, MIA_HOST_BLACK);
}

static void draw(void) {
    mia_host_clear(MIA_HOST_BLACK);
    draw_player_display();
    switch (view) {
        case GMU_VIEW_FILE_BROWSER: draw_file_browser(); break;
        case GMU_VIEW_PLAYLIST: draw_playlist(); break;
        case GMU_VIEW_TRACK_INFO: draw_track_info(); break;
    }
    draw_footer();
    mia_host_present();
    last_draw_ms = mia_host_millis();
    dirty = 0;
}

static void open_selected_file(void) {
    if (entry_count == 0 || file_selection >= entry_count) return;
    if (entries[file_selection].is_dir) {
        if (strcmp(current_path, "/") == 0)
            snprintf(current_path, sizeof(current_path), "/%s", entries[file_selection].name);
        else {
            char next_path[sizeof(current_path)];
            snprintf(next_path, sizeof(next_path), "%.63s/%.63s",
                     current_path, entries[file_selection].name);
            snprintf(current_path, sizeof(current_path), "%s", next_path);
        }
        scan();
        return;
    }

    char path[256];
    build_path(path, sizeof(path), current_path, entries[file_selection].name);
    gmu_core_playlist_set_current(NULL);
    if (gmu_core_play_file(path))
        set_noticef(ui_text("PLAYING: %s", "播放: %s"), entries[file_selection].name);
    else
        set_notice(ui_text("PLAY FAILED", "播放失败"));
}

static void add_selected_file(void) {
    if (entry_count == 0 || file_selection >= entry_count) return;
    if (entries[file_selection].is_dir) {
        char path[256];
        build_path(path, sizeof(path), current_path, entries[file_selection].name);
        if (gmu_core_playlist_add_dir(path)) set_notice(ui_text("ADDING DIRECTORY...", "正在添加目录..."));
        else set_notice(ui_text("DIRECTORY BUSY", "目录正在处理"));
        return;
    }

    char path[256];
    build_path(path, sizeof(path), current_path, entries[file_selection].name);
    if (gmu_core_playlist_add_file(path)) set_notice(ui_text("ITEM ADDED TO PLAYLIST", "已加入播放列表"));
    else set_notice(ui_text("ADD FAILED", "添加失败"));
}

static void go_parent(void) {
    if (strcmp(current_path, "/") == 0) return;
    char *slash = strrchr(current_path, '/');
    if (slash == current_path) current_path[1] = '\0';
    else if (slash != NULL) *slash = '\0';
    scan();
}

static void move_selection(int direction) {
    uint32_t *selection = view == GMU_VIEW_FILE_BROWSER ? &file_selection : &playlist_selection;
    uint32_t *offset = view == GMU_VIEW_FILE_BROWSER ? &file_offset : &playlist_offset;
    uint32_t length = view == GMU_VIEW_FILE_BROWSER ? entry_count :
                      (uint32_t)gmu_core_playlist_get_length();
    if (length == 0) return;
    if (direction > 0) {
        if (*selection + 1 < length) ++*selection;
        else *selection = 0;
    } else if (*selection > 0) {
        --*selection;
    } else {
        *selection = length - 1;
    }
    if (*selection < *offset) *offset = *selection;
    if (*selection >= *offset + GMU_VISIBLE_LINES)
        *offset = *selection - GMU_VISIBLE_LINES + 1;
}

static void move_trackinfo(int direction) {
    if (direction > 0 && trackinfo_offset < 1) ++trackinfo_offset;
    if (direction < 0 && trackinfo_offset > 0) --trackinfo_offset;
}

static const char *frontend_name(void) { return "Gmu MiaOS original-style frontend"; }

static int frontend_init(void) {
    ui_chinese = mia_host_language();
    scan();
    draw();
    return 1;
}

static void frontend_shutdown(void) {}

static void frontend_iteration(void) {
    mia_host_buttons_poll();
    if (mia_host_button_down(MIA_HOST_BUTTON_SELECT) &&
        mia_host_button_down(MIA_HOST_BUTTON_START)) {
        gmu_core_quit();
        return;
    }

    int changed = 0;
    if (launch_autoplay && !launch_autoplay_started &&
        gmu_core_playlist_get_length() > 0) {
        launch_autoplay_started = 1;
        gmu_core_play_pl_item(0);
        set_notice(ui_text("PLAYING LAUNCHED FILE", "播放指定文件"));
        changed = 1;
    }
    if (mia_host_button_pressed(MIA_HOST_BUTTON_M)) {
        view = (GmuView)((view + 1) % 3);
        if (view == GMU_VIEW_TRACK_INFO) spectrum_mode = 1;
        changed = 1;
    }
    if (mia_host_button_pressed(MIA_HOST_BUTTON_UP)) {
        if (view == GMU_VIEW_TRACK_INFO) move_trackinfo(-1);
        else move_selection(-1);
        changed = 1;
    }
    if (mia_host_button_pressed(MIA_HOST_BUTTON_DOWN)) {
        if (view == GMU_VIEW_TRACK_INFO) move_trackinfo(1);
        else move_selection(1);
        changed = 1;
    }
    if (mia_host_button_pressed(MIA_HOST_BUTTON_A)) {
        if (view == GMU_VIEW_FILE_BROWSER) open_selected_file();
        else if (view == GMU_VIEW_PLAYLIST && gmu_core_playlist_get_length() > 0)
            gmu_core_play_pl_item((int)playlist_selection);
        changed = 1;
    }
    if (mia_host_button_pressed(MIA_HOST_BUTTON_B)) {
        if (view == GMU_VIEW_FILE_BROWSER) go_parent();
        else view = GMU_VIEW_FILE_BROWSER;
        changed = 1;
    }
    if (mia_host_button_pressed(MIA_HOST_BUTTON_Y)) {
        if (view == GMU_VIEW_FILE_BROWSER) add_selected_file();
        else if (view == GMU_VIEW_PLAYLIST) gmu_core_playlist_cycle_play_mode();
        else spectrum_mode = !spectrum_mode;
        changed = 1;
    }
    if (mia_host_button_pressed(MIA_HOST_BUTTON_X)) {
        if (view == GMU_VIEW_PLAYLIST) {
            gmu_core_playlist_clear();
            playlist_selection = playlist_offset = 0;
            set_notice(ui_text("PLAYLIST CLEARED", "播放列表已清空"));
        } else {
            gmu_core_stop();
        }
        changed = 1;
    }
    if (mia_host_button_pressed(MIA_HOST_BUTTON_START)) {
        gmu_core_play_pause();
        changed = 1;
    }
    if (mia_host_button_pressed(MIA_HOST_BUTTON_R)) {
        gmu_core_next();
        changed = 1;
    }
    if (mia_host_button_pressed(MIA_HOST_BUTTON_L)) {
        gmu_core_previous();
        changed = 1;
    }
    if (mia_host_button_pressed(MIA_HOST_BUTTON_RIGHT)) {
        int volume = gmu_core_get_volume();
        if (volume < gmu_core_get_volume_max()) gmu_core_set_volume(volume + 1);
        changed = 1;
    }
    if (mia_host_button_pressed(MIA_HOST_BUTTON_LEFT)) {
        int volume = gmu_core_get_volume();
        if (volume > 0) gmu_core_set_volume(volume - 1);
        changed = 1;
    }

    uint32_t now = mia_host_millis();
    if (dirty || changed || now - last_draw_ms >= 500) draw();
    mia_host_delay_ms(50);
}

int frontend_event_callback(GmuEvent event_type, int param) {
    (void)param;
    dirty = 1;
    if (event_type == GMU_TRACK_CHANGE) {
        playlist_selection = (uint32_t)gmu_core_playlist_get_current_position();
        playlist_offset = playlist_selection >= GMU_VISIBLE_LINES ?
                          playlist_selection - GMU_VISIBLE_LINES + 1 : 0;
    }
    return 1;
}

void gmu_frontend_set_autoplay(uint8_t enabled) {
    launch_autoplay = enabled ? 1 : 0;
}

GmuFrontend *gmu_register_frontend(void) {
    static GmuFrontend frontend = {
        .identifier = "gmu_miaos",
        .get_name = frontend_name,
        .frontend_init = frontend_init,
        .frontend_shutdown = frontend_shutdown,
        .mainloop_iteration = frontend_iteration,
        .event_callback = frontend_event_callback,
        .handle = NULL,
    };
    return &frontend;
}
