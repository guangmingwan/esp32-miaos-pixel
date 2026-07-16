#include "music_i18n.h"

#include "mia_host_abi.h"

static const MusicText MUSIC_EN = {
    .title = "Music",
    .now_playing = "Now Playing",
    .browse_status = "A:Open  B:Up  SEL+ST:Exit",
    .browse_controls = "UP/DN Move  A:Open  B:Up",
    .directory_empty = "Directory empty",
    .no_audio_files = "No audio files",
    .playback_controls = "B:Stop   SEL+ST:Exit",
    .opening = "Opening...",
    .open_failed = "Open failed",
    .path_too_long = "Path too long",
    .audio_open_failed = "Open failed",
    .stopped = "Stopped",
    .done = "Done",
    .playback_failed = "Playback failed",
    .unsupported_file = "Unsupported file",
};

static const MusicText MUSIC_ZH = {
    .title = "音乐",
    .now_playing = "正在播放",
    .browse_status = "A:打开 B:上级 SEL+ST:退出",
    .browse_controls = "上/下:移动 A:打开 B:上级",
    .directory_empty = "目录为空",
    .no_audio_files = "没有音频文件",
    .playback_controls = "B:停止 SEL+ST:退出",
    .opening = "正在打开...",
    .open_failed = "打开失败",
    .path_too_long = "路径过长",
    .audio_open_failed = "打开失败",
    .stopped = "已停止",
    .done = "播放完成",
    .playback_failed = "播放失败",
    .unsupported_file = "不支持的文件",
};

const MusicText *music_text(void) {
  return mia_host_language() == 1 ? &MUSIC_ZH : &MUSIC_EN;
}
