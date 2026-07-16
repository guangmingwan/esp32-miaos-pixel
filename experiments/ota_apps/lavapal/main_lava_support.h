#ifndef MAIN_LAVA_SUPPORT_H
#define MAIN_LAVA_SUPPORT_H

#define FILE char
#define UINT long

#define PAL_POS long
#define LPSPRITE addr
#define LPBITMAPRLE addr
#define LPCBITMAPRLE addr

struct tagMAIN_LAVA_CONFIG
{
   BOOL fIsWIN95;
   BOOL fEnableGLSL;
   addr pszShader;
};
#define CONFIGURATION struct tagMAIN_LAVA_CONFIG

struct tagMAIN_LAVA_FILES
{
   addr fpPAT;
   addr fpFBP;
   addr fpDATA;
   addr fpMAP;
   addr fpGOP;
   addr fpMGO;
   addr fpRNG;
   addr fpSSS;
   addr fpFIRE;
};

struct tagMAIN_LAVA_GLOBALS
{
   struct tagMAIN_LAVA_FILES f;
   BYTE bCurrentSaveSlot;
   BOOL fInMainGame;
   BOOL fNeedToFadeIn;
   WORD wNumPalette;
   BOOL fNightPalette;
};
#define GLOBALVARS struct tagMAIN_LAVA_GLOBALS

#ifndef kDirSouth
#define kDirSouth 0
#define kDirWest 1
#define kDirNorth 2
#define kDirEast 3
#define kDirUnknown 4
#define PALDIRECTION int
#endif

struct tagMAIN_LAVA_INPUTSTATE
{
   PALDIRECTION dir;
   PALDIRECTION prevdir;
   DWORD dwKeyPress;
   DWORD dwKeyOrder[4];
   DWORD dwKeyMaxCount;
};
#define PALINPUTSTATE struct tagMAIN_LAVA_INPUTSTATE

CONFIGURATION gConfig;
GLOBALVARS g_lava_gpGlobals;
#define gpGlobals (&g_lava_gpGlobals)
SDL_Surface g_lava_gpScreen;
#define gpScreen (&g_lava_gpScreen)
PALINPUTSTATE g_InputState;
DWORD g_lava_key_hold;
DWORD g_lava_frame_hold;
addr g_lava_fpFBP;
addr g_lava_fpMGO;
addr g_lava_fpPAT;
addr g_lava_fpDATA;
addr g_lava_fpMAP;
addr g_lava_fpGOP;
addr g_lava_fpRNG;
addr g_lava_fpSSS;
addr g_lava_fpRGM;
addr g_lava_fpBALL;
addr g_lava_fpFIRE;
int g_lava_dbg_trademark_rng_ok;
int g_lava_dbg_splash_fbp_ok;
int g_lava_dbg_splash_mgo_ok;
int g_lava_cur_playing_rng;
int g_lava_pending_load_slot;
int g_lava_loading_saved_game;

void VIDEO_ShakeScreen(WORD wShakeTime, WORD wShakeLevel)
{
   if (wShakeTime == 0)
   {
      VIDEO_UpdateScreen(0);
   }
}

int g_lava_scene_num;
int g_lava_num_battle_field;
int g_lava_scene_map_num;
int g_lava_scene_script_enter;
int g_lava_scene_script_teleport;
WORD g_lava_scene_runtime_enter[300];
WORD g_lava_scene_runtime_teleport[300];
int g_lava_scene_runtime_scripts_inited;
WORD g_lava_scene_hook_enter[300];
WORD g_lava_scene_hook_teleport[300];
int g_lava_scene_enter_pending;
int g_lava_scene_event_first;
int g_lava_scene_event_last;
int g_lava_scene_event_count;
WORD g_lava_autoscript_pc[5332];
WORD g_lava_autoscript_idle[5332];
int g_lava_dbg_map_tiles_drawn;
int g_lava_dbg_event_sprites_drawn;
int g_lava_dbg_player_sprites_drawn;
int g_lava_player_sprite_num[6];
int g_lava_player_walk_frames[6];
int g_lava_party_role[3];
int g_lava_party_count;
int g_lava_follower_role[2];
int g_lava_follower_count;
int g_lava_party_trail_x[5];
int g_lava_party_trail_y[5];
int g_lava_party_trail_dir[5];
int g_lava_party_x;
int g_lava_party_y;
int g_lava_party_direction;
int g_lava_party_frame;
int g_lava_party_step_frame;
int g_lava_party_layer;
int g_lava_view_x;
int g_lava_view_y;
int g_lava_scene_ready;
int g_lava_defer_scene_preview_draw;
long g_lava_logic_frame_num;
long g_lava_render_frame_num;
int g_lava_run_logic_frame;
int g_lava_walk_substep_pending;
int g_lava_walk_substep_dir;
long g_lava_scene_event_offset;
long g_lava_scene_script_offset;
char g_lava_scene_event_raw[5332 * 32];
#define LAVA_DIALOG_EVENT_MAX 256
int g_lava_dialog_event_type[LAVA_DIALOG_EVENT_MAX];
int g_lava_dialog_event_a[LAVA_DIALOG_EVENT_MAX];
int g_lava_dialog_event_b[LAVA_DIALOG_EVENT_MAX];
int g_lava_dialog_event_c[LAVA_DIALOG_EVENT_MAX];
int g_lava_dialog_event_count;
int g_lava_dialog_before_battle;
int g_lava_dialog_silent_visual_replay;
int g_lava_dialog_face_num;
int g_lava_dialog_font_color = -1;
int g_lava_dialog_location;
int g_lava_dialog_icon_x;
int g_lava_dialog_icon_y;
int g_lava_dialog_replay_snapshot_valid;
int g_lava_pending_enter_tail_script;
int g_lava_defer_enter_tail_after_dialog;
int g_lava_suppress_trigger_visual_events;
int g_lava_intro_obj9_probe_active;
int g_lava_dialog_replay_scene_first;
int g_lava_dialog_replay_scene_last;
int g_lava_dialog_replay_party_x;
int g_lava_dialog_replay_party_y;
int g_lava_dialog_replay_party_direction;
int g_lava_dialog_replay_party_frame;
int g_lava_dialog_replay_party_count;
int g_lava_dialog_replay_party_layer;
int g_lava_dialog_replay_view_x;
int g_lava_dialog_replay_view_y;
int g_lava_dialog_replay_player_sprite_num[6];
int g_lava_dialog_replay_party_role[3];
#define LAVA_DIALOG_REPLAY_MAX 64
int g_lava_dialog_replay_apc[LAVA_DIALOG_REPLAY_MAX];
int g_lava_dialog_replay_aidle[LAVA_DIALOG_REPLAY_MAX];
char g_lava_dialog_replay_event_raw[LAVA_DIALOG_REPLAY_MAX * 32];
WORD g_lava_inventory_item[256];
WORD g_lava_inventory_amount[256];
int g_lava_autotest_search;
int g_lava_autotest_walk;
int g_lava_autotest_exits;
int g_lava_autotest_scene5;
int g_lava_autotest_scene6;
int g_lava_autotest_scene13;
int g_lava_autotest_scene9;
int g_lava_autotest_hooks;
int g_lava_autotest_door;
int g_lava_autotest_hall;
int g_lava_autotest_kitchen;
int g_lava_autotest_load;
int g_lava_autotest_menu_load;
int g_lava_autotest_status;
int g_lava_fast_script_probe;
int g_lava_autotest_input;
int g_lava_autotest_intro;
int g_lava_autotest_xianling;
int g_lava_autotest_obj204;
int g_lava_autotest_bsilence;
int g_lava_autotest_bsleep;
int g_lava_autotest_battle;
int g_lava_autotest_fengshen;
int g_lava_autotest_xueyao;
int g_lava_autotest_op48;
int g_lava_autotest_battle_magic_seen;
int g_lava_autotest_battle_magic_object_id;
int g_lava_shutdown_requested;
int g_lava_autotest_input_step;
int g_lava_cmd_i;
int g_lava_cmd_j;
int g_lava_autotouch_last_object;
int g_lava_autotouch_last_trigger;
int g_lava_touch_lock_object;
long g_lava_touch_lock_trigger;
long g_lava_touch_cooldown_until_frame;
#define LAVA_AUTOTOUCH_SEEN_MAX 64
#define LAVA_MSG_OFFSET_MAX 12881
int g_lava_autotouch_seen_object[LAVA_AUTOTOUCH_SEEN_MAX];
long g_lava_autotouch_seen_trigger[LAVA_AUTOTOUCH_SEEN_MAX];
int g_lava_autotouch_seen_count;
int g_lava_music_enabled;
int g_lava_sound_enabled;
int g_lava_num_music;
int g_lava_num_battle_music;
int g_lava_last_sound;
int g_lava_last_music_loop;
int g_lava_last_music_fade_time;
int g_lava_menu_selection;
long g_lava_cash;
int g_lava_magic_menu_party;
int g_lava_magic_menu_index;
int g_lava_item_use_index;
int g_lava_equip_index;
int g_lava_script_success;
int g_lava_role_script;
int g_lava_item_was_used;
int g_lava_last_event_object;

#define LAVA_AUTO_UNSUPPORTED_LOG_MAX 64
WORD g_lava_auto_unsupported_logged[LAVA_AUTO_UNSUPPORTED_LOG_MAX];
int g_lava_auto_unsupported_logged_count;

#define LAVA_ROLE_TEMP_STAT_MAX 128
int g_lava_role_temp_stat_percent[LAVA_ROLE_TEMP_STAT_MAX][6];

#define LAVA_MAX_SPRITES_TO_DRAW 2048
#define LAVA_MGO_FRAME_METRIC_CACHE_SLOTS 256
struct lava_sprite_to_draw {
   int source_kind;
   int source_num;
   int frame_num;
   LPCBITMAPRLE raw_frame;
   int x;
   int y;
   int layer;
};
struct lava_sprite_to_draw g_lava_sprites_to_draw[LAVA_MAX_SPRITES_TO_DRAW];
int g_lava_sprites_to_draw_count;

#define LAVA_SPRITE_SOURCE_RAW_TILE 1
#define LAVA_SPRITE_SOURCE_MGO 2

#define FPS 10
#define FRAME_TIME (1000 / FPS)
#define RENDER_FRAME_TIME 55
#define MAINMENU_BACKGROUND_FBPNUM (gConfig.fIsWIN95 ? 2 : 60)
#define MAINMENU_LABEL_NEWGAME 7
#define MAINMENU_LABEL_LOADGAME 8
#define MENUITEM_COLOR 0x4F
#define MENUITEM_COLOR_INACTIVE 0x18
#define MENUITEM_COLOR_SELECTED_INACTIVE 0x1C
#define MENUITEM_COLOR_SELECTED PAL_LavaMenuSelectedColor()
#define MENUITEM_COLOR_SELECTED_FIRST 0xF9
#define MENUITEM_COLOR_SELECTED_TOTALNUM 6
#define MENU_TEXT_X (125 * SCREEN_W / 320)
#define MENU_TEXT_Y0 (95 * SCREEN_H / 200)
#define MENU_TEXT_Y1 (112 * SCREEN_H / 200)
#define LAVA_WORD_DAT_WIDTH 10
#define LOAD_SLOT_BOX_X 195
#define LOAD_SLOT_LABEL_X 210
#define LOAD_SLOT_TIMES_X 295
#define LOAD_SLOT_PANEL_X 188
#define LOAD_SLOT_PANEL_Y 0
#define LOAD_SLOT_PANEL_W 122
#define LOAD_SLOT_PANEL_H 194
#define LOAD_SLOT_BOX_Y(slot) (7 + 38 * ((slot) - 1))
#define LOAD_SLOT_LABEL_Y(slot) (17 + 38 * ((slot) - 1))
#define LOAD_SLOT_TIMES_Y(slot) (21 + 38 * ((slot) - 1))
#define LAVA_TEXT_MODE_TRANSPARENT 0x86
#define LAVA_FONT_GB2312_FIRST 0xA1
#define LAVA_FONT_GB2312_SIDE 94
#define LAVA_FONT_GLYPH_BYTES_MAX 32
#define LAVA_FONT_ASCII_BYTES_MAX 1536
#define LAVA_FONT_MAP_MAX 9000
/* TextOut type bits: bit7=1 large font, bit7=0 small font. The transparent
   large-font mode is 0x86; clearing bit7 gives the small-font variant. */
#define LAVA_TEXT_MODE_TRANSPARENT_SMALL (LAVA_TEXT_MODE_TRANSPARENT & 0x7F)
#define LAVA_SKIP_INTRO_DIALOG 0
#define LAVA_DIALOG_EVENT_START 1
#define LAVA_DIALOG_EVENT_TEXT 2
#define LAVA_DIALOG_EVENT_RESTORE 3
#define LAVA_DIALOG_EVENT_CENTER 4
#define LAVA_DIALOG_EVENT_PARTY_POS 5
#define LAVA_DIALOG_EVENT_PARTY_POSE 6
#define LAVA_DIALOG_EVENT_PARTY_SPRITE 7
#define LAVA_DIALOG_EVENT_EVENT_POSE 8
#define LAVA_DIALOG_EVENT_EVENT_GESTURE 9
#define LAVA_DIALOG_EVENT_EVENT_STEP 10
#define LAVA_DIALOG_EVENT_EVENT_STATE 11
#define LAVA_DIALOG_EVENT_PARTY_WALK 12
#define LAVA_DIALOG_EVENT_PARTY_RIDE 13
#define LAVA_DIALOG_EVENT_WAIT 14
#define LAVA_DIALOG_EVENT_EVENT_POS 15
#define LAVA_DIALOG_UPPER 0
#define LAVA_DIALOG_LOWER 2
#define LAVA_DIALOG_CENTER 1

int PAL_StartBattle(WORD wEnemyTeam, BOOL fIsBoss);

SDL_Color g_lava_palette_cache[256];
char g_lava_fbp_buf[64000];
char g_lava_sprite_buf[65536];
#define LAVA_MGO_SPRITE_CACHE_SLOTS 32
char g_lava_mgo_sprite_cache[LAVA_MGO_SPRITE_CACHE_SLOTS][65536];
int g_lava_mgo_sprite_cache_num[LAVA_MGO_SPRITE_CACHE_SLOTS];
DWORD g_lava_mgo_sprite_cache_stamp[LAVA_MGO_SPRITE_CACHE_SLOTS];
DWORD g_lava_mgo_sprite_cache_clock;
char g_lava_ui_sprite_buf[65536];
int g_lava_ui_sprite_ready;
int g_lava_chase_range = 1;
int g_lava_chase_speed_change_cycles = 0;
DWORD g_lava_mkf_read_offset;
DWORD g_lava_mkf_read_next_offset;
DWORD g_lava_mkf_read_len;
UINT g_lava_mkf_read_chunk_count;
int g_lava_mgo_metric_sprite[LAVA_MGO_FRAME_METRIC_CACHE_SLOTS];
int g_lava_mgo_metric_frame[LAVA_MGO_FRAME_METRIC_CACHE_SLOTS];
int g_lava_mgo_metric_width[LAVA_MGO_FRAME_METRIC_CACHE_SLOTS];
int g_lava_mgo_metric_height[LAVA_MGO_FRAME_METRIC_CACHE_SLOTS];
int g_lava_mgo_metric_used;
int g_lava_mgo_metric_next;
int g_lava_dbg_mgo_decompresses;
int g_lava_autotest_exit_pairs[] = {
   77, 5280, 96, 5804, 97, 6047, 98, 4655, 99, 6031,
   100, 6039, 101, 6447, 97, 6051, 97, 4436,
   0, 0
};
int g_lava_autotest_scene6_object_pairs[] = {
   150, 2, 149, 2,
   0, 0
};
int g_lava_autotest_scene5_object_pairs[] = {
   137, 2, 140, 2, 142, 2, 125, 2, 126, 2,
   0, 0
};
int g_lava_autotest_scene9_object_pairs[] = {
   187, 2,
   0, 0
};
int g_lava_autotest_walk_scene3_pairs[] = {
   45, 6067, 46, 4661, 18, 4645, 47, 33, 48, 60,
   50, 4465, 51, 4473, 52, 4633, 53, 4639, 54, 4647,
   57, 4701, 19, 4653, 13, 4637, 95, 4459, 84, 5639,
   0, 0
};
int g_lava_autotest_walk_scene1_pairs[] = {
   1, 4669, 2, 69, 3, 42, 13, 4635, 13, 4637,
   18, 4643, 18, 4645, 19, 4651, 19, 4653,
   0, 0
};
int g_lava_autotest_walk_scene2_pairs[] = {
   50, 4463, 33, 4467, 50, 4465, 51, 4473, 18, 4645,
   0, 0
};
int g_lava_autotest_walk_scene6_pairs[] = {
   97, 4436, 97, 6051,
   0, 0
};
int g_lava_autotest_walk_final_pairs[] = {
   57, 4701, 96, 5804, 97, 6047, 98, 4655, 99, 6031,
   100, 6039, 101, 6447,
   0, 0
};
int g_lava_autotest_search_pairs[] = {
   20, 5002, 25, 4857, 26, 4872, 27, 4879, 63, 5054,
   25, 4887, 25, 4867, 25, 4960, 20, 4981, 63, 5061,
   0, 0
};
int g_lava_autotest_search_objects[] = {
   5, 6, 7, 32, 0
};
int g_lava_autotest_search_prime_params[] = {
   2, 1,
   0
};
int g_lava_autotest_search_object_params[] = {
   200,
   0
};
int g_lava_autotest_hooks_scene4_pairs[] = {
   84, 6773, 96, 5804,
   0, 0
};
int g_lava_autotest_hooks_scene1_pairs[] = {
   10, 6195, 1, 4669,
   0, 0
};
int g_lava_autotest_hooks_scene7_pairs[] = {
   172, 6718,
   0, 0
};
int g_lava_autotest_hooks_scene7_reenter_pairs[] = {
   172, 6776, 172, 39677,
   0, 0
};
int g_lava_autotest_hooks_plan[] = {
   4, 1248, 1344, 0, 1088, 784, 1, 1,
   1, 1344, 288, 2, 1184, 176, 0, 2,
   7, 1728, 944, 0, 1568, 832, 0, 3,
   7, 1728, 944, 0, 1568, 832, 1, 4,
   0, 0, 0, 0, 0, 0, 0, 0
};
int g_lava_autotest_walk_scene_plan[] = {
   4, 5,
   3, 6,
   1, 7,
   2, 8,
   6, 9,
   0, 0
};
int g_lava_autotest_search_sequence_plan[] = {
   1, 1,
   2, 2,
   5, 10,
   6, 0,
   0, 0
};
int g_lava_autotest_walk_touch_params[] = {
   32, 200, 1,
   0
};
int g_lava_autotest_scene_touch_params[] = {
   8, 120, 0,
   0
};
int g_lava_autotest_walk_sequence_plan[] = {
   3, 3,
   7, 0,
   4, 1,
   5, 11,
   0, 0
};
int g_lava_autotest_scene6_sequence_plan[] = {
   8, 1,
   3, 4,
   9, 0,
   0, 0
};
int g_lava_autotest_scene5_sequence_plan[] = {
   8, 2,
   9, 0,
   3, 4,
   0, 0
};
int g_lava_autotest_scene13_sequence_plan[] = {
   8, 3,
   3, 4,
   0, 0
};
int g_lava_autotest_scene9_sequence_plan[] = {
   8, 4,
   9, 0,
   3, 4,
   0, 0
};
int g_lava_xianling_probe_dirs[] = {
   kDirWest, kDirEast, kDirNorth, kDirSouth, -1
};
int g_lava_autotest_exits_sequence_plan[] = {
   8, 0,
   5, 5,
   0, 0
};
int g_lava_autotest_script_pair_set_ids[] = {
   1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0
};
int g_lava_autotest_object_pair_set_ids[] = {
   1, 2, 3, 0
};
int g_lava_autotest_object_scene_plan[] = {
   6, 1,
   5, 2,
   9, 3,
   0, 0
};
int g_lava_autotest_scene_setup_table[] = {
   4, 1248, 1344, 0, 1088, 784, 1,
   6, 1792, 1904, 0, 1632, 1792, 1,
   5, 684, 624, 0, 524, 512, 0,
   13, 640, 624, 0, 480, 512, 0,
   9, 960, 912, 0, 800, 416, 0,
   3, 1264, 1352, 0, 1104, 1240, 0,
   3, 992, 1472, 1, 832, 1360, 0
};
int g_lava_autotest_door_params[] = {
   4, 1152, 376, 48, 60
};
int g_lava_autotest_hall_params[] = {
   57, 4701,
   2048, 64
};
int g_lava_autotest_hall_follow_params[] = {
   57, 4771, 2,
   0
};
int g_lava_autotest_hall_range_params[] = {
   63, 5020, 5061, 4,
   0
};
int g_lava_autotest_hall_state_watch_plan[] = {
   25, 1,
   26, 1,
   27, 1,
   0, 0
};
int g_lava_autotest_hall_loop_plan[] = {
   1, 1, 0,
   2, 2, 1,
   3, 3, 0,
   0, 0, 0
};
int g_lava_autotest_hall_periodic_dump_ids[] = {
   1, 2, 0
};
int g_lava_autotest_hall_final_dump_ids[] = {
   3, 0
};
int g_lava_autotest_kitchen_params[] = {
   54, 4647, 2,
   0
};
int g_lava_autotest_kitchen_dump_ids[] = {
   1, 0
};
int g_lava_autotest_load_params[] = {
   1
};
int g_lava_hall_runtime_watch_pairs[] = {
   57, 56, 60, 59, 61, 60, 62, 61,
   0, 0
};
int g_lava_hall_plain_watch_pairs[] = {
   63, 0, 25, 0, 26, 0, 27, 0, 8, 0, 9, 0, 77, 0, 85, 0,
   0, 0
};
int g_lava_hall_final_watch_pairs[] = {
   8, 0, 9, 0, 77, 0, 85, 0,
   0, 0
};
int g_lava_kitchen_watch_pairs[] = {
   54, 0,
   0, 0
};
char g_lava_mkf_buf[65536];
char g_lava_data_buf[1024];
char g_lava_map_tiles_buf[65536];
char g_lava_map_gop_buf[65536];
long g_lava_map_tile_value;
int g_lava_map_tile_frame;
int g_lct_sx;
int g_lct_sy;
int g_lct_sh;
int g_lct_x;
int g_lct_y;
int g_lct_i;
int g_lct_l;
int g_lct_dx;
int g_lct_dy;
int g_lct_dh;
int g_lct_th;
int g_lct_tf;
long g_lct_off;
addr g_lct_tile;
int g_lmb_sx;
int g_lmb_sy;
int g_lmb_dx;
int g_lmb_dy;
int g_lmb_x;
int g_lmb_y;
int g_lmb_h;
int g_lmb_xp;
int g_lmb_yp;
addr g_lmb_tile;
long g_rle_i;
long g_rle_j;
long g_rle_k;
int g_rle_sx;
int g_rle_x;
int g_rle_y;
long g_rle_len;
int g_rle_w;
int g_rle_h;
int g_rle_srcx;
int g_rle_t;
int g_rle_dx;
int g_rle_dy;
addr g_rle_p;
char *g_rle_cp;
addr g_rle_dst;
char g_lava_dialog_icons_buf[512];
char g_lava_object_data[7200];
int g_lava_object_data_loaded;
long g_lava_object_data_size;
long g_lava_msg_offsets[LAVA_MSG_OFFSET_MAX];
int g_lava_msg_total;
FILE *g_lava_fpMSG;
FILE *g_lava_fpWORD;
FILE *g_lava_fpDESC;
int g_lava_msg_file_is_gb2312;
int g_lava_word_file_is_gb2312;
char g_lava_msg_buf[1024];
char g_lava_word_buf[64];
#define LAVA_WORD_FILE_CACHE_SIZE (256 * 1024)
char g_lava_word_file_cache[LAVA_WORD_FILE_CACHE_SIZE];
long g_lava_word_file_cache_bytes;
int g_lava_word_file_cache_ready;
char g_lava_desc_buf[256];
char g_lava_font_ascii[1536];
char g_lava_font_map[36000];
char g_lava_font_pair_map[36000];
char g_lava_font_glyph_buf[32];
FILE *g_lava_fpFONT;
long g_lava_font_gb_offset;
long g_lava_font_total_glyphs;
int g_lava_font_ascii_stride;
int g_lava_font_glyph_bytes;
int g_lava_font_ascii_width;
int g_lava_font_gb_width;
int g_lava_font_height;
int g_lava_font_loaded;
int g_lava_font_small;
int g_lava_font_map_count;
int g_lava_font_pair_map_count;
#ifdef LAVA_NATIVE_COMPILED
char g_lava_word_utf8_buf[128];
char g_lava_desc_utf8_buf[512];
#endif
char g_lava_perm_heap[4];
char g_lava_tmp_heap[4];
int g_lava_perm_heap_pos;
int g_lava_tmp_heap_pos;
char g_lava_font_offset_x[1];
char g_lava_font_offset_y[1];

#define PAL_U8(x) ((x) & 255)

static int PAL_LavaFseekOK(FILE *fp, long offset, int origin)
{
   fseek(fp, offset, origin);
   return 1;
}

/* PAL_LAVA_TEXT_PORT_PENDING */
#ifndef kKeyNone
#define kKeyNone 0
#endif
#ifndef kKeyMenu
#define kKeyMenu (1 << 0)
#endif
#ifndef kKeySearch
#define kKeySearch (1 << 1)
#endif
#ifndef kKeyDown
#define kKeyDown (1 << 2)
#endif
#ifndef kKeyLeft
#define kKeyLeft (1 << 3)
#endif
#ifndef kKeyUp
#define kKeyUp (1 << 4)
#endif
#ifndef kKeyRight
#define kKeyRight (1 << 5)
#endif
#ifndef kKeyPgUp
#define kKeyPgUp (1 << 6)
#endif
#ifndef kKeyPgDn
#define kKeyPgDn (1 << 7)
#endif
#ifndef kKeyRepeat
#define kKeyRepeat (1 << 8)
#endif
#ifndef kKeyAuto
#define kKeyAuto (1 << 9)
#endif
#ifndef kKeyDefend
#define kKeyDefend (1 << 10)
#endif
#ifndef kKeyUseItem
#define kKeyUseItem (1 << 11)
#endif
#ifndef kKeyThrowItem
#define kKeyThrowItem (1 << 12)
#endif
#ifndef kKeyFlee
#define kKeyFlee (1 << 13)
#endif
#ifndef kKeyStatus
#define kKeyStatus (1 << 14)
#endif
#ifndef kKeyForce
#define kKeyForce (1 << 15)
#endif
#ifndef kKeyHome
#define kKeyHome (1 << 16)
#endif
#ifndef kKeyEnd
#define kKeyEnd (1 << 17)
#endif

#define PAL_LAVA_KEY_LAST_MAGIC 0x1F
#define PAL_LAVA_KEY_BEST_MAGIC 0x19

#define YJ1_Decompress YJOne_Decompress
#define YJ2_Decompress YJTwo_Decompress

void PAL_Shutdown(int exit_code);
void PAL_GameMain(void);
void PAL_StartFrame(void);
int PAL_MultiByteToWideChar(addr mbs, int mbslength, addr wcs, int wcslength);
void PAL_DrawCharOnSurface(int wChar, addr lpSurface, PAL_POS pos, int bColor, int fUse8x8Font);
int PAL_CharWidth(int wChar);
FILE *UTIL_OpenFile(char *lpszFileName);
FILE *UTIL_OpenRequiredFile(char *lpszFileName);
int PAL_MKFGetChunkCount(FILE *fp);
long PAL_MKFGetChunkSize(UINT uiChunkNum, FILE *fp);
long PAL_MKFReadChunk(addr lpBuffer, UINT uiBufferSize, UINT uiChunkNum, FILE *fp);
long PAL_MKFDecompressChunk(addr lpBuffer, UINT uiBufferSize, UINT uiChunkNum, FILE *fp);
long Decompress(addr Source, addr Destination, long DestSize);
long YJ1_Decompress(addr Source, addr Destination, long DestSize);
long YJ2_Decompress(addr Source, addr Destination, long DestSize);
int PAL_FBPBlitToSurface(addr lpBitmapFBP, addr lpDstSurface);
int PAL_RLEBlitToSurface(LPCBITMAPRLE lpBitmapRLE, addr lpDstSurface, PAL_POS pos);
int PAL_RLEBlitToSurfaceWithShadow(LPCBITMAPRLE lpBitmapRLE, addr lpDstSurface, PAL_POS pos, BOOL bShadow);
long PAL_RNGReadFrame(addr lpBuffer, UINT uiBufferSize, UINT uiRngNum, UINT uiFrameNum, FILE *fpRngMKF);
long PAL_RNGBlitToSurface(addr rng, long length, addr lpDstSurface);
void PAL_ClearKeyState(void);
void PAL_ProcessEvent(void);
void PAL_ReloadInNextTick(int iSaveSlot);
void VIDEO_SetPalette(addr palette);
addr VIDEO_CreateCompatibleSurface(addr lpSource);
void VIDEO_UpdateSurfacePalette(addr lpSurface);
void VIDEO_CopySurface(addr lpSource, addr lpSrcRect, addr lpDestination, addr lpDestRect);
void VIDEO_FreeSurface(addr lpSurface);
int PAL_RLEGetWidth(LPCBITMAPRLE lpBitmapRLE);
int PAL_RLEGetHeight(LPCBITMAPRLE lpBitmapRLE);
LPCBITMAPRLE PAL_SpriteGetFrame(LPSPRITE lpSprite, int iFrameNum);
int PAL_SpriteGetNumFrames(LPSPRITE lpSprite);
int PAL_LavaLoadPlayerRoles(void);
long PAL_LavaMKFChunkOffset(FILE *fp, int chunk_num);
void PAL_WaitForAnyKey(WORD wTimeOut);
static void PAL_LavaDrawSceneFrame(void);
static void PAL_LavaDrawPartyFollowers(void);
static void PAL_LavaRefreshPartySprites(void);
static void PAL_LavaResetPartyTrail(void);
static void PAL_LavaPushPartyTrail(void);
static void PAL_LavaDrawPartyLeader(void);
static void PAL_LavaUpdateViewport(void);
static void PAL_LavaAdvanceScriptFrame(void);
static void PAL_LavaRunSceneAutoScripts(void);
static int PAL_LavaSceneWalkEventObject(addr evt, int a, int b, int c);
static int PAL_LavaSceneWalkEventObjectSpeed(addr evt, int a, int b, int c, int speed);
static void PAL_LavaLoadScenePreview(void);
static void PAL_LavaSetScene(int scene_num);
static void PAL_LavaDumpScriptEntry(int script_index);
static void PAL_LavaRunEnterScript(int script_index);
static long PAL_LavaRunTriggerScript(long script_index, int object_id);
static void PAL_LavaRebuildSceneAutoscriptRuntime(void);
static void PAL_LavaTickSceneEventObjects(void);
static void PAL_LavaInitSceneStorageOffsets(void);
static void PAL_LavaInitSceneScriptRuntime(addr scene_table, long scene_size);
static int PAL_LavaSearchScene(void);
static int PAL_LavaTouchScene(void);
static int PAL_LavaTouchSceneAt(int party_x, int party_y);
static int PAL_LavaCanSearchObject(int object_id, int party_x, int party_y, int party_dir);
static int PAL_LavaAutotestSearchObject(int object_id);
static void PAL_LavaAutotestRunTrigger(long script_index, int object_id);
static void PAL_LavaAutotestRunTriggerChain(int object_id, long initial_script, int max_steps);
static void PAL_LavaAutotestRunObjectTriggerChain(int object_id, int max_steps);
static void PAL_LavaAutotestRunBatchByScript(int *pairs);
static void PAL_LavaAutotestRunBatchByObject(int *pairs);
static int PAL_LavaAutotestEnableSearchLog(void);
static void PAL_LavaAutotestRestoreSearchLog(int saved_search_log);
static void PAL_LavaAutotestSetupScene(int scene_num, int party_x, int party_y, int party_direction,
   int view_x, int view_y, int run_enter);
static void PAL_LavaAutotestSetupSceneByIndex(int setup_index);
static void PAL_LavaAutotestRunSearchObjects(int *objects, int delay_ms);
static void PAL_LavaAutotestRunSearchSequence(void);
static void PAL_LavaAutotestRunIntroSequence(void);
static void PAL_LavaAutotestRunXianlingSequence(void);
static void PAL_LavaAutotestRunObj204Sequence(void);
static void PAL_LavaAutotestRunLoadSlot(int slot);
static void PAL_LavaAutotestRunBattleSmoke(void);
static void PAL_LavaAutotestRunHooksSequence(void);
static void PAL_LavaAutotestRunExitsSequence(void);
static void PAL_LavaAutotestRunScene6Sequence(void);
static void PAL_LavaAutotestRunScene5Sequence(void);
static void PAL_LavaAutotestRunScene13Sequence(void);
static void PAL_LavaAutotestRunScene9Sequence(void);
static void PAL_LavaAutotestRunDoorSequence(void);
static void PAL_LavaAutotestRunKitchenSequence(void);
static void PAL_LavaAutotestRunWalkSequence(void);
static void PAL_LavaAutotestRunHallSequence(void);
static void PAL_LavaAutotestRunSceneScriptBatch(int scene_num, int party_x, int party_y, int party_direction,
   int view_x, int view_y, int run_enter, int *pairs);
static void PAL_LavaAutotestRunSceneObjectBatch(int scene_num, int party_x, int party_y, int party_direction,
   int view_x, int view_y, int run_enter, int *pairs);
static int *PAL_LavaAutotestIntListById(int list_id);
static int *PAL_LavaAutotestScriptPairsById(int plan_id);
static int *PAL_LavaAutotestObjPairsById(int plan_id);
static void PAL_LavaAutotestRunSceneScriptPlan(int *plan);
static void PAL_LavaAutotestRunSceneScriptSequence(int *plan);
static int PAL_LavaAutotestRunCurrentSceneScriptPlan(int *plan);
static int PAL_LavaAutotestRunObjPlanNow(int *plan);
static void PAL_LavaAutotestPrimeSearch(int *params);
static void PAL_LavaAutotestRunSequencePlan(int *plan);
static int PAL_LavaAutotestRunWhenTriggerEquals(int object_id, int expected_trigger, int *done_flag, int max_steps);
static int PAL_LavaAutotestRunWhenStatePositive(int object_id, int *done_flag);
static int PAL_LavaAutotestTouchWhenStatePositive(int object_id, int *done_flag, int touch_x, int touch_y);
static int PAL_LavaAutotestTriggerInRange(addr evt, int min_trigger, int max_trigger);
static int PAL_LavaAutotestRunRangedChain(int object_id, int min_trigger, int max_trigger, int max_steps, int *last_trigger);
static int PAL_LavaAutotestRunRangedChainWhenStatePositive(int object_id, int min_trigger, int max_trigger, int max_steps, int *last_trigger);
static int PAL_LavaAutotestRunFollowStep(int *params, int *done_flag);
static int PAL_LavaAutotestRunRangedStep(int *params, int *last_trigger);
static int PAL_LavaAutotestRunRangedStepWhenEnabled(int *params, int *enabled_flag, int *last_trigger);
static int *PAL_LavaHallPlanById(int plan_id);
static void PAL_LavaHallRunLoopPlan(int *plan, int *done_flags, int *last_trigger);
static void PAL_LavaHallDumpById(int dump_id);
static void PAL_LavaHallDumpPlan(int *plan);
static void PAL_LavaHallDumpPeriodic(int step, int interval);
static void PAL_LavaHallDumpFinal(void);
static void PAL_LavaHallResetState(int *done_flags, int *last_trigger);
static void PAL_LavaHallBeginRun(int object_id, int trigger_script);
static void PAL_LavaHallEndRun(int saved_search_log);
static void PAL_LavaKitchenDumpById(int dump_id);
static void PAL_LavaKitchenDumpPlan(int *plan);
static void PAL_LavaKitchenBeginRun(int *params);
static void PAL_LavaKitchenEndRun(int saved_search_log);
static void PAL_LavaAutotestAdvanceLogicFrame(int step, char *tag);
static int PAL_LavaAutotestDoorStep(int step, int object_id, int *touched_door, int touch_x, int touch_y, int delay_ms);
static void PAL_LavaAutotestClearPlanDoneFlags(int *plan, int *done_flags);
static void PAL_LavaAutotestRunActionPlan(int *plan, int *done_flags);
static void PAL_LavaDumpObjStat(char *tag, int object_id, int autoscript_index, int include_runtime);
static void PAL_LavaDumpObjList(char *tag, int *pairs, int include_runtime);
static void PAL_LavaAutotestWalkProbe(void);
static int PAL_LavaAutotestTouchObject(int object_id);
static void PAL_LavaDumpTouchCandidates(void);
static int PAL_LavaAutotestFollowTouchChain(int max_passes, int delay_ms, int stop_after_first);
static void PAL_LavaFinishScriptStep(void);
static WORD PAL_LavaSceneEnterScriptFor(int scene_num);
static WORD PAL_LavaSceneTeleportScriptFor(int scene_num);
static void PAL_LavaSetSceneScriptHooks(int scene_num, int enter_script, int teleport_script);
static void PAL_LavaSetActiveSceneEnterScript(int enter_script);
static int PAL_LavaReadScriptEntry(long script_index, addr entry);
static int PAL_LavaTileBlocked(int world_x, int world_y);
static int PAL_LavaEventBlocked(int world_x, int world_y, int self_object_id);
static int PAL_LavaTriggerBlockedSearchObject(int world_x, int world_y);
static int PAL_LavaGetItemAmount(int item);
static int PAL_LavaAddItemToInventory(int item, int amount);
static int PAL_LavaHasCommandLineFlag(char *flag);
static void PAL_LavaDumpInventory(void);
static void PAL_LavaInitAutotestFlags(void);
static int PAL_LavaAnySaveExists(void);
static int PAL_LavaSaveExists(int slot);
static int PAL_LavaSaveGame(int slot);
static int PAL_LavaLoadSavedGame(int slot);
static int PAL_LavaChooseLoadSlot(int in_system_menu);
static int PAL_LavaChooseSaveSlot(void);
static long PAL_LavaReadU16(addr data, long offset);
static int PAL_LavaReadS16(addr data, long offset);
static void PAL_LavaWriteU16(addr data, long offset, int value);
static void PAL_LavaWriteS16(addr data, long offset, int value);
static void PAL_LavaWalkPartyToTarget(int target_x, int target_y);
static void PAL_LavaWalkPartyToTargetSpeed(int target_x, int target_y, int speed_x, int speed_y, char *op_name);
static void PAL_LavaRidePartyOnEventObject(int object_id, addr evt, int a, int b, int c, int speed);
static addr PAL_LavaSceneEventData(int object_id);
static addr PAL_LavaResolveEventTarget(int object_id, int target_id);
static void PAL_LavaAdvanceEventFrame(addr evt);
static void PAL_LavaNormalizeLoadedFrame(int *direction, int *frame, int frame_stride);
static void PAL_LavaNormalizeLoadedEventFrames(void);
static void PAL_LavaCaptureDialogReplaySnapshot(void);
static void PAL_LavaRestoreDialogReplaySnapshot(void);
static void PAL_LavaQueueDialogEvent(int type, int a, int b, int c);
static void PAL_LavaReplayDialogVisualEvent(int index);
static int PAL_LavaLoadFontAsset(int small_font);
static void PAL_LavaTextOutToSurfaceEx(SDL_Surface *surface, int x, int y, char *text, int color, int small_font, int gbk_first);
static void PAL_LavaTextOutToSurface(SDL_Surface *surface, int x, int y, char *text, int color, int small_font);
static int PAL_LavaMenuSelectedColor(void);
static void PAL_LavaDrawOpeningMenuText(int x, int y, int label, int color);
static void PAL_LavaDrawOpeningMenuBackground(void);
static int PAL_LavaReadCancelKey(void);
static addr PAL_LavaLoadUISprite(void);
static int PAL_LavaDrawSpriteBoxAt(addr sprite, int x, int y, int rows, int columns, int style);
static void PAL_LavaDrawSingleLineBox(int x, int y, int w);
static int PAL_LavaRoleWordByArray(int array_index, int role_index);
static void PAL_LavaClearRoleTempStats(void);
static void PAL_LavaSetRoleTempStatPercent(int array_index, int role_index, int percent);
static int PAL_LavaRoleTempStatPercent(int array_index, int role_index);
static void PAL_LavaDrawNumberText(int x, int y, int value, int color);
static char *PAL_LavaReadWord(int word_id);
static char *PAL_LavaRoleName(int role_index);
static char *PAL_LavaRoleNameForLog(int role_index);
static void PAL_LavaFormatObjectLabel(char *buf, char *prefix, int object_id);
static long PAL_LavaReadObjectField(int object_id, int field_index);
static void PAL_LavaWriteObjectField(int object_id, int field_index, long value);
static long PAL_LavaReadMagicField(int magic_id, int field_index);
static void PAL_LavaDrawItemSubMenuFrame(int selected);
static void PAL_LavaDrawItemListFrame(char *title);
static void PAL_LavaDrawMagicListFrame(void);
static void PAL_LavaDrawMagicMenuContent(int current_party, int current_index);
static void PAL_LavaDrawItemUseMenuContent(int current_index);
static void PAL_LavaDrawEquipMenuContent(int current_index);
static int PAL_LavaChooseTargetRole(char *title, int object_id, int parent_kind);
static int PAL_LavaChooseTwoItemMenu(char *title, char *first, char *second, int selected, int parent_kind);
static void PAL_LavaShowItemUseMenu(void);
static void PAL_LavaShowEquipMenu(void);
static void PAL_LavaDrawMainMenu(void);
static void PAL_LavaDrawMainMenuItem(int index, int selected);
static int PAL_LavaInGameMenu(void);
void PAL_WaitForKey(WORD wTimeOut);
static void PAL_LavaShowStatusMenu(void);
static void PAL_LavaShowMagicMenu(void);
static void PAL_LavaShowInventoryMenu(void);
static char *PAL_LavaSystemMenuLabel(int index);
static int PAL_LavaTwoChoiceMenu(char *left, char *right, int selected);
static void PAL_LavaDrawSystemMenuFrameOnly(void);
static void PAL_LavaDrawSystemMenuItem(int index);
static void PAL_LavaDrawSystemMenuLabels(void);
static void PAL_LavaDrawSystemMenu(void);
static int PAL_LavaSystemMenu(void);

static void PAL_LavaSyncPartyToViewport(void)
{
   g_lava_party_x = g_lava_view_x + 160;
   g_lava_party_y = g_lava_view_y + 112;
}

static void PAL_LavaResetPartyTrail(void)
{
   int i;

   g_lava_walk_substep_pending = 0;
   for (i = 0; i < 5; i++)
   {
      g_lava_party_trail_x[i] = g_lava_party_x;
      g_lava_party_trail_y[i] = g_lava_party_y;
      g_lava_party_trail_dir[i] = g_lava_party_direction;
   }
}

static void PAL_LavaPushPartyTrail(void)
{
   int i;

   for (i = 4; i > 0; i--)
   {
      g_lava_party_trail_x[i] = g_lava_party_trail_x[i - 1];
      g_lava_party_trail_y[i] = g_lava_party_trail_y[i - 1];
      g_lava_party_trail_dir[i] = g_lava_party_trail_dir[i - 1];
   }

   g_lava_party_trail_x[0] = g_lava_party_x;
   g_lava_party_trail_y[0] = g_lava_party_y;
   g_lava_party_trail_dir[0] = g_lava_party_direction;
}

static void PAL_LavaApplyPartyStepDelta(int step_x, int step_y)
{
   if (step_x == 0 && step_y == 0)
   {
      return;
   }

   if (step_y < 0)
   {
      g_lava_party_direction = (step_x < 0) ? 1 : 2;
   }
   else
   {
      g_lava_party_direction = (step_x < 0) ? 0 : 3;
   }

   g_lava_party_step_frame++;
   if (g_lava_player_walk_frames[0] == 4)
   {
      g_lava_party_frame = g_lava_party_step_frame % 4;
   }
   else
   {
      g_lava_party_frame = g_lava_party_step_frame % 3;
   }
}

static int PAL_LavaMovePartyStepDistance(int move_dir, int divisor, int lookahead)
{
   int collision_x;
   int collision_y;
   int dx;
   int dy;
   int next_x;
   int next_y;

   dx = 0;
   dy = 0;
   if (move_dir == kDirSouth)
   {
      g_lava_party_direction = 0;
      dx = -16;
      dy = 8;
   }
   else if (move_dir == kDirWest)
   {
      g_lava_party_direction = 1;
      dx = -16;
      dy = -8;
   }
   else if (move_dir == kDirNorth)
   {
      g_lava_party_direction = 2;
      dx = 16;
      dy = -8;
   }
   else if (move_dir == kDirEast)
   {
      g_lava_party_direction = 3;
      dx = 16;
      dy = 8;
   }
   else
   {
      return 0;
   }

   if (divisor > 1)
   {
      dx = dx < 0 ? -10 : 10;
      dy = dy < 0 ? -5 : 5;
   }

   next_x = g_lava_party_x + dx;
   next_y = g_lava_party_y + dy;
   collision_x = lookahead ? next_x + dx : next_x;
   collision_y = lookahead ? next_y + dy : next_y;
   if (PAL_LavaTileBlocked(collision_x, collision_y) ||
       PAL_LavaEventBlocked(collision_x, collision_y, 0))
   {
      if (PAL_LavaTouchSceneAt(collision_x, collision_y))
      {
         return -1;
      }
      printf("[LAVA][BLOCK] next=(%d,%d) dir=%d\n",
         collision_x, collision_y, g_lava_party_direction);
      PAL_LavaUpdateViewport();
      PAL_LavaDrawSceneFrame();
      return 0;
   }

   PAL_LavaPushPartyTrail();
   g_lava_party_x = next_x;
   g_lava_party_y = next_y;
   g_lava_party_step_frame++;
   if (g_lava_player_walk_frames[0] == 4)
   {
      g_lava_party_frame = g_lava_party_step_frame % 4;
   }
   else
   {
      int step_phase;

      step_phase = g_lava_party_step_frame % 4;
      g_lava_party_frame = (step_phase & 1) ? ((step_phase + 1) / 2) : 0;
   }

   return 1;
}

static int PAL_LavaMovePartyStep(int move_dir)
{
   return PAL_LavaMovePartyStepDistance(move_dir, 1, 0);
}

static int PAL_LavaMovePartySubstep(int move_dir, int lookahead)
{
   return PAL_LavaMovePartyStepDistance(move_dir, 2, lookahead);
}

static void PAL_LavaWalkPartyToTargetSpeed(int target_x, int target_y, int speed_x, int speed_y, char *op_name)
{
   int steps;
   int max_steps;

   if (speed_x <= 0)
   {
      speed_x = 4;
   }
   if (speed_y <= 0)
   {
      speed_y = 2;
   }

   steps = 0;
   max_steps = abs(target_x - g_lava_party_x) / speed_x;
   if (abs(target_y - g_lava_party_y) / speed_y > max_steps)
   {
      max_steps = abs(target_y - g_lava_party_y) / speed_y;
   }
   max_steps += 4;

   while ((g_lava_party_x != target_x || g_lava_party_y != target_y) && steps < max_steps)
   {
      int dx;
      int dy;
      int step_x;
      int step_y;

      dx = target_x - g_lava_party_x;
      dy = target_y - g_lava_party_y;

      if (dx > 0)
      {
         step_x = (dx < speed_x) ? dx : speed_x;
      }
      else if (dx < 0)
      {
         step_x = (dx > -speed_x) ? dx : -speed_x;
      }
      else
      {
         step_x = 0;
      }

      if (dy > 0)
      {
         step_y = (dy < speed_y) ? dy : speed_y;
      }
      else if (dy < 0)
      {
         step_y = (dy > -speed_y) ? dy : -speed_y;
      }
      else
      {
         step_y = 0;
      }

      PAL_LavaPushPartyTrail();
      g_lava_party_x += step_x;
      g_lava_party_y += step_y;
      PAL_LavaUpdateViewport();
      PAL_LavaApplyPartyStepDelta(step_x, step_y);
      if (g_lava_autotest_search || g_lava_autotest_walk || g_lava_autotest_exits || g_lava_autotest_scene5 || g_lava_autotest_scene6 || g_lava_autotest_scene13 || g_lava_autotest_scene9 || g_lava_autotest_hooks)
      {
         printf("[LAVA][STEP] op=%s view=(%d,%d) party=(%d,%d) delta=(%d,%d) dir=%d frame=%d target=(%d,%d)\n",
            op_name,
            g_lava_view_x, g_lava_view_y,
            g_lava_party_x, g_lava_party_y,
            step_x, step_y,
            g_lava_party_direction, g_lava_party_frame,
            target_x, target_y);
      }
      steps++;
   }

   g_lava_party_x = target_x;
   g_lava_party_y = target_y;
   PAL_LavaUpdateViewport();
   g_lava_party_frame = 0;
   if (g_lava_autotest_search || g_lava_autotest_walk || g_lava_autotest_exits || g_lava_autotest_scene5 || g_lava_autotest_scene6 || g_lava_autotest_scene13 || g_lava_autotest_scene9 || g_lava_autotest_hooks)
   {
      printf("[LAVA][STEP] op=%s done view=(%d,%d) party=(%d,%d) target=(%d,%d) steps=%d\n",
         op_name,
         g_lava_view_x, g_lava_view_y,
         g_lava_party_x, g_lava_party_y,
         target_x, target_y, steps);
   }
}

static void PAL_LavaWalkPartyToTarget(int target_x, int target_y)
{
   PAL_LavaWalkPartyToTargetSpeed(target_x, target_y, 4, 2, "0070");
}

static void PAL_LavaRidePartyOnEventObject(int object_id, addr evt, int a, int b, int c, int speed)
{
   int target_x;
   int target_y;
   int steps;
   int max_steps;

   target_x = a * 32 + c * 16;
   target_y = b * 16 + c * 8;

   if (evt == 0)
   {
      PAL_LavaWalkPartyToTargetSpeed(target_x, target_y, speed * 2, speed, "0044");
      return;
   }

   if (speed <= 0)
   {
      speed = 4;
   }

   steps = 0;
   max_steps = abs(target_x - PAL_LavaReadU16(evt, 2)) / (speed * 2);
   if (abs(target_y - PAL_LavaReadU16(evt, 4)) / speed > max_steps)
   {
      max_steps = abs(target_y - PAL_LavaReadU16(evt, 4)) / speed;
   }
   max_steps += 4;

   while (steps < max_steps && !PAL_LavaSceneWalkEventObjectSpeed(evt, target_x / 32, target_y / 16, (target_x % 32) ? 1 : 0, speed))
   {
      g_lava_party_x = PAL_LavaReadU16(evt, 2);
      g_lava_party_y = PAL_LavaReadU16(evt, 4);
      g_lava_party_direction = PAL_LavaReadU16(evt, 20);
      g_lava_party_frame = PAL_LavaReadU16(evt, 22);
      PAL_LavaUpdateViewport();
      PAL_LavaDrawSceneFrame();
      UTIL_Delay(60);
      steps++;
   }

   g_lava_party_x = target_x;
   g_lava_party_y = target_y;
   g_lava_party_direction = PAL_LavaReadU16(evt, 20);
   g_lava_party_frame = 0;
   PAL_LavaWriteU16(evt, 2, target_x);
   PAL_LavaWriteU16(evt, 4, target_y);
   PAL_LavaWriteU16(evt, 22, 0);
   PAL_LavaUpdateViewport();

   if (g_lava_autotest_search || g_lava_autotest_walk || g_lava_autotest_exits || g_lava_autotest_scene5 || g_lava_autotest_scene6 || g_lava_autotest_scene13 || g_lava_autotest_scene9 || g_lava_autotest_hooks)
   {
      printf("[LAVA][STEP] op=0044 object=%d view=(%d,%d) party=(%d,%d) event=(%d,%d) target=(%d,%d) steps=%d\n",
         object_id,
         g_lava_view_x, g_lava_view_y,
         g_lava_party_x, g_lava_party_y,
         PAL_LavaReadU16(evt, 2), PAL_LavaReadU16(evt, 4),
         target_x, target_y, steps);
   }
}

static void PAL_LavaCaptureDialogReplaySnapshot(void)
{
   int object_id;
   int count;
   int i;

   g_lava_dialog_replay_snapshot_valid = 1;
   g_lava_dialog_replay_scene_first = g_lava_scene_event_first;
   g_lava_dialog_replay_scene_last = g_lava_scene_event_last;
   g_lava_dialog_replay_party_x = g_lava_party_x;
   g_lava_dialog_replay_party_y = g_lava_party_y;
   g_lava_dialog_replay_party_direction = g_lava_party_direction;
   g_lava_dialog_replay_party_frame = g_lava_party_frame;
   g_lava_dialog_replay_party_count = g_lava_party_count;
   g_lava_dialog_replay_party_layer = g_lava_party_layer;
   g_lava_dialog_replay_view_x = g_lava_view_x;
   g_lava_dialog_replay_view_y = g_lava_view_y;
   for (i = 0; i < 6; i++)
   {
      g_lava_dialog_replay_player_sprite_num[i] = g_lava_player_sprite_num[i];
   }
   for (i = 0; i < 3; i++)
   {
      g_lava_dialog_replay_party_role[i] = g_lava_party_role[i];
   }

   count = g_lava_dialog_replay_scene_last - g_lava_dialog_replay_scene_first + 1;
   if (count < 0) count = 0;
   if (count > LAVA_DIALOG_REPLAY_MAX) count = LAVA_DIALOG_REPLAY_MAX;
   for (object_id = 0; object_id < count; object_id++)
   {
      g_lava_dialog_replay_apc[object_id] = g_lava_autoscript_pc[g_lava_dialog_replay_scene_first - 1 + object_id];
      g_lava_dialog_replay_aidle[object_id] = g_lava_autoscript_idle[g_lava_dialog_replay_scene_first - 1 + object_id];
      memcpy((addr)(g_lava_dialog_replay_event_raw + object_id * 32),
         (addr)(g_lava_scene_event_raw + (g_lava_dialog_replay_scene_first - 1 + object_id) * 32),
         32);
      if (g_lava_dialog_replay_apc[object_id] == 0)
      {
         g_lava_dialog_replay_apc[object_id] = PAL_LavaReadU16((addr)(g_lava_dialog_replay_event_raw + object_id * 32), 10);
      }
   }
}

static void PAL_LavaRestoreDialogReplaySnapshot(void)
{
   int object_id;
   int count;
   int i;

   if (!g_lava_dialog_replay_snapshot_valid)
   {
      return;
   }

   g_lava_party_x = g_lava_dialog_replay_party_x;
   g_lava_party_y = g_lava_dialog_replay_party_y;
   g_lava_party_direction = g_lava_dialog_replay_party_direction;
   g_lava_party_frame = g_lava_dialog_replay_party_frame;
   g_lava_party_count = g_lava_dialog_replay_party_count;
   g_lava_party_layer = g_lava_dialog_replay_party_layer;
   g_lava_view_x = g_lava_dialog_replay_view_x;
   g_lava_view_y = g_lava_dialog_replay_view_y;
   for (i = 0; i < 6; i++)
   {
      g_lava_player_sprite_num[i] = g_lava_dialog_replay_player_sprite_num[i];
   }
   for (i = 0; i < 3; i++)
   {
      g_lava_party_role[i] = g_lava_dialog_replay_party_role[i];
   }

   count = g_lava_dialog_replay_scene_last - g_lava_dialog_replay_scene_first + 1;
   if (count < 0) count = 0;
   if (count > LAVA_DIALOG_REPLAY_MAX) count = LAVA_DIALOG_REPLAY_MAX;
   for (object_id = 0; object_id < count; object_id++)
   {
      g_lava_autoscript_pc[g_lava_dialog_replay_scene_first - 1 + object_id] = g_lava_dialog_replay_apc[object_id];
      g_lava_autoscript_idle[g_lava_dialog_replay_scene_first - 1 + object_id] = g_lava_dialog_replay_aidle[object_id];
      memcpy((addr)(g_lava_scene_event_raw + (g_lava_dialog_replay_scene_first - 1 + object_id) * 32),
         (addr)(g_lava_dialog_replay_event_raw + object_id * 32),
         32);
   }
}

static void PAL_LavaEnsureDialogReplaySnapshot(void)
{
   if (!g_lava_dialog_replay_snapshot_valid)
   {
      PAL_LavaCaptureDialogReplaySnapshot();
   }
}

static void PAL_LavaQueueDialogEvent(int type, int a, int b, int c)
{
   if (g_lava_dialog_event_count >= LAVA_DIALOG_EVENT_MAX)
   {
      return;
   }

   PAL_LavaEnsureDialogReplaySnapshot();

   g_lava_dialog_event_type[g_lava_dialog_event_count] = type;
   g_lava_dialog_event_a[g_lava_dialog_event_count] = a;
   g_lava_dialog_event_b[g_lava_dialog_event_count] = b;
   g_lava_dialog_event_c[g_lava_dialog_event_count] = c;
   g_lava_dialog_event_count++;
}

static void PAL_LavaReplayDialogVisualEvent(int index)
{
   addr evt;
   int type;
   int a;
   int b;
   int c;

   type = g_lava_dialog_event_type[index];
   a = g_lava_dialog_event_a[index];
   b = g_lava_dialog_event_b[index];
   c = g_lava_dialog_event_c[index];

   if (type == LAVA_DIALOG_EVENT_PARTY_POS)
   {
      g_lava_party_x = a;
      g_lava_party_y = b;
      PAL_LavaUpdateViewport();
   }
   else if (type == LAVA_DIALOG_EVENT_PARTY_POSE)
   {
      g_lava_party_direction = a;
      g_lava_party_frame = b;
      if (g_lava_scene_num == 1 && b == 9)
      {
         g_lava_intro_obj9_probe_active = 1;
      }
    }
   else if (type == LAVA_DIALOG_EVENT_PARTY_SPRITE)
   {
      if (a >= 0 && a < 6)
      {
         g_lava_player_sprite_num[a] = b;
         if (g_lava_scene_num == 1 && b == 193)
         {
            g_lava_intro_obj9_probe_active = 1;
         }
         if (g_lava_scene_num == 1 && b != 193)
         {
            g_lava_intro_obj9_probe_active = 0;
         }
      }
   }
   else if (type == LAVA_DIALOG_EVENT_EVENT_POSE)
   {
      evt = PAL_LavaSceneEventData(a);
      if (evt != 0)
      {
         PAL_LavaWriteU16(evt, 20, b);
         PAL_LavaWriteU16(evt, 22, c);
      }
   }
   else if (type == LAVA_DIALOG_EVENT_EVENT_GESTURE)
   {
      evt = PAL_LavaSceneEventData(a);
      if (evt != 0)
      {
         PAL_LavaWriteU16(evt, 22, b);
         PAL_LavaWriteU16(evt, 20, 0);
      }
   }
   else if (type == LAVA_DIALOG_EVENT_EVENT_STEP)
   {
      int frame;

      evt = PAL_LavaSceneEventData(a);
      if (evt != 0)
      {
         if (g_lava_scene_num == 1 && a == 9)
         {
            g_lava_intro_obj9_probe_active = 1;
         }
         PAL_LavaWriteU16(evt, 2, PAL_LavaReadU16(evt, 2) + b);
         PAL_LavaWriteU16(evt, 4, PAL_LavaReadU16(evt, 4) + c);
         if (PAL_LavaReadU16(evt, 18) == 0 && PAL_LavaReadU16(evt, 28) == 0)
         {
            frame = (PAL_LavaReadU16(evt, 22) + 1) & 3;
            PAL_LavaWriteU16(evt, 22, frame);
         }
         else
         {
            PAL_LavaAdvanceEventFrame(evt);
         }
         if (g_lava_autotest_intro && g_lava_scene_num == 1 && a == 9)
         {
            printf("[LAVA][OBJ9STEP] dx=%d dy=%d pos=(%d,%d) dir=%d frame=%d sprite=%d\n",
               b, c,
               PAL_LavaReadU16(evt, 2), PAL_LavaReadU16(evt, 4),
               PAL_LavaReadU16(evt, 20), PAL_LavaReadU16(evt, 22),
               PAL_LavaReadU16(evt, 16));
         }
      }
   }
   else if (type == LAVA_DIALOG_EVENT_EVENT_STATE)
   {
      evt = PAL_LavaResolveEventTarget(0, a);
      if (evt != 0 && a != 0)
      {
         PAL_LavaWriteS16(evt, 12, b);
         if (g_lava_autotest_intro && g_lava_scene_num == 1)
         {
            printf("[LAVA][STATEEVT] target=%d value=%d now=%d sprite=%d pos=(%d,%d)\n",
               a, b,
               PAL_LavaReadS16(evt, 12),
               PAL_LavaReadU16(evt, 16),
               PAL_LavaReadU16(evt, 2), PAL_LavaReadU16(evt, 4));
         }
      }
   }
   else if (type == LAVA_DIALOG_EVENT_EVENT_POS)
   {
      evt = PAL_LavaSceneEventData(a);
      if (evt != 0)
      {
         PAL_LavaWriteU16(evt, 2, b);
         PAL_LavaWriteU16(evt, 4, c);
      }
   }
   else if (type == LAVA_DIALOG_EVENT_PARTY_WALK)
   {
      PAL_LavaWalkPartyToTarget(a, b);
   }
   else if (type == LAVA_DIALOG_EVENT_PARTY_RIDE)
   {
      evt = PAL_LavaSceneEventData(a);
      PAL_LavaRidePartyOnEventObject(a, evt, b, c, 0, 4);
      return;
   }
   else if (type == LAVA_DIALOG_EVENT_WAIT)
   {
      int wait_frames;
      int wait_i;
      addr evt11;
      addr evt12;
      addr evt9;

      wait_frames = (a > 0) ? a : 1;
      for (wait_i = 0; wait_i < wait_frames; wait_i++)
      {
         PAL_LavaAdvanceScriptFrame();
         if ((g_lava_autotest_input || g_lava_autotest_intro) && g_lava_scene_num == 1)
         {
            evt11 = PAL_LavaSceneEventData(11);
            evt12 = PAL_LavaSceneEventData(12);
            evt9 = PAL_LavaSceneEventData(9);
            if (evt11 != 0 && evt12 != 0 && evt9 != 0)
            {
               printf("[LAVA][INTROWAIT] frames=%d step=%d obj11=(%d,%d,s=%d,sp=%d,d=%d,f=%d,tr=%d,au=%d,pc=%d,id=%d) obj12=(%d,%d,s=%d,sp=%d,d=%d,f=%d,tr=%d,au=%d,pc=%d,id=%d) obj9=(%d,%d,s=%d,sp=%d,nf=%d,d=%d,f=%d,tr=%d,au=%d,pc=%d,id=%d)\n",
                  wait_frames, wait_i,
                  PAL_LavaReadU16(evt11, 2), PAL_LavaReadU16(evt11, 4),
                  PAL_LavaReadS16(evt11, 12), PAL_LavaReadU16(evt11, 16), PAL_LavaReadU16(evt11, 20), PAL_LavaReadU16(evt11, 22),
                  PAL_LavaReadU16(evt11, 8), PAL_LavaReadU16(evt11, 10),
                  g_lava_autoscript_pc[10], g_lava_autoscript_idle[10],
                  PAL_LavaReadU16(evt12, 2), PAL_LavaReadU16(evt12, 4),
                  PAL_LavaReadS16(evt12, 12), PAL_LavaReadU16(evt12, 16), PAL_LavaReadU16(evt12, 20), PAL_LavaReadU16(evt12, 22),
                  PAL_LavaReadU16(evt12, 8), PAL_LavaReadU16(evt12, 10),
                  g_lava_autoscript_pc[11], g_lava_autoscript_idle[11],
                  PAL_LavaReadU16(evt9, 2), PAL_LavaReadU16(evt9, 4),
                  PAL_LavaReadS16(evt9, 12), PAL_LavaReadU16(evt9, 16), PAL_LavaReadU16(evt9, 18), PAL_LavaReadU16(evt9, 20), PAL_LavaReadU16(evt9, 22),
                  PAL_LavaReadU16(evt9, 8), PAL_LavaReadU16(evt9, 10),
                  g_lava_autoscript_pc[8], g_lava_autoscript_idle[8]);
            }
         }
      }
      return;
   }

   if (!g_lava_dialog_silent_visual_replay)
   {
      PAL_LavaDrawSceneFrame();
      UTIL_Delay(g_lava_autotest_intro ? 1 : 90);
   }
}

static int PAL_LavaAutotouchSeen(int object_id, long trigger_script)
{
   int i;

   for (i = 0; i < g_lava_autotouch_seen_count; i++)
   {
      if (g_lava_autotouch_seen_object[i] == object_id &&
          g_lava_autotouch_seen_trigger[i] == trigger_script)
      {
         return 1;
      }
   }

   return 0;
}

static void PAL_LavaAutotouchRemember(int object_id, long trigger_script)
{
   if (g_lava_autotouch_seen_count >= LAVA_AUTOTOUCH_SEEN_MAX)
   {
      return;
   }

   g_lava_autotouch_seen_object[g_lava_autotouch_seen_count] = object_id;
   g_lava_autotouch_seen_trigger[g_lava_autotouch_seen_count] = trigger_script;
   g_lava_autotouch_seen_count++;
}

static void PAL_LavaDumpAutotouchVisited(void)
{
   int i;

   printf("[LAVA][AUTOTOUCH] visited=%d\n", g_lava_autotouch_seen_count);
   for (i = 0; i < g_lava_autotouch_seen_count; i++)
   {
      printf("[LAVA][AUTOTOUCH] visited object=%d trigger=%d\n",
         g_lava_autotouch_seen_object[i],
         g_lava_autotouch_seen_trigger[i]);
   }
}

static int PAL_LavaDirFromKeyMask(int key)
{
   if (key & kKeyDown) return kDirSouth;
   if (key & kKeyLeft) return kDirWest;
   if (key & kKeyUp) return kDirNorth;
   if (key & kKeyRight) return kDirEast;
   return kDirUnknown;
}

static int PAL_LavaGetCurrDirection(void)
{
   int i;
   int dir;

   dir = kDirSouth;
   for (i = 1; i < 4; i++)
   {
      if (g_InputState.dwKeyOrder[dir] < g_InputState.dwKeyOrder[i])
      {
         dir = i;
      }
   }

   if (g_InputState.dwKeyOrder[dir] == 0)
   {
      return kDirUnknown;
   }

   return dir;
}

static void PAL_LavaKeyDown(int key, int repeat)
{
   int dir;

   dir = PAL_LavaDirFromKeyMask(key);
   if (!repeat && dir != kDirUnknown)
   {
      g_InputState.dwKeyMaxCount++;
      g_InputState.dwKeyOrder[dir] = g_InputState.dwKeyMaxCount;
      g_InputState.prevdir = g_InputState.dir;
      g_InputState.dir = PAL_LavaGetCurrDirection();
   }

   g_InputState.dwKeyPress |= key;
}

static void PAL_LavaKeyUp(int key)
{
   int dir;

   dir = PAL_LavaDirFromKeyMask(key);
   if (dir != kDirUnknown)
   {
      g_InputState.dwKeyOrder[dir] = 0;
      g_InputState.prevdir = g_InputState.dir;
      g_InputState.dir = PAL_LavaGetCurrDirection();
      g_InputState.dwKeyMaxCount =
         (g_InputState.dir == kDirUnknown) ? 0 : g_InputState.dwKeyOrder[g_InputState.dir];
   }
}

static int PAL_LavaGetItemAmount(int item)
{
   int i;

   if (item <= 0)
   {
      return 0;
   }

   for (i = 0; i < 256; i++)
   {
      if (g_lava_inventory_item[i] == item)
      {
         return g_lava_inventory_amount[i];
      }
   }

   return 0;
}

static int PAL_LavaAddItemToInventory(int item, int amount)
{
   int i;
   int empty_slot;
   int next_amount;

   if (item <= 0 || amount == 0)
   {
      return 0;
   }

   empty_slot = -1;
   for (i = 0; i < 256; i++)
   {
      if (g_lava_inventory_item[i] == item)
      {
         next_amount = g_lava_inventory_amount[i] + amount;
         if (next_amount <= 0)
         {
            g_lava_inventory_item[i] = 0;
            g_lava_inventory_amount[i] = 0;
            return 0;
         }
         g_lava_inventory_amount[i] = next_amount;
         return next_amount;
      }
      if (empty_slot < 0 && g_lava_inventory_item[i] == 0)
      {
         empty_slot = i;
      }
   }

   if (amount < 0 || empty_slot < 0)
   {
      return 0;
   }

   g_lava_inventory_item[empty_slot] = item;
   g_lava_inventory_amount[empty_slot] = amount;
   return amount;
}

static int PAL_LavaHasCommandLineFlag(char *flag)
{
   GetCommandLine((addr)g_lava_desc_buf);
   g_lava_cmd_i = 0;
   while (g_lava_desc_buf[g_lava_cmd_i] != 0)
   {
      while (g_lava_desc_buf[g_lava_cmd_i] == ' ')
      {
         g_lava_cmd_i++;
      }
      g_lava_cmd_j = 0;
      while (flag[g_lava_cmd_j] != 0 &&
             g_lava_desc_buf[g_lava_cmd_i + g_lava_cmd_j] == flag[g_lava_cmd_j])
      {
         g_lava_cmd_j++;
      }
      if (flag[g_lava_cmd_j] == 0 &&
          (g_lava_desc_buf[g_lava_cmd_i + g_lava_cmd_j] == 0 ||
           g_lava_desc_buf[g_lava_cmd_i + g_lava_cmd_j] == ' '))
      {
         return 1;
      }
      while (g_lava_desc_buf[g_lava_cmd_i] != 0 &&
             g_lava_desc_buf[g_lava_cmd_i] != ' ')
      {
         g_lava_cmd_i++;
      }
   }

   return 0;
}

static void PAL_LavaInitAutotestFlags(void)
{
   g_lava_autotest_search = PAL_LavaHasCommandLineFlag("autotest_search");
   g_lava_autotest_walk = PAL_LavaHasCommandLineFlag("autotest_walk");
   g_lava_autotest_exits = PAL_LavaHasCommandLineFlag("autotest_exits");
   g_lava_autotest_scene5 = PAL_LavaHasCommandLineFlag("autotest_scene5");
   g_lava_autotest_scene6 = PAL_LavaHasCommandLineFlag("autotest_scene6");
   g_lava_autotest_scene13 = PAL_LavaHasCommandLineFlag("autotest_scene13");
   g_lava_autotest_scene9 = PAL_LavaHasCommandLineFlag("autotest_scene9");
   g_lava_autotest_hooks = PAL_LavaHasCommandLineFlag("autotest_hooks");
   g_lava_autotest_door = PAL_LavaHasCommandLineFlag("autotest_door");
   g_lava_autotest_hall = PAL_LavaHasCommandLineFlag("autotest_hall");
   g_lava_autotest_kitchen = PAL_LavaHasCommandLineFlag("autotest_kitchen");
   g_lava_autotest_load = PAL_LavaHasCommandLineFlag("autotest_load");
   g_lava_autotest_menu_load = PAL_LavaHasCommandLineFlag("autotest_menu_load5");
   if (g_lava_autotest_menu_load)
   {
      g_lava_autotest_load = 1;
      g_lava_autotest_load_params[0] = 5;
   }
   g_lava_autotest_status = PAL_LavaHasCommandLineFlag("autotest_status");
   g_lava_autotest_bsilence = PAL_LavaHasCommandLineFlag("autotest_bsilence");
   g_lava_autotest_bsleep = PAL_LavaHasCommandLineFlag("autotest_bsleep");
   g_lava_autotest_battle = PAL_LavaHasCommandLineFlag("autotest_battle");
   g_lava_autotest_fengshen = PAL_LavaHasCommandLineFlag("autotest_fengshen");
   g_lava_autotest_xueyao = PAL_LavaHasCommandLineFlag("autotest_xueyao");
   g_lava_autotest_op48 = PAL_LavaHasCommandLineFlag("autotest_op48");
   if (g_lava_autotest_fengshen)
   {
      g_lava_autotest_battle = 1;
   }
   if (g_lava_autotest_xueyao)
   {
      g_lava_autotest_battle = 1;
   }
   if (PAL_LavaHasCommandLineFlag("autotest_load2"))
   {
      g_lava_autotest_load = 1;
      g_lava_autotest_load_params[0] = 2;
   }
   else if (PAL_LavaHasCommandLineFlag("autotest_load3"))
   {
      g_lava_autotest_load = 1;
      g_lava_autotest_load_params[0] = 3;
   }
   else if (PAL_LavaHasCommandLineFlag("autotest_load4"))
   {
      g_lava_autotest_load = 1;
      g_lava_autotest_load_params[0] = 4;
   }
   else if (PAL_LavaHasCommandLineFlag("autotest_load5"))
   {
      g_lava_autotest_load = 1;
      g_lava_autotest_load_params[0] = 5;
   }
   else
   {
      g_lava_autotest_load_params[0] = 1;
   }
   if (g_lava_autotest_bsilence || g_lava_autotest_bsleep)
   {
      g_lava_autotest_load = 1;
      g_lava_autotest_load_params[0] = 5;
   }
   g_lava_autotest_intro = PAL_LavaHasCommandLineFlag("autotest_intro");
   g_lava_autotest_xianling = PAL_LavaHasCommandLineFlag("autotest_xianling");
   g_lava_autotest_obj204 = PAL_LavaHasCommandLineFlag("autotest_obj204");
   g_lava_autotest_input = PAL_LavaHasCommandLineFlag("autotest_input");
}

static void PAL_LavaZeroWords(WORD *buf, int count)
{
   int i;

   for (i = 0; i < count; i++)
   {
      buf[i] = 0;
   }
}

static void PAL_LavaDumpInventory(void)
{
   int i;
   int shown;

   shown = 0;
   printf("[LAVA][INV] dump begin\n");
   for (i = 0; i < 256; i++)
   {
      if (g_lava_inventory_item[i] != 0 && g_lava_inventory_amount[i] != 0)
      {
         printf("[LAVA][INV] slot=%d item=%d amount=%d\n",
            i,
            g_lava_inventory_item[i],
            g_lava_inventory_amount[i]);
          shown = 1;
      }
   }
   if (!shown)
   {
      printf("[LAVA][INV] empty\n");
   }
}

static int PAL_LavaSaveExists(int slot)
{
   FILE *fp;
   char *path;

   if (slot <= 0 || slot > 5)
   {
      return 0;
   }

   path = "5.rpg";
   if (slot == 1) path = "1.rpg";
   else if (slot == 2) path = "2.rpg";
   else if (slot == 3) path = "3.rpg";
   else if (slot == 4) path = "4.rpg";
   fp = fopen(path, "rb");
   if (fp == 0)
   {
      return 0;
   }

   fclose(fp);
   return 1;
}

static int PAL_LavaAnySaveExists(void)
{
   int slot;

   for (slot = 1; slot <= 5; slot++)
   {
      if (PAL_LavaSaveExists(slot))
      {
         return 1;
      }
   }

   return 0;
}

static int PAL_LavaGetSavedTimes(int slot)
{
   FILE *fp;
   char *path;
   int lo;
   int hi;

   if (slot <= 0 || slot > 5)
   {
      return 0;
   }

   path = "5.rpg";
   if (slot == 1) path = "1.rpg";
   else if (slot == 2) path = "2.rpg";
   else if (slot == 3) path = "3.rpg";
   else if (slot == 4) path = "4.rpg";

   fp = fopen(path, "rb");
   if (fp == 0)
   {
      return 0;
   }

   lo = getc(fp);
   hi = getc(fp);
   fclose(fp);
   if (lo < 0 || hi < 0)
   {
      return 0;
   }

   return lo | (hi << 8);
}

static int PAL_LavaMaxSavedTimes(void)
{
   int slot;
   int saved_times;
   int max_saved_times;

   max_saved_times = 0;
   for (slot = 1; slot <= 5; slot++)
   {
      saved_times = PAL_LavaGetSavedTimes(slot);
      if (saved_times > max_saved_times)
      {
         max_saved_times = saved_times;
      }
   }

   return max_saved_times;
}

static int PAL_LavaFirstSaveSlot(void)
{
   int slot;

   for (slot = 1; slot <= 5; slot++)
   {
      if (PAL_LavaSaveExists(slot))
      {
         return slot;
      }
   }

   return -1;
}

static int PAL_LavaReadSearchKey(void)
{
   int key;

   key = Inkey();
   if (key == KEY_ENTER)
   {
      return 1;
   }

   if (g_InputState.dwKeyPress & kKeySearch)
   {
      return 1;
   }

   return 0;
}

static long PAL_LavaFontGlyphOffsetForGB2312(int hi, int lo)
{
   long glyph_index;

   if (hi < LAVA_FONT_GB2312_FIRST || hi > 0xFE || lo < LAVA_FONT_GB2312_FIRST || lo > 0xFE)
   {
      return -1;
   }
   if (hi < 0xB0)
   {
      glyph_index = (hi - 0xA1) * LAVA_FONT_GB2312_SIDE + (lo - 0xA1);
   }
   else
   {
      glyph_index = (hi - 0xA7) * LAVA_FONT_GB2312_SIDE + (lo - 0xA1);
   }
   if (glyph_index < 0 || glyph_index >= g_lava_font_total_glyphs)
   {
      return -1;
   }
   return glyph_index * g_lava_font_glyph_bytes;
}

static long PAL_LavaFontGlyphOffsetForUnicode(long unicode)
{
   long left;
   long right;
   long mid;
   long entry_offset;
   long code;
   long glyph_index;

   left = 0;
   right = g_lava_font_map_count - 1;
   while (left <= right)
   {
      mid = left + (right - left) / 2;
      entry_offset = mid * 4;
      code = PAL_LavaReadU16((addr)g_lava_font_map, entry_offset);
      if (code == unicode)
      {
         glyph_index = PAL_LavaReadU16((addr)g_lava_font_map, entry_offset + 2);
         if (glyph_index >= 0 && glyph_index < g_lava_font_total_glyphs)
         {
            return glyph_index * g_lava_font_glyph_bytes;
         }
         return -1;
      }
      if (code < unicode)
      {
         left = mid + 1;
      }
      else
      {
         right = mid - 1;
      }
   }
   return -1;
}

static long PAL_LavaFontGlyphOffsetForEncodedPair(long hi, long lo)
{
   long left;
   long right;
   long mid;
   long pair;
   long entry_offset;
   long code;
   long glyph_index;

   pair = hi * 256 + lo;
   left = 0;
   right = g_lava_font_pair_map_count - 1;
   while (left <= right)
   {
      mid = left + (right - left) / 2;
      entry_offset = mid * 4;
      code = PAL_LavaReadU16((addr)g_lava_font_pair_map, entry_offset);
      if (code == pair)
      {
         glyph_index = PAL_LavaReadU16((addr)g_lava_font_pair_map, entry_offset + 2);
         if (glyph_index >= 0 && glyph_index < g_lava_font_total_glyphs)
         {
            return glyph_index * g_lava_font_glyph_bytes;
         }
         return -1;
      }
      if (code < pair)
      {
         left = mid + 1;
      }
      else
      {
         right = mid - 1;
      }
   }
   return -1;
}

static int PAL_LavaDecodeUtf8Char(char *text, long offset, long *unicode, int *consumed)
{
   int c0;
   int c1;
   int c2;

   c0 = PAL_U8(text[offset]);
   c1 = PAL_U8(text[offset + 1]);
   c2 = PAL_U8(text[offset + 2]);
   if (c0 >= 0xE0 && c0 <= 0xEF && c1 >= 0x80 && c1 <= 0xBF && c2 >= 0x80 && c2 <= 0xBF)
   {
      *unicode = ((c0 & 0x0F) << 12) | ((c1 & 0x3F) << 6) | (c2 & 0x3F);
      *consumed = 3;
      return 1;
   }
   if (c0 >= 0xC0 && c0 <= 0xDF && c1 >= 0x80 && c1 <= 0xBF)
   {
      *unicode = ((c0 & 0x1F) << 6) | (c1 & 0x3F);
      *consumed = 2;
      return 1;
   }
   return 0;
}

static void PAL_LavaDrawFontGlyph(SDL_Surface *surface, int x, int y, addr glyph, int width, int height, int color)
{
   int row;
   int bit;
   int byte0;
   int byte1;
   int px;
   int py;
   char *dst;
   char *glyph_data;
   int glyph_row;
   int draw_direct;
   char pixel[1];

   if (surface == 0 || surface->pixels == 0 || glyph == 0)
   {
      return;
   }
   glyph_data = (char *)glyph;
   draw_direct = (surface == gpScreen && g_lava_direct_screen && g_lava_text_batch_depth == 0);
   pixel[0] = (char)color;
   for (row = 0; row < height; row++)
   {
      py = y + row;
      if (py < 0 || py >= surface->h)
      {
         continue;
      }
      glyph_row = row;
      if (width > 8)
      {
         glyph_row = row * 2;
      }
      byte0 = PAL_U8(glyph_data[glyph_row]);
      byte1 = 0;
      if (width > 8)
      {
         byte1 = PAL_U8(glyph_data[glyph_row + 1]);
      }
      dst = (char *)surface->pixels + py * surface->pitch;
      for (bit = 0; bit < width; bit++)
      {
         px = x + bit;
         if (px < 0 || px >= surface->w)
         {
            continue;
         }
         if (bit < 8)
         {
            if (byte0 & (1 << (7 - bit)))
            {
               dst[px] = (char)color;
               if (draw_direct)
               {
                  WriteBlock(px, py, 1, 1, 1, (addr)pixel);
               }
            }
         }
         else
         {
            if (byte1 & (1 << (15 - bit)))
            {
               dst[px] = (char)color;
               if (draw_direct)
               {
                  WriteBlock(px, py, 1, 1, 1, (addr)pixel);
               }
            }
         }
      }
   }
}

static void PAL_LavaTextOutToSurfaceEx(SDL_Surface *surface, int x, int y, char *text, int color, int small_font, int gbk_first)
{
   int i;
   int c;
   int lo;
   long unicode;
   int consumed;
   int cx;
   long glyph_offset;
   addr glyph;

   if (PAL_LavaLoadFontAsset(small_font) != 0)
   {
      return;
   }
   if (text == 0 || g_lava_font_loaded == 0 || g_lava_fpFONT == 0)
   {
      return;
   }
   i = 0;
   cx = x;
   while (text[i] != 0)
   {
      c = PAL_U8(text[i]);
      glyph = 0;
      if (c < 128)
      {
         if (c >= 32)
         {
            glyph = (addr)(g_lava_font_ascii + (c - 32) * g_lava_font_ascii_stride);
            PAL_LavaDrawFontGlyph(surface, cx, y, glyph, g_lava_font_ascii_width, g_lava_font_height, color);
         }
         cx += g_lava_font_ascii_width;
         i++;
      }
      else
      {
         glyph_offset = -1;
         unicode = 0;
         consumed = 0;
         if (gbk_first && text[i + 1] != 0)
         {
            lo = PAL_U8(text[i + 1]);
            glyph_offset = PAL_LavaFontGlyphOffsetForGB2312(c, lo);
            if (glyph_offset < 0)
            {
               glyph_offset = PAL_LavaFontGlyphOffsetForEncodedPair(c, lo);
            }
            if (glyph_offset >= 0)
            {
               consumed = 2;
            }
         }
         if (glyph_offset < 0 && PAL_LavaDecodeUtf8Char(text, i, &unicode, &consumed))
         {
            glyph_offset = PAL_LavaFontGlyphOffsetForUnicode(unicode);
            if (glyph_offset < 0 && consumed == 2)
            {
               lo = PAL_U8(text[i + 1]);
               glyph_offset = PAL_LavaFontGlyphOffsetForGB2312(c, lo);
               if (glyph_offset < 0)
               {
                  glyph_offset = PAL_LavaFontGlyphOffsetForEncodedPair(c, lo);
               }
            }
         }
         else if (glyph_offset < 0 && text[i + 1] != 0)
         {
            lo = PAL_U8(text[i + 1]);
            glyph_offset = PAL_LavaFontGlyphOffsetForGB2312(c, lo);
            if (glyph_offset < 0)
            {
               glyph_offset = PAL_LavaFontGlyphOffsetForEncodedPair(c, lo);
            }
            consumed = 2;
         }
         if (consumed > 0)
         {
            i += consumed;
         }
         else
         {
            i++;
         }
         if (glyph_offset >= 0)
         {
            fseek(g_lava_fpFONT, g_lava_font_gb_offset + glyph_offset, SEEK_SET);
            if (fread((addr)g_lava_font_glyph_buf, 1, g_lava_font_glyph_bytes, g_lava_fpFONT) == g_lava_font_glyph_bytes)
            {
               glyph = (addr)g_lava_font_glyph_buf;
               PAL_LavaDrawFontGlyph(surface, cx, y, glyph, g_lava_font_gb_width, g_lava_font_height, color);
            }
         }
         cx += g_lava_font_gb_width;
      }
   }
}

static void PAL_LavaTextOutToSurface(SDL_Surface *surface, int x, int y, char *text, int color, int small_font)
{
   PAL_LavaTextOutToSurfaceEx(surface, x, y, text, color, small_font, 0);
}

static int PAL_LavaLoadFontAsset(int small_font)
{
   FILE *fp;
   char header[16];
   char *filename;
   long ascii_offset;
   long gb_offset;
   long total_glyphs;
   long glyph_bytes;
   long map_count;
   long pair_count;
   long map_bytes;
   long pair_bytes;
   long ascii_bytes;
   long ascii_stride;

   if (g_lava_font_loaded && g_lava_font_small == small_font)
   {
      return 0;
   }
   if (g_lava_fpFONT != 0)
   {
      fclose(g_lava_fpFONT);
      g_lava_fpFONT = 0;
   }
   g_lava_font_loaded = 0;
   g_lava_font_map_count = 0;
   g_lava_font_pair_map_count = 0;
   filename = "FONT_GB2312.DAT";
   if (small_font)
   {
      filename = "FONT_GB2312_SMALL.DAT";
   }
   fp = UTIL_OpenRequiredFile(filename);
   if (fp == 0)
   {
      return -1;
   }
   if (fread((addr)header, 1, 16, fp) != 16)
   {
      fclose(fp);
      return -1;
   }
   if (header[0] != 'L' || header[1] != 'F' || header[2] != 'G' || header[3] != '0')
   {
      fclose(fp);
      return -1;
   }
   total_glyphs = PAL_LavaReadU16((addr)header, 4);
   glyph_bytes = PAL_LavaReadU16((addr)header, 6);
   ascii_offset = PAL_LavaReadU16((addr)header, 8);
   gb_offset = PAL_LavaReadU16((addr)header, 10);
   map_count = PAL_LavaReadU16((addr)header, 12);
   pair_count = PAL_LavaReadU16((addr)header, 14);
   if (glyph_bytes != 24 && glyph_bytes != 32)
   {
      fclose(fp);
      return -1;
   }
   ascii_stride = glyph_bytes / 2;
   ascii_bytes = 96 * ascii_stride;
   if (glyph_bytes > LAVA_FONT_GLYPH_BYTES_MAX || ascii_bytes > LAVA_FONT_ASCII_BYTES_MAX)
   {
      fclose(fp);
      return -1;
   }
   if (map_count > LAVA_FONT_MAP_MAX)
   {
      map_count = LAVA_FONT_MAP_MAX;
   }
   if (pair_count > LAVA_FONT_MAP_MAX)
   {
      pair_count = LAVA_FONT_MAP_MAX;
   }
   fseek(fp, ascii_offset, SEEK_SET);
   if (fread((addr)g_lava_font_ascii, 1, ascii_bytes, fp) != ascii_bytes)
   {
      fclose(fp);
      return -1;
   }
   fseek(fp, gb_offset + total_glyphs * glyph_bytes, SEEK_SET);
   map_bytes = map_count * 4;
   if (fread((addr)g_lava_font_map, 1, map_bytes, fp) != map_bytes)
   {
      fclose(fp);
      return -1;
   }
   pair_bytes = pair_count * 4;
   if (fread((addr)g_lava_font_pair_map, 1, pair_bytes, fp) != pair_bytes)
   {
      fclose(fp);
      return -1;
   }
   g_lava_fpFONT = fp;
   g_lava_font_gb_offset = gb_offset;
   g_lava_font_total_glyphs = total_glyphs;
   g_lava_font_ascii_stride = (int)ascii_stride;
   g_lava_font_glyph_bytes = (int)glyph_bytes;
   g_lava_font_ascii_width = 8;
   g_lava_font_gb_width = 16;
   g_lava_font_height = 16;
   if (glyph_bytes == 24)
   {
      g_lava_font_ascii_width = 6;
      g_lava_font_gb_width = 12;
      g_lava_font_height = 12;
   }
   g_lava_font_small = small_font;
   g_lava_font_map_count = (int)map_count;
   g_lava_font_pair_map_count = (int)pair_count;
   g_lava_font_loaded = 1;
   return 0;
}

static void PAL_LavaDrawShadowTextEx(int x, int y, char *text, int color, int small_font)
{
   int text_w;
   int text_h;
   int i;
   int len;
   int c;
   int ascii_width;
   int gb_width;
   int font_height;

   ascii_width = 8;
   gb_width = 16;
   font_height = 16;
   if (small_font)
   {
      ascii_width = 6;
      gb_width = 12;
      font_height = 12;
   }
   len = 0;
   i = 0;
   while (text[i] != 0)
   {
      c = PAL_U8(text[i]);
      if (c < 128)
      {
         len += ascii_width;
         i++;
      }
#ifdef LAVA_NATIVE_COMPILED
      else if (c >= 0xE0 && text[i + 1] != 0 && text[i + 2] != 0)
      {
         len += gb_width;
         i += 3;
      }
      else if (c >= 0xC0 && text[i + 1] != 0)
      {
         len += gb_width;
         i += 2;
      }
#endif
      else
      {
         len += gb_width;
         i += 2;
      }
   }
   if (len <= 0)
   {
      len = ascii_width;
   }
   text_w = len;
   text_h = font_height;
   if (x + text_w > SCREEN_W)
   {
      text_w = SCREEN_W - x;
   }
   if (x < 0)
   {
      text_w += x;
      x = 0;
   }
   if (y + text_h > SCREEN_H)
   {
      text_h = SCREEN_H - y;
   }
   if (y < 0)
   {
      text_h += y;
      y = 0;
   }
   PAL_LavaTextOutToSurface(gpScreen, x, y, text, color, small_font);

   if (text_w > 0 && text_h > 0)
   {
      lava_present_current_screen();
     }
}

static void PAL_LavaDrawShadowText(int x, int y, char *text, int color)
{
   PAL_LavaDrawShadowTextEx(x, y, text, color, 0);
}

static void PAL_LavaDrawShadowTextSmall(int x, int y, char *text, int color)
{
   PAL_LavaDrawShadowTextEx(x, y, text, color, 1);
}

static int PAL_LavaTextWidthEx(char *text, int small_font)
{
   int i;
   int len;
   int c;
   int ascii_width;
   int gb_width;

   ascii_width = 8;
   gb_width = 16;
   if (small_font)
   {
      ascii_width = 6;
      gb_width = 12;
   }
   len = 0;
   i = 0;
   while (text != 0 && text[i] != 0)
   {
      c = PAL_U8(text[i]);
      if (c < 128)
      {
         len += ascii_width;
         i++;
      }
#ifdef LAVA_NATIVE_COMPILED
      else if (c >= 0xE0 && text[i + 1] != 0 && text[i + 2] != 0)
      {
         len += gb_width;
         i += 3;
      }
      else if (c >= 0xC0 && text[i + 1] != 0)
      {
         len += gb_width;
         i += 2;
      }
#endif
      else
      {
         len += gb_width;
         i += 2;
      }
   }
   return len;
}

static int PAL_LavaTextWidth(char *text)
{
   return PAL_LavaTextWidthEx(text, 0);
}

/* Right-align a small-font label so its right edge sits at right_x. */
static void PAL_LavaDrawShadowTextSmallRight(int right_x, int y, char *text, int color)
{
   int x;

   x = right_x - PAL_LavaTextWidthEx(text, 1);
   if (x < 0)
   {
      x = 0;
   }
   PAL_LavaDrawShadowTextSmall(x, y, text, color);
}

static int PAL_LavaNearestPaletteColor(int red, int green, int blue)
{
   int i;
   int best;
   long best_dist;
   long dr;
   long dg;
   long db;
   long dist;

   best = 1;
   best_dist = 200000;
   for (i = 1; i < 256; i++)
   {
      dr = (long)PAL_U8(g_lava_palette_cache[i].r) - (long)red;
      dg = (long)PAL_U8(g_lava_palette_cache[i].g) - (long)green;
      db = (long)PAL_U8(g_lava_palette_cache[i].b) - (long)blue;
      dist = dr * dr + dg * dg + db * db;
      if (dist < best_dist)
      {
         best_dist = dist;
         best = i;
      }
   }
   return best;
}

static void PAL_LavaPresent(void)
{
   lava_present_current_screen();
}

static void PAL_LavaDrawLoadSlotBox(int slot, int selected)
{
   int y0;

   y0 = LOAD_SLOT_BOX_Y(slot);
   PAL_LavaDrawSingleLineBox(LOAD_SLOT_BOX_X, y0, 6);
}

static void PAL_LavaDrawLoadSlotNumber(addr sprite, int value, int x, int y)
{
   int actual;
   int draw_x;
   int n;
   int i;

   if (sprite == 0)
   {
      PAL_LavaDrawNumberText(x - 8, y, value, 0xCF);
      return;
   }

   n = value;
   actual = 0;
   while (n > 0)
   {
      n /= 10;
      actual++;
   }
   if (actual <= 0)
   {
      actual = 1;
   }
   if (actual > 4)
   {
      actual = 4;
   }

   draw_x = x - 6;
   for (i = 0; i < actual; i++)
   {
      PAL_RLEBlitToSurface(PAL_SpriteGetFrame(sprite, 19 + value % 10),
         gpScreen, PAL_XY(draw_x, y));
      draw_x -= 6;
      value /= 10;
   }
}

static void PAL_LavaDrawLoadSlotText(int slot, int selected)
{
   int label_color;
   int selected_color;
   int saved_times;
   addr sprite;
   char *label;

   selected_color = PAL_LavaMenuSelectedColor();
   label_color = selected ? selected_color : MENUITEM_COLOR;
   sprite = PAL_LavaLoadUISprite();

   if (slot == 1) label = "进度一";
   else if (slot == 2) label = "进度二";
   else if (slot == 3) label = "进度三";
   else if (slot == 4) label = "进度四";
   else label = "进度五";

   PAL_LavaDrawShadowText(LOAD_SLOT_LABEL_X, LOAD_SLOT_LABEL_Y(slot), label, label_color);

   saved_times = PAL_LavaGetSavedTimes(slot);
   if (!PAL_LavaSaveExists(slot))
   {
      PAL_LavaDrawShadowText(240, LOAD_SLOT_LABEL_Y(slot), "空白", 0x1C);
      return;
   }

   PAL_LavaDrawLoadSlotNumber(sprite, saved_times, LOAD_SLOT_TIMES_X, LOAD_SLOT_TIMES_Y(slot));
   lava_present_current_screen();
}

static void PAL_LavaRestoreLoadSlotRow(int slot)
{
   int y;
   int h;
   int row;

   y = LOAD_SLOT_BOX_Y(slot);
   h = 38;
   if (y + h > SCREEN_H)
   {
      h = SCREEN_H - y;
   }
   if (h <= 0)
   {
      return;
   }

   for (row = 0; row < h; row++)
   {
      memcpy(g_screen_surface.pixels + (y + row) * g_screen_surface.pitch + LOAD_SLOT_PANEL_X,
         g_back_buf + (y + row) * SCREEN_W + LOAD_SLOT_PANEL_X,
         LOAD_SLOT_PANEL_W);
   }
}

static void PAL_LavaCacheCurrentSurface(void)
{
   int row;

   for (row = 0; row < SCREEN_H; row++)
   {
      memcpy(g_back_buf + row * SCREEN_W,
         g_screen_surface.pixels + row * g_screen_surface.pitch,
         SCREEN_W);
   }
}

static void PAL_LavaDrawLoadSlotSelectionChange(int old_slot, int selected_slot)
{
   if (old_slot == selected_slot)
   {
      return;
   }

   lava_begin_text_batch();
   PAL_LavaRestoreLoadSlotRow(old_slot);
   PAL_LavaDrawLoadSlotBox(old_slot, 0);
   PAL_LavaDrawLoadSlotText(old_slot, 0);

   PAL_LavaRestoreLoadSlotRow(selected_slot);
   PAL_LavaDrawLoadSlotBox(selected_slot, 1);
   PAL_LavaDrawLoadSlotText(selected_slot, 1);
   lava_end_text_batch();
   Refresh();
}

static void PAL_LavaDrawLoadSlotMenu(int selected_slot, int in_system_menu)
{
   int slot;

   if (in_system_menu)
   {
      PAL_LavaDrawSystemMenuFrameOnly();
   }
   else
   {
      PAL_LavaDrawOpeningMenuBackground();
   }

   for (slot = 1; slot <= 5; slot++)
   {
      PAL_LavaDrawLoadSlotBox(slot, slot == selected_slot);
   }
   PAL_LavaPresent();
   if (in_system_menu)
   {
      PAL_LavaCacheCurrentSurface();
      PAL_LavaDrawSystemMenuLabels();
   }
   else
   {
      PAL_LavaDrawOpeningMenuText(MENU_TEXT_X, MENU_TEXT_Y0, 0, MENUITEM_COLOR);
      PAL_LavaDrawOpeningMenuText(MENU_TEXT_X, MENU_TEXT_Y1, 1, PAL_LavaMenuSelectedColor());
   }
   PAL_LavaCacheCurrentSurface();
    for (slot = 1; slot <= 5; slot++)
    {
       PAL_LavaDrawLoadSlotText(slot, slot == selected_slot);
    }
    Refresh();
}

static int PAL_LavaChooseLoadSlot(int in_system_menu)
{
   int selected_slot;
   int old_slot;
   int dirty;

   selected_slot = PAL_LavaFirstSaveSlot();
   if (selected_slot < 0)
   {
      return -1;
   }

   PAL_ClearKeyState();
   dirty = 1;
   while (TRUE)
   {
      if (dirty)
      {
         PAL_LavaDrawLoadSlotMenu(selected_slot, in_system_menu);
         dirty = 0;
      }
      PAL_ProcessEvent();

      if (g_InputState.dwKeyPress & (kKeyDown | kKeyRight))
      {
         old_slot = selected_slot;
         selected_slot++;
         if (selected_slot > 5)
         {
            selected_slot = 1;
         }
         PAL_ClearKeyState();
         PAL_LavaDrawLoadSlotSelectionChange(old_slot, selected_slot);
      }
      else if (g_InputState.dwKeyPress & (kKeyUp | kKeyLeft))
      {
         old_slot = selected_slot;
         selected_slot--;
         if (selected_slot < 1)
         {
            selected_slot = 5;
         }
         PAL_ClearKeyState();
         PAL_LavaDrawLoadSlotSelectionChange(old_slot, selected_slot);
      }
      else if (g_InputState.dwKeyPress & kKeyMenu)
      {
         PAL_ClearKeyState();
         return -1;
      }
      else if (PAL_LavaReadSearchKey())
      {
         PAL_ClearKeyState();
         if (PAL_LavaSaveExists(selected_slot))
         {
            return selected_slot;
         }
      }

      Delay(50);
   }
}

static int PAL_LavaChooseSaveSlot(void)
{
   int selected_slot;
   int old_slot;
   int dirty;

   selected_slot = g_lava_gpGlobals.bCurrentSaveSlot;
   if (selected_slot < 1 || selected_slot > 5)
   {
      selected_slot = 1;
   }

   PAL_ClearKeyState();
   dirty = 1;
   while (TRUE)
   {
      if (dirty)
      {
         PAL_LavaDrawLoadSlotMenu(selected_slot, 1);
         dirty = 0;
      }
      PAL_ProcessEvent();

      if (g_InputState.dwKeyPress & (kKeyDown | kKeyRight))
      {
         old_slot = selected_slot;
         selected_slot++;
         if (selected_slot > 5)
         {
            selected_slot = 1;
         }
         PAL_ClearKeyState();
         PAL_LavaDrawLoadSlotSelectionChange(old_slot, selected_slot);
      }
      else if (g_InputState.dwKeyPress & (kKeyUp | kKeyLeft))
      {
         old_slot = selected_slot;
         selected_slot--;
         if (selected_slot < 1)
         {
            selected_slot = 5;
         }
         PAL_ClearKeyState();
         PAL_LavaDrawLoadSlotSelectionChange(old_slot, selected_slot);
      }
      else if (g_InputState.dwKeyPress & kKeyMenu)
      {
         PAL_ClearKeyState();
         return -1;
      }
      else if (PAL_LavaReadSearchKey())
      {
         PAL_ClearKeyState();
         return selected_slot;
      }

      Delay(50);
   }
}

static long PAL_LavaReadU16(addr data, long offset)
{
   char *p;

   p = (char *)data;
   return (long)PAL_U8(p[offset]) | ((long)PAL_U8(p[offset + 1]) << 8);
}

static long PAL_LavaReadU32(addr data, long offset)
{
   char *p;

   p = (char *)data;
   return (long)PAL_U8(p[offset]) |
      ((long)PAL_U8(p[offset + 1]) << 8) |
      ((long)PAL_U8(p[offset + 2]) << 16) |
      ((long)PAL_U8(p[offset + 3]) << 24);
}

static int PAL_LavaReadS16(addr data, long offset)
{
   long value;

   value = PAL_LavaReadU16(data, offset);
   if (value >= 32768) value -= 65536;
   return (int)value;
}

static void PAL_LavaWriteU16(addr data, long offset, int value)
{
   char *p;

   p = (char *)data;
   p[offset] = (char)(value & 255);
   p[offset + 1] = (char)((value >> 8) & 255);
}

static void PAL_LavaWriteS16(addr data, long offset, int value)
{
   if (value < 0)
   {
      value += 65536;
   }
   PAL_LavaWriteU16(data, offset, value);
}

static void PAL_LavaZeroBuffer(char *buf, int count)
{
   int i;

   for (i = 0; i < count; i++)
   {
      buf[i] = 0;
   }
}

static int PAL_LavaRoleMagicOffset(int role, int magic_slot)
{
   return 32 * 6 * 2 + magic_slot * 6 * 2 + role * 2;
}

static void PAL_LavaAddRoleMagicToDataBuf(int role, int magic_object_id)
{
   int existing_magic;
   int magic_offset;
   int magic_slot;

   if (role < 0 || role >= 6 || magic_object_id <= 0)
   {
      return;
   }

   for (magic_slot = 0; magic_slot < 32; magic_slot++)
   {
      magic_offset = PAL_LavaRoleMagicOffset(role, magic_slot);
      existing_magic = PAL_LavaReadU16((addr)g_lava_data_buf, magic_offset);
      if (existing_magic == magic_object_id)
      {
         return;
      }
      if (existing_magic == 0)
      {
         PAL_LavaWriteU16((addr)g_lava_data_buf, magic_offset, magic_object_id);
         return;
      }
   }
}

static void PAL_LavaRepairLoadedRoleMagics(void)
{
   int i;

   if (g_lava_scene_num < 2)
   {
      return;
   }

   for (i = 0; i < g_lava_party_count && i < 3; i++)
   {
      if (g_lava_party_role[i] == 0)
      {
         PAL_LavaAddRoleMagicToDataBuf(0, 345);
      }
   }
}

static int PAL_LavaLoadRoleMagicSaveData(FILE *fp)
{
   addr magic_data;
   int has_magic_data;
   int i;

   if (fp == 0)
   {
      return -1;
   }

   magic_data = (addr)(g_lava_data_buf + 32 * 6 * 2);
   has_magic_data = 0;
   for (i = 0; i < 384; i++)
   {
      if (PAL_U8(magic_data[i]) != 0)
      {
         has_magic_data = 1;
         break;
      }
   }

   if (!has_magic_data)
   {
      PAL_LavaRepairLoadedRoleMagics();
   }

   return 0;
}

static int PAL_LavaWriteZeros(FILE *fp, int count)
{
   char zero_buf[64];
   int chunk;

   PAL_LavaZeroBuffer(zero_buf, 64);
   while (count > 0)
   {
      chunk = count;
      if (chunk > 64)
      {
         chunk = 64;
      }
      if (fwrite((addr)zero_buf, 1, chunk, fp) != chunk)
      {
         return -1;
      }
      count -= chunk;
   }

   return 0;
}

static int PAL_LavaWriteZerosLong(FILE *fp, long count)
{
   char zero_buf[64];
   int chunk;

   PAL_LavaZeroBuffer(zero_buf, 64);
   while (count > 0)
   {
      chunk = (count > 64) ? 64 : (int)count;
      if (fwrite((addr)zero_buf, 1, chunk, fp) != chunk)
      {
         return -1;
      }
      count -= chunk;
   }

   return 0;
}

static int PAL_LavaWriteBytesLong(FILE *fp, addr data, long count)
{
   int chunk;

   while (count > 0)
   {
      chunk = (count > 30000) ? 30000 : (int)count;
      if (fwrite(data, 1, chunk, fp) != chunk)
      {
         return -1;
      }
      data += chunk;
      count -= chunk;
   }

   return 0;
}

static addr PAL_LavaSceneEventData(int object_id)
{
   if (object_id <= 0 || object_id > g_lava_scene_event_count)
   {
      return 0;
   }

   return (addr)(g_lava_scene_event_raw + (object_id - 1) * 32);
}

static void PAL_LavaSetRoleSpriteNum(int role, int sprite_num)
{
   if (role < 0 || role >= 6)
   {
      return;
   }

   PAL_LavaWriteU16((addr)g_lava_data_buf, 12 * 2 + role * 2, sprite_num);
}

static void PAL_LavaRebuildSceneAutoscriptRuntime(void)
{
   int object_id;

   PAL_LavaZeroWords(g_lava_autoscript_pc, 5332);
   PAL_LavaZeroWords(g_lava_autoscript_idle, 5332);

   for (object_id = g_lava_scene_event_first; object_id <= g_lava_scene_event_last; object_id++)
   {
      addr evt;

      evt = PAL_LavaSceneEventData(object_id);
      if (evt == 0)
      {
         continue;
      }

      g_lava_autoscript_pc[object_id - 1] = PAL_LavaReadU16(evt, 10);
      g_lava_autoscript_idle[object_id - 1] = PAL_LavaReadU16(evt, 30);
   }
}

static void PAL_LavaStoreAutoscriptRuntime(addr evt, int object_id)
{
   if (evt == 0 || object_id <= 0 || object_id > 5332)
   {
      return;
   }

   PAL_LavaWriteU16(evt, 10, g_lava_autoscript_pc[object_id - 1]);
   PAL_LavaWriteU16(evt, 30, g_lava_autoscript_idle[object_id - 1]);
}

static void PAL_LavaTickSceneEventObjects(void)
{
   int object_id;

   for (object_id = g_lava_scene_event_first; object_id <= g_lava_scene_event_last; object_id++)
   {
      addr evt;
      int vanish_time;
      int state;

      evt = PAL_LavaSceneEventData(object_id);
      if (evt == 0)
      {
         continue;
      }

      vanish_time = PAL_LavaReadS16(evt, 0);
      if (vanish_time != 0)
      {
         vanish_time += (vanish_time < 0) ? 1 : -1;
         PAL_LavaWriteS16(evt, 0, vanish_time);
         continue;
      }

      state = PAL_LavaReadS16(evt, 12);
      if (state < 0)
      {
         int object_x;
         int object_y;

         object_x = PAL_LavaReadU16(evt, 2);
         object_y = PAL_LavaReadU16(evt, 4);
         if (object_x < g_lava_view_x || object_x > g_lava_view_x + 320 ||
             object_y < g_lava_view_y || object_y > g_lava_view_y + 320)
         {
            PAL_LavaWriteS16(evt, 12, -state);
            PAL_LavaWriteU16(evt, 22, 0);
         }
      }
   }
}

static void PAL_LavaInitSceneStorageOffsets(void)
{
   FILE *fp;

   fp = UTIL_OpenRequiredFile("SSS.MKF");
   if (fp == 0)
   {
      return;
   }

   g_lava_scene_event_offset = PAL_LavaMKFChunkOffset(fp, 0);
   g_lava_scene_script_offset = PAL_LavaMKFChunkOffset(fp, 4);
   fclose(fp);
}

static WORD PAL_LavaReadSceneDefaultEnterScript(int scene_num)
{
   FILE *fp;
   long scene_size;
   int scene_offset;

   if (scene_num <= 0)
   {
      return 0;
   }

   fp = UTIL_OpenRequiredFile("SSS.MKF");
   if (fp == 0)
   {
      return 0;
   }

   scene_size = PAL_MKFReadChunk((addr)g_lava_mkf_buf, 65536, 1, fp);
   fclose(fp);
   scene_offset = (scene_num - 1) * 8;
   if (scene_size < scene_offset + 4)
   {
      return 0;
   }

   return (WORD)PAL_LavaReadU16((addr)g_lava_mkf_buf, scene_offset + 2);
}

static long PAL_LavaSceneEventBytes(void)
{
   FILE *fp;
   long event_bytes_long;
   long event_bytes_max;

   fp = UTIL_OpenRequiredFile("SSS.MKF");
   if (fp == 0)
   {
      return 0;
   }

   event_bytes_long = PAL_MKFGetChunkSize(0, fp);
   fclose(fp);
   event_bytes_max = (long)5332 * (long)32;
   if (event_bytes_long < 0)
   {
      return 0;
   }
   if (event_bytes_long > event_bytes_max)
   {
      return event_bytes_max;
   }
   return event_bytes_long;
}

static int PAL_LavaReadScriptEntry(long script_index, addr entry)
{
   FILE *fp;
   long script_offset;
   int ok;

   fp = UTIL_OpenRequiredFile("SSS.MKF");
   if (fp == 0) return 0;

   script_offset = PAL_LavaMKFChunkOffset(fp, 4);
   ok = 0;
   if (script_offset >= 0)
   {
      fseek(fp, script_offset + script_index * 8, SEEK_SET);
      if (fread(entry, 1, 8, fp) == 8) ok = 1;
   }
   fclose(fp);
   return ok;
}

static void PAL_LavaRepairSceneRuntimeScripts(void)
{
   WORD scene4_default_enter;

   /*
    * Scene 4 (Suzhou inn exterior) defaults to enter=3149 in SSS.MKF.
    * Older Lava runs could persist the post-enter continuation 3151 back
    * into saves, which later replays the Lin Yueru cutscene on re-entry.
    */
   scene4_default_enter = PAL_LavaReadSceneDefaultEnterScript(4);
   if (scene4_default_enter != 0 &&
       g_lava_scene_runtime_enter[3] == (WORD)3151)
   {
      printf("[LAVA][LOAD] repair scene4 enter %d -> %d\n",
         g_lava_scene_runtime_enter[3], scene4_default_enter);
      g_lava_scene_runtime_enter[3] = scene4_default_enter;
      if (g_lava_scene_hook_enter[3] == (WORD)3151)
      {
         g_lava_scene_hook_enter[3] = scene4_default_enter;
      }
   }
}

static int PAL_LavaSaveGame(int slot)
{
   FILE *fp;
   char *path;
   char header[44];
   char party_buf[10];
   char inv_buf[6];
   char scene_buf[8];
   int i;
   int saved_times;
   int relative_x;
   int relative_y;
   long event_bytes_long;
   long scene_table_size;
   long object_bytes_long;
   FILE *fp_sss;

   if (slot <= 0 || slot > 5)
   {
      return -1;
   }

   path = "5.rpg";
   if (slot == 1) path = "1.rpg";
   else if (slot == 2) path = "2.rpg";
   else if (slot == 3) path = "3.rpg";
   else if (slot == 4) path = "4.rpg";

   fp = fopen(path, "wb");
   if (fp == 0)
   {
      return -1;
   }

   saved_times = PAL_LavaMaxSavedTimes() + 1;
   PAL_LavaZeroBuffer(header, 44);
   PAL_LavaWriteU16((addr)header, 0, saved_times);
   PAL_LavaWriteU16((addr)header, 2, g_lava_view_x);
   PAL_LavaWriteU16((addr)header, 4, g_lava_view_y);
   PAL_LavaWriteU16((addr)header, 6, g_lava_party_count - 1);
   PAL_LavaWriteU16((addr)header, 8, g_lava_scene_num);
   PAL_LavaWriteU16((addr)header, 12, g_lava_party_direction);
   PAL_LavaWriteU16((addr)header, 18, g_lava_num_battle_field);
   if (fwrite((addr)header, 1, 44, fp) != 44)
   {
      fclose(fp);
      return -1;
   }

   for (i = 0; i < 5; i++)
   {
      PAL_LavaZeroBuffer(party_buf, 10);
      if (i < g_lava_party_count && i < 3)
      {
         PAL_LavaWriteU16((addr)party_buf, 0, g_lava_party_role[i]);
      }
      if (i == 0)
      {
         relative_x = g_lava_party_x - g_lava_view_x;
         relative_y = g_lava_party_y - g_lava_view_y;
         PAL_LavaWriteU16((addr)party_buf, 2, relative_x);
         PAL_LavaWriteU16((addr)party_buf, 4, relative_y);
         PAL_LavaWriteU16((addr)party_buf, 6, g_lava_party_frame);
      }
      if (fwrite((addr)party_buf, 1, 10, fp) != 10)
      {
         fclose(fp);
         return -1;
      }
   }

   if (PAL_LavaWriteZeros(fp, 508 - 94) != 0)
   {
      fclose(fp);
      return -1;
   }
   if (fwrite((addr)g_lava_data_buf, 1, 900, fp) != 900)
   {
      fclose(fp);
      return -1;
   }
   if (PAL_LavaWriteZeros(fp, 1728 - 508 - 900) != 0)
   {
      fclose(fp);
      return -1;
   }

   for (i = 0; i < 256; i++)
   {
      PAL_LavaZeroBuffer(inv_buf, 6);
      PAL_LavaWriteU16((addr)inv_buf, 0, g_lava_inventory_item[i]);
      PAL_LavaWriteU16((addr)inv_buf, 2, g_lava_inventory_amount[i]);
      if (fwrite((addr)inv_buf, 1, 6, fp) != 6)
      {
         fclose(fp);
         return -1;
      }
   }

   scene_table_size = 0;
   fp_sss = UTIL_OpenRequiredFile("SSS.MKF");
   if (fp_sss != 0)
   {
      scene_table_size = PAL_MKFReadChunk((addr)g_lava_mkf_buf, 65536, 1, fp_sss);
      fclose(fp_sss);
   }

   for (i = 0; i < 300; i++)
   {
      int scene_offset;

      PAL_LavaZeroBuffer(scene_buf, 8);
      scene_offset = i * 8;
      if (scene_table_size >= scene_offset + 8)
      {
         PAL_LavaWriteU16((addr)scene_buf, 0,
            PAL_LavaReadU16((addr)g_lava_mkf_buf, scene_offset));
         PAL_LavaWriteU16((addr)scene_buf, 6,
            PAL_LavaReadU16((addr)g_lava_mkf_buf, scene_offset + 6));
      }
      PAL_LavaWriteU16((addr)scene_buf, 2, g_lava_scene_runtime_enter[i]);
      PAL_LavaWriteU16((addr)scene_buf, 4, g_lava_scene_runtime_teleport[i]);
      if (fwrite((addr)scene_buf, 1, 8, fp) != 8)
      {
         fclose(fp);
         return -1;
      }
   }

   if (g_lava_object_data_loaded == 0)
   {
      PAL_LavaReadObjectField(1, 0);
   }
   object_bytes_long = g_lava_object_data_size;
   if (object_bytes_long > 7200)
   {
      object_bytes_long = 7200;
   }
   if (object_bytes_long > 0 &&
       PAL_LavaWriteBytesLong(fp, (addr)g_lava_object_data, object_bytes_long) != 0)
   {
      fclose(fp);
      return -1;
   }
   if (PAL_LavaWriteZeros(fp, 7200 - (int)object_bytes_long) != 0)
   {
      fclose(fp);
      return -1;
   }

   event_bytes_long = PAL_LavaSceneEventBytes();
   if (PAL_LavaWriteBytesLong(fp, (addr)g_lava_scene_event_raw, event_bytes_long) != 0)
   {
      fclose(fp);
      return -1;
   }

   fclose(fp);
   g_lava_gpGlobals.bCurrentSaveSlot = slot;
   printf("[LAVA][SAVE] slot=%d times=%d scene=%d view=(%d,%d) party=(%d,%d)\n",
      slot, saved_times, g_lava_scene_num, g_lava_view_x, g_lava_view_y, g_lava_party_x, g_lava_party_y);
   return 0;
}

static int PAL_LavaLoadSavedGame(int slot)
{
    FILE *fp;
    int i;
    char *path;
    int saved_scene;
    int saved_view_x;
    int saved_view_y;
    int saved_party_dir;
    int saved_party_x;
    int saved_party_y;
    int saved_battle_field;
    int saved_party_count;
    long event_bytes_long;
    int load_visible;
   int load_obj;
   addr evt;
   char header[44];
    char party_buf[10];
    char role_word[2];
    char inv_buf[6];
   char scene_buf[8];

   path = "5.rpg";
   if (slot == 1) path = "1.rpg";
   else if (slot == 2) path = "2.rpg";
   else if (slot == 3) path = "3.rpg";
   else if (slot == 4) path = "4.rpg";

   fp = fopen(path, "rb");
   if (fp == 0)
   {
      return -1;
   }

   if (fread((addr)header, 1, 44, fp) != 44)
   {
      goto load_failed;
   }

   saved_view_x = PAL_U8(header[2]) | (PAL_U8(header[3]) << 8);
   saved_view_y = PAL_U8(header[4]) | (PAL_U8(header[5]) << 8);
   saved_party_count = (PAL_U8(header[6]) | (PAL_U8(header[7]) << 8)) + 1;
   saved_scene = PAL_U8(header[8]) | (PAL_U8(header[9]) << 8);
   saved_party_dir = PAL_U8(header[12]) | (PAL_U8(header[13]) << 8);
   saved_battle_field = PAL_U8(header[18]) | (PAL_U8(header[19]) << 8);

   g_lava_gpGlobals.bCurrentSaveSlot = slot;
    g_lava_scene_num = saved_scene;
    g_lava_view_x = saved_view_x;
    g_lava_view_y = saved_view_y;
   g_lava_party_direction = saved_party_dir;
   g_lava_num_battle_field = saved_battle_field;
   g_lava_follower_count = 0;
   g_lava_party_count = saved_party_count;
   if (g_lava_party_count < 1) g_lava_party_count = 1;
   if (g_lava_party_count > 3) g_lava_party_count = 3;

    for (i = 0; i < 5; i++)
   {
      if (fread((addr)party_buf, 1, 10, fp) != 10)
      {
         goto load_failed;
      }
      if (i < 3)
      {
         g_lava_party_role[i] = PAL_U8(party_buf[0]) | (PAL_U8(party_buf[1]) << 8);
      }
       if (i == 0)
       {
          g_lava_party_x = (PAL_U8(party_buf[2]) | (PAL_U8(party_buf[3]) << 8)) + g_lava_view_x;
          g_lava_party_y = (PAL_U8(party_buf[4]) | (PAL_U8(party_buf[5]) << 8)) + g_lava_view_y;
          g_lava_party_frame = PAL_U8(party_buf[6]) | (PAL_U8(party_buf[7]) << 8);
        }
     }

    // 读取角色属性数据（900字节）- 包含等级、HP、MP、攻击、防御等所有战斗属性
    if (!PAL_LavaFseekOK(fp, 508, SEEK_SET))
    {
       goto load_failed;
    }
    if (fread((addr)g_lava_data_buf, 1, 900, fp) != 900)
    {
       goto load_failed;
    }

    // 调试：打印加载后的角色核心数据
    printf("[LAVA][LOAD] Player roles data loaded from save file:\n");
    for (i = 0; i < g_lava_party_count && i < 3; i++)
    {
       printf("[LAVA][LOAD] role=%d level=%d hp=%d/%d mp=%d/%d atk=%d mag=%d def=%d dex=%d\n",
          g_lava_party_role[i],
          PAL_LavaRoleWordByArray(6, g_lava_party_role[i]),   // 等级
          PAL_LavaRoleWordByArray(9, g_lava_party_role[i]),   // HP
          PAL_LavaRoleWordByArray(7, g_lava_party_role[i]),   // MaxHP
          PAL_LavaRoleWordByArray(10, g_lava_party_role[i]),  // MP
          PAL_LavaRoleWordByArray(8, g_lava_party_role[i]),   // MaxMP
          PAL_LavaRoleWordByArray(17, g_lava_party_role[i]),  // 攻击力
          PAL_LavaRoleWordByArray(18, g_lava_party_role[i]),  // 灵力
          PAL_LavaRoleWordByArray(19, g_lava_party_role[i]),  // 防御力
          PAL_LavaRoleWordByArray(20, g_lava_party_role[i])   // 身法
       );
    }

    if (!PAL_LavaFseekOK(fp, 532, SEEK_SET))
    {
       goto load_failed;
    }
    for (i = 0; i < g_lava_party_count && i < 3; i++)
    {
       if (!PAL_LavaFseekOK(fp, 532 + g_lava_party_role[i] * 2, SEEK_SET))
       {
          goto load_failed;
       }
       if (fread((addr)role_word, 1, 2, fp) != 2)
       {
          goto load_failed;
       }
       g_lava_player_sprite_num[i] = PAL_U8(role_word[0]) | (PAL_U8(role_word[1]) << 8);
       if (g_lava_player_sprite_num[i] <= 0)
       {
          g_lava_player_sprite_num[i] = PAL_LavaReadU16((addr)g_lava_data_buf, 12 * 2 + g_lava_party_role[i] * 2);
       }

       if (!PAL_LavaFseekOK(fp, 1276 + g_lava_party_role[i] * 2, SEEK_SET))
       {
          goto load_failed;
       }
       if (fread((addr)role_word, 1, 2, fp) != 2)
       {
          goto load_failed;
       }
       g_lava_player_walk_frames[i] = PAL_U8(role_word[0]) | (PAL_U8(role_word[1]) << 8);
       if (g_lava_player_walk_frames[i] <= 0)
       {
          g_lava_player_walk_frames[i] = PAL_LavaReadU16((addr)g_lava_data_buf, 384 * 2 + g_lava_party_role[i] * 2);
       }
       if (g_lava_player_walk_frames[i] <= 0)
       {
          g_lava_player_walk_frames[i] = 3;
       }
        printf("[LAVA][PARTY] slot=%d role=%d name=%s sprite=%d walk=%d\n",
           i, g_lava_party_role[i], PAL_LavaRoleNameForLog(g_lava_party_role[i]),
           g_lava_player_sprite_num[i], g_lava_player_walk_frames[i]);
     }
      if (PAL_LavaLoadRoleMagicSaveData(fp) != 0)
      {
         goto load_failed;
      }
     PAL_LavaResetPartyTrail();

      saved_party_x = g_lava_party_x;
     saved_party_y = g_lava_party_y;

   if (!PAL_LavaFseekOK(fp, 1728, SEEK_SET))
   {
      goto load_failed;
   }

   for (i = 0; i < 256; i++)
   {
      if (fread((addr)inv_buf, 1, 6, fp) != 6)
      {
         goto load_failed;
      }
      g_lava_inventory_item[i] = PAL_U8(inv_buf[0]) | (PAL_U8(inv_buf[1]) << 8);
      g_lava_inventory_amount[i] = PAL_U8(inv_buf[2]) | (PAL_U8(inv_buf[3]) << 8);
   }

   if (!PAL_LavaFseekOK(fp, 3264, SEEK_SET))
   {
      goto load_failed;
   }

   for (i = 0; i < 300; i++)
   {
      if (fread((addr)scene_buf, 1, 8, fp) != 8)
      {
         goto load_failed;
      }
      g_lava_scene_runtime_enter[i] = PAL_U8(scene_buf[2]) | (PAL_U8(scene_buf[3]) << 8);
      g_lava_scene_runtime_teleport[i] = PAL_U8(scene_buf[4]) | (PAL_U8(scene_buf[5]) << 8);
   }
   g_lava_scene_runtime_scripts_inited = 1;

   PAL_LavaRepairSceneRuntimeScripts();

   if (!PAL_LavaFseekOK(fp, 12864, SEEK_SET))
   {
      goto load_failed;
   }

    PAL_LavaInitSceneStorageOffsets();
    event_bytes_long = PAL_LavaSceneEventBytes();
    g_lava_scene_event_count = 0;
    if (event_bytes_long > 0)
    {
       int total_hi;
       int total_lo;
       int chunk;
       addr dest;

       total_hi = (int)(event_bytes_long >> 16);
       total_lo = (int)(event_bytes_long & 0xFFFF);
       dest = (addr)g_lava_scene_event_raw;
       while (total_hi > 0 || total_lo > 0)
       {
          chunk = total_lo;
          if (total_hi > 0) chunk = 30000;
          if (chunk <= 0) break;
          if (fread(dest, 1, chunk, fp) != chunk)
          {
             goto load_failed;
          }
          dest += chunk;
          total_lo -= chunk;
          if (total_lo < 0)
          {
             total_hi--;
             total_lo += 65536;
          }
       }
       g_lava_scene_event_count = (int)(event_bytes_long / (long)32);
    }

    if (PAL_LavaFseekOK(fp, 5664, SEEK_SET))
    {
      if (fread((addr)g_lava_object_data, 1, sizeof(g_lava_object_data), fp) == sizeof(g_lava_object_data))
      {
         g_lava_object_data_size = sizeof(g_lava_object_data);
         g_lava_object_data_loaded = 1;
      }
    }

    fclose(fp);

     g_lava_loading_saved_game = 1;
    g_lava_defer_scene_preview_draw = 1;
    PAL_LavaSetScene(g_lava_scene_num);
    g_lava_defer_scene_preview_draw = 0;
    g_lava_loading_saved_game = 0;
    g_lava_view_x = saved_view_x;
    g_lava_view_y = saved_view_y;
    g_lava_party_x = saved_party_x;
    g_lava_party_y = saved_party_y;
    g_lava_party_direction = saved_party_dir;
    g_lava_scene_enter_pending = 0;
    g_lava_party_step_frame = 0;
    PAL_LavaRebuildSceneAutoscriptRuntime();
    PAL_LavaResetPartyTrail();
    PAL_LavaNormalizeLoadedFrame(&g_lava_party_direction, &g_lava_party_frame,
       g_lava_player_walk_frames[0] > 0 ? g_lava_player_walk_frames[0] : 3);
    PAL_LavaNormalizeLoadedEventFrames();
    PAL_LavaDrawSceneFrame();
   load_visible = 0;
   for (load_obj = g_lava_scene_event_first; load_obj <= g_lava_scene_event_last; load_obj++)
   {
      evt = PAL_LavaSceneEventData(load_obj);
      if (evt != 0 && PAL_LavaReadS16(evt, 12) > 0 && PAL_LavaReadU16(evt, 16) != 0)
      {
         load_visible++;
      }
   }
   printf("[LAVA][LOADRAW] visible_objects=%d range=%d..%d\n",
      load_visible,
      g_lava_scene_event_first,
      g_lava_scene_event_last);
   if (g_lava_autotest_load)
   {
      printf("[LAVA][LOADRAW] post scene=%d party=(%d,%d) view=(%d,%d) dir=%d frame=%d\n",
         g_lava_scene_num,
         g_lava_party_x, g_lava_party_y,
         g_lava_view_x, g_lava_view_y,
         g_lava_party_direction, g_lava_party_frame);
   }
   return 0;

load_failed:
   fclose(fp);
   return -1;
}

addr PAL_PermAlloc(int size)
{
   int aligned;
   addr p;
   int heap_limit;

   if (size <= 0) return 0;
   aligned = (size + 3) & ~3;
   heap_limit = (int)sizeof(g_lava_perm_heap);
   if (g_lava_perm_heap_pos > heap_limit - aligned) return 0;
   p = (addr)(g_lava_perm_heap + g_lava_perm_heap_pos);
   g_lava_perm_heap_pos += aligned;
   return p;
}

addr PAL_PermCalloc(int n, int size)
{
   addr p;
   int total;
   int i;
   char *q;

   total = n * size;
   p = PAL_PermAlloc(total);
   if (p == 0) return 0;
   q = p;
   for (i = 0; i < total; i++)
   {
      q[i] = 0;
   }
   return p;
}

void PAL_TmpReset(void)
{
   g_lava_tmp_heap_pos = 0;
}

addr PAL_TmpAlloc(int size)
{
   int aligned;
   addr p;
   int heap_limit;

   if (size <= 0) return 0;
   aligned = (size + 3) & ~3;
   heap_limit = (int)sizeof(g_lava_tmp_heap);
   if (g_lava_tmp_heap_pos > heap_limit - aligned) return 0;
   p = (addr)(g_lava_tmp_heap + g_lava_tmp_heap_pos);
   g_lava_tmp_heap_pos += aligned;
   return p;
}

void PAL_TmpFree(addr p)
{
}

#define PAL_MEM_CALLOC PAL_PermCalloc
#define PAL_TMP_ALLOC PAL_TmpAlloc
#define PAL_TMP_FREE PAL_TmpFree

int PAL_IsWINVersion(BOOL *pfIsWIN95)
{
   FILE *fps[2];
   char data[4];
   int count;
   long size;
   int j;
   int dos_score;
   int win_score;
   int i;

   fps[0] = (FILE *)g_lava_fpFBP;
   fps[1] = (FILE *)g_lava_fpMGO;
   dos_score = 0;
   win_score = 0;

   for (i = 0; i < 2; i++)
   {
      count = PAL_MKFGetChunkCount(fps[i]);
      j = 0;
      while (j < count)
      {
         size = PAL_MKFGetChunkSize(j, fps[i]);
         if (size >= 4) break;
         j++;
      }
      if (j >= count) return FALSE;
      if (PAL_MKFReadChunk((addr)data, 4, j, fps[i]) != 4) return FALSE;
      if (data[0] == 'Y' && data[1] == 'J' && data[2] == '_' && data[3] == '1')
      {
         dos_score++;
      }
      else
      {
         win_score++;
      }
   }

   if (pfIsWIN95)
   {
      *pfIsWIN95 = (win_score > dos_score) ? TRUE : FALSE;
   }
   return TRUE;
}


char *UTIL_va(char *buffer, int buflen, char *format, ...)
{
   if (buffer && buflen > 0)
   {
      buffer[0] = '\0';
      return buffer;
   }
   return format;
}

addr UTIL_calloc(int n, int size)
{
   return PAL_PermCalloc(n, size);
}

int PAL_InitGlobals(void)
{
    g_lava_fpPAT = 0;
    g_lava_fpFBP = 0;
    g_lava_fpDATA = 0;
    g_lava_fpMAP = 0;
    g_lava_fpGOP = 0;
    g_lava_fpMGO = 0;
    g_lava_fpRNG = 0;
    g_lava_fpSSS = 0;
    g_lava_fpRGM = 0;
    g_lava_fpBALL = 0;
    g_lava_fpFIRE = 0;
#ifdef __LAVA__
    gConfig.fIsWIN95 = FALSE;
#else
    if (!PAL_IsWINVersion(&gConfig.fIsWIN95)) return -1;
#endif
    printf("[LAVA][INIT] fIsWIN95=%d\n", gConfig.fIsWIN95);
    g_lava_gpGlobals.fNeedToFadeIn = FALSE;
    g_lava_gpGlobals.wNumPalette = 0;
   g_lava_gpGlobals.fNightPalette = FALSE;
   return 0;
}

void PAL_FreeGlobals(void)
{
   if (g_lava_fpDATA) fclose((FILE *)g_lava_fpDATA);
   if (g_lava_fpMGO) fclose((FILE *)g_lava_fpMGO);
   if (g_lava_fpSSS) fclose((FILE *)g_lava_fpSSS);
   g_lava_fpDATA = 0;
   g_lava_fpMGO = 0;
   g_lava_fpSSS = 0;
}

int VIDEO_Startup(void)
{
    lava_init_video();
    g_lava_gpScreen.w = g_screen_surface.w;
    g_lava_gpScreen.h = g_screen_surface.h;
    g_lava_gpScreen.pitch = g_screen_surface.pitch;
    g_lava_gpScreen.pixels = g_screen_surface.pixels;
#ifndef LAVA_NATIVE_COMPILED
    g_lava_gpScreen.format_palette = g_screen_surface.format_palette;
#endif
    return SDL_OK;
}

void VIDEO_Shutdown(void)
{
}

void VIDEO_SetWindowTitle(char *title)
{
   printf("[LAVA] title: %s\n", title);
}

FILE *UTIL_OpenFile(char *lpszFileName)
{
   return fopen(lpszFileName, "rb");
}

FILE *UTIL_OpenRequiredFile(char *lpszFileName)
{
   FILE *fp = UTIL_OpenFile(lpszFileName);
   if (fp == NULL)
   {
      printf("[LAVA] missing file: %s\n", lpszFileName);
   }
   return fp;
}

int PAL_InitUI(void)
{
   return 0;
}

void PAL_FreeUI(void)
{
}

int PAL_InitText(void)
{
    long word_file_size;
    int count;
    int i;
    FILE *fp;
    FILE *fp_sss;

    g_lava_fpMSG = 0;
    g_lava_fpWORD = 0;
    g_lava_fpDESC = 0;
    g_lava_msg_file_is_gb2312 = 0;
    g_lava_word_file_is_gb2312 = 0;
    g_lava_word_file_cache_bytes = 0;
    g_lava_word_file_cache_ready = 0;

    fp = UTIL_OpenRequiredFile("M_GB2312.MSG");
    if (fp != 0)
    {
       g_lava_msg_file_is_gb2312 = 1;
       fclose(fp);
    }

    fp = UTIL_OpenRequiredFile("WORD_GB2312.DAT");
    if (fp != 0)
    {
       g_lava_word_file_is_gb2312 = 1;
       fclose(fp);
    }
    fp = g_lava_word_file_is_gb2312 ?
       UTIL_OpenFile("WORD_GB2312.DAT") : UTIL_OpenRequiredFile("WORD.DAT");
    if (fp != 0)
    {
       fseek(fp, 0, SEEK_END);
       word_file_size = ftell(fp);
       if (word_file_size > 0 && word_file_size <= LAVA_WORD_FILE_CACHE_SIZE &&
           PAL_LavaFseekOK(fp, 0, SEEK_SET) &&
           fread((addr)g_lava_word_file_cache, 1, word_file_size, fp) == word_file_size)
       {
          g_lava_word_file_cache_bytes = word_file_size;
          g_lava_word_file_cache_ready = 1;
          printf("[LAVA][WORD] cached=%ld bytes sample295='%s'\n",
             word_file_size, PAL_LavaReadWord(295));
       }
       fclose(fp);
    }
    fp = UTIL_OpenRequiredFile("desc_gb2312.dat");
    if (fp != 0)
    {
       fclose(fp);
    }
    fp_sss = UTIL_OpenRequiredFile("SSS.MKF");
    if (fp_sss == 0)
    {
       return 0;
    }

   count = PAL_MKFGetChunkSize(3, fp_sss) / 4;
   if (count <= 1)
   {
      fclose(fp_sss);
      return 0;
   }

   if (count > LAVA_MSG_OFFSET_MAX)
   {
      count = LAVA_MSG_OFFSET_MAX;
   }

   PAL_MKFReadChunk((addr)g_lava_msg_offsets, count * 4, 3, fp_sss);
   fclose(fp_sss);
   for (i = 0; i < count; i++)
   {
      g_lava_msg_offsets[i] = SDL_SwapLE32(g_lava_msg_offsets[i]);
   }
   g_lava_msg_total = count - 1;

   return 0;
}

void PAL_FreeText(void)
{
    g_lava_fpMSG = 0;
    g_lava_fpWORD = 0;
    g_lava_fpDESC = 0;
}

int PAL_InitFont(CONFIGURATION *cfg)
{
   cfg = cfg;
   g_lava_font_loaded = 0;
   g_lava_font_map_count = 0;
   g_lava_font_pair_map_count = 0;
   g_lava_fpFONT = 0;
   g_lava_font_gb_offset = 0;
   g_lava_font_total_glyphs = 0;
   PAL_LavaLoadFontAsset(0);
   return 0;
}

void PAL_FreeFont(void)
{
   g_lava_font_loaded = 0;
   g_lava_font_map_count = 0;
   g_lava_font_pair_map_count = 0;
   if (g_lava_fpFONT != 0)
   {
      fclose(g_lava_fpFONT);
   }
   g_lava_fpFONT = 0;
   g_lava_font_gb_offset = 0;
   g_lava_font_total_glyphs = 0;
}

void PAL_InitInput(void)
{
   g_InputState.dir = 0;
   g_InputState.prevdir = 0;
   g_InputState.dwKeyPress = 0;
   g_InputState.dwKeyOrder[0] = 0;
   g_InputState.dwKeyOrder[1] = 0;
   g_InputState.dwKeyOrder[2] = 0;
   g_InputState.dwKeyOrder[3] = 0;
   g_InputState.dwKeyMaxCount = 0;
   g_InputState.dir = kDirUnknown;
   g_InputState.prevdir = kDirUnknown;
   g_InputState.dwKeyPress = 0;
   g_lava_key_hold = 0;
   g_lava_frame_hold = 0;
   g_lava_autotouch_last_object = 0;
   g_lava_autotouch_last_trigger = 0;
   g_lava_touch_lock_object = 0;
   g_lava_touch_lock_trigger = 0;
   g_lava_autotouch_seen_count = 0;
   PAL_LavaInitAutotestFlags();
}

void PAL_ShutdownInput(void)
{
}

void PAL_InitResources(void)
{
}

void PAL_FreeResources(void)
{
}

void PAL_ProcessEvent(void)
{
   int key;
   int hold_up;
   int hold_down;
   int hold_left;
   int hold_right;
   int last_hold_up;
   int last_hold_down;
   int last_hold_left;
   int last_hold_right;
   static DWORD last_hold;
   static DWORD last_press;

   last_hold_up = (g_lava_key_hold & kKeyUp) != 0;
   last_hold_down = (g_lava_key_hold & kKeyDown) != 0;
   last_hold_left = (g_lava_key_hold & kKeyLeft) != 0;
   last_hold_right = (g_lava_key_hold & kKeyRight) != 0;

   hold_up = CheckKey(KEY_UP);
   hold_down = CheckKey(KEY_DOWN);
   hold_left = CheckKey(KEY_LEFT);
   hold_right = CheckKey(KEY_RIGHT);

   if (g_lava_autotest_input && g_lava_autotest_input_step > 20 && g_lava_autotest_input_step < 28)
   {
      hold_right = 1;
   }

   g_lava_key_hold = 0;
   if (hold_up)
      g_lava_key_hold |= kKeyUp;
   if (hold_down)
      g_lava_key_hold |= kKeyDown;
   if (hold_left)
      g_lava_key_hold |= kKeyLeft;
   if (hold_right)
      g_lava_key_hold |= kKeyRight;

   if (hold_up && !last_hold_up)
      PAL_LavaKeyDown(kKeyUp, FALSE);
   else if (!hold_up && last_hold_up)
      PAL_LavaKeyUp(kKeyUp);

   if (hold_down && !last_hold_down)
      PAL_LavaKeyDown(kKeyDown, FALSE);
   else if (!hold_down && last_hold_down)
      PAL_LavaKeyUp(kKeyDown);

   if (hold_left && !last_hold_left)
      PAL_LavaKeyDown(kKeyLeft, FALSE);
   else if (!hold_left && last_hold_left)
      PAL_LavaKeyUp(kKeyLeft);

   if (hold_right && !last_hold_right)
      PAL_LavaKeyDown(kKeyRight, FALSE);
   else if (!hold_right && last_hold_right)
      PAL_LavaKeyUp(kKeyRight);

   /*
    * 锁存本帧里出现过的持续方向输入，避免轮询时序抖动导致
    * PAL_StartFrame() 读到 0，从而错过本应生效的一步移动。
    */
   g_lava_frame_hold |= g_lava_key_hold;

   key = Inkey();
   if (key == KEY_UP)
   {
      if (hold_up) PAL_LavaKeyDown(kKeyUp, TRUE);
      else g_InputState.dwKeyPress |= kKeyUp;
   }
   else if (key == KEY_DOWN)
   {
      if (hold_down) PAL_LavaKeyDown(kKeyDown, TRUE);
      else g_InputState.dwKeyPress |= kKeyDown;
   }
   else if (key == KEY_LEFT)
   {
      if (hold_left) PAL_LavaKeyDown(kKeyLeft, TRUE);
      else g_InputState.dwKeyPress |= kKeyLeft;
   }
   else if (key == KEY_RIGHT)
   {
      if (hold_right) PAL_LavaKeyDown(kKeyRight, TRUE);
      else g_InputState.dwKeyPress |= kKeyRight;
   }
   else if (key == KEY_ENTER)
      g_InputState.dwKeyPress |= kKeySearch;
    else if (key == KEY_B)
       g_InputState.dwKeyPress |= kKeyMenu;
   else if (key == PAL_LAVA_KEY_LAST_MAGIC || key == 'x' || key == 'X' || key == 'p' || key == 'P')
   {
      g_InputState.dwKeyPress |= kKeyForce;
      printf("[LAVA][HOTKEY] input key=%d action=last press=%ld\n",
         key, (long)g_InputState.dwKeyPress);
   }
   else if (key == PAL_LAVA_KEY_BEST_MAGIC || key == 'y' || key == 'Y' || key == 'o' || key == 'O')
   {
      g_InputState.dwKeyPress |= kKeyUseItem;
      printf("[LAVA][HOTKEY] input key=%d action=best press=%ld\n",
         key, (long)g_InputState.dwKeyPress);
   }

    if (g_lava_autotest_input &&
        ((g_lava_key_hold != last_hold || g_InputState.dwKeyPress != last_press) &&
         (key != 0 || g_lava_key_hold != 0 || g_InputState.dwKeyPress != 0)))
   {
      printf("[LAVA][INPUT] inkey=%d hold=%d press=%d dir=%d prev=%d\n",
         key, g_lava_key_hold, g_InputState.dwKeyPress,
         g_InputState.dir, g_InputState.prevdir);
   }

   last_hold = g_lava_key_hold;
   last_press = g_InputState.dwKeyPress;
}

void PAL_ClearKeyState(void)
{
   g_InputState.dwKeyPress = 0;
   g_lava_frame_hold = 0;
}

void AUDIO_OpenDevice(void)
{
   g_lava_music_enabled = 1;
   g_lava_sound_enabled = 1;
}

void AUDIO_CloseDevice(void)
{
}

void AUDIO_PlayMusic(int num, BOOL loop, int fadeTime)
{
   g_lava_num_music = num;
   g_lava_last_music_loop = loop ? 1 : 0;
   g_lava_last_music_fade_time = fadeTime;
}

void AUDIO_PlaySound(int num)
{
   g_lava_last_sound = num;
}

void PAL_LavaSetBackgroundMusic(int num, int mode)
{
   g_lava_num_music = num;
   if (!g_lava_fast_script_probe)
   {
      AUDIO_PlayMusic(num, mode != 1, (mode == 3 && num != 9) ? 3 : 0);
   }
}

void PAL_LavaSetBattleMusic(int num)
{
   g_lava_num_battle_music = num;
}

void PAL_LavaPlaySoundEffect(int num)
{
   g_lava_last_sound = num;
   if (!g_lava_fast_script_probe)
   {
      AUDIO_PlaySound(num);
   }
}

void PAL_LavaStopMusic(int fade)
{
   g_lava_num_music = 0;
   if (!g_lava_fast_script_probe)
   {
      AUDIO_PlayMusic(0, FALSE, fade == 0 ? 2 : fade * 3);
   }
}

void AUDIO_EnableMusic(BOOL fEnable)
{
   g_lava_music_enabled = fEnable ? 1 : 0;
}

BOOL AUDIO_MusicEnabled(void)
{
   return g_lava_music_enabled ? TRUE : FALSE;
}

void AUDIO_EnableSound(BOOL fEnable)
{
   g_lava_sound_enabled = fEnable ? 1 : 0;
}

BOOL AUDIO_SoundEnabled(void)
{
   return g_lava_sound_enabled ? TRUE : FALSE;
}

BOOL AUDIO_PlayCDTrack(int iTrackNum)
{
   return FALSE;
}

void PAL_AVIInit(void)
{
}

void PAL_AVIShutdown(void)
{
}

BOOL PAL_PlayAVI(char *lpszPath)
{
   return FALSE;
}

static int PAL_LavaLoadPaletteCache(int iPaletteNum, BOOL fNight)
{
    int i;
    long size;
    FILE *fp;

    fp = g_lava_fpPAT ? (FILE *)g_lava_fpPAT : fopen("PAT.MKF", "rb");
    if (fp == NULL)
    {
       printf("[LAVA] missing file: %s\n", "PAT.MKF");
       return FALSE;
    }

    size = PAL_MKFReadChunk((addr)g_lava_mkf_buf, 1536, iPaletteNum, fp);
    if (!g_lava_fpPAT)
    {
       fclose(fp);
    }
    if (size < 0) return FALSE;
    if (size <= 256 * 3) fNight = FALSE;

    g_lava_gpGlobals.wNumPalette = iPaletteNum;
    g_lava_gpGlobals.fNightPalette = fNight;

   for (i = 0; i < 256; i++)
   {
      int base = (fNight ? 256 * 3 : 0) + i * 3;
      g_lava_palette_cache[i].r = PAL_U8(g_lava_mkf_buf[base]) << 2;
      g_lava_palette_cache[i].g = PAL_U8(g_lava_mkf_buf[base + 1]) << 2;
      g_lava_palette_cache[i].b = PAL_U8(g_lava_mkf_buf[base + 2]) << 2;
       g_lava_palette_cache[i].a = 0;
    }

    return TRUE;
}

void PAL_SetPalette(int iPaletteNum, BOOL fNight)
{
    if (!PAL_LavaLoadPaletteCache(iPaletteNum, fNight)) return;

    VIDEO_SetPalette((addr)g_lava_palette_cache);
}

addr PAL_GetPalette(int iPaletteNum, BOOL fNight)
{
   PAL_SetPalette(iPaletteNum, fNight);
   return (addr)g_lava_palette_cache;
}

BOOL PAL_LavaStartupSkipRequested(void)
{
   PAL_ProcessEvent();
   if (g_InputState.dwKeyPress & (kKeyMenu | kKeySearch))
   {
      PAL_ClearKeyState();
      return TRUE;
   }
   return FALSE;
}

BOOL PAL_LavaDelaySkippable(int ms)
{
   int elapsed;

   elapsed = 0;
   while (elapsed < ms)
   {
      if (PAL_LavaStartupSkipRequested()) return TRUE;
      Delay(10);
      elapsed += 10;
   }
   return FALSE;
}

BOOL PAL_RNGPlay(int iNumRNG, int iStartFrame, int iEndFrame, int iSpeed)
{
   FILE *fp;
   long frame_len;
   int delay;
   long rng_len;
   int frames_drawn;

   fp = g_lava_fpRNG ? (FILE *)g_lava_fpRNG : UTIL_OpenRequiredFile("RNG.MKF");
   if (fp == NULL) return FALSE;

   delay = iSpeed == 0 ? 60 : (1000 / iSpeed);
   if (delay < 1) delay = 1;
   if (iEndFrame > 0) iEndFrame++;
   frames_drawn = 0;

   while (iStartFrame != iEndFrame)
   {
      if (PAL_LavaStartupSkipRequested())
      {
         break;
      }

      if (PAL_RNGReadFrame((addr)g_lava_mkf_buf, 65536, iNumRNG, iStartFrame, fp) < 0)
      {
         break;
      }

      PAL_TmpReset();
      rng_len = Decompress((addr)g_lava_mkf_buf, (addr)g_lava_fbp_buf, 64000);
      frame_len = PAL_RNGBlitToSurface((addr)g_lava_fbp_buf, rng_len, gpScreen);
      if (rng_len < 0 || frame_len < 0)
      {
         break;
      }

      VIDEO_UpdateScreen(0);
      frames_drawn++;
      if (PAL_LavaDelaySkippable(delay))
      {
         break;
      }
      iStartFrame++;
   }

   if (!g_lava_fpRNG)
   {
      fclose(fp);
   }
   printf("[LAVA][RNG] num=%d frames=%d\n", iNumRNG, frames_drawn);
   return frames_drawn > 0;
}

long PAL_RNGReadFrame(addr lpBuffer, UINT uiBufferSize, UINT uiRngNum, UINT uiFrameNum, FILE *fpRngMKF)
{
   DWORD uiOffset;
   DWORD uiSubOffset;
   DWORD uiNextOffset;
   UINT uiChunkCount;
   DWORD uiChunkLen;

   if (lpBuffer == 0 || fpRngMKF == 0 || uiBufferSize == 0) return -1;

   uiChunkCount = PAL_MKFGetChunkCount(fpRngMKF);
   if (uiRngNum >= uiChunkCount) return -1;

   fseek(fpRngMKF, 4 * uiRngNum, SEEK_SET);
   if (fread(&uiOffset, 1, 4, fpRngMKF) != 4) return -1;
   if (fread(&uiNextOffset, 1, 4, fpRngMKF) != 4) return -1;
   uiOffset = SDL_SwapLE32(uiOffset);
   uiNextOffset = SDL_SwapLE32(uiNextOffset);
   if (uiNextOffset <= uiOffset) return -1;

   fseek(fpRngMKF, uiOffset, SEEK_SET);
   if (fread(&uiChunkCount, 1, 4, fpRngMKF) != 4) return -1;
   uiChunkCount = (SDL_SwapLE32(uiChunkCount) - 4) / 4;
   if (uiFrameNum >= uiChunkCount) return -1;

   fseek(fpRngMKF, uiOffset + 4 * uiFrameNum, SEEK_SET);
   if (fread(&uiSubOffset, 1, 4, fpRngMKF) != 4) return -1;
   if (fread(&uiNextOffset, 1, 4, fpRngMKF) != 4) return -1;
   uiSubOffset = SDL_SwapLE32(uiSubOffset);
   uiNextOffset = SDL_SwapLE32(uiNextOffset);
   if (uiNextOffset <= uiSubOffset) return -1;

   uiChunkLen = uiNextOffset - uiSubOffset;
   if (uiChunkLen > uiBufferSize) return -2;

   fseek(fpRngMKF, uiOffset + uiSubOffset, SEEK_SET);
   return fread(lpBuffer, 1, uiChunkLen, fpRngMKF);
}

int PAL_RNGWritePair(char *pixels, int pitch, long dst_ptr, char left, char right)
{
   int x;
   int y;

   if (dst_ptr < 0 || dst_ptr >= 64000) return -1;
   x = dst_ptr % 320;
   y = dst_ptr / 320;
   pixels[y * pitch + x] = left;
   dst_ptr++;
   if (dst_ptr >= 64000) return -1;
   x = dst_ptr % 320;
   y = dst_ptr / 320;
   pixels[y * pitch + x] = right;
   return 0;
}

long PAL_RNGBlitToSurface(addr rng, long length, addr lpDstSurface)
{
   SDL_Surface *dst;
   char *data;
   char *pixels;
   long ptr;
   long dst_ptr;
   long i;
   long n;
   int op;
   long wdata;

   dst = lpDstSurface;
   data = rng;
   if (data == 0 || dst == 0 || dst->w != 320 || dst->h != 200 || length < 0) return -1;

   pixels = (char *)dst->pixels;
   ptr = 0;
   dst_ptr = 0;

   while (ptr < length)
   {
      op = PAL_U8(data[ptr++]);
      if (op == 0x00 || op == 0x13) return 0;
      if (op == 0x02)
      {
         dst_ptr += 2;
      }
      else if (op == 0x03)
      {
         if (ptr >= length) return -1;
         dst_ptr += (PAL_U8(data[ptr++]) + 1) * 2;
      }
      else if (op == 0x04)
      {
         if (ptr + 1 >= length) return -1;
         wdata = PAL_U8(data[ptr]) | (PAL_U8(data[ptr + 1]) << 8);
         ptr += 2;
         dst_ptr += (wdata + 1) * 2;
      }
      else if (op >= 0x06 && op <= 0x0a)
      {
         for (i = 0; i < op - 0x05; i++)
         {
            if (ptr + 1 >= length) return -1;
            if (PAL_RNGWritePair(pixels, dst->pitch, dst_ptr, data[ptr], data[ptr + 1]) < 0) return -1;
            ptr += 2;
            dst_ptr += 2;
         }
      }
      else if (op == 0x0b || op == 0x11)
      {
         if (ptr >= length) return -1;
         n = PAL_U8(data[ptr++]) + 1;
         if (ptr + 1 >= length) return -1;
         for (i = 0; i < n; i++)
         {
            if (PAL_RNGWritePair(pixels, dst->pitch, dst_ptr, data[ptr], data[ptr + 1]) < 0) return -1;
            if (op == 0x0b) ptr += 2;
            dst_ptr += 2;
         }
         if (op == 0x11) ptr += 2;
      }
      else if (op == 0x0c || op == 0x12)
      {
         if (ptr + 1 >= length) return -1;
         n = (PAL_U8(data[ptr]) | (PAL_U8(data[ptr + 1]) << 8)) + 1;
         ptr += 2;
         if (ptr + 1 >= length) return -1;
         for (i = 0; i < n; i++)
         {
            if (PAL_RNGWritePair(pixels, dst->pitch, dst_ptr, data[ptr], data[ptr + 1]) < 0) return -1;
            if (op == 0x0c) ptr += 2;
            dst_ptr += 2;
         }
         if (op == 0x12) ptr += 2;
      }
      else if (op >= 0x0d && op <= 0x10)
      {
         n = op - 0x0b;
         if (ptr + 1 >= length) return -1;
         for (i = 0; i < n; i++)
         {
            if (PAL_RNGWritePair(pixels, dst->pitch, dst_ptr, data[ptr], data[ptr + 1]) < 0) return -1;
            dst_ptr += 2;
         }
         ptr += 2;
      }
      else
      {
         return -1;
      }

      if (dst_ptr > (long)64000) return -1;
   }

   return 0;
}

addr VIDEO_CreateCompatibleSurface(addr lpSource)
{
   SDL_Surface *surface;
   surface = PAL_MEM_CALLOC(1, 20);
   if (surface == 0) return 0;
   surface->w = SCREEN_W;
   surface->h = SCREEN_H;
   surface->pitch = SCREEN_W;
   surface->pixels = PAL_MEM_CALLOC(SCREEN_W, SCREEN_H);
#ifndef LAVA_NATIVE_COMPILED
   surface->format_palette = 0;
#endif
   return (addr)surface;
}

void VIDEO_SetPalette(addr palette)
{
   SDL_SetPalette(palette, 0, 256);
}

void VIDEO_UpdateSurfacePalette(addr lpSurface)
{
}

void VIDEO_CopySurface(addr lpSource, addr lpSrcRect, addr lpDestination, addr lpDestRect)
{
   SDL_Surface *src;
   SDL_Surface *dst;
   SDL_Rect *sr;
   SDL_Rect *dr;
   char *sp;
   char *dp;
   int x, y;

   src = lpSource;
   dst = lpDestination;
   sr = lpSrcRect;
   dr = lpDestRect;

   if (!src || !dst || !sr || !dr) return;

   for (y = 0; y < sr->h; y++)
   {
      for (x = 0; x < sr->w; x++)
      {
         sp = (char *)src->pixels + (sr->y + y) * src->pitch + sr->x + x;
         dp = (char *)dst->pixels + (dr->y + y) * dst->pitch + dr->x + x;
         *dp = *sp;
      }
   }
}

void VIDEO_FreeSurface(addr lpSurface)
{
}

long PAL_MKFReadChunk(addr lpBuffer, UINT uiBufferSize, UINT uiChunkNum, FILE *fp)
{
    g_lava_mkf_read_chunk_count = 0;
    g_lava_mkf_read_offset = 0;
    g_lava_mkf_read_next_offset = 0;
    g_lava_mkf_read_len = 0;

   if (lpBuffer == 0 || fp == 0 || uiBufferSize == 0) return -1;

   g_lava_mkf_read_chunk_count = PAL_MKFGetChunkCount(fp);
    if (uiChunkNum >= g_lava_mkf_read_chunk_count) return -1;

    fseek(fp, 4 * uiChunkNum, SEEK_SET);
    fread(&g_lava_mkf_read_offset, 1, 4, fp);
    fread(&g_lava_mkf_read_next_offset, 1, 4, fp);
   g_lava_mkf_read_offset = SDL_SwapLE32(g_lava_mkf_read_offset);
   g_lava_mkf_read_next_offset = SDL_SwapLE32(g_lava_mkf_read_next_offset);
   g_lava_mkf_read_len = g_lava_mkf_read_next_offset - g_lava_mkf_read_offset;
   if (g_lava_mkf_read_len > uiBufferSize) return -2;
   if (g_lava_mkf_read_len == 0) return -1;

   fseek(fp, g_lava_mkf_read_offset, SEEK_SET);
   return fread(lpBuffer, 1, g_lava_mkf_read_len, fp);
}

int PAL_MKFGetChunkCount(FILE *fp)
{
     if (fp == 0) return 0;
     fseek(fp, 0, SEEK_SET);
     if (fread(&g_lava_mkf_read_offset, 1, 4, fp) == 4)
        return (SDL_SwapLE32(g_lava_mkf_read_offset) - 4) >> 2;
     return 0;
}

long PAL_MKFGetChunkSize(UINT uiChunkNum, FILE *fp)
{
   g_lava_mkf_read_chunk_count = PAL_MKFGetChunkCount(fp);
   if (uiChunkNum >= g_lava_mkf_read_chunk_count) return -1;

   fseek(fp, 4 * uiChunkNum, SEEK_SET);
   fread(&g_lava_mkf_read_offset, 1, 4, fp);
   fread(&g_lava_mkf_read_next_offset, 1, 4, fp);
   g_lava_mkf_read_offset = SDL_SwapLE32(g_lava_mkf_read_offset);
   g_lava_mkf_read_next_offset = SDL_SwapLE32(g_lava_mkf_read_next_offset);
   return g_lava_mkf_read_next_offset - g_lava_mkf_read_offset;
}

long PAL_MKFDecompressChunk(addr lpBuffer, UINT uiBufferSize, UINT uiChunkNum, FILE *fp)
{
    long len;
    addr buf;
    long ret;

    len = PAL_MKFGetChunkSize(uiChunkNum, fp);
    if (len <= 0) return len;
    if (len > 65536) return -2;

    buf = (addr)g_lava_mkf_buf;

    PAL_MKFReadChunk(buf, len, uiChunkNum, fp);
    PAL_TmpReset();
    ret = Decompress(buf, lpBuffer, uiBufferSize);
    return ret;
}

int PAL_LavaDecompressOK(long ret, long expected_size)
{
   if (ret > 0) return TRUE;

   /*
    * Some Lava VM return paths still surface large positive lengths as
    * signed 16-bit values. Keep this local to resource-size validation.
    */
   if (expected_size == (long)64000 && ret == (long)-1536) return TRUE;
   if (expected_size > (long)32767 && ret == expected_size - (long)65536) return TRUE;

   return FALSE;
}

static void PAL_LavaAdvanceEventFrame(addr evt)
{
   int frame;
   int sprite_frames;
   int auto_frames;

   if (evt == 0)
   {
      return;
   }

   frame = PAL_LavaReadU16(evt, 22);
   sprite_frames = PAL_LavaReadU16(evt, 18);
   auto_frames = PAL_LavaReadU16(evt, 28);

   if (sprite_frames > 0)
   {
      frame++;
      if (sprite_frames == 3)
      {
         frame %= 4;
      }
      else
      {
         frame %= sprite_frames;
      }
      PAL_LavaWriteU16(evt, 22, frame);
   }
   else if (auto_frames > 0)
   {
      frame++;
      frame %= auto_frames;
      PAL_LavaWriteU16(evt, 22, frame);
   }
}

static void PAL_LavaNormalizeLoadedFrame(int *direction, int *frame, int frame_stride)
{
   if (frame_stride <= 0)
   {
      if (*frame < 0)
      {
         *frame = 0;
      }
      return;
   }

   if (*frame >= frame_stride && *frame < frame_stride * 4)
   {
      *direction = *frame / frame_stride;
      *frame %= frame_stride;
   }

   if (*direction < 0)
   {
      *direction = 0;
   }
   else if (*direction > 3)
   {
      *direction &= 3;
   }

   if (*frame < 0)
   {
      *frame = 0;
   }
   else if (*frame >= frame_stride)
   {
      *frame %= frame_stride;
   }
}

static void PAL_LavaNormalizeLoadedEventFrames(void)
{
   int object_id;

   for (object_id = g_lava_scene_event_first; object_id <= g_lava_scene_event_last; object_id++)
   {
      addr evt;
      int sprite_frames;
      int frame_stride;
      int direction;
      int current_frame;

      evt = PAL_LavaSceneEventData(object_id);
      if (evt == 0)
      {
         continue;
      }

      sprite_frames = PAL_LavaReadU16(evt, 18);
      frame_stride = sprite_frames;
      if (frame_stride <= 0)
      {
         continue;
      }

      direction = PAL_LavaReadU16(evt, 20);
      current_frame = PAL_LavaReadU16(evt, 22);
      PAL_LavaNormalizeLoadedFrame(&direction, &current_frame, frame_stride);
      PAL_LavaWriteU16(evt, 20, direction);
      PAL_LavaWriteU16(evt, 22, current_frame);
   }
}

static void PAL_LavaSceneWalkEventOneStep(addr evt, int dir, int speed)
{
   int x;
   int y;

   if (evt == 0)
   {
      return;
   }

   if (speed <= 0)
   {
      speed = 2;
   }

   x = PAL_LavaReadU16(evt, 2);
   y = PAL_LavaReadU16(evt, 4);
   x += ((dir == 1 || dir == 0) ? -2 : 2) * speed;
   y += ((dir == 1 || dir == 2) ? -1 : 1) * speed;
   PAL_LavaWriteU16(evt, 2, x);
   PAL_LavaWriteU16(evt, 4, y);
   PAL_LavaWriteU16(evt, 20, dir);
   PAL_LavaAdvanceEventFrame(evt);
}

static addr PAL_LavaResolveEventTarget(int object_id, int target_id)
{
    if (target_id <= 0 || target_id == 0xFFFF)
    {
       return PAL_LavaSceneEventData(object_id);
    }

     return PAL_LavaSceneEventData(target_id);
}

static int PAL_LavaResolveTargetObjectId(int object_id, int target_id)
{
   if (target_id <= 0 || target_id == 0xFFFF)
   {
      return object_id;
   }

   return target_id;
}

static int PAL_LavaInterpretAutoFallback(int object_id, addr evt, int op, int a, int b, int c, addr entry)
{
   addr target_evt;

   if (evt == 0)
   {
      return 0;
   }

   switch (op)
   {
   case 0x000F:
      if (a != 0xFFFF)
      {
         PAL_LavaWriteU16(evt, 20, a);
      }
      if (b != 0xFFFF)
      {
         PAL_LavaWriteU16(evt, 22, b);
      }
      return 1;

   case 0x0014:
      PAL_LavaWriteU16(evt, 22, a);
      PAL_LavaWriteU16(evt, 20, 0);
      return 1;

   case 0x0016:
      if (a != 0)
      {
         target_evt = PAL_LavaResolveEventTarget(object_id, a);
         if (target_evt != 0)
         {
            PAL_LavaWriteU16(target_evt, 20, b);
            PAL_LavaWriteU16(target_evt, 22, c);
         }
      }
      return 1;

   case 0x0025:
      target_evt = PAL_LavaResolveEventTarget(object_id, a);
      if (target_evt != 0)
      {
         PAL_LavaWriteU16(target_evt, 8, b);
      }
      return 1;

   case 0x0040:
      target_evt = PAL_LavaResolveEventTarget(object_id, a);
      if (target_evt != 0)
      {
         PAL_LavaWriteU16(target_evt, 14, b);
      }
      return 1;

   case 0x0049:
      target_evt = PAL_LavaResolveEventTarget(object_id, a);
      if (target_evt != 0)
      {
         PAL_LavaWriteS16(target_evt, 12, b);
      }
      return 1;

   case 0x006C:
      PAL_LavaWriteU16(evt, 2, PAL_LavaReadU16(evt, 2) + PAL_LavaReadS16(entry, 4));
      PAL_LavaWriteU16(evt, 4, PAL_LavaReadU16(evt, 4) + PAL_LavaReadS16(entry, 6));
      PAL_LavaAdvanceEventFrame(evt);
      return 1;

   case 0x006F:
      target_evt = PAL_LavaResolveEventTarget(object_id, a);
      if (target_evt != 0 && PAL_LavaReadS16(target_evt, 12) == b)
      {
         PAL_LavaWriteS16(evt, 12, b);
      }
      return 1;

   case 0x0087:
      PAL_LavaAdvanceEventFrame(evt);
      return 1;
   }

   return 0;
}

static int PAL_LavaShouldLogAutoUnsupported(void)
{
   return g_lava_fast_script_probe ||
      g_lava_autotest_search || g_lava_autotest_walk ||
      g_lava_autotest_exits || g_lava_autotest_scene5 ||
      g_lava_autotest_scene6 || g_lava_autotest_scene13 ||
      g_lava_autotest_scene9 || g_lava_autotest_hooks ||
      g_lava_autotest_door || g_lava_autotest_hall ||
      g_lava_autotest_kitchen || g_lava_autotest_load;
}

static void PAL_LavaLogAutoUnsupported(int object_id, long script_entry, int op, int a, int b, int c)
{
   int i;

   if (!PAL_LavaShouldLogAutoUnsupported())
   {
      return;
   }

   for (i = 0; i < g_lava_auto_unsupported_logged_count; i++)
   {
      if (g_lava_auto_unsupported_logged[i] == (WORD)op)
      {
         return;
      }
   }

   if (g_lava_auto_unsupported_logged_count < LAVA_AUTO_UNSUPPORTED_LOG_MAX)
   {
      g_lava_auto_unsupported_logged[g_lava_auto_unsupported_logged_count] = (WORD)op;
      g_lava_auto_unsupported_logged_count++;
   }

   printf("[LAVA][AUTO] skip unsupported op=%d obj=%d idx=%d a=%d b=%d c=%d\n",
      op, object_id, (int)script_entry, a, b, c);
}

static int PAL_LavaBig5ToGbkPair(int lead, int trail, char *out)
{
   if (lead == 0 || trail == 0 || out == 0)
   {
      return 0;
   }
   out[0] = (char)0xA3;
   out[1] = (char)0xBF;
   return 2;
}

static void PAL_LavaNormalizeSimplifiedMsg(char *buf)
{
   return;
}

static void PAL_LavaNormalizeReadableMsg(char *buf)
{
   return;
}

static void PAL_LavaConvertBig5MsgToGbk(char *buf, int len)
{
   return;
}

static char *PAL_LavaReadMsg(int msg_id)
{
   long start;
   long end;
   int len;
   FILE *fp;

   if (msg_id < 0 || msg_id >= g_lava_msg_total)
   {
      return 0;
   }

   start = g_lava_msg_offsets[msg_id];
   end = g_lava_msg_offsets[msg_id + 1];
   len = end - start;
   if (len <= 0)
   {
      return 0;
   }
   if (len >= 1024)
   {
      len = 1023;
   }

   fp = g_lava_msg_file_is_gb2312 ?
      UTIL_OpenRequiredFile("M_GB2312.MSG") : UTIL_OpenRequiredFile("M.MSG");
   if (fp == 0)
   {
      return 0;
   }
   fseek(fp, start, SEEK_SET);
   if (fread((addr)g_lava_msg_buf, 1, len, fp) != len)
   {
      fclose(fp);
      return 0;
   }
   fclose(fp);
   g_lava_msg_buf[len] = 0;
   return g_lava_msg_buf;
}

static char *PAL_LavaScene1MsgText(int msg_id)
{
   return PAL_LavaReadMsg(msg_id);
#if 0
   char *msg;

     if (msg_id == 585) return "嘿嘿...";
   if (msg_id == 586) return "李大娘：";
   if (msg_id == 587) return "逍遥！窝在房里做啥？";
   if (msg_id == 588) return "还不快出来帮忙招呼客人。";
   if (msg_id == 589) return "李逍遥：";
   if (msg_id == 590) return "啊！...我马上就去。";
   if (msg_id == 591) return "双手端着物品无法爬下去。";
   if (msg_id == 592) return "双手端着物品无法爬上去。";
   if (msg_id == 593) return "赵灵儿：";
   if (msg_id == 594) return "哦...这通道是...?";
   if (msg_id == 595) return "李逍遥：";
   if (msg_id == 596) return "哈~那些苗人怎么也不会料到，";
   if (msg_id == 597) return "我在这里做了一个密道，结果";
   if (msg_id == 598) return "就刚好救了你呢...";
   if (msg_id == 599) return "李逍遥：";
   if (msg_id == 600) return "灵儿...你要不要爬爬看？";
   if (msg_id == 601) return "赵灵儿：";
   if (msg_id == 602) return "你好坏~叫女孩子爬这种东西";
   if (msg_id == 603) return "很难看呢！人家才不要...";
   if (msg_id == 604) return "李大娘：";
   if (msg_id == 605) return "搞什么~慢吞吞的！";
   if (msg_id == 606) return "李大娘：";
   if (msg_id == 607) return "各位客倌...里边儿请...";
   if (msg_id == 608) return "李大娘：";
   if (msg_id == 609) return "逍遥！帮我招呼客倌们歇歇腿，";
   if (msg_id == 610) return "我到厨房准备酒菜...";
   if (msg_id == 611) return "苗人头领：";
   if (msg_id == 612) return "小二！这间客栈我们包下了，";
   if (msg_id == 613) return "除了老板和伙计，其他不相干";
   if (msg_id == 614) return "的人全都给我请出去。";
   if (msg_id == 615) return "李逍遥：";
   if (msg_id == 616) return "小店今天没别的客人，各位客";
   if (msg_id == 617) return "倌...啊~不！请问各位大爷";
   if (msg_id == 618) return "们还有啥吩咐的？";
   if (msg_id == 619) return "苗人头领：";
   if (msg_id == 620) return "以后没有我们的吩咐，不许闲";
   if (msg_id == 621) return "杂人等上楼来，知道了吗？";
   if (msg_id == 622) return "李逍遥：";
   if (msg_id == 623) return "是...这容易，小的一定照办。";
   if (msg_id == 624) return "苗人头领：";
   if (msg_id == 625) return "很好！这些银子你拿去，往后";
   if (msg_id == 626) return "这几天只要你乖乖听我们的话";
   if (msg_id == 627) return "办事，赏银不会少你的。";
   if (msg_id == 628) return "李逍遥：";
   if (msg_id == 629) return "是~谢大爷的赏...";
   if (msg_id == 630) return "小店一定让您宾至如归！";
   if (msg_id == 631) return "得到500文钱。";
   if (msg_id == 632) return "李逍遥：";
   if (msg_id == 633) return "哇哈！真是遇到财神爷了。";
   if (msg_id == 634) return "别怠慢了客人。";
   if (msg_id == 635) return "李大娘：";
   if (msg_id == 636) return "逍遥你来的正好，快把门口";
   if (msg_id == 637) return "那个臭要饭的赶走，免得妨";
   if (msg_id == 638) return "碍咱们做生意。";
   if (msg_id == 639) return "记得喔！";
   if (msg_id == 640) return "等一下到厨房来帮忙端菜。";
   if (msg_id == 641) return "苗人头领：";
   if (msg_id == 642) return "没事了...去忙你的吧。";
   if (msg_id == 643) return "苗人喽啰：";
   if (msg_id == 644) return "没事...你可以走了。";
   if (msg_id == 645) return "苗人喽啰：";
   if (msg_id == 646) return "喂！店小二，大爷们饿了。";
   if (msg_id == 647) return "快点把酒菜送上来。";
   if (msg_id == 648) return "去忙你的吧...";
    if (msg_id == 649) return "苗人头领：";
    if (msg_id == 650) return "呵~累了，想早点休息。";
    if (msg_id == 657) return "李大娘：";
    if (msg_id == 658) return "瞧你闲成这样，还不快去帮忙。";
    if (msg_id == 659) return "别在这儿挡手挡脚的。";
    if (msg_id == 663) return "苗人头领：";
    if (msg_id == 664) return "你去忙吧..";
    if (msg_id == 665) return "有事我再叫你。";
    if (msg_id == 666) return "苗人喽啰：";
    if (msg_id == 667) return "嗯..这烧鸡味道不错。";
    if (msg_id == 670) return "苗人喽啰：";
    if (msg_id == 671) return "从苗疆一路赶到这儿来，";
    if (msg_id == 672) return "总算可以好好吃个一顿。";
    if (msg_id == 674) return "李逍遥：";
    if (msg_id == 675) return "这酒菜我先端过去好了。";
    if (msg_id == 676) return "各位大爷，酒菜来了。";
    if (msg_id == 677) return "苗人头领：";
    if (msg_id == 678) return "嗯...这是什么酒？";
    if (msg_id == 679) return "一点香味也没有。";
    if (msg_id == 680) return "李逍遥：";
    if (msg_id == 681) return "哦...大爷您有所不知，";
    if (msg_id == 682) return "这道菜得配桂花酒，喝起来";
    if (msg_id == 683) return "才够味，不然再好的菜也都";
    if (msg_id == 684) return "少了点意思呢。";
    if (msg_id == 685) return "苗人头领：";
    if (msg_id == 686) return "胡扯，胡扯！我们喝惯了烈酒，";
    if (msg_id == 687) return "才不要你们这淡酒。";
    if (msg_id == 688) return "李逍遥：";
    if (msg_id == 689) return "怎么会，小的这就给您换一壶。";
    if (msg_id == 690) return "苗人头领：";
    if (msg_id == 691) return "算了，随便你，快去拿来。";
    if (msg_id == 692) return "桂花酒摆在桌上。";
    if (msg_id == 693) return "醉道士：";
    if (msg_id == 694) return "你们店里的酒菜也没什么嘛。";
    if (msg_id == 695) return "我瞧...还是刚才那位小哥";
    if (msg_id == 696) return "端来的那桌菜，看着更下酒。";
    if (msg_id == 697) return "说不定还有更香的好酒。";
    if (msg_id == 708) return "李大娘：";
    if (msg_id == 709) return "你别磨蹭了，快去帮忙吧。";
    if (msg_id == 710) return "我这边都忙不过来了。";
    if (msg_id == 720) return "李大娘：";
    if (msg_id == 721) return "逍遥，别发呆了。";
    if (msg_id == 722) return "赶紧去照看楼上的客人。";
     if (msg_id == 744) return "醉道士：";
     if (msg_id == 745) return "喂，小子，你这儿有好酒没有？";
     if (msg_id == 746) return "李逍遥：";
     if (msg_id == 747) return "有是有...那得看客官喝不喝得惯。";
     if (msg_id == 748) return "醉道士：";
     if (msg_id == 749) return "好！只要有酒喝，别的都好说。";
     if (msg_id == 750) return "快给我先来一壶尝尝。";
     if (msg_id >= 804 && msg_id <= 819) return "……";
     if (msg_id >= 824 && msg_id <= 832) return "……";
     if (msg_id >= 841 && msg_id <= 843) return "……";
     if (msg_id >= 1002 && msg_id <= 1004) return "……";
     if (msg_id >= 1049 && msg_id <= 1060) return "……";
     if (msg_id >= 1131 && msg_id <= 1145) return "……";
     if (msg_id >= 1163 && msg_id <= 1177) return "……";
     if (msg_id == 313) return "发现净衣符。";
    if (msg_id == 1265) return "得到止血草。";
   if (msg_id == 1266) return "得到皮帽。";
   if (msg_id == 1267) return "得到木鞋。";
   if (msg_id == 1513) return "一只空的麻布袋。";
   if (msg_id == 1898) return "李大娘：";
   if (msg_id == 1899) return "李逍遥！你皮痒啊？";
   if (msg_id == 1900) return "敢说老娘是什么鬼婆！";
   if (msg_id == 1901) return "李逍遥：";
   if (msg_id == 1902) return "哎哟，疼啊！";
   if (msg_id == 1903) return "李大娘：";
   if (msg_id == 1904) return "又在作白日梦了！你也老大不小了。";
    if (msg_id == 1905) return "整天疯疯癫癫地，也不学做正经事！";
    if (msg_id == 1906) return "还不快去干活！";
    if (msg_id >= 2105 && msg_id <= 2115) return "……";
    if (msg_id == 1907) return "李逍遥：";
   if (msg_id == 1908) return "婶婶~";
   if (msg_id == 1909) return "你不要每次叫人起床都拿锅呀、";
   if (msg_id == 1910) return "铲呀，乱敲一通的，会吓死人哪！";
   if (msg_id == 1911) return "咱们这木床又不牢靠，万一我";
   if (msg_id == 1912) return "给摔死了，咱们李家就绝后啦。";
   if (msg_id == 1913) return "咱们李家就绝后啦。";
   if (msg_id == 1914) return "李大娘：";
   if (msg_id == 1915) return "不这样叫得醒你吗？好歹你也";
   if (msg_id == 1916) return "跟林师傅学过几个月的木工，";
   if (msg_id == 1917) return "床不牢自己动手修一修不就好了？";
    if (msg_id == 1918) return "了？";
   if (msg_id == 1919) return "就只会削些木刀木剑的，成天";
   if (msg_id == 1920) return "学你爹舞刀弄剑，没个定性，";
   if (msg_id == 1921) return "有哪家姑娘愿意嫁给你哦...";
   if (msg_id == 1922) return "李逍遥：";
   if (msg_id == 1923) return "那我爹怎么会娶到我娘？";
   if (msg_id == 1924) return "李大娘：";
   if (msg_id == 1925) return "啧！你娘也是跟你爹一个样儿，";
   if (msg_id == 1926) return "嫁到咱们李家来，也不做针线女红，";
   if (msg_id == 1927) return "就只会跟着你爹疯...";
   if (msg_id == 1928) return "李逍遥：";
   if (msg_id == 1929) return "嘿...大家都说~他们是江湖上";
   if (msg_id == 1930) return "人人羡慕的鸳鸯侠侣呢！";
   if (msg_id == 1931) return "李大娘：";
   if (msg_id == 1932) return "是哦~侠侣？说要去行侠仗义，";
   if (msg_id == 1933) return "丢下你这惹祸精，一去无回。";
   if (msg_id == 1934) return "还不是我这老太婆省吃俭用地";
   if (msg_id == 1935) return "开了这么一家小小的客栈，才";
   if (msg_id == 1936) return "把你拉拔到这么大，结果养出";
   if (msg_id == 1937) return "这么一个懒鬼！";
   if (msg_id == 1938) return "李逍遥：";
   if (msg_id == 1939) return "谁说我是懒鬼啦？";
   if (msg_id == 1940) return "李逍遥：";
   if (msg_id == 1941) return "我将来要像我爹娘一样，";
   if (msg_id == 1942) return "练成绝世武功，纵横四海、";
   if (msg_id == 1943) return "称霸江湖的一代大侠！";
   if (msg_id == 1944) return "李大娘：";
   if (msg_id == 1945) return "少跟老娘鬼扯淡！";
   if (msg_id == 1946) return "你呀~游手好闲是出了名的。";
   if (msg_id == 1947) return "要不是这回我忙不过来，才";
   if (msg_id == 1948) return "不指望你这懒鬼来帮忙呢！";
   if (msg_id == 1949) return "李逍遥：";
   if (msg_id == 1950) return "一大早就有客人上门啦？";
   if (msg_id == 1951) return "李大娘：";
   if (msg_id == 1952) return "是啊...还不快过来帮忙！";
   if (msg_id == 1953) return "李逍遥：";
   if (msg_id == 1954) return "真没意思...一大清早就要";
   if (msg_id == 1955) return "人家做这个又做那个的...";
   if (msg_id == 1956) return "嘿...";
   if (msg_id == 1957) return "昨晚做好的密道正好派上用场。";
   if (msg_id == 1958) return "这次就从这里溜出去吧...";
   if (msg_id == 1959) return "李大娘：";
   if (msg_id == 1960) return "逍遥！还窝在房里干啥？";
   if (msg_id == 1961) return "快出来帮忙招呼客人。";
   if (msg_id == 1962) return "喔！...我马上就去。";
    if (msg_id == 1963) return "啧~算了，晚上再用密道吧。";
    if (msg_id == 1964) return "现在被发现就惨了。";

    msg = PAL_LavaReadMsg(msg_id);
    if (msg != 0 && msg[0] != 0)
    {
       return msg;
    }

    return 0;
#endif
}

static int PAL_LavaMsgUsesFallback(int msg_id)
{
   return 0;
}

static void PAL_LavaDrawDialogFace(void)
{
    int size;
    int w;
    int h;
    int x;
    int y;
    FILE *fp;
    LPCBITMAPRLE face;

    if (g_lava_dialog_face_num <= 0)
    {
       return;
    }

    fp = UTIL_OpenFile("RGM.MKF");
    if (fp == 0)
    {
       return;
    }

    size = PAL_MKFReadChunk((addr)g_lava_sprite_buf, 64000,
      g_lava_dialog_face_num, fp);
    fclose(fp);
    if (size <= 0)
    {
       return;
   }

   face = (LPCBITMAPRLE)g_lava_sprite_buf;
   w = PAL_RLEGetWidth(face);
   h = PAL_RLEGetHeight(face);
   if (g_lava_dialog_location == LAVA_DIALOG_LOWER)
   {
      x = 270 - w / 2;
      y = 144 - h / 2;
   }
   else
   {
      x = 48 - w / 2;
      y = 55 - h / 2;
   }
   if (x < 0) x = 0;
   if (y < 0) y = 0;

   PAL_RLEBlitToSurface(face, gpScreen, PAL_XY(x, y));
}

static int PAL_LavaIsDialogTitle(char *text)
{
   int len;

   if (text == 0)
   {
      return 0;
   }

   len = strlen(text);
   if (len <= 0)
   {
      return 0;
   }

   if (text[len - 1] == ':')
   {
      return 1;
   }

   if (len >= 2 && PAL_U8(text[len - 2]) == 0xA3 && PAL_U8(text[len - 1]) == 0xBA)
   {
      return 1;
   }

   return 0;
}

static int PAL_LavaTextPixelWidth(char *text)
{
   int i;
   int width;
   int ch;

   if (text == 0)
   {
      return 0;
   }

   i = 0;
   width = 0;
   while (text[i] != 0)
   {
      ch = PAL_U8(text[i]);
      if (ch < 0x80)
      {
         width += 8;
         i++;
      }
      else
      {
         width += 16;
         if (text[i + 1] != 0)
         {
            i += 2;
         }
         else
         {
            i++;
         }
      }
   }

    return width;
}

static void PAL_LavaDebugDumpTextBytes(int msg_id, char *text)
{
   int i;

   if (text == 0)
   {
      return;
   }

   printf("[LAVA][TEXTBYTES] msg=%d bytes=", msg_id);
   for (i = 0; text[i] != 0 && i < 64; i++)
   {
      printf("%02X", PAL_U8(text[i]));
      if (text[i + 1] != 0 && i < 63)
      {
         printf(" ");
      }
   }
   printf("\n");
}

static void PAL_LavaDrawDialogWaitIcon(void)
{
    int size;
    FILE *fp;
    LPCBITMAPRLE icon;

    if (g_lava_dialog_icon_x <= 0)
    {
       return;
    }

    fp = UTIL_OpenRequiredFile("DATA.MKF");
    if (fp == 0)
    {
       return;
    }

    size = PAL_MKFReadChunk((addr)g_lava_dialog_icons_buf, 512,
      12, fp);
    fclose(fp);
    if (size <= 0)
    {
      return;
   }

   icon = PAL_SpriteGetFrame((addr)g_lava_dialog_icons_buf, 0);
   if (icon != 0)
   {
      PAL_RLEBlitToSurface(icon, gpScreen, PAL_XY(g_lava_dialog_icon_x, g_lava_dialog_icon_y));
   }
}

static int PAL_LavaDialogTitleX(void)
{
   if (g_lava_dialog_location == LAVA_DIALOG_CENTER)
   {
      return 44;
   }
   if (g_lava_dialog_location == LAVA_DIALOG_LOWER)
   {
      return (g_lava_dialog_face_num > 0) ? 4 : 12;
   }
   return (g_lava_dialog_face_num > 0) ? 80 : 12;
}

static int PAL_LavaDialogTitleY(void)
{
   if (g_lava_dialog_location == LAVA_DIALOG_CENTER)
   {
      return 72;
   }
   return (g_lava_dialog_location == LAVA_DIALOG_LOWER) ? 100 : 8;
}

static int PAL_LavaDialogTextX(void)
{
   if (g_lava_dialog_location == LAVA_DIALOG_CENTER)
   {
      return 44;
   }
   if (g_lava_dialog_location == LAVA_DIALOG_LOWER)
   {
      return (g_lava_dialog_face_num > 0) ? 20 : 44;
   }
   return (g_lava_dialog_face_num > 0) ? 96 : 44;
}

static int PAL_LavaDialogTextY(void)
{
   if (g_lava_dialog_location == LAVA_DIALOG_CENTER)
   {
      return 90;
   }
   return (g_lava_dialog_location == LAVA_DIALOG_LOWER) ? 118 : 26;
}

static void PAL_LavaDrawDialogPage(int start, int end)
{
   int y;
   int i;
   char *text;
   int title_x;
   int title_y;
   int text_x;
   int text_y;
   int title_done;
   int text_color;

   printf("[LAVA][DIALOGPAGE] page start=%d end=%d loc=%d face=%d color=%d\n",
      start, end, g_lava_dialog_location, g_lava_dialog_face_num, g_lava_dialog_font_color);
   text_color = (g_lava_dialog_font_color < 0) ? 79 : g_lava_dialog_font_color;
   for (i = start; i < end; i++)
   {
      text = PAL_LavaScene1MsgText(g_lava_dialog_event_a[i]);
      if (text != 0)
      {
         printf("[LAVA][DIALOGTEXT] msg=%d src=%s\n", g_lava_dialog_event_a[i],
            PAL_LavaMsgUsesFallback(g_lava_dialog_event_a[i]) ? "fallback" : "mmsg");
         if (g_lava_dialog_event_a[i] == 723)
         {
            PAL_LavaDebugDumpTextBytes(g_lava_dialog_event_a[i], text);
         }
      }
   }

   PAL_LavaDrawSceneFrame();
   PAL_LavaDrawDialogFace();
   title_x = PAL_LavaDialogTitleX();
   title_y = PAL_LavaDialogTitleY();
   text_x = PAL_LavaDialogTextX();
   text_y = PAL_LavaDialogTextY();
   y = text_y;
   title_done = 0;
   g_lava_dialog_icon_x = 0;
   g_lava_dialog_icon_y = 0;
   for (i = start; i < end; i++)
   {
      text = PAL_LavaScene1MsgText(g_lava_dialog_event_a[i]);
      if (text != 0)
      {
         if (title_done == 0 && PAL_LavaIsDialogTitle(text))
         {
            title_done = 1;
         }
         else
         {
            g_lava_dialog_icon_x = text_x + PAL_LavaTextPixelWidth(text);
            g_lava_dialog_icon_y = y;
            y += 18;
         }
      }
   }
   PAL_LavaDrawDialogWaitIcon();
   PAL_LavaPresent();

   y = text_y;
   title_done = 0;
   for (i = start; i < end; i++)
   {
      text = PAL_LavaScene1MsgText(g_lava_dialog_event_a[i]);
      if (text != 0)
      {
         if (title_done == 0 && PAL_LavaIsDialogTitle(text))
         {
            PAL_LavaTextOutToSurfaceEx(gpScreen, title_x, title_y, text, 0x8C, 0, 1);
            title_done = 1;
         }
         else
         {
            PAL_LavaTextOutToSurfaceEx(gpScreen, text_x, y, text, text_color, 0, 1);
            y += 18;
         }
      }
   }
   lava_present_current_screen();
}

static void PAL_LavaDialogPause(void)
{
       if (g_lava_fast_script_probe || g_lava_autotest_search || g_lava_autotest_walk || g_lava_autotest_exits || g_lava_autotest_scene5 || g_lava_autotest_scene6 || g_lava_autotest_scene13 || g_lava_autotest_scene9 || g_lava_autotest_hooks || g_lava_autotest_door || g_lava_autotest_hall || g_lava_autotest_input || g_lava_autotest_intro || g_lava_autotest_xianling)
    {
       UTIL_Delay((g_lava_autotest_intro || g_lava_autotest_xianling) ? 1 : 120);
   }
   else
   {
   PAL_ClearKeyState();
   PAL_WaitForAnyKey(0);
   PAL_ClearKeyState();
   }
}

static void PAL_LavaRunPendingDialog(void)
{
    int i;
    int page_start;
    int lines;
    int has_page;
    int suppress_trailing_visuals;
    char *text;

   if (g_lava_dialog_event_count <= 0) return;
   printf("[LAVA][DIALOG] events=%d\n", g_lava_dialog_event_count);
   g_lava_dialog_font_color = -1;
    if (g_lava_autotest_intro)
    {
       for (i = 126; i < g_lava_dialog_event_count && i < 151; i++)
       {
          printf("[LAVA][EVENTQ] idx=%d type=%d a=%d b=%d c=%d\n",
             i,
             g_lava_dialog_event_type[i],
             g_lava_dialog_event_a[i],
             g_lava_dialog_event_b[i],
             g_lava_dialog_event_c[i]);
       }
    }
    PAL_LavaRestoreDialogReplaySnapshot();

    suppress_trailing_visuals = g_lava_dialog_before_battle;
    g_lava_dialog_before_battle = 0;
    page_start = -1;
    lines = 0;
    has_page = 0;
    for (i = 0; i < g_lava_dialog_event_count; i++)
   {
      if (g_lava_dialog_event_type[i] == LAVA_DIALOG_EVENT_START)
      {
         if (has_page)
         {
            PAL_LavaDrawDialogPage(page_start, i);
            PAL_LavaDialogPause();
         }
         g_lava_dialog_location = g_lava_dialog_event_a[i];
         g_lava_dialog_face_num = g_lava_dialog_event_b[i];
         g_lava_dialog_font_color = g_lava_dialog_event_c[i];
         page_start = i + 1;
         lines = 0;
         has_page = 0;
      }
      else if (g_lava_dialog_event_type[i] == LAVA_DIALOG_EVENT_CENTER)
      {
         if (g_lava_dialog_event_count < LAVA_DIALOG_EVENT_MAX)
         {
            g_lava_dialog_event_type[i] = LAVA_DIALOG_EVENT_START;
         }
         g_lava_dialog_location = LAVA_DIALOG_CENTER;
         g_lava_dialog_face_num = 0;
         g_lava_dialog_font_color = 0x4F;
         page_start = i + 1;
         lines = 0;
         has_page = 0;
      }
      else if (g_lava_dialog_event_type[i] == LAVA_DIALOG_EVENT_RESTORE)
      {
         if (has_page)
         {
            PAL_LavaDrawDialogPage(page_start, i);
            PAL_LavaDialogPause();
            page_start = -1;
            lines = 0;
            has_page = 0;
         }
         PAL_LavaDrawSceneFrame();
      }
      else if (g_lava_dialog_event_type[i] == LAVA_DIALOG_EVENT_TEXT)
      {
         text = PAL_LavaScene1MsgText(g_lava_dialog_event_a[i]);
         if (text != 0)
         {
            if (page_start < 0)
            {
               page_start = i;
            }
            if (!(lines == 0 && PAL_LavaIsDialogTitle(text)))
            {
               if (lines >= 4)
               {
                  PAL_LavaDrawDialogPage(page_start, i);
                  PAL_LavaDialogPause();
                  page_start = i;
                  lines = 0;
               }
               lines++;
            }
            has_page = 1;
         }
      }
      else
      {
         if (has_page)
         {
            PAL_LavaDrawDialogPage(page_start, i);
            PAL_LavaDialogPause();
            page_start = -1;
            lines = 0;
            has_page = 0;
         }
         if (suppress_trailing_visuals && i + 1 >= g_lava_dialog_event_count)
         {
            g_lava_dialog_silent_visual_replay = 1;
         }
         PAL_LavaReplayDialogVisualEvent(i);
         g_lava_dialog_silent_visual_replay = 0;
      }
    }

    if (has_page)
    {
       PAL_LavaDrawDialogPage(page_start, g_lava_dialog_event_count);
      PAL_LavaDialogPause();
   }

    g_lava_dialog_event_count = 0;
    g_lava_dialog_before_battle = 0;
    g_lava_dialog_replay_snapshot_valid = 0;
    if (!suppress_trailing_visuals)
    {
       PAL_LavaDrawSceneFrame();
   }
}

static int PAL_LavaEnterScriptCollectsDialogOp(long op)
{
   return op == 0x0005 || op == 0x003C || op == 0x003D || op == 0xFFFF;
}

static void PAL_LavaRunPendingEnterTail(void)
{
   if (g_lava_pending_enter_tail_script <= 0)
   {
      return;
   }

   printf("[LAVA][ENTER] discard deferred tail idx=%d\n",
      g_lava_pending_enter_tail_script);
   g_lava_pending_enter_tail_script = 0;
}

static void PAL_LavaRunPendingSceneEnter(void)
{
   WORD enter_script;

   if (!g_lava_scene_enter_pending)
   {
      return;
   }

   g_lava_scene_enter_pending = 0;
   enter_script = PAL_LavaSceneEnterScriptFor(g_lava_scene_num);
   if (enter_script <= 0)
   {
      return;
   }

   printf("[LAVA][SCENE] enter=%d\n", enter_script);
   PAL_LavaDumpScriptEntry(enter_script - 1);
   g_lava_defer_enter_tail_after_dialog = 1;
   PAL_LavaRunEnterScript(enter_script);
   g_lava_defer_enter_tail_after_dialog = 0;
   PAL_LavaUpdateViewport();
   if (g_lava_gpGlobals.fNeedToFadeIn)
   {
      PAL_SetPalette(g_lava_gpGlobals.wNumPalette, g_lava_gpGlobals.fNightPalette);
      lava_apply_palette_scale(g_current_palette, 0, 15);
   }
   PAL_LavaDrawSceneFrame();
   if (g_lava_gpGlobals.fNeedToFadeIn)
   {
      PAL_FadeIn(1);
      g_lava_gpGlobals.fNeedToFadeIn = FALSE;
   }
   PAL_LavaRunPendingDialog();
   PAL_LavaRunPendingEnterTail();
   PAL_ClearKeyState();
}

#include "lava_script.c"

static long PAL_LavaRunRoleTriggerScript(long script_index, int role_id)
{
   long next_script;
   int old_role_script;

   printf("[LAVA][ROLESCRIPT] enter script=%ld role=%d\n",
      script_index, role_id);
   old_role_script = g_lava_role_script;
   g_lava_role_script = 1;
   next_script = PAL_LavaRunTriggerScript(script_index, role_id);
   g_lava_role_script = old_role_script;
   printf("[LAVA][ROLESCRIPT] exit script=%ld role=%d next=%ld success=%d\n",
      script_index, role_id, next_script, g_lava_script_success);

   return next_script;
}

static void PAL_LavaInitSceneScriptRuntime(addr scene_table, long scene_size)
{
   int scene_count;
   int scene_index;

   if (g_lava_scene_runtime_scripts_inited)
   {
      return;
   }

   scene_count = (int)(scene_size / 8);
   if (scene_count > 300)
   {
      scene_count = 300;
   }

   for (scene_index = 0; scene_index < scene_count; scene_index++)
   {
      int scene_offset;

      scene_offset = scene_index * 8;
      g_lava_scene_runtime_enter[scene_index] =
         (WORD)PAL_LavaReadU16(scene_table, scene_offset + 2);
      g_lava_scene_runtime_teleport[scene_index] =
         (WORD)PAL_LavaReadU16(scene_table, scene_offset + 4);
   }

   g_lava_scene_runtime_scripts_inited = 1;
}

static void PAL_LavaDumpScriptEntry(int script_index)
{
   FILE *fp;
   long script_offset;
   char entry[8];
   long op;
   long a;
   long b;
   long c;
   int i;

   fp = UTIL_OpenRequiredFile("SSS.MKF");
   if (fp == 0) return;
   script_offset = PAL_LavaMKFChunkOffset(fp, 4);
   if (script_offset < 0)
   {
      fclose(fp);
      return;
   }

   for (i = 0; i < 260; i++)
   {
      fseek(fp, script_offset + (script_index + i) * 8, SEEK_SET);
      if (fread((addr)entry, 1, 8, fp) != 8)
      {
         fclose(fp);
         return;
      }

      op = PAL_LavaReadU16((addr)entry, 0);
      a = PAL_LavaReadU16((addr)entry, 2);
      b = PAL_LavaReadU16((addr)entry, 4);
      c = PAL_LavaReadU16((addr)entry, 6);
      printf("[LAVA][SCRIPT] idx=%d op=%d a=%d b=%d c=%d\n",
         script_index + i, (int)op, (int)a, (int)b, (int)c);
   }
   fclose(fp);
}

static void PAL_LavaRunEnterScript(int script_index)
{
   char entry[8];
   int idx;
   int steps;
   int dialog_started;
   int started;
   int enter_idle;
   int next_enter;
   int scene_changed;
   int target_x;
   int target_y;
   int x_offset;
   int y_offset;
   WORD teleport_script;
   addr evt;

   idx = script_index;
   steps = 0;
   dialog_started = 0;
   started = 0;
   enter_idle = 0;
   next_enter = script_index;
   scene_changed = 0;
   PAL_LavaCaptureDialogReplaySnapshot();
   g_lava_dialog_event_count = 0;
   while (steps < 256)
   {
      long op;
      long a;
      long b;
      long c;

      if (!PAL_LavaReadScriptEntry(idx, (addr)entry))
      {
         return;
      }

      op = PAL_LavaReadU16((addr)entry, 0);
      a = PAL_LavaReadU16((addr)entry, 2);
      b = PAL_LavaReadU16((addr)entry, 4);
      c = PAL_LavaReadU16((addr)entry, 6);

      if (g_lava_defer_enter_tail_after_dialog && dialog_started &&
          !PAL_LavaEnterScriptCollectsDialogOp(op))
      {
         started = 1;
         g_lava_pending_enter_tail_script = idx;
         next_enter = idx;
         printf("[LAVA][ENTER] defer tail idx=%d op=%d a=%d b=%d c=%d\n",
            idx, (int)op, (int)a, (int)b, (int)c);
         break;
      }

      if (op == 0)
      {
         break;
      }
      else if (op == 0x0001)
      {
         started = 1;
         next_enter = idx + 1;
         break;
      }
      else if (op == 0x0002)
      {
         started = 1;
         if (b == 0 || ++enter_idle < b)
         {
            next_enter = (int)a;
            break;
         }
         enter_idle = 0;
         idx++;
         next_enter = idx + 1;
      }
      else if (op == 0x0003)
      {
         started = 1;
         if (b == 0 || ++enter_idle < b)
         {
            idx = (int)a;
            next_enter = idx + 1;
            continue;
         }
         enter_idle = 0;
         idx++;
         next_enter = idx + 1;
      }
      else if (op == 0x0009)
      {
         started = 1;
         PAL_LavaQueueDialogEvent(LAVA_DIALOG_EVENT_WAIT, (int)a, 0, 0);
         idx++;
         next_enter = idx + 1;
      }
      else if (op == 0x0010)
      {
         started = 1;
         evt = PAL_LavaSceneEventData((int)a);
         if (evt != 0)
         {
            if (PAL_LavaSceneWalkEventObject(evt, (int)b, (int)c, 0))
            {
               idx++;
               next_enter = idx + 1;
            }
         }
         else
         {
            idx++;
            next_enter = idx + 1;
          }
       }
      else if (op == 0x0004)
      {
         started = 1;
         PAL_LavaRunTriggerScript((long)a, b == 0 ? 0 : (int)b);
         idx++;
         next_enter = idx + 1;
      }
      else if (op == 0x0043)
      {
         started = 1;
         PAL_LavaSetBackgroundMusic((int)a, (int)b);
         idx++;
         next_enter = idx + 1;
      }
      else if (op == 0x0044)
      {
         started = 1;
         evt = PAL_LavaSceneEventData((int)a);
         PAL_LavaQueueDialogEvent(LAVA_DIALOG_EVENT_PARTY_RIDE, (int)a, (int)b, (int)c);
         PAL_LavaRidePartyOnEventObject((int)a, evt, (int)a, (int)b, (int)c, 4);
         idx++;
         next_enter = idx + 1;
      }
      else if (op == 0x0040)
      {
         started = 1;
         evt = PAL_LavaResolveEventTarget(0, (int)a);
         if (evt != 0 && a != 0)
         {
            PAL_LavaWriteU16(evt, 14, (int)b);
         }
         idx++;
         next_enter = idx + 1;
       }
      else if (op == 0x0045)
      {
         started = 1;
         PAL_LavaSetBattleMusic((int)a);
         idx++;
         next_enter = idx + 1;
      }
      else if (op == 0x004A)
      {
         started = 1;
         g_lava_num_battle_field = a;
         idx++;
         next_enter = idx + 1;
      }
       else if (op == 0x0016)
       {
          started = 1;
          evt = PAL_LavaSceneEventData((int)a);
         if (evt != 0)
         {
            PAL_LavaQueueDialogEvent(LAVA_DIALOG_EVENT_EVENT_POSE, (int)a, (int)b, (int)c);
            PAL_LavaWriteU16(evt, 20, (int)b);
            PAL_LavaWriteU16(evt, 22, (int)c);
         }
          idx++;
          next_enter = idx + 1;
       }
       else if (op == 0x0012)
       {
          started = 1;
          evt = PAL_LavaSceneEventData((int)a);
          if (evt != 0)
          {
             PAL_LavaQueueDialogEvent(LAVA_DIALOG_EVENT_EVENT_POS, (int)a,
                g_lava_party_x + (int)b, g_lava_party_y + (int)c);
             PAL_LavaWriteU16(evt, 2, g_lava_party_x + (int)b);
             PAL_LavaWriteU16(evt, 4, g_lava_party_y + (int)c);
          }
          idx++;
          next_enter = idx + 1;
       }
       else if (op == 0x0024)
       {
          started = 1;
          if (a != 0)
          {
             int target_id;

             target_id = PAL_LavaResolveTargetObjectId(0, (int)a);
             evt = PAL_LavaResolveEventTarget(0, target_id);
             if (evt != 0 && target_id > 0)
             {
                PAL_LavaWriteU16(evt, 10, (int)b);
                g_lava_autoscript_pc[target_id - 1] = 0;
                g_lava_autoscript_idle[target_id - 1] = 0;
             }
          }
          idx++;
          next_enter = idx + 1;
       }
       else if (op == 0x0025)
       {
          started = 1;
          if (a != 0)
          {
             evt = PAL_LavaResolveEventTarget(0, (int)a);
             if (evt != 0)
             {
                PAL_LavaWriteU16(evt, 8, (int)b);
             }
          }
          idx++;
          next_enter = idx + 1;
       }
        else if (op == 0x0047)
        {
           started = 1;
           PAL_LavaPlaySoundEffect((int)a);
           idx++;
          next_enter = idx + 1;
       }
      else if (op == 0x0049)
      {
         started = 1;
         evt = PAL_LavaResolveEventTarget(0, (int)a);
         if (evt != 0 && a != 0)
         {
            PAL_LavaQueueDialogEvent(LAVA_DIALOG_EVENT_EVENT_STATE, (int)a, (int)b, 0);
            PAL_LavaWriteS16(evt, 12, (int)b);
         }
         idx++;
         next_enter = idx + 1;
      }
      else if (op == 0x0038)
      {
         started = 1;
         teleport_script = PAL_LavaSceneTeleportScriptFor(g_lava_scene_num);
         if (teleport_script != 0)
         {
            PAL_LavaRunTriggerScript((long)teleport_script, 0);
            break;
         }
         idx = (int)a;
         next_enter = idx + 1;
      }
      else if (op == 0x0050)
      {
         started = 1;
         VIDEO_UpdateScreen(0);
         PAL_FadeOut(a ? (int)a : 1);
         g_lava_gpGlobals.fNeedToFadeIn = TRUE;
         idx++;
         next_enter = idx + 1;
      }
      else if (op == 0x0051)
      {
         started = 1;
         VIDEO_UpdateScreen(0);
         PAL_SetPalette(g_lava_gpGlobals.wNumPalette, g_lava_gpGlobals.fNightPalette);
         PAL_FadeIn(a ? (int)a : 1);
         g_lava_gpGlobals.fNeedToFadeIn = FALSE;
         idx++;
         next_enter = idx + 1;
      }
      else if (op == 0x0053)
      {
         started = 1;
         g_lava_gpGlobals.fNightPalette = FALSE;
         idx++;
         next_enter = idx + 1;
      }
      else if (op == 0x0054)
      {
         started = 1;
         g_lava_gpGlobals.fNightPalette = TRUE;
         idx++;
         next_enter = idx + 1;
      }
      else if (op == 0x0059)
      {
         started = 1;
         if (a > 0 && a != g_lava_scene_num)
         {
            g_lava_defer_scene_preview_draw = 1;
            PAL_LavaSetScene((int)a);
            g_lava_defer_scene_preview_draw = 0;
            scene_changed = 1;
         }
         idx++;
         next_enter = idx + 1;
       }
      else if (op == 0x0077)
      {
         started = 1;
         PAL_LavaStopMusic((int)a);
         idx++;
         next_enter = idx + 1;
        }
      else if (op == 0x0046)
      {
         started = 1;
         PAL_LavaQueueDialogEvent(LAVA_DIALOG_EVENT_PARTY_POS, a * 32 + c * 16, b * 16 + c * 8, 0);
         g_lava_party_x = a * 32 + c * 16;
         g_lava_party_y = b * 16 + c * 8;
         idx++;
         next_enter = idx + 1;
      }
      else if (op == 0x006D)
      {
         started = 1;
         PAL_LavaSetSceneScriptHooks((int)a, (int)b, (int)c);
         if ((int)a == g_lava_scene_num)
         {
            if (b != 0)
            {
               g_lava_scene_script_enter = b;
            }
            if (c != 0)
            {
               g_lava_scene_script_teleport = c;
            }
         }
         idx++;
         next_enter = idx + 1;
       }
      else if (op == 0x0065)
      {
         int party_index;

         started = 1;
         if (a >= 0 && a < 6)
         {
            PAL_LavaSetRoleSpriteNum((int)a, (int)b);
            for (party_index = 0; party_index < g_lava_party_count && party_index < 3; party_index++)
            {
               if (g_lava_party_role[party_index] == (int)a)
               {
                  PAL_LavaQueueDialogEvent(LAVA_DIALOG_EVENT_PARTY_SPRITE, party_index, (int)b, 0);
                  g_lava_player_sprite_num[party_index] = b;
               }
            }
            if (c != 0)
            {
               PAL_LavaRefreshPartySprites();
            }
         }
         idx++;
         next_enter = idx + 1;
      }
        else if (op == 0x0014)
        {
           started = 1;
           evt = PAL_LavaSceneEventData((int)a);
           if (evt != 0)
           {
              PAL_LavaQueueDialogEvent(LAVA_DIALOG_EVENT_EVENT_GESTURE, (int)a, (int)a, 0);
              PAL_LavaWriteU16(evt, 22, (int)a);
              PAL_LavaWriteU16(evt, 20, 0);
           }
           idx++;
           next_enter = idx + 1;
        }
       else if (op == 0x008B)
       {
          started = 1;
          idx++;
          next_enter = idx + 1;
       }
       else if (op == 0x0093)
       {
          started = 1;
          idx++;
          next_enter = idx + 1;
       }
        else if (op == 0x006C)
        {
           started = 1;
           evt = PAL_LavaSceneEventData((int)a);
           if (evt != 0)
           {
              PAL_LavaQueueDialogEvent(LAVA_DIALOG_EVENT_EVENT_STEP, (int)a,
                 PAL_LavaReadS16((addr)entry, 4), PAL_LavaReadS16((addr)entry, 6));
              PAL_LavaWriteU16(evt, 2, PAL_LavaReadU16(evt, 2) + PAL_LavaReadS16((addr)entry, 4));
              PAL_LavaWriteU16(evt, 4, PAL_LavaReadU16(evt, 4) + PAL_LavaReadS16((addr)entry, 6));
              PAL_LavaAdvanceEventFrame(evt);
          }
          idx++;
          next_enter = idx + 1;
       }
      else if (op == 0x0070)
      {
         started = 1;
         target_x = a * 32 + c * 16;
         target_y = b * 16 + c * 8;
         PAL_LavaQueueDialogEvent(LAVA_DIALOG_EVENT_PARTY_WALK, target_x, target_y, 0);
         PAL_LavaWalkPartyToTarget(target_x, target_y);
         idx++;
         next_enter = idx + 1;
      }
      else if (op == 0x007A)
      {
         started = 1;
         target_x = a * 32 + c * 16;
         target_y = b * 16 + c * 8;
         PAL_LavaQueueDialogEvent(LAVA_DIALOG_EVENT_PARTY_WALK, target_x, target_y, 0);
         PAL_LavaWalkPartyToTargetSpeed(target_x, target_y, 8, 4, "007A");
         idx++;
          next_enter = idx + 1;
       }
      else if (op == 0x0015)
      {
         started = 1;
         PAL_LavaQueueDialogEvent(LAVA_DIALOG_EVENT_PARTY_POSE, (int)a, (int)b, 0);
         g_lava_party_direction = a;
         g_lava_party_frame = b;
         idx++;
         next_enter = idx + 1;
      }
      else if (op == 0x0075)
      {
         started = 1;
         g_lava_follower_count = 0;
         g_lava_party_count = 0;
         if (a != 0)
         {
            g_lava_party_role[g_lava_party_count] = a - 1;
            g_lava_party_count++;
         }
         if (b != 0)
         {
            g_lava_party_role[g_lava_party_count] = b - 1;
            g_lava_party_count++;
         }
         if (c != 0)
         {
            g_lava_party_role[g_lava_party_count] = c - 1;
            g_lava_party_count++;
         }
         if (g_lava_party_count == 0)
         {
            g_lava_party_role[0] = 0;
            g_lava_party_count = 1;
          }
          PAL_LavaRefreshPartySprites();
          PAL_LavaResetPartyTrail();
          idx++;
          next_enter = idx + 1;
      }
      else if (op == 0x0005)
      {
         started = 1;
         if (!g_lava_fast_script_probe && dialog_started)
         {
            PAL_LavaQueueDialogEvent(LAVA_DIALOG_EVENT_RESTORE, 0, 0, 0);
         }
         idx++;
         next_enter = idx + 1;
      }
      else if (op == 0x008E)
      {
         started = 1;
         if (g_lava_dialog_event_count < LAVA_DIALOG_EVENT_MAX)
         {
            g_lava_dialog_event_type[g_lava_dialog_event_count] = LAVA_DIALOG_EVENT_RESTORE;
            g_lava_dialog_event_a[g_lava_dialog_event_count] = 0;
            g_lava_dialog_event_b[g_lava_dialog_event_count] = 0;
            g_lava_dialog_event_c[g_lava_dialog_event_count] = 0;
            g_lava_dialog_event_count++;
          }
          idx++;
          next_enter = idx + 1;
       }
      else if (op == 0x00A1)
      {
         started = 1;
         g_lava_party_frame = 0;
         idx++;
         next_enter = idx + 1;
       }
      else if (op == 0x003C)
      {
         started = 1;
         if (g_lava_dialog_event_count < LAVA_DIALOG_EVENT_MAX)
         {
            g_lava_dialog_event_type[g_lava_dialog_event_count] = LAVA_DIALOG_EVENT_START;
            g_lava_dialog_event_a[g_lava_dialog_event_count] = LAVA_DIALOG_UPPER;
            g_lava_dialog_event_b[g_lava_dialog_event_count] = a;
            g_lava_dialog_event_c[g_lava_dialog_event_count] = (b == 0) ? -1 : b;
            g_lava_dialog_event_count++;
         }
         dialog_started = 1;
         idx++;
         next_enter = idx + 1;
      }
      else if (op == 0x003D)
      {
         started = 1;
         if (g_lava_dialog_event_count < LAVA_DIALOG_EVENT_MAX)
         {
            g_lava_dialog_event_type[g_lava_dialog_event_count] = LAVA_DIALOG_EVENT_START;
            g_lava_dialog_event_a[g_lava_dialog_event_count] = LAVA_DIALOG_LOWER;
            g_lava_dialog_event_b[g_lava_dialog_event_count] = a;
            g_lava_dialog_event_c[g_lava_dialog_event_count] = (b == 0) ? -1 : b;
            g_lava_dialog_event_count++;
         }
         dialog_started = 1;
         idx++;
         next_enter = idx + 1;
      }
      else if (op == 0xFFFF)
      {
         started = 1;
         if (g_lava_dialog_event_count < LAVA_DIALOG_EVENT_MAX && PAL_LavaScene1MsgText(a) != 0)
         {
            g_lava_dialog_event_type[g_lava_dialog_event_count] = LAVA_DIALOG_EVENT_TEXT;
            g_lava_dialog_event_a[g_lava_dialog_event_count] = a;
            g_lava_dialog_event_b[g_lava_dialog_event_count] = 0;
            g_lava_dialog_event_c[g_lava_dialog_event_count] = 0;
            g_lava_dialog_event_count++;
         }
         else if (dialog_started && PAL_LavaScene1MsgText(a) == 0)
         {
            printf("[LAVA][DIALOG] missing msg=%d\n", (int)a);
         }
         idx++;
         next_enter = idx + 1;
      }
      else
      {
         started = 1;
         printf("[LAVA][ENTER] skip unknown op=%d a=%d b=%d c=%d\n",
            (int)op, (int)a, (int)b, (int)c);
         idx++;
         next_enter = idx + 1;
      }

       steps++;
    }

    if (!scene_changed)
    {
       PAL_LavaSetActiveSceneEnterScript(next_enter);
    }
    printf("[LAVA][DIALOG] collected events=%d\n", g_lava_dialog_event_count);
   printf("[LAVA][SCRIPT] applied party_count=%d role0=%d sprite0=%d pos=(%d,%d) dir=%d frame=%d\n",
      g_lava_party_count, g_lava_party_role[0], g_lava_player_sprite_num[0],
      g_lava_party_x, g_lava_party_y, g_lava_party_direction, g_lava_party_frame);
   printf("[LAVA][HUB] view=(%d,%d) obj9=(%d,%d,s=%d,sp=%d,d=%d,f=%d) obj11=(%d,%d,s=%d,sp=%d,d=%d,f=%d) obj12=(%d,%d,s=%d,sp=%d,d=%d,f=%d)\n",
      g_lava_view_x, g_lava_view_y,
       PAL_LavaReadU16(PAL_LavaSceneEventData(9), 2), PAL_LavaReadU16(PAL_LavaSceneEventData(9), 4),
       PAL_LavaReadS16(PAL_LavaSceneEventData(9), 12), PAL_LavaReadU16(PAL_LavaSceneEventData(9), 16), PAL_LavaReadU16(PAL_LavaSceneEventData(9), 20), PAL_LavaReadU16(PAL_LavaSceneEventData(9), 22),
       PAL_LavaReadU16(PAL_LavaSceneEventData(11), 2), PAL_LavaReadU16(PAL_LavaSceneEventData(11), 4),
       PAL_LavaReadS16(PAL_LavaSceneEventData(11), 12), PAL_LavaReadU16(PAL_LavaSceneEventData(11), 16), PAL_LavaReadU16(PAL_LavaSceneEventData(11), 20), PAL_LavaReadU16(PAL_LavaSceneEventData(11), 22),
       PAL_LavaReadU16(PAL_LavaSceneEventData(12), 2), PAL_LavaReadU16(PAL_LavaSceneEventData(12), 4),
       PAL_LavaReadS16(PAL_LavaSceneEventData(12), 12), PAL_LavaReadU16(PAL_LavaSceneEventData(12), 16), PAL_LavaReadU16(PAL_LavaSceneEventData(12), 20), PAL_LavaReadU16(PAL_LavaSceneEventData(12), 22));
}

int PAL_LavaLoadPlayerRoles(void)
{
   FILE *fp_data;
   FILE *fp_mgo;
   long size;
   int sprite_base;
   int walk_base;
   long mgo_size;
   int total_frames;

   fp_data = UTIL_OpenRequiredFile("DATA.MKF");
   if (fp_data == 0) return -1;
   size = PAL_MKFReadChunk((addr)g_lava_data_buf, sizeof(g_lava_data_buf), 3, fp_data);
   fclose(fp_data);
   if (size <= 0) return -1;
   sprite_base = 12 * 2;
   walk_base = 384 * 2;
   g_lava_player_sprite_num[0] = PAL_LavaReadU16((addr)g_lava_data_buf, sprite_base + 0 * 2);
   g_lava_player_walk_frames[0] = PAL_LavaReadU16((addr)g_lava_data_buf, walk_base + 0 * 2);
   if (g_lava_player_walk_frames[0] <= 0 && g_lava_player_sprite_num[0] > 0)
   {
      fp_mgo = UTIL_OpenRequiredFile("MGO.MKF");
      if (fp_mgo == 0) return -1;
      mgo_size = PAL_MKFDecompressChunk((addr)g_lava_mkf_buf, 65536,
         g_lava_player_sprite_num[0], fp_mgo);
      fclose(fp_mgo);
      if (mgo_size > 0)
      {
         total_frames = PAL_SpriteGetNumFrames((addr)g_lava_mkf_buf);
         if (total_frames >= 16)
         {
            g_lava_player_walk_frames[0] = 4;
         }
         else if (total_frames >= 12)
         {
            g_lava_player_walk_frames[0] = 3;
         }
      }
   }
   if (g_lava_player_walk_frames[0] <= 0)
   {
      g_lava_player_walk_frames[0] = 3;
   }
   printf("[LAVA][PLAYER] sprite0=%d walk0=%d\n",
      g_lava_player_sprite_num[0], g_lava_player_walk_frames[0]);
   return 0;
}

static void PAL_LavaRefreshPartySprites(void)
{
   FILE *fp_mgo;
   int i;
   int sprite_base;
   int walk_base;
   long mgo_size;
   int total_frames;
   int role;

   if (g_lava_party_count <= 0)
   {
      g_lava_party_count = 1;
      g_lava_party_role[0] = 0;
   }

   if (g_lava_data_buf == 0)
   {
      return;
   }

   sprite_base = 12 * 2;
   walk_base = 384 * 2;
   fp_mgo = UTIL_OpenRequiredFile("MGO.MKF");
   for (i = 0; i < g_lava_party_count && i < 3; i++)
   {
      role = g_lava_party_role[i];
      if (role < 0 || role >= 6)
      {
         role = 0;
      }

      g_lava_player_sprite_num[i] = PAL_LavaReadU16((addr)g_lava_data_buf, sprite_base + role * 2);
      g_lava_player_walk_frames[i] = PAL_LavaReadU16((addr)g_lava_data_buf, walk_base + role * 2);
      if (g_lava_player_walk_frames[i] <= 0 && fp_mgo != 0 && g_lava_player_sprite_num[i] > 0)
      {
         mgo_size = PAL_MKFDecompressChunk((addr)g_lava_mkf_buf, 65536,
            g_lava_player_sprite_num[i], fp_mgo);
         if (mgo_size > 0)
         {
            total_frames = PAL_SpriteGetNumFrames((addr)g_lava_mkf_buf);
            if (total_frames >= 16)
            {
               g_lava_player_walk_frames[i] = 4;
            }
            else if (total_frames >= 12)
            {
               g_lava_player_walk_frames[i] = 3;
            }
         }
      }
      if (g_lava_player_walk_frames[i] <= 0)
      {
         g_lava_player_walk_frames[i] = 3;
      }
      printf("[LAVA][PARTY] slot=%d role=%d name=%s sprite=%d walk=%d\n",
         i, role, PAL_LavaRoleNameForLog(role), g_lava_player_sprite_num[i], g_lava_player_walk_frames[i]);
   }

   for (; i < 6; i++)
   {
      g_lava_player_sprite_num[i] = 0;
      g_lava_player_walk_frames[i] = 0;
   }

   for (i = 0; i < g_lava_follower_count && i < 2; i++)
   {
      role = g_lava_follower_role[i];
      if (role < 0 || role >= 6)
      {
         continue;
      }

      g_lava_player_sprite_num[3 + i] = PAL_LavaReadU16((addr)g_lava_data_buf, sprite_base + role * 2);
      g_lava_player_walk_frames[3 + i] = PAL_LavaReadU16((addr)g_lava_data_buf, walk_base + role * 2);
      if (g_lava_player_walk_frames[3 + i] <= 0)
      {
         g_lava_player_walk_frames[3 + i] = 3;
      }
      printf("[LAVA][FOLLOWER] slot=%d role=%d name=%s sprite=%d walk=%d\n",
         i, role, PAL_LavaRoleNameForLog(role), g_lava_player_sprite_num[3 + i], g_lava_player_walk_frames[3 + i]);
   }
   if (fp_mgo != 0) fclose(fp_mgo);
}

long PAL_LavaMKFChunkOffset(FILE *fp, int chunk_num)
{
   long offset;

   if (fp == 0) return -1;
   fseek(fp, 4 * chunk_num, SEEK_SET);
   if (fread(&offset, 1, 4, fp) != 4) return -1;
   return SDL_SwapLE32(offset);
}

static int PAL_LavaRLEWidth(LPCBITMAPRLE rle)
{
   if (rle == 0) return 0;
   if (PAL_U8(rle[0]) == 0x02 && PAL_U8(rle[1]) == 0x00 &&
      PAL_U8(rle[2]) == 0x00 && PAL_U8(rle[3]) == 0x00)
   {
      rle += 4;
   }
   return PAL_U8(rle[0]) | (PAL_U8(rle[1]) << 8);
}

static int PAL_LavaRLEHeight(LPCBITMAPRLE rle)
{
   if (rle == 0) return 0;
   if (PAL_U8(rle[0]) == 0x02 && PAL_U8(rle[1]) == 0x00 &&
      PAL_U8(rle[2]) == 0x00 && PAL_U8(rle[3]) == 0x00)
   {
      rle += 4;
   }
   return PAL_U8(rle[2]) | (PAL_U8(rle[3]) << 8);
}

static LPCBITMAPRLE PAL_LavaMapTileBitmap(int x, int y, int h, int layer)
{
   if (x < 0 || x >= 64 || y < 0 || y >= 128 || h < 0 || h > 1)
   {
      return 0;
   }

   g_lava_map_tile_value = PAL_LavaReadU32((addr)g_lava_map_tiles_buf, (((y * 64 + x) * 2 + h) * 4));
   if (layer == 0)
   {
      g_lava_map_tile_frame = (int)((g_lava_map_tile_value & 0xFF) | ((g_lava_map_tile_value >> 4) & 0x100));
   }
   else
   {
      g_lava_map_tile_value >>= 16;
      g_lava_map_tile_frame = (int)(((g_lava_map_tile_value & 0xFF) | ((g_lava_map_tile_value >> 4) & 0x100)) - 1);
   }

   return PAL_SpriteGetFrame((addr)g_lava_map_gop_buf, g_lava_map_tile_frame);
}

static void PAL_LavaMapBlitLayer(int view_x, int view_y, int layer)
{
   g_lmb_sy = view_y / 16 - 1;
   g_lmb_dy = (view_y + SCREEN_H) / 16 + 2;
   g_lmb_sx = view_x / 32 - 1;
   g_lmb_dx = (view_x + SCREEN_W) / 32 + 2;

   g_lmb_yp = g_lmb_sy * 16 - 8 - view_y;
   for (g_lmb_y = g_lmb_sy; g_lmb_y < g_lmb_dy; g_lmb_y++)
   {
      for (g_lmb_h = 0; g_lmb_h < 2; g_lmb_h++, g_lmb_yp += 8)
      {
         g_lmb_xp = g_lmb_sx * 32 + g_lmb_h * 16 - 16 - view_x;
         for (g_lmb_x = g_lmb_sx; g_lmb_x < g_lmb_dx; g_lmb_x++, g_lmb_xp += 32)
         {
            g_lmb_tile = PAL_LavaMapTileBitmap(g_lmb_x, g_lmb_y, g_lmb_h, layer);
            if (g_lmb_tile == 0)
            {
               if (layer != 0) continue;
               g_lmb_tile = PAL_LavaMapTileBitmap(0, 0, 0, layer);
            }
            if (g_lmb_tile != 0 && (PAL_U8(g_lmb_tile[0]) | (PAL_U8(g_lmb_tile[1]) << 8)) > 0)
            {
               PAL_RLEBlitToSurface(g_lmb_tile, gpScreen,
                  ((long)g_lmb_yp << 16) | (long)(g_lmb_xp & 0xFFFF));
               g_lava_dbg_map_tiles_drawn++;
            }
         }
      }
   }
}

static int PAL_LavaEventObjectBottom(addr evt, int view_y)
{
   int object_y;
   int layer;

   object_y = PAL_LavaReadU16(evt, 4);
   layer = PAL_LavaReadS16(evt, 6);
   return object_y - view_y + layer * 8 + 9;
}

static void PAL_LavaAddSpriteToDrawRawTile(LPCBITMAPRLE frame, int x, int y, int layer)
{
   if (frame == 0 || g_lava_sprites_to_draw_count >= LAVA_MAX_SPRITES_TO_DRAW)
   {
      return;
   }

   g_lava_sprites_to_draw[g_lava_sprites_to_draw_count].source_kind = LAVA_SPRITE_SOURCE_RAW_TILE;
   g_lava_sprites_to_draw[g_lava_sprites_to_draw_count].source_num = 0;
   g_lava_sprites_to_draw[g_lava_sprites_to_draw_count].frame_num = 0;
   g_lava_sprites_to_draw[g_lava_sprites_to_draw_count].raw_frame = frame;
   g_lava_sprites_to_draw[g_lava_sprites_to_draw_count].x = x;
   g_lava_sprites_to_draw[g_lava_sprites_to_draw_count].y = y;
   g_lava_sprites_to_draw[g_lava_sprites_to_draw_count].layer = layer;
   g_lava_sprites_to_draw_count++;
}

static void PAL_LavaAddSpriteToDrawMgo(int sprite_num, int frame_num, int x, int y, int layer)
{
   if (sprite_num <= 0 || g_lava_sprites_to_draw_count >= LAVA_MAX_SPRITES_TO_DRAW)
   {
      return;
   }

   g_lava_sprites_to_draw[g_lava_sprites_to_draw_count].source_kind = LAVA_SPRITE_SOURCE_MGO;
   g_lava_sprites_to_draw[g_lava_sprites_to_draw_count].source_num = sprite_num;
   g_lava_sprites_to_draw[g_lava_sprites_to_draw_count].frame_num = frame_num;
   g_lava_sprites_to_draw[g_lava_sprites_to_draw_count].raw_frame = 0;
   g_lava_sprites_to_draw[g_lava_sprites_to_draw_count].x = x;
   g_lava_sprites_to_draw[g_lava_sprites_to_draw_count].y = y;
   g_lava_sprites_to_draw[g_lava_sprites_to_draw_count].layer = layer;
   g_lava_sprites_to_draw_count++;
}

static int PAL_LavaFindMgoFrameMetric(int sprite_num, int frame_num)
{
   int i;

   for (i = 0; i < g_lava_mgo_metric_used; i++)
   {
      if (g_lava_mgo_metric_sprite[i] == sprite_num &&
          g_lava_mgo_metric_frame[i] == frame_num)
      {
         return i;
      }
   }
   return -1;
}

static int PAL_LavaRememberMgoFrameMetric(int sprite_num, int frame_num, int width, int height)
{
   int slot;

   if (g_lava_mgo_metric_used < LAVA_MGO_FRAME_METRIC_CACHE_SLOTS)
   {
      slot = g_lava_mgo_metric_used;
      g_lava_mgo_metric_used++;
   }
   else
   {
      slot = g_lava_mgo_metric_next;
      g_lava_mgo_metric_next++;
      if (g_lava_mgo_metric_next >= LAVA_MGO_FRAME_METRIC_CACHE_SLOTS)
      {
         g_lava_mgo_metric_next = 0;
      }
   }

   g_lava_mgo_metric_sprite[slot] = sprite_num;
   g_lava_mgo_metric_frame[slot] = frame_num;
   g_lava_mgo_metric_width[slot] = width;
   g_lava_mgo_metric_height[slot] = height;
   return slot;
}

static addr PAL_LavaGetCachedMgoSprite(int sprite_num)
{
   int i;
   int slot;

   if (sprite_num <= 0 || g_lava_fpMGO == 0)
   {
      return 0;
   }

   g_lava_mgo_sprite_cache_clock++;
   if (g_lava_mgo_sprite_cache_clock == 0)
   {
      g_lava_mgo_sprite_cache_clock = 1;
   }
   slot = -1;
   for (i = 0; i < LAVA_MGO_SPRITE_CACHE_SLOTS; i++)
   {
      if (g_lava_mgo_sprite_cache_num[i] == sprite_num)
      {
         g_lava_mgo_sprite_cache_stamp[i] = g_lava_mgo_sprite_cache_clock;
         return (addr)g_lava_mgo_sprite_cache[i];
      }
      if (slot < 0 || g_lava_mgo_sprite_cache_num[i] == 0 ||
          g_lava_mgo_sprite_cache_stamp[i] < g_lava_mgo_sprite_cache_stamp[slot])
      {
         slot = i;
         if (g_lava_mgo_sprite_cache_num[i] == 0)
         {
            break;
         }
      }
   }

   if (slot < 0 || PAL_MKFDecompressChunk((addr)g_lava_mgo_sprite_cache[slot], 65536,
       sprite_num, (FILE *)g_lava_fpMGO) < 0)
   {
      return 0;
   }
   g_lava_mgo_sprite_cache_num[slot] = sprite_num;
   g_lava_mgo_sprite_cache_stamp[slot] = g_lava_mgo_sprite_cache_clock;
   g_lava_dbg_mgo_decompresses++;
   return (addr)g_lava_mgo_sprite_cache[slot];
}

static int PAL_LavaMapTileHeight(int x, int y, int h, int layer)
{
   long tile;

   if (x < 0 || x >= 64 || y < 0 || y >= 128 || h < 0 || h > 1)
   {
      return 0;
   }

   tile = PAL_LavaReadU32((addr)g_lava_map_tiles_buf, (((y * 64 + x) * 2 + h) * 4));
   if (layer != 0)
   {
      tile >>= 16;
   }

   return (int)((tile >> 8) & 0x0F);
}

static void PAL_LavaCalcCoverTilesBySize(int width, int height, int screen_x, int screen_y, int layer)
{
   if (width <= 0 || height <= 0)
   {
      return;
   }

   g_lct_sx = g_lava_view_x + screen_x - layer / 2;
   g_lct_sy = g_lava_view_y + screen_y - layer;
   g_lct_sh = ((g_lct_sx % 32) != 0) ? 1 : 0;
   for (g_lct_y = (g_lct_sy - height - 15) / 16; g_lct_y <= g_lct_sy / 16; g_lct_y++)
   {
      for (g_lct_x = (g_lct_sx - width / 2) / 32;
           g_lct_x <= (g_lct_sx + width / 2) / 32;
           g_lct_x++)
      {
         for (g_lct_i = ((g_lct_x == (g_lct_sx - width / 2) / 32) ? 0 : 3);
              g_lct_i < 5;
              g_lct_i++)
         {
            switch (g_lct_i)
            {
            case 0:
               g_lct_dx = g_lct_x;
               g_lct_dy = g_lct_y;
               g_lct_dh = g_lct_sh;
               break;
            case 1:
               g_lct_dx = g_lct_x - 1;
               g_lct_dy = g_lct_y;
               g_lct_dh = g_lct_sh;
               break;
            case 2:
               g_lct_dx = g_lct_sh ? g_lct_x : (g_lct_x - 1);
               g_lct_dy = g_lct_sh ? (g_lct_y + 1) : g_lct_y;
               g_lct_dh = 1 - g_lct_sh;
               break;
            case 3:
               g_lct_dx = g_lct_x + 1;
               g_lct_dy = g_lct_y;
               g_lct_dh = g_lct_sh;
               break;
            default:
               g_lct_dx = g_lct_sh ? (g_lct_x + 1) : g_lct_x;
               g_lct_dy = g_lct_sh ? (g_lct_y + 1) : g_lct_y;
               g_lct_dh = 1 - g_lct_sh;
               break;
            }

            for (g_lct_l = 0; g_lct_l < 2; g_lct_l++)
            {
               g_lct_tile = 0;
               g_lct_th = 0;
               if (!(g_lct_dx < 0 || g_lct_dx >= 64 ||
                     g_lct_dy < 0 || g_lct_dy >= 128 ||
                     g_lct_dh < 0 || g_lct_dh > 1))
               {
                  g_lct_off = (((g_lct_dy * 64 + g_lct_dx) * 2 + g_lct_dh) * 4);
                  g_lava_map_tile_value = (long)PAL_U8(g_lava_map_tiles_buf[g_lct_off]) |
                     ((long)PAL_U8(g_lava_map_tiles_buf[g_lct_off + 1]) << 8) |
                     ((long)PAL_U8(g_lava_map_tiles_buf[g_lct_off + 2]) << 16) |
                     ((long)PAL_U8(g_lava_map_tiles_buf[g_lct_off + 3]) << 24);
                  if (g_lct_l == 0)
                  {
                     g_lct_tf = (int)((g_lava_map_tile_value & 0xFF) |
                        ((g_lava_map_tile_value >> 4) & 0x100));
                     g_lct_th = (int)((g_lava_map_tile_value >> 8) & 0x0F);
                  }
                  else
                  {
                     g_lava_map_tile_value >>= 16;
                     g_lct_tf = (int)(((g_lava_map_tile_value & 0xFF) |
                        ((g_lava_map_tile_value >> 4) & 0x100)) - 1);
                     g_lct_th = (int)((g_lava_map_tile_value >> 8) & 0x0F);
                  }
                  g_lct_tile = PAL_SpriteGetFrame((addr)g_lava_map_gop_buf, g_lct_tf);
               }
               if (g_lct_tile != 0 && g_lct_th > 0 &&
                   (g_lct_dy + g_lct_th) * 16 + g_lct_dh * 8 >= g_lct_sy)
               {
                  PAL_LavaAddSpriteToDrawRawTile(g_lct_tile,
                     g_lct_dx * 32 + g_lct_dh * 16 - 16 - g_lava_view_x,
                     g_lct_dy * 16 + g_lct_dh * 8 + 7 + g_lct_l + g_lct_th * 8 - g_lava_view_y,
                     g_lct_th * 8 + g_lct_l);
               }
            }
         }
      }
   }
}

static int PAL_LavaDrawSceneEventObject(int object_id, int view_x, int view_y)
{
   int vanish_time;
   int object_x;
   int layer;
   int state;
   int sprite_num;
   int sprite_frames;
   int frame_stride;
   int direction;
   int current_frame;
   int frame;
   int metric;
   int width;
   int height;
   addr sprite;
   LPCBITMAPRLE frame_rle;
   int screen_x;
   int screen_y;
   addr evt;

   if (g_lava_fpMGO == 0 || g_lava_scene_ready == 0)
   {
      return 0;
   }

   evt = PAL_LavaSceneEventData(object_id);
   if (evt == 0)
   {
      return 0;
   }

   vanish_time = PAL_LavaReadS16((addr)evt, 0);
   object_x = PAL_LavaReadU16((addr)evt, 2);
   layer = PAL_LavaReadS16((addr)evt, 6);
   state = PAL_LavaReadS16((addr)evt, 12);
   sprite_num = PAL_LavaReadU16((addr)evt, 16);
   sprite_frames = PAL_LavaReadU16((addr)evt, 18);
   direction = PAL_LavaReadU16((addr)evt, 20);
   current_frame = PAL_LavaReadU16((addr)evt, 22);

   if (state == 0 || state < 0 || vanish_time > 0 || sprite_num == 0)
   {
      return 0;
   }

   frame = current_frame;
   frame_stride = sprite_frames;

   /* Frame animation is controlled by auto scripts (op 0x0087 etc.),
      not by wall-clock time. Using SDL_GetTicks here causes objects with
      multiple frames (like doors) to cycle continuously without trigger. */
   if (sprite_frames == 3)
   {
      if (frame == 2) frame = 0;
      if (frame == 3) frame = 2;
   }
   frame = direction * frame_stride + frame;
   metric = PAL_LavaFindMgoFrameMetric(sprite_num, frame);
   if (metric < 0)
   {
       sprite = PAL_LavaGetCachedMgoSprite(sprite_num);
       if (sprite == 0)
       {
          return 0;
       }
       frame_rle = PAL_SpriteGetFrame(sprite, frame);
      if (frame_rle == 0)
      {
         return 0;
      }
      width = PAL_LavaRLEWidth(frame_rle);
      height = PAL_LavaRLEHeight(frame_rle);
      metric = PAL_LavaRememberMgoFrameMetric(sprite_num, frame, width, height);
   }
   width = g_lava_mgo_metric_width[metric];
   height = g_lava_mgo_metric_height[metric];

   screen_x = object_x - view_x - width / 2;
   screen_y = PAL_LavaEventObjectBottom(evt, view_y);
   screen_y = screen_y - height - layer * 8 + 2;

   if (screen_x >= SCREEN_W || screen_x < -width)
   {
      return 0;
   }
   if (screen_y >= SCREEN_H || screen_y < -height)
   {
      return 0;
   }

   PAL_LavaAddSpriteToDrawMgo(sprite_num, frame, screen_x, PAL_LavaEventObjectBottom(evt, view_y), layer * 8 + 2);
   PAL_LavaCalcCoverTilesBySize(width, height, screen_x, PAL_LavaEventObjectBottom(evt, view_y), layer * 8 + 2);
   g_lava_dbg_event_sprites_drawn++;
   return 1;
}

static void PAL_LavaDrawSceneActors(int view_x, int view_y)
{
   int object_id;
   addr evt;

    g_lava_dbg_event_sprites_drawn = 0;
    g_lava_dbg_player_sprites_drawn = 0;

    if (g_lava_player_sprite_num[0] > 0 && !g_lava_intro_obj9_probe_active)
    {
       PAL_LavaDrawPartyLeader();
       PAL_LavaDrawPartyFollowers();
    }

    for (object_id = g_lava_scene_event_first;
       object_id <= g_lava_scene_event_last;
       object_id++)
    {
       evt = PAL_LavaSceneEventData(object_id);
       if (evt == 0) continue;
       if (PAL_LavaReadS16((addr)evt, 0) != 0 ||
           PAL_LavaReadS16((addr)evt, 12) <= 0 ||
           PAL_LavaReadU16((addr)evt, 16) == 0)
       {
          continue;
       }
       PAL_LavaDrawSceneEventObject(object_id, view_x, view_y);
    }

}

static void PAL_LavaDrawPartyLeader(void)
{
   LPCBITMAPRLE frame_rle;
   int frame;
   int frame_count;
   int sprite_num;
   int width;
   int height;
   int x;
   int y;
   addr sprite;

   sprite_num = g_lava_player_sprite_num[0];
   frame_count = g_lava_player_walk_frames[0];
   if (sprite_num <= 0) return;
   if (frame_count == 0) frame_count = 3;
   if (g_lava_party_frame < 0)
   {
      g_lava_party_frame = 0;
   }
   else if (g_lava_party_frame >= frame_count)
   {
      g_lava_party_frame %= frame_count;
   }

   sprite = PAL_LavaGetCachedMgoSprite(sprite_num);
   if (sprite == 0) return;

   frame = g_lava_party_direction * frame_count + g_lava_party_frame;
   frame_rle = PAL_SpriteGetFrame(sprite, frame);
   if (frame_rle == 0) return;

   width = PAL_LavaRLEWidth(frame_rle);
   height = PAL_LavaRLEHeight(frame_rle);
   x = g_lava_party_x - g_lava_view_x - width / 2;
   y = g_lava_party_y - g_lava_view_y + 10 - height;
   y += g_lava_party_layer;
   if (x >= SCREEN_W || x < -width) return;
   if (y >= SCREEN_H || y < -height) return;
   PAL_LavaAddSpriteToDrawMgo(sprite_num, frame, x, g_lava_party_y - g_lava_view_y + g_lava_party_layer + 10, g_lava_party_layer + 6);
   PAL_LavaCalcCoverTilesBySize(width, height, x, g_lava_party_y - g_lava_view_y + g_lava_party_layer + 10, g_lava_party_layer + 6);
   g_lava_dbg_player_sprites_drawn = 1;
}

static void PAL_LavaDrawPartyFollowers(void)
{
    int i;

    for (i = 1; i < g_lava_party_count && i < 3; i++)
    {
      LPCBITMAPRLE frame_rle;
      int frame;
      int frame_count;
      int sprite_num;
      int width;
      int height;
      int x;
      int y;
      int base_x;
      int base_y;
      int base_dir;
      int frame_dir;
      addr sprite;

      sprite_num = g_lava_player_sprite_num[i];
      frame_count = g_lava_player_walk_frames[i];
      if (sprite_num <= 0) continue;
      if (frame_count <= 0) frame_count = 3;

       sprite = PAL_LavaGetCachedMgoSprite(sprite_num);
       if (sprite == 0) continue;

      base_x = g_lava_party_trail_x[1];
      base_y = g_lava_party_trail_y[1];
      base_dir = g_lava_party_trail_dir[1];
      frame_dir = g_lava_party_trail_dir[2];
      if (i == 2)
      {
         base_x += (base_dir == kDirEast || base_dir == kDirWest) ? -16 : 16;
         base_y += 8;
      }
      else
      {
         base_x += (base_dir == kDirWest || base_dir == kDirSouth) ? 16 : -16;
         base_y += (base_dir == kDirWest || base_dir == kDirNorth) ? 8 : -8;
      }

      frame = frame_dir * frame_count + g_lava_party_frame;
       frame_rle = PAL_SpriteGetFrame(sprite, frame);
      if (frame_rle == 0) continue;

      width = PAL_LavaRLEWidth(frame_rle);
      height = PAL_LavaRLEHeight(frame_rle);
      x = base_x - g_lava_view_x - width / 2;
      y = base_y - g_lava_view_y + 10 - height + g_lava_party_layer;
      if (x >= SCREEN_W || x < -width) continue;
      if (y >= SCREEN_H || y < -height) continue;

      PAL_LavaAddSpriteToDrawMgo(sprite_num, frame, x,
         base_y - g_lava_view_y + g_lava_party_layer + 10,
         g_lava_party_layer + 4);
      PAL_LavaCalcCoverTilesBySize(width, height, x,
         base_y - g_lava_view_y + g_lava_party_layer + 10,
         g_lava_party_layer + 4);
      g_lava_dbg_player_sprites_drawn++;
   }

   if (g_lava_party_count == 1)
   {
      for (i = 0; i < g_lava_follower_count && i < 2; i++)
      {
         LPCBITMAPRLE f_frame_rle;
         int f_frame;
         int f_frame_count;
         int f_sprite_num;
         int f_width;
         int f_height;
         int f_x;
         int f_y;
         int trail_index;
         addr f_sprite;

         f_sprite_num = g_lava_player_sprite_num[3 + i];
         f_frame_count = g_lava_player_walk_frames[3 + i];
         if (f_sprite_num <= 0) continue;
         if (f_frame_count <= 0) f_frame_count = 3;

          f_sprite = PAL_LavaGetCachedMgoSprite(f_sprite_num);
          if (f_sprite == 0) continue;

         trail_index = 3 + i;
         f_frame = g_lava_party_trail_dir[trail_index] * f_frame_count + g_lava_party_frame;
          f_frame_rle = PAL_SpriteGetFrame(f_sprite, f_frame);
         if (f_frame_rle == 0) continue;

         f_width = PAL_LavaRLEWidth(f_frame_rle);
         f_height = PAL_LavaRLEHeight(f_frame_rle);
         f_x = g_lava_party_trail_x[trail_index] - g_lava_view_x - f_width / 2;
         f_y = g_lava_party_trail_y[trail_index] - g_lava_view_y + 10 - f_height + g_lava_party_layer;
         if (f_x >= SCREEN_W || f_x < -f_width) continue;
         if (f_y >= SCREEN_H || f_y < -f_height) continue;

         PAL_LavaAddSpriteToDrawMgo(f_sprite_num, f_frame, f_x,
            g_lava_party_trail_y[trail_index] - g_lava_view_y + g_lava_party_layer + 10,
            g_lava_party_layer + 4);
         PAL_LavaCalcCoverTilesBySize(f_width, f_height, f_x,
            g_lava_party_trail_y[trail_index] - g_lava_view_y + g_lava_party_layer + 10,
            g_lava_party_layer + 4);
         g_lava_dbg_player_sprites_drawn++;
      }
   }
}

static void PAL_LavaDrawSceneFrame(void)
{
   FILE *fp_mgo;
   int i;
   int j;
   int screen_x;
   int screen_y;
   int vy;
   int tmp_source_kind;
   int tmp_source_num;
   int tmp_frame_num;
   int tmp_x;
   int tmp_y;
   int tmp_layer;
   LPCBITMAPRLE tmp_raw_frame;
   LPCBITMAPRLE frame_rle;
   LPSPRITE sprite;
   int saved_direct_screen;

   if (g_lava_fpMGO == 0)
   {
      g_lava_fpMGO = (addr)UTIL_OpenRequiredFile("MGO.MKF");
   }
   fp_mgo = (FILE *)g_lava_fpMGO;
   saved_direct_screen = g_lava_direct_screen;
   g_lava_direct_screen = 1;
   ClearScreen();
   memset(g_screen_surface.pixels, 0, SCREEN_W * SCREEN_H);
   g_lava_dbg_map_tiles_drawn = 0;
   g_lava_dbg_mgo_decompresses = 0;
   g_lava_sprites_to_draw_count = 0;
   PAL_LavaMapBlitLayer(g_lava_view_x, g_lava_view_y, 0);
   PAL_LavaMapBlitLayer(g_lava_view_x, g_lava_view_y, 1);

   PAL_LavaDrawSceneActors(g_lava_view_x, g_lava_view_y);
   for (i = 0; i < g_lava_sprites_to_draw_count - 1; i++)
   {
      for (j = 0; j < g_lava_sprites_to_draw_count - 1 - i; j++)
      {
         if (g_lava_sprites_to_draw[j].y > g_lava_sprites_to_draw[j + 1].y)
         {
            tmp_source_kind = g_lava_sprites_to_draw[j].source_kind;
            tmp_source_num = g_lava_sprites_to_draw[j].source_num;
            tmp_frame_num = g_lava_sprites_to_draw[j].frame_num;
            tmp_raw_frame = g_lava_sprites_to_draw[j].raw_frame;
            tmp_x = g_lava_sprites_to_draw[j].x;
            tmp_y = g_lava_sprites_to_draw[j].y;
            tmp_layer = g_lava_sprites_to_draw[j].layer;

            g_lava_sprites_to_draw[j].source_kind = g_lava_sprites_to_draw[j + 1].source_kind;
            g_lava_sprites_to_draw[j].source_num = g_lava_sprites_to_draw[j + 1].source_num;
            g_lava_sprites_to_draw[j].frame_num = g_lava_sprites_to_draw[j + 1].frame_num;
            g_lava_sprites_to_draw[j].raw_frame = g_lava_sprites_to_draw[j + 1].raw_frame;
            g_lava_sprites_to_draw[j].x = g_lava_sprites_to_draw[j + 1].x;
            g_lava_sprites_to_draw[j].y = g_lava_sprites_to_draw[j + 1].y;
            g_lava_sprites_to_draw[j].layer = g_lava_sprites_to_draw[j + 1].layer;

            g_lava_sprites_to_draw[j + 1].source_kind = tmp_source_kind;
            g_lava_sprites_to_draw[j + 1].source_num = tmp_source_num;
            g_lava_sprites_to_draw[j + 1].frame_num = tmp_frame_num;
            g_lava_sprites_to_draw[j + 1].raw_frame = tmp_raw_frame;
            g_lava_sprites_to_draw[j + 1].x = tmp_x;
            g_lava_sprites_to_draw[j + 1].y = tmp_y;
            g_lava_sprites_to_draw[j + 1].layer = tmp_layer;
         }
      }
   }

   for (i = 0; i < g_lava_sprites_to_draw_count; i++)
   {
      frame_rle = 0;
      if (g_lava_sprites_to_draw[i].source_kind == LAVA_SPRITE_SOURCE_RAW_TILE)
      {
         frame_rle = g_lava_sprites_to_draw[i].raw_frame;
      }
       else if (g_lava_sprites_to_draw[i].source_kind == LAVA_SPRITE_SOURCE_MGO)
       {
          sprite = PAL_LavaGetCachedMgoSprite(g_lava_sprites_to_draw[i].source_num);
          if (sprite == 0) continue;
          frame_rle = PAL_SpriteGetFrame(sprite, g_lava_sprites_to_draw[i].frame_num);
      }
      if (frame_rle == 0)
      {
         continue;
      }
      screen_x = g_lava_sprites_to_draw[i].x;
      vy = g_lava_sprites_to_draw[i].y;
      screen_y = vy - PAL_LavaRLEHeight(frame_rle) - g_lava_sprites_to_draw[i].layer;
      PAL_RLEBlitToSurface(frame_rle, gpScreen, PAL_XY(screen_x, screen_y));
   }

   VIDEO_UpdateScreen(0);
   g_lava_direct_screen = saved_direct_screen;
}

static void PAL_LavaAdvanceScriptFrame(void)
{
   if (g_lava_fast_script_probe)
   {
      g_lava_logic_frame_num++;
      return;
   }

   g_lava_logic_frame_num++;
   PAL_LavaRunSceneAutoScripts();
   PAL_LavaUpdateViewport();
   PAL_LavaDrawSceneFrame();
   UTIL_Delay(g_lava_autotest_intro ? 1 : 60);
}

static int PAL_LavaSceneWalkEventObject(addr evt, int a, int b, int c)
{
   return PAL_LavaSceneWalkEventObjectSpeed(evt, a, b, c, 3);
}

static int PAL_LavaSceneWalkEventObjectSpeed(addr evt, int a, int b, int c, int speed)
{
   int x;
   int y;
   int target_x;
   int target_y;
   int step_x;
   int step_y;
   int dx;
   int dy;
   int dir;

   x = PAL_LavaReadU16(evt, 2);
   y = PAL_LavaReadU16(evt, 4);
   target_x = a * 32 + c * 16;
   target_y = b * 16 + c * 8;
   dx = target_x - x;
   dy = target_y - y;

   if (dx == 0 && dy == 0)
   {
      return TRUE;
   }

   if (speed <= 0)
   {
      speed = 3;
   }

   if (abs(dx) < speed * 2 || abs(dy) < speed)
   {
      x = target_x;
      y = target_y;
      step_x = dx;
      step_y = dy;
   }
   else
   {
      if (dx > 0)
      {
         step_x = speed * 2;
      }
      else if (dx < 0)
      {
         step_x = -(speed * 2);
      }
      else
      {
         step_x = 0;
      }

      if (dy > 0)
      {
         step_y = speed;
      }
      else if (dy < 0)
      {
         step_y = -speed;
      }
      else
      {
         step_y = 0;
      }

      x += step_x;
      y += step_y;
   }

    if (dy < 0)
    {
       dir = (dx < 0) ? 1 : 2;
    }
    else
    {
       dir = (dx < 0) ? 0 : 3;
    }

   PAL_LavaWriteU16(evt, 2, x);
   PAL_LavaWriteU16(evt, 4, y);
   PAL_LavaWriteU16(evt, 20, dir);
   PAL_LavaWriteU16(evt, 22, 0);

   return (x == target_x && y == target_y);
}

static void PAL_LavaMonsterChasePlayer(int object_id, addr evt, int chase_range, int speed, int floating)
{
   int x;
   int y;
   int dx;
   int dy;
   int dir;
   int next_x;
   int next_y;
   long distance;
   long threshold;

   if (evt == 0)
   {
      return;
   }

   if (chase_range == 0)
   {
      chase_range = 8;
   }
   if (speed == 0)
   {
      speed = 4;
   }
   if (g_lava_chase_range == 0)
   {
      return;
   }

   x = PAL_LavaReadU16(evt, 2);
   y = PAL_LavaReadU16(evt, 4);
   dx = g_lava_party_x - x;
   dy = g_lava_party_y - y;
   distance = abs(dx);
   distance += abs(dy) * 2;
   threshold = chase_range;
   threshold *= 32;
   threshold *= g_lava_chase_range;
   if (distance >= threshold)
   {
      return;
   }

   if (dx < 0)
   {
      dir = (dy < 0) ? 1 : 0;
   }
   else
   {
      dir = (dy < 0) ? 2 : 3;
   }

   next_x = x;
   next_y = y;
   if (dx != 0)
   {
      next_x += (dx < 0) ? -(speed * 2) : (speed * 2);
   }
   if (dy != 0)
   {
      next_y += (dy < 0) ? -speed : speed;
   }

   if (!floating && (PAL_LavaTileBlocked(next_x, next_y) || PAL_LavaEventBlocked(next_x, next_y, object_id)))
   {
      PAL_LavaWriteU16(evt, 20, dir);
      return;
   }

   PAL_LavaWriteU16(evt, 2, next_x);
   PAL_LavaWriteU16(evt, 4, next_y);
   PAL_LavaWriteU16(evt, 20, dir);
   PAL_LavaAdvanceEventFrame(evt);
}

static void PAL_LavaRunSceneAutoScripts(void)
{
   int object_id;

   if (g_lava_scene_ready == 0 || g_lava_scene_script_offset < 0)
   {
      return;
   }

   for (object_id = g_lava_scene_event_first;
      object_id <= g_lava_scene_event_last;
      object_id++)
   {
      addr evt;
      addr target_evt;
      long script_entry;
      char entry[8];
      int op;
      int a;
      int b;
      int c;
      int idle;
      int wait_frames;
      int advance_next;

      evt = PAL_LavaSceneEventData(object_id);
      if (evt == 0) continue;
      if (PAL_LavaReadS16(evt, 0) != 0 || PAL_LavaReadS16(evt, 12) <= 0)
      {
         continue;
      }

      script_entry = g_lava_autoscript_pc[object_id - 1];
      if (script_entry == 0)
      {
         script_entry = PAL_LavaReadU16(evt, 10);
      }
      if (((g_lava_autotest_walk || g_lava_autotest_search || g_lava_autotest_hooks || g_lava_autotest_door) &&
           g_lava_scene_num == 1 && (object_id == 4 || object_id == 11)) ||
          (g_lava_autotest_load && g_lava_scene_num == 4 &&
           (object_id == 77 || (object_id >= 85 && object_id <= 94))))
      {
         printf("[LAVA][AUTOOBJ] obj=%d state=%d trigger=%d mode=%d auto_entry=%d pc=%d idle=%d pos=(%d,%d)\n",
            object_id,
            PAL_LavaReadS16(evt, 12),
            PAL_LavaReadU16(evt, 8),
            PAL_LavaReadU16(evt, 14),
            PAL_LavaReadU16(evt, 10),
            script_entry,
            g_lava_autoscript_idle[object_id - 1],
            PAL_LavaReadU16(evt, 2),
            PAL_LavaReadU16(evt, 4));
      }
      if (script_entry == 0)
      {
         continue;
      }

      advance_next = 0;
      if (!PAL_LavaReadScriptEntry(script_entry, (addr)entry))
      {
         continue;
      }

      op = PAL_LavaReadU16((addr)entry, 0);
      a = PAL_LavaReadU16((addr)entry, 2);
      b = PAL_LavaReadU16((addr)entry, 4);
      c = PAL_LavaReadU16((addr)entry, 6);
      if (((g_lava_autotest_walk || g_lava_autotest_search || g_lava_autotest_hooks || g_lava_autotest_door) &&
           g_lava_scene_num == 1 && object_id == 11) ||
          (g_lava_autotest_load && g_lava_scene_num == 4 &&
           (object_id == 77 || (object_id >= 85 && object_id <= 94))))
      {
         printf("[LAVA][AUTOSTEP] obj=%d idx=%d op=%d a=%d b=%d c=%d\n",
            object_id, (int)script_entry, op, a, b, c);
      }

      if (op == 0x0000)
      {
         PAL_LavaStoreAutoscriptRuntime(evt, object_id);
         continue;
      }
      else if (op >= 0x000B && op <= 0x000E)
      {
         PAL_LavaSceneWalkEventOneStep(evt, op - 0x000B, 2);
         g_lava_autoscript_pc[object_id - 1] = script_entry + 1;
      }
      else if (op == 0x0001)
      {
         g_lava_autoscript_pc[object_id - 1] = script_entry + 1;
      }
      else if (op == 0x0002)
      {
         idle = g_lava_autoscript_idle[object_id - 1];
         if (b == 0 || ++idle < b)
         {
            g_lava_autoscript_idle[object_id - 1] = idle;
            g_lava_autoscript_pc[object_id - 1] = a;
         }
         else
         {
            g_lava_autoscript_idle[object_id - 1] = 0;
            g_lava_autoscript_pc[object_id - 1] = script_entry + 1;
          }
       }
      else if (op == 0x0003)
      {
         idle = g_lava_autoscript_idle[object_id - 1];
         if (b == 0 || ++idle < b)
         {
            g_lava_autoscript_idle[object_id - 1] = idle;
            g_lava_autoscript_pc[object_id - 1] = a;
            advance_next = 1;
         }
         else
         {
            g_lava_autoscript_idle[object_id - 1] = 0;
            g_lava_autoscript_pc[object_id - 1] = script_entry + 1;
         }
      }
      else if (op == 0x0004)
      {
         PAL_LavaRunTriggerScript((long)a, b == 0 ? object_id : (int)b);
         g_lava_autoscript_pc[object_id - 1] = script_entry + 1;
      }
      else if (op == 0x0006)
      {
         if (RandomLong(1, 100) >= a)
         {
            if (b != 0)
            {
               g_lava_autoscript_pc[object_id - 1] = b;
               advance_next = 1;
            }
            else
            {
               g_lava_autoscript_pc[object_id - 1] = script_entry;
            }
         }
         else
         {
            g_lava_autoscript_pc[object_id - 1] = script_entry + 1;
         }
      }
      else if (op == 0x0024)
      {
         int target_id;

         target_id = PAL_LavaResolveTargetObjectId(object_id, (int)a);
         target_evt = PAL_LavaResolveEventTarget(object_id, target_id);
         if (target_evt != 0 && target_id > 0)
         {
            PAL_LavaWriteU16(target_evt, 10, b);
            g_lava_autoscript_pc[target_id - 1] = 0;
            g_lava_autoscript_idle[target_id - 1] = 0;
         }
         g_lava_autoscript_pc[object_id - 1] = script_entry + 1;
      }
      else if (op == 0x0009)
      {
         wait_frames = (a != 0) ? a : 1;
         idle = g_lava_autoscript_idle[object_id - 1] + 1;
         if (idle >= wait_frames)
         {
            g_lava_autoscript_idle[object_id - 1] = 0;
            g_lava_autoscript_pc[object_id - 1] = script_entry + 1;
         }
         else
         {
            g_lava_autoscript_idle[object_id - 1] = idle;
            g_lava_autoscript_pc[object_id - 1] = script_entry;
         }
      }
      else if (op == 0x000F)
      {
         if (a != 0xFFFF)
         {
            PAL_LavaWriteU16(evt, 20, a);
         }
         if (b != 0xFFFF)
         {
            PAL_LavaWriteU16(evt, 22, b);
         }
         g_lava_autoscript_pc[object_id - 1] = script_entry + 1;
      }
      else if (op == 0x0014)
      {
         PAL_LavaWriteU16(evt, 22, a);
         PAL_LavaWriteU16(evt, 20, 0);
         g_lava_autoscript_pc[object_id - 1] = script_entry + 1;
      }
      else if (op == 0x0010)
      {
         if (PAL_LavaSceneWalkEventObject(evt, a, b, c))
         {
            g_lava_autoscript_pc[object_id - 1] = script_entry + 1;
         }
         else
         {
            g_lava_autoscript_pc[object_id - 1] = script_entry;
          }
       }
      else if (op == 0x0011)
      {
         if (((object_id & 1) ^ (g_lava_logic_frame_num & 1)) == 0)
         {
            g_lava_autoscript_pc[object_id - 1] = script_entry;
         }
         else if (PAL_LavaSceneWalkEventObjectSpeed(evt, a, b, c, 2))
         {
            g_lava_autoscript_pc[object_id - 1] = script_entry + 1;
         }
         else
         {
            g_lava_autoscript_pc[object_id - 1] = script_entry;
         }
       }
      else if (op == 0x0040)
      {
         target_evt = PAL_LavaResolveEventTarget(object_id, a);
         if (target_evt != 0)
         {
            PAL_LavaWriteU16(target_evt, 14, b);
         }
         g_lava_autoscript_pc[object_id - 1] = script_entry + 1;
      }
      else if (op == 0x0047)
      {
         PAL_LavaPlaySoundEffect(a);
         g_lava_autoscript_pc[object_id - 1] = script_entry + 1;
      }
      else if (op == 0x0049)
      {
         target_evt = PAL_LavaResolveEventTarget(object_id, a);
         if (target_evt != 0)
         {
            PAL_LavaWriteS16(target_evt, 12, b);
         }
         g_lava_autoscript_pc[object_id - 1] = script_entry + 1;
      }
      else if (op == 0x004C)
      {
         PAL_LavaMonsterChasePlayer(object_id, evt, a, b, c);
         g_lava_autoscript_pc[object_id - 1] = script_entry + 1;
      }
      else if (op == 0x006C)
      {
         target_evt = PAL_LavaResolveEventTarget(object_id, object_id);
         if (target_evt != 0)
         {
            PAL_LavaWriteU16(target_evt, 2, PAL_LavaReadU16(target_evt, 2) + PAL_LavaReadS16((addr)entry, 4));
            PAL_LavaWriteU16(target_evt, 4, PAL_LavaReadU16(target_evt, 4) + PAL_LavaReadS16((addr)entry, 6));
            PAL_LavaAdvanceEventFrame(target_evt);
         }
         g_lava_autoscript_pc[object_id - 1] = script_entry + 1;
      }
      else if (op == 0x0025)
      {
         target_evt = PAL_LavaResolveEventTarget(object_id, a);
         if (target_evt != 0)
         {
            PAL_LavaWriteU16(target_evt, 8, b);
         }
         g_lava_autoscript_pc[object_id - 1] = script_entry + 1;
      }
      else if (op == 0x0087)
      {
         PAL_LavaAdvanceEventFrame(evt);
         g_lava_autoscript_pc[object_id - 1] = script_entry + 1;
      }
      else if (op == -1 || op == 0xFFFF)
      {
         g_lava_autoscript_pc[object_id - 1] = script_entry + 1;
      }
      else
      {
         if (!PAL_LavaInterpretAutoFallback(object_id, evt, op, a, b, c, (addr)entry))
         {
            PAL_LavaLogAutoUnsupported(object_id, script_entry, op, a, b, c);
            g_lava_autoscript_pc[object_id - 1] = script_entry + 1;
         }
         else
         {
            g_lava_autoscript_pc[object_id - 1] = script_entry + 1;
         }
      }

      PAL_LavaStoreAutoscriptRuntime(evt, object_id);
      if (advance_next)
      {
         object_id--;
      }
   }
}

static int PAL_LavaSearchScene(void)
{
   long range_x[13];
   long range_y[13];
   int x;
   int y;
   int x_offset;
   int y_offset;
   int i;
   int object_id;

   x = g_lava_party_x;
   y = g_lava_party_y;
   x_offset = (g_lava_party_direction == 2 || g_lava_party_direction == 3) ? 16 : -16;
   y_offset = (g_lava_party_direction == 3 || g_lava_party_direction == 0) ? 8 : -8;

   range_x[0] = x;
   range_y[0] = y;
   for (i = 0; i < 4; i++)
   {
      range_x[i * 3 + 1] = x + x_offset;
      range_y[i * 3 + 1] = y + y_offset;
      range_x[i * 3 + 2] = x;
      range_y[i * 3 + 2] = y + y_offset * 2;
      range_x[i * 3 + 3] = x + x_offset * 2;
      range_y[i * 3 + 3] = y;
      x += x_offset;
      y += y_offset;
   }

   for (i = 0; i < 13; i++)
   {
      int dx;
      int dy;
      int dh;

      dx = (int)(range_x[i] / 32);
      dy = (int)(range_y[i] / 16);
      dh = ((int)range_x[i] % 32) ? 1 : 0;

      for (object_id = g_lava_scene_event_first;
         object_id <= g_lava_scene_event_last;
         object_id++)
      {
         addr evt;
         int ex;
         int ey;
         int eh;
         int state;
         int trigger_mode;
         long trigger_script;

         evt = PAL_LavaSceneEventData(object_id);
         if (evt == 0) continue;

         state = PAL_LavaReadS16(evt, 12);
         trigger_mode = PAL_LavaReadU16(evt, 14);
         trigger_script = PAL_LavaReadU16(evt, 8);
         ex = PAL_LavaReadU16(evt, 2) / 32;
         ey = PAL_LavaReadU16(evt, 4) / 16;
         eh = (PAL_LavaReadU16(evt, 2) % 32) ? 1 : 0;

         if (state <= 0 || trigger_mode >= 4 || trigger_script == 0 ||
             trigger_mode * 6 - 4 < i || dx != ex || dy != ey || dh != eh)
         {
            continue;
         }

         if (PAL_LavaReadU16(evt, 18) > 0)
         {
            PAL_LavaWriteU16(evt, 22, 0);
            PAL_LavaWriteU16(evt, 20, (g_lava_party_direction + 2) % 4);
         }

         printf("[LAVA][SEARCH] hit object=%d trigger=%d mode=%d checkpoint=%d pos=(%d,%d)\n",
            object_id, trigger_script, trigger_mode, i,
            PAL_LavaReadU16(evt, 2), PAL_LavaReadU16(evt, 4));
         PAL_LavaWriteU16(evt, 8,
            PAL_LavaRunTriggerScript(trigger_script, object_id));
         PAL_LavaFinishScriptStep();
         return 1;
      }
   }

   printf("[LAVA][SEARCH] miss pos=(%d,%d) dir=%d\n",
      g_lava_party_x, g_lava_party_y, g_lava_party_direction);
   return 0;
}

static int PAL_LavaCanSearchObject(int object_id, int party_x, int party_y, int party_dir)
{
   long range_x[13];
   long range_y[13];
   int x;
   int y;
   int x_offset;
   int y_offset;
   int i;
   addr evt;
   int ex;
   int ey;
   int eh;
   int state;
   int trigger_mode;
    long trigger_script;

   evt = PAL_LavaSceneEventData(object_id);
   if (evt == 0)
   {
      return 0;
   }

   state = PAL_LavaReadS16(evt, 12);
   trigger_mode = PAL_LavaReadU16(evt, 14);
   trigger_script = PAL_LavaReadU16(evt, 8);
   if (state <= 0 || trigger_mode >= 4 || trigger_script == 0)
   {
      return 0;
   }

   x = party_x;
   y = party_y;
   x_offset = (party_dir == 2 || party_dir == 3) ? 16 : -16;
   y_offset = (party_dir == 3 || party_dir == 0) ? 8 : -8;
   range_x[0] = x;
   range_y[0] = y;
   for (i = 0; i < 4; i++)
   {
      range_x[i * 3 + 1] = x + x_offset;
      range_y[i * 3 + 1] = y + y_offset;
      range_x[i * 3 + 2] = x;
      range_y[i * 3 + 2] = y + y_offset * 2;
      range_x[i * 3 + 3] = x + x_offset * 2;
      range_y[i * 3 + 3] = y;
      x += x_offset;
      y += y_offset;
   }

   ex = PAL_LavaReadU16(evt, 2) / 32;
   ey = PAL_LavaReadU16(evt, 4) / 16;
   eh = (PAL_LavaReadU16(evt, 2) % 32) ? 1 : 0;
   for (i = 0; i < 13; i++)
   {
      int dx;
      int dy;
      int dh;

      dx = (int)(range_x[i] / 32);
      dy = (int)(range_y[i] / 16);
      dh = ((int)range_x[i] % 32) ? 1 : 0;
      if (trigger_mode * 6 - 4 >= i && dx == ex && dy == ey && dh == eh)
      {
         return 1;
      }
   }

   return 0;
}

static int PAL_LavaAutotestSearchObject(int object_id)
{
   addr evt;
   int base_x;
   int base_y;
   int dir;
   int ox;
   int oy;

   evt = PAL_LavaSceneEventData(object_id);
   if (evt == 0)
   {
      return 0;
   }

   ox = PAL_LavaReadU16(evt, 2);
   oy = PAL_LavaReadU16(evt, 4);
   for (base_x = ox - 64; base_x <= ox + 64; base_x += 16)
   {
      for (base_y = oy - 32; base_y <= oy + 32; base_y += 8)
      {
         for (dir = 0; dir < 4; dir++)
         {
            if (!PAL_LavaCanSearchObject(object_id, base_x, base_y, dir))
            {
               continue;
            }

            g_lava_party_x = base_x;
            g_lava_party_y = base_y;
            g_lava_party_direction = dir;
            PAL_LavaUpdateViewport();
            PAL_LavaDrawSceneFrame();
            printf("[LAVA][AUTOTEST] try object=%d from pos=(%d,%d) dir=%d\n",
               object_id, g_lava_party_x, g_lava_party_y, g_lava_party_direction);
            return PAL_LavaSearchScene();
         }
      }
   }

   printf("[LAVA][AUTOTEST] no search position for object=%d\n", object_id);
   return 0;
}

static int PAL_LavaTileBlocked(int world_x, int world_y)
{
   int tile_x;
   int tile_y;
   int half;
   int xr;
   int yr;
   long tile;

   tile_x = world_x / 32;
   tile_y = world_y / 16;
   half = 0;
   xr = world_x % 32;
   yr = world_y % 16;

   if (xr + yr * 2 >= 16)
   {
      if (xr + yr * 2 >= 48)
      {
         tile_x++;
         tile_y++;
      }
      else if (32 - xr + yr * 2 < 16)
      {
         tile_x++;
      }
      else if (32 - xr + yr * 2 < 48)
      {
         half = 1;
      }
      else
      {
         tile_y++;
      }
   }

   if (tile_x < 0 || tile_x >= 64 || tile_y < 0 || tile_y >= 128)
   {
      if (g_lava_autotest_xianling)
      {
         printf("[LAVA][XIANLING][BLOCKSRC] tile out-of-range next=(%d,%d) tile=(%d,%d) half=%d\n",
            world_x, world_y, tile_x, tile_y, half);
      }
      return 1;
   }

   tile = PAL_LavaReadU32((addr)g_lava_map_tiles_buf, (((tile_y * 64 + tile_x) * 2 + half) * 4));
   if (g_lava_autotest_xianling && (tile & 0x2000))
   {
      printf("[LAVA][XIANLING][BLOCKSRC] tile next=(%d,%d) tile=(%d,%d) half=%d value=%ld\n",
         world_x, world_y, tile_x, tile_y, half, tile);
   }
   return (tile & 0x2000) ? 1 : 0;
}

static int PAL_LavaEventBlocked(int world_x, int world_y, int self_object_id)
{
   int object_id;

   for (object_id = g_lava_scene_event_first;
      object_id <= g_lava_scene_event_last;
      object_id++)
   {
      addr evt;
      int state;

      if (object_id == self_object_id)
      {
         continue;
      }

      evt = PAL_LavaSceneEventData(object_id);
      if (evt == 0)
      {
         continue;
      }

      state = PAL_LavaReadS16(evt, 12);
      if (state >= 2)
      {
         if (abs(PAL_LavaReadU16(evt, 2) - world_x) + abs(PAL_LavaReadU16(evt, 4) - world_y) * 2 < 16)
         {
            if (g_lava_autotest_xianling)
            {
               printf("[LAVA][XIANLING][BLOCKSRC] event object=%d next=(%d,%d) pos=(%d,%d) state=%d mode=%d trigger=%d auto=%d sprite=%d\n",
                  object_id,
                  world_x, world_y,
                  PAL_LavaReadU16(evt, 2), PAL_LavaReadU16(evt, 4),
                  state,
                  PAL_LavaReadU16(evt, 14),
                  PAL_LavaReadU16(evt, 8),
                  PAL_LavaReadU16(evt, 10),
                  PAL_LavaReadU16(evt, 16));
            }
             return 1;
          }
       }
   }

   return 0;
}

static int PAL_LavaTriggerBlockedSearchObject(int world_x, int world_y)
{
   int object_id;

   for (object_id = g_lava_scene_event_first;
      object_id <= g_lava_scene_event_last;
      object_id++)
   {
      addr evt;
      int state;
      int trigger_mode;
      long trigger_script;

      evt = PAL_LavaSceneEventData(object_id);
      if (evt == 0)
      {
         continue;
      }

      state = PAL_LavaReadS16(evt, 12);
      trigger_mode = PAL_LavaReadU16(evt, 14);
      trigger_script = PAL_LavaReadU16(evt, 8);
      if (state < 2 || trigger_mode >= 4 || trigger_script == 0)
      {
         continue;
      }

      if (abs(PAL_LavaReadU16(evt, 2) - world_x) + abs(PAL_LavaReadU16(evt, 4) - world_y) * 2 >= 16)
      {
         continue;
      }

      if (PAL_LavaReadU16(evt, 18) > 0)
      {
         PAL_LavaWriteU16(evt, 22, 0);
         PAL_LavaWriteU16(evt, 20, (g_lava_party_direction + 2) % 4);
      }
      printf("[LAVA][SEARCH] bump object=%d trigger=%d mode=%d pos=(%d,%d)\n",
         object_id, trigger_script, trigger_mode,
         PAL_LavaReadU16(evt, 2), PAL_LavaReadU16(evt, 4));
      PAL_LavaWriteU16(evt, 8,
         PAL_LavaRunTriggerScript(trigger_script, object_id));
      PAL_LavaFinishScriptStep();
      return 1;
   }

   return 0;
}

static void PAL_LavaAutotestRunTrigger(long script_index, int object_id)
{
   addr evt;
   long next_entry;

   printf("[LAVA][AUTOTEST] run trigger=%d object=%d\n", script_index, object_id);
   next_entry = PAL_LavaRunTriggerScript(script_index, object_id);
   evt = PAL_LavaSceneEventData(object_id);
   if (evt != 0 && PAL_LavaReadU16(evt, 8) == script_index)
   {
      PAL_LavaWriteU16(evt, 8, next_entry);
   }
   PAL_LavaFinishScriptStep();
}

static void PAL_LavaAutotestRunTriggerChain(int object_id, long initial_script, int max_steps)
{
   int step;
   long current_script;

   current_script = initial_script;
   for (step = 0; step < max_steps && current_script != 0; step++)
   {
      addr evt;
      long next_script;

      PAL_LavaAutotestRunTrigger(current_script, object_id);
      evt = PAL_LavaSceneEventData(object_id);
      if (evt == 0)
      {
         break;
      }

      next_script = PAL_LavaReadU16(evt, 8);
      if (next_script == 0 || next_script == current_script)
      {
         break;
      }

      printf("[LAVA][AUTOTEST] chain object=%d step=%d trigger=%d -> %d\n",
         object_id, step + 1, current_script, next_script);
      current_script = next_script;
   }
}

static void PAL_LavaAutotestRunObjectTriggerChain(int object_id, int max_steps)
{
   addr evt;
   long trigger_script;

   evt = PAL_LavaSceneEventData(object_id);
   if (evt == 0)
   {
      return;
   }

   trigger_script = PAL_LavaReadU16(evt, 8);
   if (trigger_script == 0)
   {
      return;
   }

   PAL_LavaAutotestRunTriggerChain(object_id, trigger_script, max_steps);
}

static void PAL_LavaAutotestRunBatchByScript(int *pairs)
{
   int i;

   i = 0;
   while (!(pairs[i] == 0 && pairs[i + 1] == 0))
   {
      PAL_LavaAutotestRunTrigger((long)pairs[i + 1], pairs[i]);
      i += 2;
   }
}

static void PAL_LavaAutotestRunBatchByObject(int *pairs)
{
   int i;

   i = 0;
   while (!(pairs[i] == 0 && pairs[i + 1] == 0))
   {
      PAL_LavaAutotestRunObjectTriggerChain(pairs[i], pairs[i + 1]);
      i += 2;
   }
}

static int PAL_LavaAutotestEnableSearchLog(void)
{
   int saved_search_log;

   saved_search_log = g_lava_autotest_search;
   g_lava_autotest_search = 1;
   return saved_search_log;
}

static void PAL_LavaAutotestRestoreSearchLog(int saved_search_log)
{
   g_lava_autotest_search = saved_search_log;
}

static void PAL_LavaAutotestSetupScene(int scene_num, int party_x, int party_y, int party_direction,
   int view_x, int view_y, int run_enter)
{
   PAL_LavaSetScene(scene_num);
   g_lava_party_x = party_x;
   g_lava_party_y = party_y;
   g_lava_party_direction = party_direction;
   g_lava_party_frame = 0;
   g_lava_view_x = view_x;
   g_lava_view_y = view_y;
   if (run_enter)
   {
      PAL_LavaRunPendingSceneEnter();
   }
   else
   {
      PAL_LavaUpdateViewport();
      PAL_LavaDrawSceneFrame();
   }
}

static void PAL_LavaAutotestSetupSceneByIndex(int setup_index)
{
   int i;

   i = setup_index * 7;
   PAL_LavaAutotestSetupScene(
      g_lava_autotest_scene_setup_table[i],
      g_lava_autotest_scene_setup_table[i + 1],
      g_lava_autotest_scene_setup_table[i + 2],
      g_lava_autotest_scene_setup_table[i + 3],
      g_lava_autotest_scene_setup_table[i + 4],
      g_lava_autotest_scene_setup_table[i + 5],
      g_lava_autotest_scene_setup_table[i + 6]);
}

static void PAL_LavaAutotestRunSearchObjects(int *objects, int delay_ms)
{
   int i;

   i = 0;
   while (objects[i] != 0)
   {
      UTIL_Delay(delay_ms);
      PAL_LavaAutotestSearchObject(objects[i]);
      i++;
   }
   UTIL_Delay(delay_ms);
}

static void PAL_LavaAutotestRunSearchSequence(void)
{
   PAL_LavaAutotestRunSequencePlan(g_lava_autotest_search_sequence_plan);
}

static void PAL_LavaAutotestRunIntroSequence(void)
{
   addr evt9;
   addr evt11;
   addr evt12;

   PAL_LavaRunPendingDialog();
   evt9 = PAL_LavaSceneEventData(9);
   evt11 = PAL_LavaSceneEventData(11);
   evt12 = PAL_LavaSceneEventData(12);
   printf("[LAVA][INTROFINAL] scene=%d enter=%d view=(%d,%d) party=(%d,%d) dir=%d frame=%d\n",
      g_lava_scene_num,
      g_lava_scene_script_enter,
      g_lava_view_x, g_lava_view_y,
      g_lava_party_x, g_lava_party_y,
      g_lava_party_direction, g_lava_party_frame);
   if (evt9 != 0 && evt11 != 0 && evt12 != 0)
   {
      printf("[LAVA][INTROFINAL] obj9=(%d,%d,s=%d,sp=%d,d=%d,f=%d) obj11=(%d,%d,s=%d,sp=%d,d=%d,f=%d,tr=%d,au=%d,pc=%d) obj12=(%d,%d,s=%d,sp=%d,d=%d,f=%d,tr=%d,au=%d,pc=%d)\n",
         PAL_LavaReadU16(evt9, 2), PAL_LavaReadU16(evt9, 4),
         PAL_LavaReadS16(evt9, 12), PAL_LavaReadU16(evt9, 16), PAL_LavaReadU16(evt9, 20), PAL_LavaReadU16(evt9, 22),
         PAL_LavaReadU16(evt11, 2), PAL_LavaReadU16(evt11, 4),
         PAL_LavaReadS16(evt11, 12), PAL_LavaReadU16(evt11, 16), PAL_LavaReadU16(evt11, 20), PAL_LavaReadU16(evt11, 22),
         PAL_LavaReadU16(evt11, 8), PAL_LavaReadU16(evt11, 10), g_lava_autoscript_pc[10],
         PAL_LavaReadU16(evt12, 2), PAL_LavaReadU16(evt12, 4),
         PAL_LavaReadS16(evt12, 12), PAL_LavaReadU16(evt12, 16), PAL_LavaReadU16(evt12, 20), PAL_LavaReadU16(evt12, 22),
         PAL_LavaReadU16(evt12, 8), PAL_LavaReadU16(evt12, 10), g_lava_autoscript_pc[11]);
   }
   PAL_ClearKeyState();
}

static void PAL_LavaAutotestDumpXianlingObject(int object_id, char *label)
{
   addr evt;

   evt = PAL_LavaSceneEventData(object_id);
   if (evt == 0)
   {
      printf("[LAVA][XIANLING] %s object=%d missing\n", label, object_id);
      return;
   }

   printf("[LAVA][XIANLING] %s object=%d pos=(%d,%d) state=%d mode=%d trigger=%d auto=%d sprite=%d dir=%d frame=%d\n",
      label,
      object_id,
      PAL_LavaReadU16(evt, 2), PAL_LavaReadU16(evt, 4),
      PAL_LavaReadS16(evt, 12),
      PAL_LavaReadU16(evt, 14),
      PAL_LavaReadU16(evt, 8),
      PAL_LavaReadU16(evt, 10),
      PAL_LavaReadU16(evt, 16),
      PAL_LavaReadU16(evt, 20),
      PAL_LavaReadU16(evt, 22));
}

static void PAL_LavaAutotestProbeXianlingMoves(char *label)
{
   int i;
   int saved_x;
   int saved_y;
   int saved_dir;
   int saved_frame;

   saved_x = g_lava_party_x;
   saved_y = g_lava_party_y;
   saved_dir = g_lava_party_direction;
   saved_frame = g_lava_party_frame;

   for (i = 0; g_lava_xianling_probe_dirs[i] >= 0; i++)
   {
      int result;

      g_lava_party_x = saved_x;
      g_lava_party_y = saved_y;
      g_lava_party_direction = saved_dir;
      g_lava_party_frame = saved_frame;
      result = PAL_LavaMovePartyStep(g_lava_xianling_probe_dirs[i]);
      printf("[LAVA][XIANLING] %s probe dir=%d result=%d pos=(%d,%d)\n",
         label, g_lava_xianling_probe_dirs[i], result, g_lava_party_x, g_lava_party_y);
   }

   g_lava_party_x = saved_x;
   g_lava_party_y = saved_y;
   g_lava_party_direction = saved_dir;
   g_lava_party_frame = saved_frame;
   PAL_LavaUpdateViewport();
}

static void PAL_LavaAutotestRunXianlingSequence(void)
{
   int object_id;

   object_id = 343;
   PAL_LavaAutotestSetupScene(20, 1184, 1136, kDirSouth, 1024, 1024, 0);
   printf("[LAVA][XIANLING] setup scene=%d events=%d..%d party=(%d,%d) view=(%d,%d)\n",
      g_lava_scene_num,
      g_lava_scene_event_first,
      g_lava_scene_event_last,
      g_lava_party_x,
      g_lava_party_y,
      g_lava_view_x,
      g_lava_view_y);
   PAL_LavaAutotestDumpXianlingObject(object_id, "before");
   PAL_LavaAutotestDumpXianlingObject(344, "before");
   PAL_LavaAutotestDumpXianlingObject(345, "before");
   PAL_LavaAutotestDumpXianlingObject(348, "before");
   PAL_LavaAutotestRunTriggerChain(object_id, 8741, 260);
   g_lava_party_x = 1184;
   g_lava_party_y = 1136;
   g_lava_party_direction = kDirSouth;
   PAL_LavaUpdateViewport();
   printf("[LAVA][XIANLING] run late-dialog trigger=8947 party=(%d,%d) view=(%d,%d)\n",
      g_lava_party_x,
      g_lava_party_y,
      g_lava_view_x,
      g_lava_view_y);
   PAL_LavaAutotestRunTrigger(8947, object_id);
   PAL_LavaAutotestDumpXianlingObject(object_id, "after");
   PAL_LavaAutotestDumpXianlingObject(344, "after");
   PAL_LavaAutotestDumpXianlingObject(345, "after");
   PAL_LavaAutotestDumpXianlingObject(348, "after");
   printf("[LAVA][XIANLING] after script scene=%d party=(%d,%d) view=(%d,%d) dir=%d frame=%d\n",
      g_lava_scene_num,
      g_lava_party_x,
      g_lava_party_y,
      g_lava_view_x,
      g_lava_view_y,
      g_lava_party_direction,
      g_lava_party_frame);
   PAL_LavaAutotestProbeXianlingMoves("after");
   g_lava_shutdown_requested = 1;
}

static void PAL_LavaAutotestRunObj204Sequence(void)
{
   int saved_search_log;

   PAL_LavaAutotestSetupScene(14, 1104, 1432, kDirWest, 944, 1320, 0);
   printf("[LAVA][OBJ204] setup scene=%d events=%d..%d party=(%d,%d) view=(%d,%d)\n",
      g_lava_scene_num,
      g_lava_scene_event_first,
      g_lava_scene_event_last,
      g_lava_party_x,
      g_lava_party_y,
      g_lava_view_x,
      g_lava_view_y);
   saved_search_log = PAL_LavaAutotestEnableSearchLog();
   PAL_LavaAutotestRunTrigger(9642, 204);
   PAL_LavaAutotestRestoreSearchLog(saved_search_log);
   g_lava_shutdown_requested = 1;
}

static void PAL_LavaAutotestRunLoadSlot(int slot)
{
    int saved_search_log;
    int object_id;

   if (PAL_LavaLoadSavedGame(slot) == 0)
   {
      printf("[LAVA][LOADTEST] slot=%d scene=%d view=(%d,%d) party=(%d,%d) dir=%d count=%d\n",
         slot,
         g_lava_scene_num,
         g_lava_view_x, g_lava_view_y,
         g_lava_party_x, g_lava_party_y,
         g_lava_party_direction, g_lava_party_count);
      saved_search_log = PAL_LavaAutotestEnableSearchLog();
      printf("[LAVA][AUTOTOUCH] scan scene=%d events=%d..%d\n",
         g_lava_scene_num, g_lava_scene_event_first, g_lava_scene_event_last);
      for (object_id = g_lava_scene_event_first; object_id <= g_lava_scene_event_last; object_id++)
      {
         addr evt;
         int state;
         int trigger_mode;
         int trigger_script;

         evt = PAL_LavaSceneEventData(object_id);
         if (evt == 0)
         {
            continue;
         }
         state = PAL_LavaReadS16(evt, 12);
         trigger_mode = PAL_LavaReadU16(evt, 14);
         trigger_script = PAL_LavaReadU16(evt, 8);
         if (state > 0 && trigger_mode >= 4 && trigger_script != 0)
         {
            printf("[LAVA][AUTOTOUCH] candidate object=%d trigger=%d mode=%d state=%d pos=(%d,%d)\n",
               object_id, trigger_script, trigger_mode, state,
               PAL_LavaReadU16(evt, 2), PAL_LavaReadU16(evt, 4));
         }
      }
      if (slot == 2)
      {
         int slot2_touched_any;

         slot2_touched_any = 0;
         printf("[LAVA][LOADTEST] autotouch slot2 encounter object=654\n");
         if (PAL_LavaAutotestTouchObject(654))
         {
            slot2_touched_any = 1;
         }
         else
         {
            printf("[LAVA][LOADTEST] autotouch slot2 encounter object=653\n");
            if (PAL_LavaAutotestTouchObject(653))
            {
               slot2_touched_any = 1;
            }
         }

         if (!slot2_touched_any)
         {
            printf("[LAVA][LOADTEST] slot2 encounter touch failed\n");
         }
      }
      else if (slot == 5)
      {
         int touch_object_id;
         int touched_any;

         touched_any = 0;
         for (touch_object_id = g_lava_scene_event_first; touch_object_id <= g_lava_scene_event_last; touch_object_id++)
         {
            addr touch_evt;
            int touch_state;
            int touch_trigger_mode;
            int touch_trigger_script;

            touch_evt = PAL_LavaSceneEventData(touch_object_id);
            if (touch_evt == 0)
            {
               continue;
            }

            touch_state = PAL_LavaReadS16(touch_evt, 12);
            touch_trigger_mode = PAL_LavaReadU16(touch_evt, 14);
            touch_trigger_script = PAL_LavaReadU16(touch_evt, 8);
            if (touch_state <= 0 || touch_trigger_mode < 4 || touch_trigger_script == 0)
            {
               continue;
            }

            printf("[LAVA][LOADTEST] autotouch object=%d trigger=%d mode=%d\n",
               touch_object_id, touch_trigger_script, touch_trigger_mode);
            if (PAL_LavaAutotestTouchObject(touch_object_id))
            {
               touched_any = 1;
               break;
            }
         }

         if (!touched_any)
         {
            printf("[LAVA][LOADTEST] no touchable object succeeded in scene=%d\n",
               g_lava_scene_num);
         }
      }
      PAL_LavaAutotestRestoreSearchLog(saved_search_log);
   }
   else
   {
      printf("[LAVA][LOADTEST] slot=%d load failed\n", slot);
   }
}

static void PAL_LavaAutotestRunBattleSmoke(void)
{
   int battle_result;
   int i;
   int role;

   g_lava_autotest_battle_magic_seen = 0;
   g_lava_autotest_battle_magic_object_id = 0;
   printf("[LAVA][BATTLESMOKE] start slot=2 team=1 fengshen=%d\n",
       g_lava_autotest_fengshen);
   if (PAL_LavaLoadSavedGame(2) != 0)
   {
      printf("[LAVA][BATTLESMOKE] load failed slot=2\n");
      PAL_Shutdown(1);
      return;
   }

   printf("[LAVA][BATTLESMOKE] loaded scene=%d party=%d view=(%d,%d)\n",
      g_lava_scene_num, g_lava_party_count, g_lava_view_x, g_lava_view_y);
   if (g_lava_autotest_fengshen)
   {
      for (i = 0; i < g_lava_party_count && i < 3; i++)
      {
         role = g_lava_party_role[i];
         PAL_LavaWriteU16((addr)g_lava_data_buf, 8 * 6 * 2 + role * 2, 999);
         PAL_LavaWriteU16((addr)g_lava_data_buf, 10 * 6 * 2 + role * 2, 999);
      }
   }
   battle_result = PAL_StartBattle(1, FALSE);
   printf("[LAVA][BATTLESMOKE] end result=%d magic_seen=%d\n",
      battle_result, g_lava_autotest_battle_magic_seen);
   PAL_Shutdown(g_lava_autotest_battle_magic_seen >= 3 ? 0 : 1);
}

static void PAL_LavaAutotestRunOp48(void)
{
   int attack_percent;
   int dexterity_percent;
   long next_script;

   PAL_LavaClearRoleTempStats();
   next_script = PAL_LavaRunRoleTriggerScript(42044, 2);
   attack_percent = PAL_LavaRoleTempStatPercent(17, 2);
   dexterity_percent = PAL_LavaRoleTempStatPercent(20, 2);
   printf("[LAVA][AUTOTEST_OP48] next=%ld attack=%d dexterity=%d\n",
      next_script, attack_percent, dexterity_percent);
   PAL_Shutdown(attack_percent == 100 && dexterity_percent == 100 ? 0 : 1);
}

static void PAL_LavaAutotestRunHooksSequence(void)
{
   PAL_LavaAutotestRunSceneScriptSequence(g_lava_autotest_hooks_plan);
}

static void PAL_LavaAutotestRunExitsSequence(void)
{
   int saved_search_log;

   saved_search_log = PAL_LavaAutotestEnableSearchLog();
   PAL_LavaAutotestRunSequencePlan(g_lava_autotest_exits_sequence_plan);
   PAL_LavaAutotestRestoreSearchLog(saved_search_log);
}

static void PAL_LavaAutotestRunScene6Sequence(void)
{
   int saved_search_log;

   saved_search_log = PAL_LavaAutotestEnableSearchLog();
   PAL_LavaAutotestRunSequencePlan(g_lava_autotest_scene6_sequence_plan);
   PAL_LavaAutotestRestoreSearchLog(saved_search_log);
}

static void PAL_LavaAutotestRunScene5Sequence(void)
{
   int saved_search_log;

   saved_search_log = PAL_LavaAutotestEnableSearchLog();
   PAL_LavaAutotestRunSequencePlan(g_lava_autotest_scene5_sequence_plan);
   PAL_LavaAutotestRestoreSearchLog(saved_search_log);
}

static void PAL_LavaAutotestRunScene13Sequence(void)
{
   int saved_search_log;

   saved_search_log = PAL_LavaAutotestEnableSearchLog();
   PAL_LavaAutotestRunSequencePlan(g_lava_autotest_scene13_sequence_plan);
   PAL_LavaAutotestRestoreSearchLog(saved_search_log);
}

static void PAL_LavaAutotestRunScene9Sequence(void)
{
   int saved_search_log;

   saved_search_log = PAL_LavaAutotestEnableSearchLog();
   PAL_LavaAutotestRunSequencePlan(g_lava_autotest_scene9_sequence_plan);
   PAL_LavaAutotestRestoreSearchLog(saved_search_log);
}

static void PAL_LavaAutotestRunDoorSequence(void)
{
   int step;
   int touched_door;
   int door_object_id;
   int door_touch_x;
   int door_touch_y;
   int door_max_steps;
   int door_delay_ms;

   door_object_id = g_lava_autotest_door_params[0];
   door_touch_x = g_lava_autotest_door_params[1];
   door_touch_y = g_lava_autotest_door_params[2];
   door_max_steps = g_lava_autotest_door_params[3];
   door_delay_ms = g_lava_autotest_door_params[4];
   touched_door = 0;

   for (step = 0; step < door_max_steps; step++)
   {
      if (PAL_LavaAutotestDoorStep(step, door_object_id, &touched_door, door_touch_x, door_touch_y, door_delay_ms))
      {
         break;
      }
   }
}

static void PAL_LavaAutotestRunKitchenSequence(void)
{
   int saved_search_log;
   int kitchen_object_id;
   int kitchen_trigger_script;
   int kitchen_follow_steps;
   addr kitchen_evt;

   saved_search_log = PAL_LavaAutotestEnableSearchLog();
   kitchen_object_id = g_lava_autotest_kitchen_params[0];
   kitchen_trigger_script = g_lava_autotest_kitchen_params[1];
   kitchen_follow_steps = g_lava_autotest_kitchen_params[2];

   PAL_LavaKitchenBeginRun(g_lava_autotest_kitchen_params);
   PAL_LavaAutotestRunTrigger((long)kitchen_trigger_script, kitchen_object_id);
   PAL_LavaKitchenDumpPlan(g_lava_autotest_kitchen_dump_ids);

   kitchen_evt = PAL_LavaSceneEventData(kitchen_object_id);
   if (g_lava_scene_num == 3 && kitchen_evt != 0 && PAL_LavaReadU16(kitchen_evt, 8) != kitchen_trigger_script)
   {
      PAL_LavaAutotestRunTriggerChain(kitchen_object_id, PAL_LavaReadU16(kitchen_evt, 8), kitchen_follow_steps);
      PAL_LavaKitchenDumpPlan(g_lava_autotest_kitchen_dump_ids);
   }

   PAL_LavaKitchenEndRun(saved_search_log);
}

static void PAL_LavaAutotestRunWalkSequence(void)
{
   int saved_search_log;

   saved_search_log = PAL_LavaAutotestEnableSearchLog();
   PAL_LavaAutotestRunSequencePlan(g_lava_autotest_walk_sequence_plan);
   PAL_LavaAutotestRestoreSearchLog(saved_search_log);
}

static void PAL_LavaAutotestRunHallSequence(void)
{
   int saved_hall_search_log;
   int hall_step;
   int hall_done[4];
   int hall_last63_trigger;
   int hall_object57;
   int hall_start_trigger;
   int hall_max_steps;
   int hall_log_interval;

   saved_hall_search_log = g_lava_autotest_search;
   hall_object57 = g_lava_autotest_hall_params[0];
   hall_start_trigger = g_lava_autotest_hall_params[1];
   hall_max_steps = g_lava_autotest_hall_params[2];
   hall_log_interval = g_lava_autotest_hall_params[3];

   PAL_LavaHallResetState(hall_done, &hall_last63_trigger);
   PAL_LavaHallBeginRun(hall_object57, hall_start_trigger);

   for (hall_step = 0; hall_step < hall_max_steps; hall_step++)
   {
      PAL_LavaAutotestAdvanceLogicFrame(-1, 0);
      PAL_LavaHallRunLoopPlan(g_lava_autotest_hall_loop_plan, hall_done, &hall_last63_trigger);
      PAL_LavaHallDumpPeriodic(hall_step, hall_log_interval);
   }

   PAL_LavaHallEndRun(saved_hall_search_log);
}

static void PAL_LavaAutotestRunSceneScriptBatch(int scene_num, int party_x, int party_y, int party_direction,
   int view_x, int view_y, int run_enter, int *pairs)
{
   PAL_LavaAutotestSetupScene(scene_num, party_x, party_y, party_direction, view_x, view_y, run_enter);
   PAL_LavaAutotestRunBatchByScript(pairs);
}

static void PAL_LavaAutotestRunSceneObjectBatch(int scene_num, int party_x, int party_y, int party_direction,
   int view_x, int view_y, int run_enter, int *pairs)
{
   PAL_LavaAutotestSetupScene(scene_num, party_x, party_y, party_direction, view_x, view_y, run_enter);
   PAL_LavaAutotestRunBatchByObject(pairs);
}

static int *PAL_LavaAutotestIntListById(int list_id)
{
   if (list_id == 1)
   {
      return g_lava_autotest_search_objects;
   }
   if (list_id == 2)
   {
      return g_lava_autotest_search_prime_params;
   }
   if (list_id == 3)
   {
      return g_lava_autotest_walk_touch_params;
   }
   if (list_id == 4)
   {
      return g_lava_autotest_scene_touch_params;
   }
   if (list_id == 10)
   {
      return g_lava_autotest_search_object_params;
   }
   return 0;
}

static int *PAL_LavaAutotestScriptPairsById(int plan_id)
{
   int i;

   i = 0;
   while (g_lava_autotest_script_pair_set_ids[i] != 0)
   {
      if (g_lava_autotest_script_pair_set_ids[i] == plan_id)
      {
         if (plan_id == 1)
         {
            return g_lava_autotest_hooks_scene4_pairs;
         }
         if (plan_id == 2)
         {
            return g_lava_autotest_hooks_scene1_pairs;
         }
         if (plan_id == 3)
         {
            return g_lava_autotest_hooks_scene7_pairs;
         }
         if (plan_id == 4)
         {
            return g_lava_autotest_hooks_scene7_reenter_pairs;
         }
         if (plan_id == 5)
         {
            return g_lava_autotest_exit_pairs;
         }
         if (plan_id == 6)
         {
            return g_lava_autotest_walk_scene3_pairs;
         }
         if (plan_id == 7)
         {
            return g_lava_autotest_walk_scene1_pairs;
         }
         if (plan_id == 8)
         {
            return g_lava_autotest_walk_scene2_pairs;
         }
         if (plan_id == 9)
         {
            return g_lava_autotest_walk_scene6_pairs;
         }
         if (plan_id == 10)
         {
            return g_lava_autotest_search_pairs;
         }
         if (plan_id == 11)
         {
            return g_lava_autotest_walk_final_pairs;
         }
         return 0;
      }
      i++;
   }
   return 0;
}

static int *PAL_LavaAutotestObjPairsById(int plan_id)
{
   int i;

   i = 0;
   while (g_lava_autotest_object_pair_set_ids[i] != 0)
   {
      if (g_lava_autotest_object_pair_set_ids[i] == plan_id)
      {
         if (plan_id == 1)
         {
            return g_lava_autotest_scene6_object_pairs;
         }
         if (plan_id == 2)
         {
            return g_lava_autotest_scene5_object_pairs;
         }
         if (plan_id == 3)
         {
            return g_lava_autotest_scene9_object_pairs;
         }
         return 0;
      }
      i++;
   }
   return 0;
}

static void PAL_LavaAutotestRunSceneScriptPlan(int *plan)
{
   int i;

   i = 0;
   while (!(plan[i] == 0 && plan[i + 1] == 0 && plan[i + 2] == 0 && plan[i + 3] == 0 &&
            plan[i + 4] == 0 && plan[i + 5] == 0 && plan[i + 6] == 0 && plan[i + 7] == 0))
   {
      int *pairs;

      pairs = PAL_LavaAutotestScriptPairsById(plan[i + 7]);
      if (pairs != 0)
      {
         PAL_LavaAutotestRunSceneScriptBatch(plan[i], plan[i + 1], plan[i + 2], plan[i + 3],
            plan[i + 4], plan[i + 5], plan[i + 6], pairs);
      }
      i += 8;
   }
}

static void PAL_LavaAutotestRunSceneScriptSequence(int *plan)
{
   int saved_search_log;

   saved_search_log = PAL_LavaAutotestEnableSearchLog();
   PAL_LavaAutotestRunSceneScriptPlan(plan);
   PAL_LavaAutotestRestoreSearchLog(saved_search_log);
}

static int PAL_LavaAutotestRunCurrentSceneScriptPlan(int *plan)
{
   int i;

   i = 0;
   while (!(plan[i] == 0 && plan[i + 1] == 0))
   {
      int *pairs;

      if (g_lava_scene_num == plan[i])
      {
         pairs = PAL_LavaAutotestScriptPairsById(plan[i + 1]);
         if (pairs != 0)
         {
            PAL_LavaAutotestRunBatchByScript(pairs);
            return 1;
         }
         return 0;
      }
      i += 2;
   }
   return 0;
}

static int PAL_LavaAutotestRunObjPlanNow(int *plan)
{
   int i;

   i = 0;
   while (!(plan[i] == 0 && plan[i + 1] == 0))
   {
      int *pairs;

      if (g_lava_scene_num == plan[i])
      {
         pairs = PAL_LavaAutotestObjPairsById(plan[i + 1]);
         if (pairs != 0)
         {
            PAL_LavaAutotestRunBatchByObject(pairs);
            return 1;
         }
         return 0;
      }
      i += 2;
   }
   return 0;
}

static void PAL_LavaAutotestPrimeSearch(int *params)
{
   g_lava_party_direction = params[0];
   PAL_LavaUpdateViewport();
   PAL_LavaDrawSceneFrame();
   if (params[1])
   {
      PAL_LavaSearchScene();
   }
}

static void PAL_LavaAutotestRunSequencePlan(int *plan)
{
   int i;
   int *params;
   int *objects;
   int *pairs;

   i = 0;
   while (!(plan[i] == 0 && plan[i + 1] == 0))
   {
      params = 0;
      objects = 0;
      pairs = 0;
      if (plan[i] == 1)
      {
         params = PAL_LavaAutotestIntListById(plan[i + 1]);
         if (params != 0)
         {
            PAL_LavaAutotestPrimeSearch(params);
         }
      }
      else if (plan[i] == 2)
      {
         objects = PAL_LavaAutotestIntListById(1);
         params = PAL_LavaAutotestIntListById(plan[i + 1]);
         if (objects != 0 && params != 0)
         {
            PAL_LavaAutotestRunSearchObjects(objects, params[0]);
         }
      }
      else if (plan[i] == 3)
      {
         params = PAL_LavaAutotestIntListById(plan[i + 1]);
         if (params != 0)
         {
            PAL_LavaAutotestFollowTouchChain(params[0], params[1], params[2]);
         }
      }
      else if (plan[i] == 4)
      {
         PAL_LavaDumpAutotouchVisited();
      }
      else if (plan[i] == 5)
      {
         pairs = PAL_LavaAutotestScriptPairsById(plan[i + 1]);
         if (pairs != 0)
         {
            PAL_LavaAutotestRunBatchByScript(pairs);
         }
      }
      else if (plan[i] == 6)
      {
         PAL_LavaDumpInventory();
      }
      else if (plan[i] == 7)
      {
         PAL_LavaAutotestRunCurrentSceneScriptPlan(g_lava_autotest_walk_scene_plan);
      }
      else if (plan[i] == 8)
      {
         PAL_LavaAutotestSetupSceneByIndex(plan[i + 1]);
      }
      else if (plan[i] == 9)
      {
         PAL_LavaAutotestRunObjPlanNow(g_lava_autotest_object_scene_plan);
      }
      i += 2;
   }
}

static int PAL_LavaAutotestRunWhenTriggerEquals(int object_id, int expected_trigger, int *done_flag, int max_steps)
{
   addr evt;

   if (*done_flag)
   {
      return 0;
   }

   evt = PAL_LavaSceneEventData(object_id);
   if (evt == 0 || PAL_LavaReadU16(evt, 8) != expected_trigger)
   {
      return 0;
   }

   *done_flag = 1;
   PAL_LavaAutotestRunTriggerChain(object_id, expected_trigger, max_steps);
   return 1;
}

static int PAL_LavaAutotestRunWhenStatePositive(int object_id, int *done_flag)
{
   addr evt;
   long trigger_script;

   if (*done_flag)
   {
      return 0;
   }

   evt = PAL_LavaSceneEventData(object_id);
   if (evt == 0 || PAL_LavaReadS16(evt, 12) <= 0)
   {
      return 0;
   }

   trigger_script = PAL_LavaReadU16(evt, 8);
   if (trigger_script == 0)
   {
      return 0;
   }

   *done_flag = 1;
   PAL_LavaRunTriggerScript(trigger_script, object_id);
   g_lava_dialog_event_count = 0;
   return 1;
}

static int PAL_LavaAutotestTouchWhenStatePositive(int object_id, int *done_flag, int touch_x, int touch_y)
{
   addr evt;

   if (*done_flag)
   {
      return 0;
   }

   evt = PAL_LavaSceneEventData(object_id);
   if (evt == 0 || PAL_LavaReadS16(evt, 12) <= 0)
   {
      return 0;
   }

   *done_flag = 1;
   printf("[LAVA][DOORTEST] try touch after open\n");
   if (PAL_LavaTouchSceneAt(touch_x, touch_y))
   {
      printf("[LAVA][DOORTEST] touch switched scene=%d\n", g_lava_scene_num);
      return 1;
   }

   printf("[LAVA][DOORTEST] touch did not trigger\n");
   return 0;
}

static int PAL_LavaAutotestTriggerInRange(addr evt, int min_trigger, int max_trigger)
{
   int trigger_value;

   if (evt == 0)
   {
      return 0;
   }

   trigger_value = PAL_LavaReadU16(evt, 8);
   return trigger_value >= min_trigger && trigger_value <= max_trigger;
}

static int PAL_LavaAutotestRunRangedChain(int object_id, int min_trigger, int max_trigger, int max_steps, int *last_trigger)
{
   addr evt;
   int trigger_value;

   evt = PAL_LavaSceneEventData(object_id);
   if (evt == 0 || !PAL_LavaAutotestTriggerInRange(evt, min_trigger, max_trigger))
   {
      return 0;
   }

   trigger_value = PAL_LavaReadU16(evt, 8);
   if (trigger_value == *last_trigger)
   {
      return 0;
   }

   PAL_LavaAutotestRunTriggerChain(object_id, trigger_value, max_steps);
   *last_trigger = PAL_LavaReadU16(evt, 8);
   return 1;
}

static int PAL_LavaAutotestRunRangedChainWhenStatePositive(int object_id, int min_trigger, int max_trigger, int max_steps, int *last_trigger)
{
   addr evt;

   evt = PAL_LavaSceneEventData(object_id);
   if (evt == 0 || PAL_LavaReadS16(evt, 12) <= 0)
   {
      return 0;
   }

   return PAL_LavaAutotestRunRangedChain(object_id, min_trigger, max_trigger, max_steps, last_trigger);
}

static int PAL_LavaAutotestRunFollowStep(int *params, int *done_flag)
{
   return PAL_LavaAutotestRunWhenTriggerEquals(params[0], params[1], done_flag, params[2]);
}

static int PAL_LavaAutotestRunRangedStep(int *params, int *last_trigger)
{
   return PAL_LavaAutotestRunRangedChainWhenStatePositive(params[0], params[1], params[2], params[3], last_trigger);
}

static int PAL_LavaAutotestRunRangedStepWhenEnabled(int *params, int *enabled_flag, int *last_trigger)
{
   if (!(*enabled_flag))
   {
      return 0;
   }

   return PAL_LavaAutotestRunRangedStep(params, last_trigger);
}

static int *PAL_LavaHallPlanById(int plan_id)
{
   if (plan_id == 1)
   {
      return g_lava_autotest_hall_follow_params;
   }
   if (plan_id == 2)
   {
      return g_lava_autotest_hall_state_watch_plan;
   }
   if (plan_id == 3)
   {
      return g_lava_autotest_hall_range_params;
   }
   return 0;
}

static void PAL_LavaHallRunLoopPlan(int *plan, int *done_flags, int *last_trigger)
{
   int i;

   i = 0;
   while (!(plan[i] == 0 && plan[i + 1] == 0 && plan[i + 2] == 0))
   {
      int *params;

      params = PAL_LavaHallPlanById(plan[i + 1]);
      if (params != 0)
      {
         if (plan[i] == 1)
         {
            PAL_LavaAutotestRunFollowStep(params, &done_flags[plan[i + 2]]);
         }
         else if (plan[i] == 2)
         {
            PAL_LavaAutotestRunActionPlan(params, &done_flags[plan[i + 2]]);
         }
         else if (plan[i] == 3)
         {
            PAL_LavaAutotestRunRangedStepWhenEnabled(params, &done_flags[plan[i + 2]], last_trigger);
         }
      }
      i += 3;
   }
}

static void PAL_LavaHallDumpById(int dump_id)
{
   if (dump_id == 1)
   {
      PAL_LavaDumpObjList("HALLSTATE", g_lava_hall_runtime_watch_pairs, 1);
      return;
   }
   if (dump_id == 2)
   {
      PAL_LavaDumpObjList("HALLPLAIN", g_lava_hall_plain_watch_pairs, 0);
      return;
   }
   if (dump_id == 3)
   {
      PAL_LavaDumpObjList("HALLFINAL", g_lava_hall_final_watch_pairs, 0);
   }
}

static void PAL_LavaHallDumpPlan(int *plan)
{
   int i;

   i = 0;
   while (plan[i] != 0)
   {
      PAL_LavaHallDumpById(plan[i]);
      i++;
   }
}

static void PAL_LavaHallDumpPeriodic(int step, int interval)
{
   if ((step % interval) != 0)
   {
      return;
   }

   printf("[LAVA][HALLTEST] frame=%d\n", step);
   PAL_LavaHallDumpPlan(g_lava_autotest_hall_periodic_dump_ids);
}

static void PAL_LavaHallDumpFinal(void)
{
   PAL_LavaHallDumpPlan(g_lava_autotest_hall_final_dump_ids);
}

static void PAL_LavaHallResetState(int *done_flags, int *last_trigger)
{
   done_flags[0] = 0;
   PAL_LavaAutotestClearPlanDoneFlags(g_lava_autotest_hall_state_watch_plan, &done_flags[1]);
   *last_trigger = 0;
}

static void PAL_LavaHallBeginRun(int object_id, int trigger_script)
{
   PAL_LavaAutotestSetupSceneByIndex(5);
   g_lava_autotest_search = 0;
   g_lava_fast_script_probe = 1;
   g_lava_logic_frame_num = 0;
   PAL_LavaRunTriggerScript(trigger_script, object_id);
   printf("[LAVA][HALLTEST] trigger4701 next=%d dialog=%d\n",
      PAL_LavaReadU16(PAL_LavaSceneEventData(object_id), 8), g_lava_dialog_event_count);
   g_lava_dialog_event_count = 0;
}

static void PAL_LavaHallEndRun(int saved_search_log)
{
   printf("[LAVA][HALLFINAL] scene=%d scene1_enter=%d scene1_teleport=%d\n",
      g_lava_scene_num,
      g_lava_scene_hook_enter[0],
      g_lava_scene_hook_teleport[0]);
   PAL_LavaHallDumpFinal();
   g_lava_fast_script_probe = 0;
   g_lava_autotest_search = saved_search_log;
}

static void PAL_LavaKitchenDumpById(int dump_id)
{
   if (dump_id == 1)
   {
      PAL_LavaDumpObjList("KITCHEN", g_lava_kitchen_watch_pairs, 0);
   }
}

static void PAL_LavaKitchenDumpPlan(int *plan)
{
   int i;

   i = 0;
   while (plan[i] != 0)
   {
      PAL_LavaKitchenDumpById(plan[i]);
      i++;
   }
}

static void PAL_LavaKitchenBeginRun(int *params)
{
   PAL_LavaAutotestSetupSceneByIndex(6);
   printf("[LAVA][KITCHENTEST] begin scene=%d object=%d trigger=%d follow=%d\n",
      g_lava_scene_num,
      params[0],
      params[1],
      params[2]);
   PAL_LavaKitchenDumpPlan(g_lava_autotest_kitchen_dump_ids);
}

static void PAL_LavaKitchenEndRun(int saved_search_log)
{
   printf("[LAVA][KITCHENFINAL] scene=%d\n", g_lava_scene_num);
   PAL_LavaKitchenDumpPlan(g_lava_autotest_kitchen_dump_ids);
   g_lava_autotest_search = saved_search_log;
}

static void PAL_LavaAutotestAdvanceLogicFrame(int step, char *tag)
{
   g_lava_logic_frame_num++;
   if (tag != 0 && step >= 0)
   {
      printf("[LAVA][%s] frame=%d\n", tag, step);
   }
   PAL_LavaRunSceneAutoScripts();
}

static int PAL_LavaAutotestDoorStep(int step, int object_id, int *touched_door, int touch_x, int touch_y, int delay_ms)
{
   addr door_evt;

   PAL_LavaAutotestAdvanceLogicFrame(step, "DOORTEST");
   door_evt = PAL_LavaSceneEventData(object_id);
   if (door_evt != 0)
   {
      printf("[LAVA][DOORSTATE] obj%d state=%d trigger=%d mode=%d pos=(%d,%d)\n",
         object_id,
         PAL_LavaReadS16(door_evt, 12),
         PAL_LavaReadU16(door_evt, 8),
         PAL_LavaReadU16(door_evt, 14),
         PAL_LavaReadU16(door_evt, 2),
         PAL_LavaReadU16(door_evt, 4));
      if (PAL_LavaAutotestTouchWhenStatePositive(object_id, touched_door, touch_x, touch_y))
      {
         return 1;
      }
   }
   PAL_LavaUpdateViewport();
   PAL_LavaDrawSceneFrame();
   UTIL_Delay(delay_ms);
   return 0;
}

static void PAL_LavaAutotestClearPlanDoneFlags(int *plan, int *done_flags)
{
   int i;

   i = 0;
   while (!(plan[i] == 0 && plan[i + 1] == 0))
   {
      done_flags[i / 2] = 0;
      i += 2;
   }
}

static void PAL_LavaAutotestRunActionPlan(int *plan, int *done_flags)
{
   int i;

   i = 0;
   while (!(plan[i] == 0 && plan[i + 1] == 0))
   {
      if (plan[i + 1] == 1)
      {
         PAL_LavaAutotestRunWhenStatePositive(plan[i], &done_flags[i / 2]);
      }
      i += 2;
   }
}

static void PAL_LavaDumpObjStat(char *tag, int object_id, int autoscript_index, int include_runtime)
{
   addr evt;

   evt = PAL_LavaSceneEventData(object_id);
   if (evt == 0)
   {
      return;
   }

   if (include_runtime)
   {
      printf("[LAVA][%s] obj=%d state=%d trigger=%d auto=%d pc=%d idle=%d pos=(%d,%d)\n",
         tag,
         object_id,
         PAL_LavaReadS16(evt, 12),
         PAL_LavaReadU16(evt, 8),
         PAL_LavaReadU16(evt, 10),
         g_lava_autoscript_pc[autoscript_index],
         g_lava_autoscript_idle[autoscript_index],
         PAL_LavaReadU16(evt, 2),
         PAL_LavaReadU16(evt, 4));
   }
   else
   {
      printf("[LAVA][%s] obj=%d state=%d trigger=%d auto=%d pos=(%d,%d)\n",
         tag,
         object_id,
         PAL_LavaReadS16(evt, 12),
         PAL_LavaReadU16(evt, 8),
         PAL_LavaReadU16(evt, 10),
         PAL_LavaReadU16(evt, 2),
         PAL_LavaReadU16(evt, 4));
   }
}

static void PAL_LavaDumpObjList(char *tag, int *pairs, int include_runtime)
{
   int i;

   i = 0;
   while (!(pairs[i] == 0 && pairs[i + 1] == 0))
   {
      PAL_LavaDumpObjStat(tag, pairs[i], pairs[i + 1], include_runtime);
      i += 2;
   }
}

static int PAL_LavaAutotestFollowTouchChain(int max_passes, int delay_ms, int stop_after_first)
{
   int pass;
   int touched_any;

   touched_any = 0;
   for (pass = 0; pass < max_passes; pass++)
   {
      int object_id;
      int touched;

      touched = 0;
      PAL_LavaDumpTouchCandidates();
      for (object_id = g_lava_scene_event_first;
           object_id <= g_lava_scene_event_last;
           object_id++)
      {
         if (PAL_LavaAutotestTouchObject(object_id))
         {
            touched = 1;
            touched_any = 1;
            UTIL_Delay(delay_ms);
            if (stop_after_first)
            {
               break;
            }
         }
      }

      if (!touched || stop_after_first)
      {
         if (!touched)
         {
            break;
         }
         if (stop_after_first)
         {
            continue;
         }
      }
   }

   return touched_any;
}

static void PAL_LavaAutotestWalkProbe(void)
{
   int i;
   int next_x;
   int next_y;

   for (i = 0; i < 8; i++)
   {
      g_lava_party_direction = 3;
      next_x = g_lava_party_x + 4;
      next_y = g_lava_party_y + 2;
      if (PAL_LavaTileBlocked(next_x, next_y) || PAL_LavaEventBlocked(next_x, next_y, 0))
      {
         printf("[LAVA][AUTOWALK] blocked step=%d next=(%d,%d)\n", i, next_x, next_y);
         break;
      }
      g_lava_party_x = next_x;
      g_lava_party_y = next_y;
      g_lava_party_step_frame++;
      g_lava_party_frame = (g_lava_player_walk_frames[0] == 4) ? (g_lava_party_step_frame % 4) : (g_lava_party_step_frame % 3);
      PAL_LavaUpdateViewport();
      PAL_LavaDrawSceneFrame();
      printf("[LAVA][AUTOWALK] moved step=%d pos=(%d,%d) view=(%d,%d)\n",
         i, g_lava_party_x, g_lava_party_y, g_lava_view_x, g_lava_view_y);
      UTIL_Delay(80);
   }
}

static int PAL_LavaTouchScene(void)
{
   return PAL_LavaTouchSceneAt(g_lava_party_x, g_lava_party_y);
}

static int PAL_LavaTouchSceneAt(int party_x, int party_y)
{
   static int s_last_touch_log_object = -1;
   static int s_last_touch_log_trigger = -1;
   static int s_last_touch_log_party_x = -1;
   static int s_last_touch_log_party_y = -1;
   int object_id;
   int lock_still_active;

   lock_still_active = 0;

   if (g_lava_touch_lock_object > 0)
   {
      addr lock_evt;

      lock_evt = PAL_LavaSceneEventData(g_lava_touch_lock_object);
      if (lock_evt != 0)
      {
          int lock_mode;
          int lock_dist;
          int lock_range;

          lock_mode = PAL_LavaReadU16(lock_evt, 14);
          lock_dist = abs(party_x - PAL_LavaReadU16(lock_evt, 2)) +
             abs(party_y - PAL_LavaReadU16(lock_evt, 4)) * 2;
          lock_range = (lock_mode >= 4) ? ((lock_mode - 4) * 32 + 16) : 0;
         if (PAL_LavaReadU16(lock_evt, 8) == g_lava_touch_lock_trigger &&
             lock_mode >= 4 && lock_dist < lock_range)
         {
            lock_still_active = 1;
         }
      }

      if (!lock_still_active)
      {
         g_lava_touch_lock_object = 0;
         g_lava_touch_lock_trigger = 0;
      }
   }

   for (object_id = g_lava_scene_event_first;
      object_id <= g_lava_scene_event_last;
      object_id++)
   {
      addr evt;
      int state;
      int trigger_mode;
       long trigger_script;
      int distance;
      int trigger_range;

      evt = PAL_LavaSceneEventData(object_id);
      if (evt == 0) continue;

      state = PAL_LavaReadS16(evt, 12);
      trigger_mode = PAL_LavaReadU16(evt, 14);
      trigger_script = PAL_LavaReadU16(evt, 8);
      if (state <= 0 || trigger_mode < 4 || trigger_script == 0)
      {
         continue;
      }

      if (lock_still_active &&
          object_id == g_lava_touch_lock_object &&
          trigger_script == g_lava_touch_lock_trigger)
      {
         continue;
      }

      distance = abs(party_x - PAL_LavaReadU16(evt, 2)) +
         abs(party_y - PAL_LavaReadU16(evt, 4)) * 2;
      trigger_range = (trigger_mode - 4) * 32 + 16;
      if (distance >= trigger_range)
      {
         continue;
      }

      if (PAL_LavaReadU16(evt, 18) > 0)
      {
         int x_offset;
         int y_offset;

         x_offset = party_x - PAL_LavaReadU16(evt, 2);
         y_offset = party_y - PAL_LavaReadU16(evt, 4);
         PAL_LavaWriteU16(evt, 22, 0);
         if (x_offset > 0)
         {
            PAL_LavaWriteU16(evt, 20, (y_offset > 0) ? 3 : 2);
         }
         else
         {
            PAL_LavaWriteU16(evt, 20, (y_offset > 0) ? 0 : 1);
         }
      }

      if (object_id != s_last_touch_log_object ||
          trigger_script != s_last_touch_log_trigger ||
          party_x != s_last_touch_log_party_x ||
          party_y != s_last_touch_log_party_y)
      {
         printf("[LAVA][TOUCH] hit object=%d trigger=%d mode=%d dist=%d pos=(%d,%d)\n",
            object_id, trigger_script, trigger_mode, distance,
            PAL_LavaReadU16(evt, 2), PAL_LavaReadU16(evt, 4));
         s_last_touch_log_object = object_id;
         s_last_touch_log_trigger = trigger_script;
         s_last_touch_log_party_x = party_x;
         s_last_touch_log_party_y = party_y;
      }
      PAL_LavaWriteU16(evt, 8,
         PAL_LavaRunTriggerScript(trigger_script, object_id));
      g_lava_touch_lock_object = object_id;
      g_lava_touch_lock_trigger = PAL_LavaReadU16(evt, 8);
      PAL_LavaFinishScriptStep();
      return 1;
   }

   return 0;
}

static void PAL_LavaUpdateViewport(void)
{
   g_lava_view_x = g_lava_party_x - 160;
   g_lava_view_y = g_lava_party_y - 112;
   if (g_lava_view_x < 0) g_lava_view_x = 0;
   if (g_lava_view_y < 0) g_lava_view_y = 0;
   PAL_LavaSyncPartyToViewport();
}

static void PAL_LavaReturnFromBattle(void)
{
   PAL_ClearKeyState();
   g_lava_key_hold = 0;
   g_lava_frame_hold = 0;
   g_lava_party_frame = 0;
   g_lava_touch_cooldown_until_frame = g_lava_logic_frame_num + 12;
   PAL_LavaResetPartyTrail();
   PAL_LavaUpdateViewport();
   g_lava_defer_scene_preview_draw = 1;
   PAL_LavaLoadScenePreview();
   g_lava_defer_scene_preview_draw = 0;
}

static void PAL_LavaSetScene(int scene_num)
{
   FILE *fp;
   long scene_size;
   int scene_offset;
   int next_scene_offset;
   int map_num;
   int script_enter;
   int script_teleport;
   int event_first;
   int event_last;

   if (scene_num <= 0)
   {
      return;
   }

   fp = UTIL_OpenRequiredFile("SSS.MKF");
   if (fp == 0) return;
   scene_size = PAL_MKFReadChunk((addr)g_lava_mkf_buf, 65536, 1, fp);
   fclose(fp);
   if (scene_size < scene_num * 8 + 8)
   {
      return;
   }

   PAL_LavaInitSceneScriptRuntime((addr)g_lava_mkf_buf, scene_size);

   scene_offset = (scene_num - 1) * 8;
   next_scene_offset = scene_offset + 8;
   map_num = PAL_LavaReadU16((addr)g_lava_mkf_buf, scene_offset);
   script_enter = PAL_LavaReadU16((addr)g_lava_mkf_buf, scene_offset + 2);
   script_teleport = PAL_LavaReadU16((addr)g_lava_mkf_buf, scene_offset + 4);
   event_first = PAL_LavaReadU16((addr)g_lava_mkf_buf, scene_offset + 6) + 1;
   event_last = PAL_LavaReadU16((addr)g_lava_mkf_buf, next_scene_offset + 6);

    g_lava_scene_num = scene_num;
    g_lava_scene_map_num = map_num;
    g_lava_scene_script_enter = g_lava_scene_runtime_enter[scene_num - 1];
    g_lava_scene_script_teleport = g_lava_scene_runtime_teleport[scene_num - 1];
    g_lava_scene_enter_pending = (PAL_LavaSceneEnterScriptFor(scene_num) > 0) ? 1 : 0;
    g_lava_scene_event_first = event_first;
     g_lava_scene_event_last = event_last;
     g_lava_party_layer = 0;
     g_lava_touch_lock_object = 0;
     g_lava_touch_lock_trigger = 0;
     g_lava_touch_cooldown_until_frame = g_lava_logic_frame_num + 12;
      if (!g_lava_loading_saved_game)
      {
         PAL_LavaRebuildSceneAutoscriptRuntime();
      }
      PAL_LavaLoadScenePreview();
   printf("[LAVA][SCENE] scene=%d map=%d enter=%d teleport=%d events=%d..%d\n",
      g_lava_scene_num, g_lava_scene_map_num, g_lava_scene_script_enter,
      g_lava_scene_script_teleport, g_lava_scene_event_first, g_lava_scene_event_last);
}

static void PAL_LavaLoadScenePreview(void)
{
    long map_ret;
    long gop_size;
    FILE *fp_map;
    FILE *fp_gop;

    if (g_lava_scene_map_num <= 0)
    {
       return;
    }

    fp_map = UTIL_OpenRequiredFile("MAP.MKF");
    fp_gop = UTIL_OpenRequiredFile("GOP.MKF");
    if (fp_map == 0 || fp_gop == 0)
    {
       if (fp_map != 0) fclose(fp_map);
       if (fp_gop != 0) fclose(fp_gop);
       return;
    }

    map_ret = -1;
    fseek(fp_map, 0, SEEK_SET);
    if (fread(&g_lava_mkf_read_offset, 1, 4, fp_map) == 4)
    {
       g_lava_mkf_read_chunk_count = (SDL_SwapLE32(g_lava_mkf_read_offset) - 4) >> 2;
       if (g_lava_scene_map_num < g_lava_mkf_read_chunk_count)
       {
          fseek(fp_map, 4 * g_lava_scene_map_num, SEEK_SET);
          fread(&g_lava_mkf_read_offset, 1, 4, fp_map);
          fread(&g_lava_mkf_read_next_offset, 1, 4, fp_map);
          g_lava_mkf_read_offset = SDL_SwapLE32(g_lava_mkf_read_offset);
          g_lava_mkf_read_next_offset = SDL_SwapLE32(g_lava_mkf_read_next_offset);
          g_lava_mkf_read_len = g_lava_mkf_read_next_offset - g_lava_mkf_read_offset;
          if (g_lava_mkf_read_len > 0 && g_lava_mkf_read_len <= 65536)
          {
             fseek(fp_map, g_lava_mkf_read_offset, SEEK_SET);
             fread((addr)g_lava_mkf_buf, 1, g_lava_mkf_read_len, fp_map);
             PAL_TmpReset();
             map_ret = Decompress((addr)g_lava_mkf_buf, (addr)g_lava_map_tiles_buf, 65536);
          }
       }
    }

    gop_size = -1;
    fseek(fp_gop, 0, SEEK_SET);
    if (fread(&g_lava_mkf_read_offset, 1, 4, fp_gop) == 4)
    {
       g_lava_mkf_read_chunk_count = (SDL_SwapLE32(g_lava_mkf_read_offset) - 4) >> 2;
       if (g_lava_scene_map_num < g_lava_mkf_read_chunk_count)
       {
          fseek(fp_gop, 4 * g_lava_scene_map_num, SEEK_SET);
          fread(&g_lava_mkf_read_offset, 1, 4, fp_gop);
          fread(&g_lava_mkf_read_next_offset, 1, 4, fp_gop);
          g_lava_mkf_read_offset = SDL_SwapLE32(g_lava_mkf_read_offset);
          g_lava_mkf_read_next_offset = SDL_SwapLE32(g_lava_mkf_read_next_offset);
          g_lava_mkf_read_len = g_lava_mkf_read_next_offset - g_lava_mkf_read_offset;
          if (g_lava_mkf_read_len > 0 && g_lava_mkf_read_len <= 65536)
          {
             fseek(fp_gop, g_lava_mkf_read_offset, SEEK_SET);
             gop_size = fread((addr)g_lava_map_gop_buf, 1, g_lava_mkf_read_len, fp_gop);
          }
       }
    }
    fclose(fp_map);
    fclose(fp_gop);
    printf("[LAVA][SCENE] map=%d mapdec=%d gop=%d\n",
       g_lava_scene_map_num, map_ret, gop_size);
   if (map_ret < 0 || gop_size <= 0)
   {
      return;
   }

   PAL_SetPalette(0, FALSE);
   g_lava_scene_ready = 1;
   PAL_LavaUpdateViewport();
   if (!g_lava_defer_scene_preview_draw)
   {
      PAL_LavaDrawSceneFrame();
      printf("[LAVA][SCENE] rendered scene=%d view=(%d,%d) tiles=%d events=%d player=%d mgo_dec=%d\n",
         g_lava_scene_num, g_lava_view_x, g_lava_view_y, g_lava_dbg_map_tiles_drawn, g_lava_dbg_event_sprites_drawn,
         g_lava_dbg_player_sprites_drawn, g_lava_dbg_mgo_decompresses);
   }
   else
   {
      printf("[LAVA][SCENE] preview deferred scene=%d view=(%d,%d)\n",
         g_lava_scene_num, g_lava_view_x, g_lava_view_y);
   }
}

static void PAL_LavaProbeNewGameData(void)
{
   FILE *fp;
   long scene_size;
   long event_size;
   long script_size;
   int scene_offset;
   int next_scene_offset;
   int map_num;
   int script_enter;
   int script_teleport;
   int event_first;
   int event_last;

   fp = UTIL_OpenRequiredFile("SSS.MKF");
   if (fp == 0) return;

   scene_size = PAL_MKFReadChunk((addr)g_lava_mkf_buf, 65536, 1, fp);
   event_size = PAL_MKFGetChunkSize(0, fp);
   script_size = PAL_MKFGetChunkSize(4, fp);
   if (scene_size < 16)
   {
      printf("[LAVA][NEWGAME] scene table read failed size=%d\n", scene_size);
      fclose(fp);
      return;
   }

   PAL_LavaInitSceneScriptRuntime((addr)g_lava_mkf_buf, scene_size);
   scene_offset = 0;
   next_scene_offset = 8;
   map_num = PAL_LavaReadU16((addr)g_lava_mkf_buf, scene_offset);
   script_enter = PAL_LavaReadU16((addr)g_lava_mkf_buf, scene_offset + 2);
   script_teleport = PAL_LavaReadU16((addr)g_lava_mkf_buf, scene_offset + 4);
   event_first = PAL_LavaReadU16((addr)g_lava_mkf_buf, scene_offset + 6) + 1;
   event_last = PAL_LavaReadU16((addr)g_lava_mkf_buf, next_scene_offset + 6);

   g_lava_scene_num = 1;
   g_lava_scene_map_num = map_num;
    g_lava_scene_script_enter = g_lava_scene_runtime_enter[0];
    g_lava_scene_script_teleport = g_lava_scene_runtime_teleport[0];
    g_lava_scene_enter_pending = 0;
    g_lava_scene_event_first = event_first;
    g_lava_scene_event_last = event_last;
    PAL_LavaInitSceneStorageOffsets();
    g_lava_scene_event_count = event_size / 32;
    g_lava_touch_lock_object = 0;
    g_lava_touch_lock_trigger = 0;
    g_lava_touch_cooldown_until_frame = 0;
    PAL_LavaZeroWords(g_lava_autoscript_pc, 5332);
    PAL_LavaZeroWords(g_lava_autoscript_idle, 5332);
    if (g_lava_scene_event_offset >= 0)
    {
       int object_id;

       for (object_id = 1; object_id <= g_lava_scene_event_count; object_id++)
       {
          long object_offset;

         object_offset = g_lava_scene_event_offset + (object_id - 1) * 32;
         fseek(fp, object_offset, SEEK_SET);
         fread((addr)(g_lava_scene_event_raw + (object_id - 1) * 32), 1, 32, fp);
      }
   }
   fclose(fp);
   g_lava_party_x = 1152 + 160;
   g_lava_party_y = 176 + 112;
   g_lava_view_x = 1152;
   g_lava_view_y = 176;
   g_lava_party_direction = 0;
   g_lava_party_frame = 0;
   g_lava_party_layer = 0;
   g_lava_party_role[0] = 0;
   g_lava_party_count = 1;

   printf("[LAVA][NEWGAME] sss scene=%d event=%d script=%d\n",
      scene_size, event_size, script_size);
    printf("[LAVA][NEWGAME] scene=1 map=%d enter=%d teleport=%d events=%d..%d\n",
       map_num, script_enter, script_teleport, event_first, event_last);
    PAL_LavaDumpScriptEntry(script_enter - 1);
    PAL_LavaRunEnterScript(script_enter);
}

static void PAL_LavaFinishScriptStep(void)
{
   if (g_lava_scene_enter_pending)
   {
      PAL_LavaRunPendingSceneEnter();
      return;
   }

   PAL_LavaRunPendingDialog();
   PAL_LavaUpdateViewport();
   if (g_lava_gpGlobals.fNeedToFadeIn)
   {
      PAL_SetPalette(g_lava_gpGlobals.wNumPalette, g_lava_gpGlobals.fNightPalette);
      lava_apply_palette_scale(g_current_palette, 0, 15);
   }
   PAL_LavaDrawSceneFrame();
   if (g_lava_gpGlobals.fNeedToFadeIn)
   {
      PAL_FadeIn(1);
      g_lava_gpGlobals.fNeedToFadeIn = FALSE;
   }
   PAL_ClearKeyState();
}

static WORD PAL_LavaSceneEnterScriptFor(int scene_num)
{
   if (scene_num <= 0 || scene_num > 300)
   {
      return 0;
   }

   if (g_lava_scene_hook_enter[scene_num - 1] != 0)
   {
      return g_lava_scene_hook_enter[scene_num - 1];
   }

   return g_lava_scene_runtime_enter[scene_num - 1];
}

static WORD PAL_LavaSceneTeleportScriptFor(int scene_num)
{
   if (scene_num <= 0 || scene_num > 300)
   {
      return 0;
   }

   if (g_lava_scene_hook_teleport[scene_num - 1] != 0)
   {
      return g_lava_scene_hook_teleport[scene_num - 1];
   }

   return g_lava_scene_runtime_teleport[scene_num - 1];
}

static void PAL_LavaSetSceneScriptHooks(int scene_num, int enter_script, int teleport_script)
{
   if (scene_num <= 0 || scene_num > 300)
   {
      return;
   }

   if (enter_script == 0 && teleport_script == 0)
   {
      g_lava_scene_hook_enter[scene_num - 1] = 0;
      g_lava_scene_hook_teleport[scene_num - 1] = 0;
      return;
   }

   if (enter_script != 0)
   {
      g_lava_scene_hook_enter[scene_num - 1] = (WORD)enter_script;
   }
   if (teleport_script != 0)
   {
      g_lava_scene_hook_teleport[scene_num - 1] = (WORD)teleport_script;
   }
}

static void PAL_LavaSetActiveSceneEnterScript(int enter_script)
{
   if (g_lava_scene_num <= 0 || g_lava_scene_num > 300)
   {
      return;
   }

   g_lava_scene_runtime_enter[g_lava_scene_num - 1] = (WORD)enter_script;
   g_lava_scene_script_enter = enter_script;

   if (g_lava_scene_hook_enter[g_lava_scene_num - 1] != 0)
   {
      g_lava_scene_hook_enter[g_lava_scene_num - 1] = (WORD)enter_script;
   }
}

void PAL_WaitForAnyKey(WORD wTimeOut)
{
   DWORD dwBegin = SDL_GetTicks();

   while (wTimeOut == 0 || SDL_GetTicks() - dwBegin < wTimeOut)
   {
      PAL_ProcessEvent();
      if (g_InputState.dwKeyPress != 0)
      {
         break;
      }
      Delay(10);
   }
}

void PAL_WaitForKey(WORD wTimeOut)
{
   PAL_WaitForAnyKey(wTimeOut);
}

static int PAL_LavaMenuSelectedColor(void)
{
    return MENUITEM_COLOR_SELECTED_FIRST +
       SDL_GetTicks() / (600 / MENUITEM_COLOR_SELECTED_TOTALNUM) %
       MENUITEM_COLOR_SELECTED_TOTALNUM;
}

static int PAL_LavaReadConfirmKey(void)
{
   int key;

   key = Inkey();
   if (key == KEY_ENTER)
   {
      return 1;
   }

   if (g_InputState.dwKeyPress & (kKeySearch | kKeyMenu))
   {
      return 1;
   }

   return 0;
}

static int PAL_LavaReadCancelKey(void)
{
   if (g_InputState.dwKeyPress & kKeyMenu)
   {
      return 1;
   }

   return 0;
}

static void PAL_LavaDrawOpeningMenuText(int x, int y, int label, int color)
{
   char *text;

   if (label == 0)
   {
      text = "新的故事";
   }
   else
   {
      text = "旧的回忆";
   }

   PAL_LavaTextOutToSurface(gpScreen, x + 1, y + 1, text, 0, 0);
   PAL_LavaTextOutToSurface(gpScreen, x, y, text, color, 0);
   lava_present_current_screen();
}

static void PAL_LavaRestoreOpeningMenuTextArea(void)
{
   int x;
   int y;
   int w;
   int h;
   int row;

   x = MENU_TEXT_X - 2;
   y = MENU_TEXT_Y0 - 2;
   w = 96;
   h = MENU_TEXT_Y1 - MENU_TEXT_Y0 + 22;
   if (x < 0)
   {
      w += x;
      x = 0;
   }
   if (y < 0)
   {
      h += y;
      y = 0;
   }
   if (x + w > SCREEN_W)
   {
      w = SCREEN_W - x;
   }
   if (y + h > SCREEN_H)
   {
      h = SCREEN_H - y;
   }
   if (w <= 0 || h <= 0)
   {
      return;
   }

   for (row = 0; row < h; row++)
   {
      memcpy(g_screen_surface.pixels + (y + row) * g_screen_surface.pitch + x,
         g_back_buf + (y + row) * SCREEN_W + x,
         w);
   }
}

static void PAL_LavaDrawOpeningMenuLabels(int selected)
{
   int selected_color;

   selected_color = PAL_LavaMenuSelectedColor();
   lava_begin_text_batch();
   if (selected == 0)
   {
      PAL_LavaDrawOpeningMenuText(MENU_TEXT_X, MENU_TEXT_Y0, 0, selected_color);
      PAL_LavaDrawOpeningMenuText(MENU_TEXT_X, MENU_TEXT_Y1, 1, MENUITEM_COLOR);
   }
   else
   {
      PAL_LavaDrawOpeningMenuText(MENU_TEXT_X, MENU_TEXT_Y0, 0, MENUITEM_COLOR);
      PAL_LavaDrawOpeningMenuText(MENU_TEXT_X, MENU_TEXT_Y1, 1, selected_color);
   }
   lava_end_text_batch();
}

static void PAL_LavaRedrawOpeningMenuSelection(int selected)
{
   PAL_LavaRestoreOpeningMenuTextArea();
   PAL_LavaDrawOpeningMenuLabels(selected);
   Refresh();
}

static void PAL_LavaDrawSystemMenuText(int x, int y, char *text, int color)
{
   PAL_LavaDrawShadowText(x, y, text, color);
}

static void PAL_LavaUpdateRect(int x, int y, int w, int h)
{
   if (x < 0)
   {
      w += x;
      x = 0;
   }
   if (y < 0)
   {
      h += y;
      y = 0;
   }
   if (x + w > SCREEN_W)
   {
      w = SCREEN_W - x;
   }
   if (y + h > SCREEN_H)
   {
      h = SCREEN_H - y;
   }
   if (w <= 0 || h <= 0)
   {
      return;
   }
   WriteBlock(x, y, w, h, 1, (addr)(g_screen_surface.pixels + y * g_screen_surface.pitch + x));
   Refresh();
}

static addr PAL_LavaLoadUISprite(void)
{
    FILE *fp;
    long size;

   if (g_lava_ui_sprite_ready)
   {
      return (addr)g_lava_ui_sprite_buf;
   }

   fp = UTIL_OpenRequiredFile("DATA.MKF");
   if (fp == 0) return 0;
   size = PAL_MKFReadChunk((addr)g_lava_ui_sprite_buf,
      sizeof(g_lava_ui_sprite_buf), 9, fp);
   fclose(fp);
   if (size <= 0)
   {
      return 0;
   }

   g_lava_ui_sprite_ready = 1;
   return (addr)g_lava_ui_sprite_buf;
}

static int PAL_LavaDrawSpriteBoxAt(addr sprite, int x, int y, int rows, int columns, int style)
{
   int i;
   int j;
   int m;
   int n;
   int draw_x;
   int draw_y;
   LPCBITMAPRLE frame;

   if (sprite == 0)
   {
      return 0;
   }

   rows += 2;
   columns += 2;
   draw_y = y;
   for (i = 0; i < rows; i++)
   {
      draw_x = x;
      m = (i == 0) ? 0 : ((i == rows - 1) ? 2 : 1);
      for (j = 0; j < columns; j++)
      {
         n = (j == 0) ? 0 : ((j == columns - 1) ? 2 : 1);
         frame = PAL_SpriteGetFrame(sprite, m * 3 + n + style * 9);
         PAL_RLEBlitToSurfaceWithShadow(frame, gpScreen, PAL_XY(draw_x + 6, draw_y + 6), TRUE);
         PAL_RLEBlitToSurface(frame, gpScreen, PAL_XY(draw_x, draw_y));
         draw_x += PAL_RLEGetWidth(frame);
      }
      frame = PAL_SpriteGetFrame(sprite, m * 3 + style * 9);
      draw_y += PAL_RLEGetHeight(frame);
   }

   return 1;
}

static void PAL_LavaDrawSingleLineBox(int x, int y, int w)
{
   addr sprite;
   LPCBITMAPRLE left;
   LPCBITMAPRLE mid;
   LPCBITMAPRLE right;
   int i;
   int draw_x;
   int len;

   sprite = PAL_LavaLoadUISprite();
   if (sprite == 0)
   {
      return;
   }

   left = PAL_SpriteGetFrame(sprite, 44);
   mid = PAL_SpriteGetFrame(sprite, 45);
   right = PAL_SpriteGetFrame(sprite, 46);
   len = w;
   draw_x = x;
   PAL_RLEBlitToSurfaceWithShadow(left, gpScreen, PAL_XY(draw_x + 6, y + 6), TRUE);
   draw_x += PAL_RLEGetWidth(left);
   for (i = 0; i < len; i++)
   {
      PAL_RLEBlitToSurfaceWithShadow(mid, gpScreen, PAL_XY(draw_x + 6, y + 6), TRUE);
      draw_x += PAL_RLEGetWidth(mid);
   }
   PAL_RLEBlitToSurfaceWithShadow(right, gpScreen, PAL_XY(draw_x + 6, y + 6), TRUE);

   draw_x = x;
   PAL_RLEBlitToSurface(left, gpScreen, PAL_XY(draw_x, y));
   draw_x += PAL_RLEGetWidth(left);
   for (i = 0; i < len; i++)
   {
      PAL_RLEBlitToSurface(mid, gpScreen, PAL_XY(draw_x, y));
      draw_x += PAL_RLEGetWidth(mid);
   }
   PAL_RLEBlitToSurface(right, gpScreen, PAL_XY(draw_x, y));
}

static void PAL_LavaDrawDirectSingleLineBox(int x, int y, int w)
{
   SetFgColor(0x08);
   Box(x + 3, y + 3, x + w + 3, y + 21, 1, 1);
   SetFgColor(0x21);
   Box(x, y, x + w, y + 18, 1, 1);
   SetFgColor(0);
   Box(x + 1, y + 1, x + w - 1, y + 17, 0, 1);
   SetFgColor(0xDA);
   Line(x + 1, y + 1, x + w - 1, y + 1, 1);
   Line(x + 1, y + 1, x + 1, y + 17, 1);
   SetFgColor(0x2F);
   Line(x + 1, y + 17, x + w - 1, y + 17, 1);
    Line(x + w - 1, y + 1, x + w - 1, y + 17, 1);
}

static int PAL_LavaRoleWordByArray(int array_index, int role_index)
{
   int offset;
   int percent;
   int value;

   if (role_index < 0 || role_index >= 6)
   {
      return 0;
   }

   offset = array_index * 6 * 2 + role_index * 2;
   value = PAL_LavaReadU16((addr)g_lava_data_buf, offset);
   if (array_index >= 0 && array_index < LAVA_ROLE_TEMP_STAT_MAX)
   {
      percent = g_lava_role_temp_stat_percent[array_index][role_index];
      if (percent > 0)
      {
         value = (int)(((long)value * (long)percent) / 100);
      }
   }
   return value;
}

static void PAL_LavaSetRoleTempStatPercent(int array_index, int role_index, int percent)
{
   if (array_index < 0 || array_index >= LAVA_ROLE_TEMP_STAT_MAX ||
       role_index < 0 || role_index >= 6)
   {
      return;
   }
   if (percent < 0)
   {
      percent = 0;
   }
   g_lava_role_temp_stat_percent[array_index][role_index] = percent;
}

static int PAL_LavaRoleTempStatPercent(int array_index, int role_index)
{
   if (array_index < 0 || array_index >= LAVA_ROLE_TEMP_STAT_MAX ||
       role_index < 0 || role_index >= 6)
   {
      return 0;
   }
   return g_lava_role_temp_stat_percent[array_index][role_index];
}

static void PAL_LavaClearRoleTempStats(void)
{
   int array_index;
   int role_index;

   for (array_index = 0; array_index < LAVA_ROLE_TEMP_STAT_MAX; array_index++)
   {
      for (role_index = 0; role_index < 6; role_index++)
      {
         g_lava_role_temp_stat_percent[array_index][role_index] = 0;
      }
   }
}

static void PAL_LavaDrawNumberText(int x, int y, int value, int color)
{
   char buf[16];

   sprintf(buf, "%d", value);
   PAL_LavaDrawShadowText(x, y, buf, color);
}

static char *PAL_LavaReadWord(int word_id)
{
   int offset;
   int len;
   int i;
   FILE *fp;

   if (word_id < 0)
   {
      return 0;
   }

   offset = word_id * LAVA_WORD_DAT_WIDTH;
   if (g_lava_word_file_cache_ready &&
       offset >= 0 && offset + LAVA_WORD_DAT_WIDTH <= g_lava_word_file_cache_bytes)
   {
      memcpy(g_lava_word_buf, g_lava_word_file_cache + offset,
         LAVA_WORD_DAT_WIDTH);
   }
   else
   {
      fp = g_lava_word_file_is_gb2312 ?
         UTIL_OpenFile("WORD_GB2312.DAT") : UTIL_OpenRequiredFile("WORD.DAT");
      if (fp == 0)
      {
         return 0;
      }
      if (!PAL_LavaFseekOK(fp, offset, SEEK_SET) ||
          fread((addr)g_lava_word_buf, 1, LAVA_WORD_DAT_WIDTH, fp) != LAVA_WORD_DAT_WIDTH)
      {
         fclose(fp);
         return 0;
      }
      fclose(fp);
   }

   len = LAVA_WORD_DAT_WIDTH;
   while (len > 0 && g_lava_word_buf[len - 1] == ' ')
   {
      len--;
   }
   g_lava_word_buf[len] = 0;
    if (!g_lava_word_file_is_gb2312)
    {
       PAL_LavaConvertBig5MsgToGbk(g_lava_word_buf, len);
    }
    for (i = 0; g_lava_word_buf[i] != 0; i++)
    {
       if (g_lava_word_buf[i] == '1' && g_lava_word_buf[i + 1] == 0)
       {
          g_lava_word_buf[i] = 0;
          break;
       }
    }
#ifdef LAVA_NATIVE_COMPILED
    if (g_lava_word_buf[0] != 0)
    {
       g_lava_word_utf8_buf[0] = 0;
       if (gb_to_utf8(g_lava_word_buf, g_lava_word_utf8_buf) &&
           g_lava_word_utf8_buf[0] != 0)
       {
          return g_lava_word_utf8_buf;
       }
    }
#endif
    return g_lava_word_buf;
}

static char *PAL_LavaRoleName(int role_index)
{
   char *name;

   name = PAL_LavaReadWord(PAL_LavaRoleWordByArray(3, role_index));
   if (name != 0 && name[0] != 0)
   {
      return name;
   }

   if (role_index == 0) return "李逍遥";
   if (role_index == 1) return "赵灵儿";
   if (role_index == 2) return "林月如";
   if (role_index == 3) return "巫后";
   if (role_index == 4) return "阿奴";
   if (role_index == 5) return "盖罗娇";
   return "同伴";
}

static char *PAL_LavaRoleNameForLog(int role_index)
{
#ifdef LAVA_NATIVE_COMPILED
   return PAL_LavaRoleName(role_index);
#endif
   return PAL_LavaRoleName(role_index);
}

static char *PAL_LavaReadObjectDesc(int object_id)
{
   char key_buf[8];
   char line_buf[128];
   char one[1];
   int key_len;
   int n;
   int eq_pos;
   int out_pos;
   int line_len;
   long old_pos;
   FILE *fp;

   if (object_id <= 0)
   {
      return 0;
   }

   sprintf(key_buf, "%x(", object_id);
   key_len = (int)strlen(key_buf);

   /* Scan desc.dat line-by-line, reading one byte at a time (the Lava
      compiler has no fgets/fgetc, so use fread of 1 byte). No whole-file
      caching (that overflows the device heap). Records look like:
      "3e(name)=description*effect". */
   fp = UTIL_OpenFile("desc_gb2312.dat");
   if (fp == 0)
   {
      fp = UTIL_OpenFile("desc.dat");
   }
   if (fp == 0)
   {
      return 0;
   }
   old_pos = ftell(fp);
   fseek(fp, 0, SEEK_SET);

   line_len = 0;
   while (fread((addr)one, 1, 1, fp) == 1)
   {
      if (one[0] == '\n')
      {
         line_buf[line_len] = 0;
         if (line_len >= key_len)
         {
            int match = 1;
            for (n = 0; n < key_len; n++)
            {
               if (line_buf[n] != key_buf[n])
               {
                  match = 0;
                  break;
               }
            }
            if (match != 0)
            {
               eq_pos = -1;
               for (n = key_len; n < line_len; n++)
               {
                  if (line_buf[n] == '=')
                  {
                     eq_pos = n;
                     break;
                  }
               }
               if (eq_pos >= 0)
               {
                  out_pos = 0;
                  for (n = eq_pos + 1;
                       n < line_len && out_pos < (int)sizeof(g_lava_desc_buf) - 1;
                       n++)
                  {
                     g_lava_desc_buf[out_pos++] = line_buf[n];
                  }
                  g_lava_desc_buf[out_pos] = 0;
                  for (n = 0; n < out_pos; n++)
                  {
                     if (g_lava_desc_buf[n] == '*')
                     {
                        g_lava_desc_buf[n] = ' ';
                     }
                  }
#ifdef LAVA_NATIVE_COMPILED
                  if (g_lava_desc_buf[0] != 0)
                  {
                     g_lava_desc_utf8_buf[0] = 0;
                     if (gb_to_utf8(g_lava_desc_buf, g_lava_desc_utf8_buf) &&
                         g_lava_desc_utf8_buf[0] != 0)
                     {
                        fseek(fp, old_pos, SEEK_SET);
                        fclose(fp);
                        return g_lava_desc_utf8_buf;
                     }
                  }
#endif
                   fseek(fp, old_pos, SEEK_SET);
                   fclose(fp);
                   return g_lava_desc_buf;
                }
            }
         }
         line_len = 0;
      }
      else
      {
         if (line_len < (int)sizeof(line_buf) - 1)
         {
            line_buf[line_len++] = one[0];
         }
      }
   }

   fseek(fp, old_pos, SEEK_SET);
   fclose(fp);
   return 0;
}

static void PAL_LavaFormatObjectLabel(char *buf, char *prefix, int object_id)
{
   char *name;

   name = PAL_LavaReadWord(object_id);
   if (name != 0 && name[0] != 0)
   {
      strcpy(buf, name);
      return;
   }

   sprintf(buf, "%s%d", prefix, object_id);
}

static long PAL_LavaReadObjectField(int object_id, int field_index)
{
   FILE *fp;
   long object_size;
   long object_offset;
   int obj_size;

   if (object_id <= 0)
   {
      return 0;
   }

   if (g_lava_object_data_loaded == 0)
   {
        fp = UTIL_OpenRequiredFile("SSS.MKF");
        if (fp == 0)
        {
           return 0;
        }
        obj_size = gConfig.fIsWIN95 ? 14 : 12;
        object_size = PAL_MKFReadChunk(
           (addr)g_lava_object_data, sizeof(g_lava_object_data), 2,
           fp);
        fclose(fp);
       printf("[LAVA][OBJECTDATA] read chunk 2 size=%d obj_size=%d\n",
          (int)object_size, obj_size);
       if (object_size <= 0)
       {
          return 0;
       }
       g_lava_object_data_size = object_size;
       g_lava_object_data_loaded = 1;
   }

   if (gConfig.fIsWIN95)
   {
      object_offset = (long)object_id * 14 + field_index * 2;
   }
   else
   {
      object_offset = (long)object_id * 12 + field_index * 2;
   }

   if (object_offset + 2 > g_lava_object_data_size)
   {
      return 0;
   }

    return PAL_LavaReadU16((addr)g_lava_object_data, object_offset);
}

static void PAL_LavaWriteObjectField(int object_id, int field_index, long value)
{
   long object_offset;

   if (PAL_LavaReadObjectField(object_id, field_index) == 0 &&
       (g_lava_object_data_loaded == 0 || object_id <= 0))
   {
      return;
   }

   if (gConfig.fIsWIN95)
   {
      object_offset = (long)object_id * 14 + field_index * 2;
   }
   else
   {
      object_offset = (long)object_id * 12 + field_index * 2;
   }

   if (object_offset + 2 > g_lava_object_data_size)
   {
      return;
   }

   PAL_LavaWriteU16((addr)g_lava_object_data, object_offset, (int)value);
}

static long PAL_LavaReadMagicField(int magic_id, int field_index)
{
   FILE *fp;
   long chunk_size;
   long magic_offset;
   char magic_buf[4];

   if (magic_id < 0)
   {
      return 0;
   }

   fp = UTIL_OpenRequiredFile("DATA.MKF");
   if (fp == 0) return 0;
   chunk_size = PAL_MKFGetChunkSize(4, fp);
    magic_offset = (long)magic_id * 32 + field_index * 2;
   if (magic_offset + 2 > chunk_size)
   {
      fclose(fp);
      return 0;
   }
   if (!PAL_LavaFseekOK(fp, PAL_LavaMKFChunkOffset(fp, 4) + magic_offset, SEEK_SET))
   {
      fclose(fp);
      return 0;
   }
   if (fread((addr)magic_buf, 1, 2, fp) != 2)
   {
      fclose(fp);
      return 0;
   }
   fclose(fp);
   return PAL_LavaReadU16((addr)magic_buf, 0);
}

static int PAL_LavaChooseTargetRole(char *title, int object_id, int parent_kind)
{
   int selected;
   int need_full_redraw;

   selected = 0;
   need_full_redraw = 1;
   PAL_ClearKeyState();
   while (TRUE)
   {
      int i;
      int role;

      lava_begin_text_batch();
      if (need_full_redraw)
      {
         need_full_redraw = 0;
         if (parent_kind >= 0)
         {
            PAL_LavaDrawMainMenu();
         }
      }
      if (!PAL_LavaDrawSpriteBoxAt(PAL_LavaLoadUISprite(), 96, 20, 8, 8, 0))
      {
         SetFgColor(0x21);
         Box(96, 20, 280, 174, 1, 1);
      }
      PAL_LavaUpdateRect(96, 20, 190, 160);

      PAL_LavaDrawShadowText(108, 30, title, 0x2C);
      PAL_LavaFormatObjectLabel(g_lava_data_buf, title, object_id);
      PAL_LavaDrawShadowText(108, 48, g_lava_data_buf, MENUITEM_COLOR);

      for (i = 0; i < g_lava_party_count; i++)
      {
         int color;

         role = g_lava_party_role[i];
         color = (i == selected) ? MENUITEM_COLOR_SELECTED : MENUITEM_COLOR;
         PAL_LavaDrawShadowText(108, 72 + i * 22, PAL_LavaRoleName(role), color);
         PAL_LavaDrawShadowText(180, 72 + i * 22, "体", MENUITEM_COLOR);
         PAL_LavaDrawNumberText(200, 72 + i * 22, PAL_LavaRoleWordByArray(9, role), color);
         PAL_LavaDrawShadowText(232, 72 + i * 22, "/", MENUITEM_COLOR);
         PAL_LavaDrawNumberText(244, 72 + i * 22, PAL_LavaRoleWordByArray(7, role), 0x2F);
      }
       PAL_LavaDrawShadowText(108, 182, "上下选择 确认使用 B返回", 0x1C);
      lava_end_text_batch();


       while (TRUE)
      {
         PAL_ProcessEvent();
         if (PAL_LavaReadCancelKey())
         {
            PAL_ClearKeyState();
            return -1;
         }
         if (g_InputState.dwKeyPress & (kKeyUp | kKeyLeft))
         {
            selected--;
            if (selected < 0) selected = g_lava_party_count - 1;
            PAL_ClearKeyState();
            break;
         }
         if (g_InputState.dwKeyPress & (kKeyDown | kKeyRight))
         {
            selected++;
            if (selected >= g_lava_party_count) selected = 0;
            PAL_ClearKeyState();
            break;
         }
         if (PAL_LavaReadConfirmKey())
         {
            PAL_ClearKeyState();
            return g_lava_party_role[selected];
         }
         Delay(50);
      }
   }
}

static int PAL_LavaChooseTwoItemMenu(char *title, char *first, char *second, int selected, int parent_kind)
{
   int current;
   int need_full_redraw;

   current = selected;
   need_full_redraw = 1;
   PAL_ClearKeyState();
   while (TRUE)
   {
      lava_begin_text_batch();
      if (need_full_redraw)
      {
         need_full_redraw = 0;
         if (parent_kind >= 0)
         {
            PAL_LavaDrawMainMenu();
             if (parent_kind == 0)
             {
                PAL_LavaDrawItemSubMenuFrame(1);
             }
            else if (parent_kind == 1)
            {
               PAL_LavaDrawMagicMenuContent(g_lava_magic_menu_party, g_lava_magic_menu_index);
            }
            else if (parent_kind == 2)
            {
               PAL_LavaDrawItemSubMenuFrame(0);
               PAL_LavaDrawEquipMenuContent(g_lava_equip_index);
            }
         }
      }
      if (!PAL_LavaDrawSpriteBoxAt(PAL_LavaLoadUISprite(), 30, 60, 1, 2, 0))
      {
         SetFgColor(0x21);
         Box(30, 60, 96, 114, 1, 1);
      }
      PAL_LavaDrawShadowText(43, 73, first, current == 0 ? MENUITEM_COLOR_SELECTED : MENUITEM_COLOR);
      PAL_LavaDrawShadowText(43, 91, second, current == 1 ? MENUITEM_COLOR_SELECTED : MENUITEM_COLOR);
      if (title != 0)
      {
         PAL_LavaDrawShadowText(112, 66, title, 0x2C);
      }
      lava_end_text_batch();

      while (TRUE)
      {
         PAL_ProcessEvent();
         if (PAL_LavaReadCancelKey())
         {
            PAL_ClearKeyState();
            return -1;
         }
         if (g_InputState.dwKeyPress & (kKeyUp | kKeyLeft))
         {
            current = 0;
            PAL_ClearKeyState();
            break;
         }
         if (g_InputState.dwKeyPress & (kKeyDown | kKeyRight))
         {
            current = 1;
            PAL_ClearKeyState();
            break;
         }
         if (PAL_LavaReadConfirmKey())
         {
            PAL_ClearKeyState();
            return current;
         }
         Delay(50);
      }
   }
}

static char *PAL_LavaMainMenuLabel(int index)
{
   if (index == 0) return "状态";
   if (index == 1) return "仙术";
   if (index == 2) return "物品";
   return "系统";
}

static void PAL_LavaDrawMainMenuItem(int index, int selected)
{
   PAL_LavaDrawSystemMenuText(28, 52 + index * 18,
      PAL_LavaMainMenuLabel(index),
      selected ? MENUITEM_COLOR_SELECTED : MENUITEM_COLOR);
}

static void PAL_LavaDrawMainMenuBox(int selected)
{
   if (!PAL_LavaDrawSpriteBoxAt(PAL_LavaLoadUISprite(), 16, 40, 3, 2, 0))
   {
      SetFgColor(0x21);
      Box(16, 40, 82, 118, 1, 1);
   }
   PAL_LavaDrawSingleLineBox(0, 0, 5);
}

static void PAL_LavaDrawMainMenuText(int selected)
{
   int i;
   int cash_x;
   char cash_buf[16];

   PAL_LavaDrawShadowText(10, 10, "银两", MENUITEM_COLOR);
   sprintf(cash_buf, "%d", g_lava_cash);
    cash_x = 84 - (int)strlen(cash_buf) * 8;
    if (cash_x < 44)
   {
      cash_x = 49;
   }
   PAL_LavaDrawShadowText(cash_x, 10, cash_buf, 0xCF);
    for (i = 0; i < 4; i++)
    {
       int color = (i == selected) ? MENUITEM_COLOR_SELECTED : MENUITEM_COLOR;
       PAL_LavaDrawShadowText(28, 52 + i * 18, PAL_LavaMainMenuLabel(i), color);
    }

}

static void PAL_LavaDrawMainMenuFrame(int selected)
{
   lava_begin_text_batch();
   PAL_LavaDrawMainMenuBox(selected);
   PAL_LavaDrawMainMenuText(selected);
   lava_end_text_batch();

}

static void PAL_LavaDrawMainMenu(void)
{
   PAL_LavaDrawSceneFrame();
   PAL_LavaDrawMainMenuFrame(g_lava_menu_selection);
}

static void PAL_LavaDrawItemSubMenuFrame(int selected)
{
   if (!PAL_LavaDrawSpriteBoxAt(PAL_LavaLoadUISprite(), 30, 60, 1, 2, 0))
   {
      SetFgColor(0x21);
      Box(30, 60, 96, 114, 1, 1);
   }
    PAL_LavaDrawShadowText(43, 73, "装备", selected == 0 ? MENUITEM_COLOR_SELECTED : MENUITEM_COLOR);
    PAL_LavaDrawShadowText(43, 91, "使用", selected == 1 ? MENUITEM_COLOR_SELECTED : MENUITEM_COLOR);

}

static void PAL_LavaDrawItemListFrame(char *title)
{
   if (!PAL_LavaDrawSpriteBoxAt(PAL_LavaLoadUISprite(), 20, 18, 8, 9, 0))
   {
      SetFgColor(0x21);
      Box(20, 18, 244, 174, 1, 1);
   }
    PAL_LavaDrawShadowText(32, 28, title, 0x2C);

}

static void PAL_LavaDrawMagicListFrame(void)
{
    if (!PAL_LavaDrawSpriteBoxAt(PAL_LavaLoadUISprite(), 20, 18, 8, 8, 0))
    {
       SetFgColor(0x21);
       Box(20, 18, 220, 174, 1, 1);
    }
    PAL_LavaDrawShadowText(32, 28, "仙术", 0x2C);

}

static void PAL_LavaDrawMagicMenuContent(int current_party, int current_index)
{
   int role;
   int magic_ids[32];
   int magic_mp[32];
   int magic_count;
   int start;
   int i;
   int j;
   int color;
   char *name;
   addr sprite;
   int mp_cur;
   int page_line;
   int col;
   int tmp_id;
   int tmp_mp;
   int magic_num;

   role = g_lava_party_role[current_party];
   mp_cur = PAL_LavaRoleWordByArray(10, role);
   magic_count = 0;
   for (i = 0; i < 32; i++)
   {
      int mid;

      mid = PAL_LavaRoleWordByArray(32 + i, role);
      if (mid != 0)
      {
         magic_ids[magic_count] = mid;
         magic_num = PAL_LavaReadObjectField(mid, 0);
         magic_mp[magic_count] = PAL_LavaReadMagicField(magic_num, 12);
         magic_count++;
      }
   }

   for (i = 0; i < magic_count - 1; i++)
   {
      for (j = 0; j < magic_count - 1 - i; j++)
      {
         if (magic_ids[j] > magic_ids[j + 1])
         {
            tmp_id = magic_ids[j];
            tmp_mp = magic_mp[j];
            magic_ids[j] = magic_ids[j + 1];
            magic_mp[j] = magic_mp[j + 1];
            magic_ids[j + 1] = tmp_id;
            magic_mp[j + 1] = tmp_mp;
         }
      }
   }

   lava_begin_text_batch();
   PAL_LavaDrawSceneFrame();

   PAL_LavaDrawSingleLineBox(0, 0, 5);
   PAL_LavaDrawShadowText(10, 10, "银两", MENUITEM_COLOR);
   {
      char cash_buf[16];
      int cash_x;
      sprintf(cash_buf, "%d", g_lava_cash);
      cash_x = 89 - (int)strlen(cash_buf) * 8;
      if (cash_x < 49) cash_x = 49;
      PAL_LavaDrawShadowText(cash_x, 10, cash_buf, 0xCF);
   }

   PAL_LavaDrawSingleLineBox(215, 0, 5);
   sprite = PAL_LavaLoadUISprite();
   if (sprite != 0)
   {
      PAL_RLEBlitToSurface((LPCBITMAPRLE)PAL_SpriteGetFrame(sprite, 39), gpScreen, PAL_XY(260, 14));
   }
   if (magic_count > 0)
   {
      PAL_LavaDrawNumberText(230, 14, magic_mp[current_index], 0xCF);
   }
   PAL_LavaDrawNumberText(265, 14, mp_cur, 0x2F);

   if (!PAL_LavaDrawSpriteBoxAt(sprite, 10, 42, 4, 16, 1))
   {
      SetFgColor(0x21);
      Box(10, 42, 298, 132, 1, 1);
   }

   start = current_index - 8;
   if (start < 0) start = 0;
   if (start > magic_count - 20) start = magic_count - 20;
   if (start < 0) start = 0;

   if (magic_count <= 0)
   {
      PAL_LavaDrawShadowText(35, 54, "尚未习得仙术", 0x1C);
   }
   else
   {
      page_line = 0;
      for (i = start; i < magic_count && page_line < 5; )
      {
         for (col = 0; col < 4 && i < magic_count; col++)
         {
            if (magic_mp[i] > mp_cur)
            {
               color = (i == current_index) ? MENUITEM_COLOR_SELECTED_INACTIVE : MENUITEM_COLOR_INACTIVE;
            }
            else
            {
               color = (i == current_index) ? MENUITEM_COLOR_SELECTED : MENUITEM_COLOR;
            }
            name = PAL_LavaReadWord(magic_ids[i]);
            if (name != 0 && name[0] != 0)
            {
               PAL_LavaDrawShadowText(35 + col * 71, 54 + page_line * 18, name, color);
            }
            if (i == current_index)
            {
               if (sprite != 0)
               {
                  PAL_RLEBlitToSurface((LPCBITMAPRLE)PAL_SpriteGetFrame(sprite, 69),
                     gpScreen, PAL_XY(35 + col * 71 + 20, 64 + page_line * 18));
               }
            }
            i++;
         }
         page_line++;
       }
    }
   lava_end_text_batch();
}

static int PAL_LavaBuildUsableItemList(int *item_ids, int *item_amounts)
{
   int item_count;
   int i;
   int item_flags;

   item_count = 0;
   for (i = 0; i < 256; i++)
   {
      if (g_lava_inventory_item[i] != 0 && g_lava_inventory_amount[i] > 0)
      {
         item_flags = PAL_LavaReadObjectField(g_lava_inventory_item[i], gConfig.fIsWIN95 ? 6 : 5);
         if (item_flags & 0x0001)
         {
            if (item_ids != 0)
            {
               item_ids[item_count] = g_lava_inventory_item[i];
            }
            if (item_amounts != 0)
            {
               item_amounts[item_count] = g_lava_inventory_amount[i];
            }
            item_count++;
         }
      }
   }

   return item_count;
}

static void PAL_LavaDrawItemUseMenuContent(int current_index)
{
   int item_ids[256];
   int item_amounts[256];
   int item_count;
   int start;
   int i;
   int line;
   int col;
   int item_x;
   int item_y;
   int cursor_x;
   int cursor_y;
   int item_per_line;
   int lines_per_page;
   int text_width;
   int bitmap_num;
   long bitmap_read;
   addr sprite;
   char *desc;
   char label_buf[24];

   item_count = PAL_LavaBuildUsableItemList(item_ids, item_amounts);
   sprite = PAL_LavaLoadUISprite();
   item_per_line = 3;
   lines_per_page = 7;
   text_width = 90;

   if (!PAL_LavaDrawSpriteBoxAt(sprite, 2, 0, lines_per_page - 1, 17, 1))
   {
      SetFgColor(0x21);
      Box(2, 0, 306, 132, 1, 1);
   }
   if (item_count <= 0)
   {
      PAL_LavaDrawShadowText(15, 12, "暂无可用物品", 0x1C);
   }
   else
   {
      start = (current_index / item_per_line) * item_per_line - item_per_line * 3;
      if (start < 0) start = 0;
      if (start > item_count - item_per_line * lines_per_page)
      {
         start = item_count - item_per_line * lines_per_page;
      }
      if (start < 0) start = 0;
      cursor_x = 0;
      cursor_y = 0;
      for (i = 0; i < item_per_line * lines_per_page && start + i < item_count; i++)
      {
          int color;
          char amount_buf[16];

          line = i / item_per_line;
          col = i % item_per_line;
          item_x = 15 + col * text_width;
          item_y = 12 + line * 18;
          color = (start + i == current_index) ? MENUITEM_COLOR_SELECTED : MENUITEM_COLOR;
          PAL_LavaFormatObjectLabel(label_buf, "物品", item_ids[start + i]);
          PAL_LavaDrawShadowText(item_x, item_y, label_buf, color);
          if (item_amounts[start + i] > 1)
          {
             sprintf(amount_buf, "%d", item_amounts[start + i]);
             PAL_LavaDrawShadowText(item_x + 66 - (int)strlen(amount_buf) * 8, item_y + 5, amount_buf, 0x2F);
          }
          if (start + i == current_index)
          {
             cursor_x = item_x + 25;
             cursor_y = item_y + 10;
          }
      }
      if (sprite != 0)
      {
         PAL_RLEBlitToSurface((LPCBITMAPRLE)PAL_SpriteGetFrame(sprite, 69), gpScreen,
            PAL_XY(cursor_x, cursor_y));
         PAL_RLEBlitToSurfaceWithShadow((LPCBITMAPRLE)PAL_SpriteGetFrame(sprite, 70), gpScreen,
            PAL_XY(5, 145), TRUE);
         PAL_RLEBlitToSurface((LPCBITMAPRLE)PAL_SpriteGetFrame(sprite, 70), gpScreen,
            PAL_XY(0, 140));
      }
      bitmap_num = PAL_LavaReadObjectField(item_ids[current_index], 0);
      if (bitmap_num > 0)
      {
         FILE *fp_ball;
         fp_ball = UTIL_OpenFile("BALL.MKF");
         bitmap_read = fp_ball != 0 ?
            PAL_MKFReadChunk((addr)g_lava_mkf_buf, 65536, bitmap_num, fp_ball) : -1;
         if (fp_ball != 0)
         {
            fclose(fp_ball);
         }
         if (bitmap_read > 0)
         {
            PAL_RLEBlitToSurface((LPCBITMAPRLE)g_lava_mkf_buf, gpScreen, PAL_XY(8, 147));
         }
      }
      desc = PAL_LavaReadObjectDesc(item_ids[current_index]);
      if (desc != 0 && desc[0] != 0)
      {
         PAL_LavaDrawShadowTextSmall(75, 150, desc, 0x3C);
      }
   }
   PAL_LavaPresent();
 }

static void PAL_LavaDrawEquipMenuContent(int current_index)
{
   int item_ids[256];
   int item_amounts[256];
   int item_count;
   int start;
   int i;
   int item_flags;
   int color;
   int amount_x;
   char label_buf[24];
   char amount_buf[16];

   item_count = 0;
   for (i = 0; i < 256; i++)
   {
      if (g_lava_inventory_item[i] != 0 && g_lava_inventory_amount[i] > 0)
      {
         item_flags = PAL_LavaReadObjectField(g_lava_inventory_item[i], gConfig.fIsWIN95 ? 6 : 5);
         if (item_flags & 0x0002)
         {
            item_ids[item_count] = g_lava_inventory_item[i];
            item_amounts[item_count] = g_lava_inventory_amount[i];
            item_count++;
         }
      }
   }

   PAL_LavaDrawItemListFrame("装备");
   if (item_count <= 0)
   {
      PAL_LavaDrawShadowText(32, 76, "暂无可装备物品", 0x1C);
   }
   else
   {
      start = current_index - 3;
      if (start < 0) start = 0;
      if (start > item_count - 7) start = item_count - 7;
      if (start < 0) start = 0;
      for (i = 0; i < 7 && start + i < item_count; i++)
      {
         color = (start + i == current_index) ? MENUITEM_COLOR_SELECTED : MENUITEM_COLOR;
         PAL_LavaFormatObjectLabel(label_buf, "装备", item_ids[start + i]);
         PAL_LavaDrawShadowText(32, 64 + i * 16, label_buf, color);
         sprintf(amount_buf, "%d", item_amounts[start + i]);
         amount_x = 220 - (int)strlen(amount_buf) * 8;
         PAL_LavaDrawShadowText(amount_x, 64 + i * 16, amount_buf, color);
      }
   }
    PAL_LavaDrawShadowText(32, 182, "上下浏览 选择角色装备 B返回", 0x1C);

}

static void PAL_LavaShowStatusMenu(void)
{
   int current_party;
   int role;
   char *label;
   addr sprite;
   addr avatar_data;
   long avatar_read;
   long avatar_dec;
   int avatar_sprite;
   int i;
   int rgwHP;
   int rgwMaxHP;
    int rgwMP;
    int rgwMaxMP;
    int name_x;
    int name_y;
    int equip_item;
    FILE *fp_fbp;
    FILE *fp_rgm;

    current_party = 0;
   PAL_ClearKeyState();
   while (TRUE)
   {
      if (current_party < 0)
      {
         current_party = g_lava_party_count - 1;
      }
      if (current_party >= g_lava_party_count)
      {
         current_party = 0;
      }

      role = g_lava_party_role[current_party];
       fp_fbp = UTIL_OpenFile("FBP.MKF");
       if (fp_fbp != 0 &&
          PAL_LavaDecompressOK(PAL_MKFDecompressChunk((addr)g_lava_fbp_buf, 64000, 0, fp_fbp), 64000))
       {
          PAL_FBPBlitToSurface((addr)g_lava_fbp_buf, gpScreen);
       }
       else if (fp_fbp != 0 &&
          PAL_LavaDecompressOK(PAL_MKFDecompressChunk((addr)g_lava_fbp_buf, 64000, MAINMENU_BACKGROUND_FBPNUM, fp_fbp), 64000))
       {
          PAL_FBPBlitToSurface((addr)g_lava_fbp_buf, gpScreen);
       }
        else
        {
           PAL_LavaDrawSceneFrame();
        }
        if (fp_fbp != 0) fclose(fp_fbp);
        lava_begin_text_batch();

       avatar_sprite = PAL_LavaRoleWordByArray(0, role);
       if (avatar_sprite > 0)
       {
          fp_rgm = UTIL_OpenFile("RGM.MKF");
          avatar_read = fp_rgm != 0 ?
             PAL_MKFReadChunk((addr)g_lava_mkf_buf, 65536, avatar_sprite, fp_rgm) : -1;
          if (fp_rgm != 0) fclose(fp_rgm);
          if (avatar_read > 0)
          {
             PAL_RLEBlitToSurface((LPCBITMAPRLE)g_lava_mkf_buf, gpScreen, PAL_XY(110, 30));
         }
      }

      sprite = PAL_LavaLoadUISprite();
      if (sprite != 0)
      {
         PAL_RLEBlitToSurface((LPCBITMAPRLE)PAL_SpriteGetFrame(sprite, 39), gpScreen, PAL_XY(65, 58));
         PAL_RLEBlitToSurface((LPCBITMAPRLE)PAL_SpriteGetFrame(sprite, 39), gpScreen, PAL_XY(65, 80));
      }

      {
          int equip_bitmap;
          long equip_read;
          int box_x;
          int box_y;

          for (i = 0; i < 6; i++)
         {
            if (i == 0) { box_x = 189; box_y = 0; name_x = 195; name_y = 38; }
            else if (i == 1) { box_x = 247; box_y = 39; name_x = 253; name_y = 78; }
            else if (i == 2) { box_x = 251; box_y = 101; name_x = 257; name_y = 140; }
            else if (i == 3) { box_x = 201; box_y = 133; name_x = 207; name_y = 172; }
            else if (i == 4) { box_x = 141; box_y = 141; name_x = 147; name_y = 180; }
            else { box_x = 81; box_y = 125; name_x = 87; name_y = 164; }

            equip_item = PAL_LavaRoleWordByArray(11 + i, role);
            if (equip_item <= 0) continue;

            equip_bitmap = PAL_LavaReadObjectField(equip_item, 0);
            if (g_lava_autotest_status)
            {
               printf("[LAVA][AUTOTEST_STATUS] equip slot=%d item=%d bitmap=%d\n", i, equip_item, equip_bitmap);
            }
            if (equip_bitmap > 0)
            {
               FILE *fp_ball;
               fp_ball = UTIL_OpenFile("BALL.MKF");
               equip_read = fp_ball != 0 ?
                  PAL_MKFReadChunk((addr)g_lava_mkf_buf, 65536, equip_bitmap, fp_ball) : -1;
               if (fp_ball != 0) fclose(fp_ball);
               if (equip_read > 0)
               {
                  PAL_RLEBlitToSurface((LPCBITMAPRLE)g_lava_mkf_buf,
                     gpScreen, PAL_XY(box_x + 1, box_y + 1));
               }
            }
         }
      }

      PAL_LavaDrawShadowText(110, 8, PAL_LavaRoleName(role), MENUITEM_COLOR_SELECTED);
      PAL_LavaDrawNumberText(62, 1, 0, 0xCF);
      PAL_LavaDrawNumberText(62, 13, 15, 0x2F);

      {
         for (i = 0; i < 6; i++)
         {
            if (i == 0) { name_x = 195; name_y = 38; }
            else if (i == 1) { name_x = 253; name_y = 78; }
            else if (i == 2) { name_x = 257; name_y = 140; }
            else if (i == 3) { name_x = 207; name_y = 172; }
            else if (i == 4) { name_x = 147; name_y = 176; }
            else { name_x = 87; name_y = 164; }

            equip_item = PAL_LavaRoleWordByArray(11 + i, role);
            if (equip_item <= 0) continue;

            label = PAL_LavaReadWord(equip_item);
            if (label != 0)
            {
               PAL_LavaDrawShadowText(name_x, name_y, label, MENUITEM_COLOR);
            }
         }
      }

      PAL_LavaDrawShadowText(6, 6, "经验值", MENUITEM_COLOR);
      PAL_LavaDrawShadowText(6, 29, "修行", MENUITEM_COLOR);
      PAL_LavaDrawShadowText(6, 51, "体力", MENUITEM_COLOR);
      PAL_LavaDrawShadowText(6, 73, "真气", MENUITEM_COLOR);
      PAL_LavaDrawShadowText(6, 95, "武术", MENUITEM_COLOR);
      PAL_LavaDrawShadowText(6, 115, "灵力", MENUITEM_COLOR);
      PAL_LavaDrawShadowText(6, 135, "防御", MENUITEM_COLOR);
      PAL_LavaDrawShadowText(6, 155, "身法", MENUITEM_COLOR);
      PAL_LavaDrawShadowText(6, 175, "吉运", MENUITEM_COLOR);

      PAL_LavaDrawNumberText(54, 32, PAL_LavaRoleWordByArray(6, role), 0xCF);

      rgwHP = PAL_LavaRoleWordByArray(9, role);
      rgwMaxHP = PAL_LavaRoleWordByArray(7, role);
      rgwMP = PAL_LavaRoleWordByArray(10, role);
      rgwMaxMP = PAL_LavaRoleWordByArray(8, role);

      PAL_LavaDrawNumberText(42, 53, rgwHP, 0xCF);
      PAL_LavaDrawNumberText(63, 58, rgwMaxHP, 0x2F);
      PAL_LavaDrawNumberText(42, 75, rgwMP, 0xCF);
      PAL_LavaDrawNumberText(63, 80, rgwMaxMP, 0x2F);

      PAL_LavaDrawNumberText(42, 99, PAL_LavaRoleWordByArray(17, role), 0xCF);
      PAL_LavaDrawNumberText(42, 119, PAL_LavaRoleWordByArray(18, role), 0xCF);
      PAL_LavaDrawNumberText(42, 139, PAL_LavaRoleWordByArray(19, role), 0xCF);
      PAL_LavaDrawNumberText(42, 159, PAL_LavaRoleWordByArray(20, role), 0xCF);
      PAL_LavaDrawNumberText(42, 179, PAL_LavaRoleWordByArray(21, role), 0xCF);

      lava_end_text_batch();
      if (g_lava_autotest_status)
      {
         printf("[LAVA][AUTOTEST_STATUS] rendered role=%d party=%d\n", role, current_party);
      }

      while (TRUE)
      {
         PAL_ProcessEvent();
         if (PAL_LavaReadCancelKey())
         {
            PAL_ClearKeyState();
            return;
         }
         if (g_InputState.dwKeyPress & (kKeyLeft | kKeyUp))
         {
            current_party--;
            PAL_ClearKeyState();
            break;
         }
         if (g_InputState.dwKeyPress & (kKeyRight | kKeyDown | kKeySearch))
         {
            current_party++;
            PAL_ClearKeyState();
            break;
         }
         Delay(50);
      }
   }
}

static void PAL_LavaShowMagicMenu(void)
{
   int current_party;
   int current_index;
   int role;
   int magic_ids[32];
   int magic_count;
   int i;
   int selected;
   int y;
   char *name;

   current_party = 0;
   current_index = 0;
   PAL_ClearKeyState();

   if (g_lava_party_count > 1)
   {
      lava_begin_text_batch();
      PAL_LavaDrawSceneFrame();

      y = 45;
      for (i = 0; i < g_lava_party_count; i++)
      {
         role = g_lava_party_role[i];
         name = PAL_LavaRoleName(role);
         PAL_LavaDrawShadowText(y, 170, name, MENUITEM_COLOR);
          y += 78;
       }


       if (!PAL_LavaDrawSpriteBoxAt(PAL_LavaLoadUISprite(), 35, 62, g_lava_party_count - 1, 1, 0))
      {
         SetFgColor(0x21);
         Box(35, 62, 95, 62 + g_lava_party_count * 18, 1, 1);
      }

      selected = current_party;
      while (TRUE)
      {
         int color;
         for (i = 0; i < g_lava_party_count; i++)
         {
            color = (i == selected) ? MENUITEM_COLOR_SELECTED : MENUITEM_COLOR;
            if (PAL_LavaRoleWordByArray(9, g_lava_party_role[i]) <= 0)
            {
               color = MENUITEM_COLOR_INACTIVE;
            }
            name = PAL_LavaRoleName(g_lava_party_role[i]);
             PAL_LavaDrawShadowText(48, 75 + i * 18, name, color);
          }

          lava_end_text_batch();
          PAL_ProcessEvent();
         if (PAL_LavaReadCancelKey())
         {
            PAL_ClearKeyState();
            PAL_LavaDrawSceneFrame();
            return;
         }
          if (g_InputState.dwKeyPress & kKeyUp)
          {
             if (selected > 0) selected--;
             PAL_ClearKeyState();
             lava_begin_text_batch();
             continue;
          }
          if (g_InputState.dwKeyPress & kKeyDown)
          {
             if (selected + 1 < g_lava_party_count) selected++;
             PAL_ClearKeyState();
             lava_begin_text_batch();
             continue;
          }
         if (PAL_LavaReadConfirmKey())
         {
            if (PAL_LavaRoleWordByArray(9, g_lava_party_role[selected]) > 0)
            {
             current_party = selected;
             PAL_ClearKeyState();
             break;
            }
            PAL_ClearKeyState();
         }
         Delay(50);
         lava_begin_text_batch();
      }
   }

   current_index = 0;
   PAL_ClearKeyState();
   while (TRUE)
   {
      if (current_party < 0)
      {
         current_party = g_lava_party_count - 1;
      }
      if (current_party >= g_lava_party_count)
      {
         current_party = 0;
      }

      role = g_lava_party_role[current_party];
      magic_count = 0;
      for (i = 0; i < 32; i++)
      {
         int mid;
         mid = PAL_LavaRoleWordByArray(32 + i, role);
         if (mid != 0)
         {
            magic_ids[magic_count++] = mid;
         }
      }
      if (current_index >= magic_count)
      {
         current_index = magic_count - 1;
      }
      if (current_index < 0)
      {
         current_index = 0;
      }

      lava_begin_text_batch();
      PAL_LavaDrawSceneFrame();

   g_lava_magic_menu_party = current_party;
   g_lava_magic_menu_index = current_index;
   PAL_LavaDrawMagicMenuContent(current_party, current_index);
   lava_end_text_batch();

   while (TRUE)
      {
         PAL_ProcessEvent();
         if (PAL_LavaReadCancelKey())
         {
            PAL_ClearKeyState();
            return;
         }
         if (g_InputState.dwKeyPress & kKeyUp)
         {
            if (current_index >= 4) current_index -= 4;
            PAL_ClearKeyState();
            break;
         }
         if (g_InputState.dwKeyPress & kKeyDown)
         {
            if (current_index + 4 < magic_count) current_index += 4;
            PAL_ClearKeyState();
            break;
         }
         if (g_InputState.dwKeyPress & kKeyLeft)
         {
            if (current_index > 0) current_index--;
            PAL_ClearKeyState();
            break;
         }
         if (g_InputState.dwKeyPress & kKeyRight)
         {
            if (current_index + 1 < magic_count) current_index++;
            PAL_ClearKeyState();
            break;
         }
         if (PAL_LavaReadConfirmKey() && magic_count > 0)
         {
            int magic_id;
            int magic_flags;
            int magic_num;
            int mp_cost;
            int target_role;

            magic_id = magic_ids[current_index];
            magic_flags = PAL_LavaReadObjectField(magic_id, gConfig.fIsWIN95 ? 6 : 5);
            magic_num = PAL_LavaReadObjectField(magic_id, 0);
            mp_cost = PAL_LavaReadMagicField(magic_num, 12);
            PAL_ClearKeyState();
            if (mp_cost <= PAL_LavaRoleWordByArray(10, role))
            {
               target_role = 0xFFFF;
               if ((magic_flags & 0x0010) == 0)
               {
                  target_role = PAL_LavaChooseTargetRole("仙术", magic_id, 1);
               }
               if (target_role >= 0)
               {
                  PAL_LavaRunRoleTriggerScript((long)PAL_LavaReadObjectField(magic_id, 3), target_role);
                  PAL_LavaFinishScriptStep();
                  PAL_LavaWriteU16((addr)g_lava_data_buf, 10 * 6 * 2 + role * 2,
                     PAL_LavaRoleWordByArray(10, role) - mp_cost);
               }
            }
            break;
         }
         Delay(50);
      }
   }
}

static void PAL_LavaShowItemUseMenu(void)
{
   int current_index;

   current_index = 0;
   PAL_ClearKeyState();
   while (TRUE)
   {
      int item_ids[256];
      int item_amounts[256];
      int item_count;

      item_count = PAL_LavaBuildUsableItemList(item_ids, item_amounts);
      if (current_index >= item_count)
      {
         current_index = item_count - 1;
      }
      if (current_index < 0)
      {
         current_index = 0;
      }

      g_lava_item_use_index = current_index;
      lava_begin_text_batch();
      PAL_LavaDrawSceneFrame();
      PAL_LavaDrawItemUseMenuContent(current_index);
      lava_end_text_batch();

      while (TRUE)
      {
         PAL_ProcessEvent();
         if (PAL_LavaReadCancelKey())
         {
            PAL_ClearKeyState();
            return;
         }
          if (g_InputState.dwKeyPress & kKeyUp)
          {
             if (current_index >= 3) current_index -= 3;
             PAL_ClearKeyState();
             break;
          }
          if (g_InputState.dwKeyPress & kKeyDown)
          {
             if (current_index + 3 < item_count) current_index += 3;
             PAL_ClearKeyState();
             break;
          }
          if (g_InputState.dwKeyPress & kKeyLeft)
          {
             if (current_index > 0) current_index--;
             PAL_ClearKeyState();
             break;
          }
          if (g_InputState.dwKeyPress & kKeyRight)
          {
             if (current_index + 1 < item_count) current_index++;
             PAL_ClearKeyState();
             break;
          }
         if (PAL_LavaReadConfirmKey() && item_count > 0)
         {
             int item_id;
             long script_on_use;
             int item_flags;
             int target_role;
             int use_done;

              item_id = item_ids[current_index];
              script_on_use = PAL_LavaReadObjectField(item_id, 2);
              item_flags = PAL_LavaReadObjectField(item_id, gConfig.fIsWIN95 ? 6 : 5);
              PAL_ClearKeyState();
              printf("[LAVA][ITEMUSE] item_id=%d script_on_use=%ld item_flags=0x%04X\n",
                  item_id, script_on_use, item_flags);
               if ((item_flags & 0x0001) && script_on_use != 0)
               {
                  if ((item_flags & 0x0010) == 0)
                  {
                     use_done = 0;
                     while (!use_done && PAL_LavaGetItemAmount(item_id) > 0)
                     {
                        int hp_before;
                        int mp_before;
                        int hp_after;
                        int mp_after;

                        target_role = PAL_LavaChooseTargetRole("物品", item_id, 0);
                        printf("[LAVA][ITEMUSE] target_role=%d\n", target_role);
                        if (target_role < 0)
                        {
                           use_done = 1;
                        }
                        else
                        {
                           hp_before = PAL_LavaRoleWordByArray(9, target_role);
                           mp_before = PAL_LavaRoleWordByArray(10, target_role);
                           printf("[LAVA][ITEMUSE] before hp=%d mp=%d role=%d\n",
                              hp_before, mp_before, target_role);
                            script_on_use = PAL_LavaRunRoleTriggerScript((long)script_on_use, target_role);
                            PAL_LavaWriteObjectField(item_id, 2, script_on_use);
                           PAL_LavaFinishScriptStep();
                           hp_after = PAL_LavaRoleWordByArray(9, target_role);
                           mp_after = PAL_LavaRoleWordByArray(10, target_role);
                           printf("[LAVA][ITEMUSE] after hp=%d mp=%d success=%d\n",
                              hp_after, mp_after, g_lava_script_success);
                           if ((item_flags & 0x0008) && g_lava_script_success)
                           {
                              PAL_LavaAddItemToInventory(item_id, -1);
                              printf("[LAVA][ITEMUSE] consumed item=%d remaining=%d\n",
                                 item_id, PAL_LavaGetItemAmount(item_id));
                           }
                           g_lava_item_was_used = 1;
                           use_done = 1;
                        }
                     }
                  }
                  else
                  {
                     printf("[LAVA][ITEMUSE] apply-to-all item=%d\n", item_id);
                     script_on_use = PAL_LavaRunTriggerScript((long)script_on_use, 0xFFFF);
                     PAL_LavaWriteObjectField(item_id, 2, script_on_use);
                     PAL_LavaFinishScriptStep();
                      printf("[LAVA][ITEMUSE] apply-to-all done success=%d\n",
                         g_lava_script_success);
                      if ((item_flags & 0x0008) && g_lava_script_success)
                      {
                         PAL_LavaAddItemToInventory(item_id, -1);
                      }
                      if (g_lava_script_success && g_lava_last_event_object > 0)
                      {
                         PAL_LavaSearchScene();
                      }
                      g_lava_item_was_used = 1;
                      return;
                   }
               }
              else
              {
                  printf("[LAVA][ITEMUSE] skipped: usable=%d script=%ld\n",
                     item_flags & 0x0001, script_on_use);
              }
             break;
         }
         Delay(50);
      }
   }
}

static void PAL_LavaShowEquipMenu(void)
{
   int current_index;
   int item_id;
    long script_on_equip;
   int item_flags;
   int target_role;
   int item_ids[256];
   int item_amounts[256];
   int item_count;
   int start;
   int i;
   int color;
   int amount_x;
   char label_buf[24];
   char amount_buf[16];

   current_index = 0;
   PAL_ClearKeyState();
   while (TRUE)
   {
      item_count = 0;
      for (i = 0; i < 256; i++)
      {
         if (g_lava_inventory_item[i] != 0 && g_lava_inventory_amount[i] > 0)
         {
            item_flags = PAL_LavaReadObjectField(g_lava_inventory_item[i], gConfig.fIsWIN95 ? 6 : 5);
            if (item_flags & 0x0002)
            {
               item_ids[item_count] = g_lava_inventory_item[i];
               item_amounts[item_count] = g_lava_inventory_amount[i];
               item_count++;
            }
         }
      }
      if (current_index >= item_count)
      {
         current_index = item_count - 1;
      }
      if (current_index < 0)
      {
         current_index = 0;
      }

       g_lava_equip_index = current_index;
       lava_begin_text_batch();
       PAL_LavaDrawSceneFrame();
       PAL_LavaDrawEquipMenuContent(current_index);
       lava_end_text_batch();

      while (TRUE)
      {
         PAL_ProcessEvent();
         if (PAL_LavaReadCancelKey())
         {
            PAL_ClearKeyState();
            return;
         }
         if (g_InputState.dwKeyPress & kKeyUp)
         {
            if (current_index > 0) current_index--;
            PAL_ClearKeyState();
            break;
         }
         if (g_InputState.dwKeyPress & (kKeyDown | kKeySearch))
         {
            if (current_index + 1 < item_count) current_index++;
            PAL_ClearKeyState();
            break;
          }
          if (PAL_LavaReadConfirmKey())
          {
            PAL_ClearKeyState();
            if (item_count > 0)
            {
               item_id = item_ids[current_index];
               item_flags = PAL_LavaReadObjectField(item_id, gConfig.fIsWIN95 ? 6 : 5);
               script_on_equip = PAL_LavaReadObjectField(item_id, 3);
               target_role = PAL_LavaChooseTargetRole("装备", item_id, 2);
               if (target_role >= 0 && (item_flags & 0x0002) && script_on_equip != 0)
               {
                   PAL_LavaRunRoleTriggerScript((long)script_on_equip, target_role);
                  PAL_LavaFinishScriptStep();
               }
            }
            break;
          }
         Delay(50);
      }
   }
}

static void PAL_LavaShowInventoryMenu(void)
{
   int selected;

   g_lava_item_was_used = 0;
   selected = PAL_LavaChooseTwoItemMenu(0, "装备", "使用", 1, 0);
   if (selected < 0)
   {
      return;
   }
   if (selected == 0)
   {
      PAL_LavaShowEquipMenu();
   }
   else
   {
      PAL_LavaShowItemUseMenu();
   }
}

static int PAL_LavaInGameMenu(void)
{
   int ignore_cancel;

   g_lava_menu_selection = 0;
   ignore_cancel = 1;
   PAL_ClearKeyState();
   while (TRUE)
   {
      PAL_LavaDrawMainMenu();
      while (TRUE)
      {
         int old_selection;

         PAL_ProcessEvent();
         if (ignore_cancel)
         {
            ignore_cancel = 0;
            PAL_ClearKeyState();
            Delay(50);
            continue;
         }
         if (g_InputState.dwKeyPress & kKeyUp)
         {
            old_selection = g_lava_menu_selection;
            if (g_lava_menu_selection > 0)
            {
               g_lava_menu_selection--;
            }
            PAL_ClearKeyState();
            if (old_selection != g_lava_menu_selection)
            {
               PAL_LavaDrawMainMenuFrame(g_lava_menu_selection);
            }
            continue;
         }
         if (g_InputState.dwKeyPress & kKeyDown)
         {
            old_selection = g_lava_menu_selection;
            if (g_lava_menu_selection < 3)
            {
               g_lava_menu_selection++;
            }
            PAL_ClearKeyState();
            if (old_selection != g_lava_menu_selection)
            {
               PAL_LavaDrawMainMenuFrame(g_lava_menu_selection);
            }
            continue;
         }
         if (PAL_LavaReadCancelKey())
         {
            PAL_ClearKeyState();
            PAL_LavaDrawSceneFrame();
            return 0;
         }
         if (PAL_LavaReadConfirmKey())
         {
            int selected;

            selected = g_lava_menu_selection;
            PAL_ClearKeyState();
            if (selected == 0)
            {
               PAL_LavaShowStatusMenu();
            }
            else if (selected == 1)
            {
               PAL_LavaShowMagicMenu();
            }
            else if (selected == 2)
             {
                PAL_LavaShowInventoryMenu();
                if (g_lava_item_was_used)
                {
                   PAL_ClearKeyState();
                   PAL_LavaDrawSceneFrame();
                   return 0;
                }
             }
            else
            {
               if (PAL_LavaSystemMenu())
               {
                  return 1;
               }
            }
            PAL_ClearKeyState();
            break;
         }
         Delay(50);
      }
   }
}

static char *PAL_LavaSystemMenuLabel(int index)
{
   if (index == 0) return "存档";
   if (index == 1) return "读档";
   if (index == 2) return "音乐";
   if (index == 3) return "音效";
   return "离开";
}

static void PAL_LavaDrawSystemMenuFrameOnly(void)
{
     int i;
     lava_begin_text_batch();
     if (!PAL_LavaDrawSpriteBoxAt(PAL_LavaLoadUISprite(), 50, 60, 4, 2, 0))
    {
       SetFgColor(0x21);
       Box(50, 60, 116, 156, 1, 1);
    }
     PAL_LavaCacheCurrentSurface();
      for (i = 0; i < 5; i++)
     {
        PAL_LavaDrawSystemMenuItem(i);
     }
      lava_end_text_batch();
}

static void PAL_LavaDrawSystemMenuLabels(void)
{
     int i;
     lava_begin_text_batch();
     memcpy((addr)g_screen_surface.pixels, (addr)g_back_buf, 320 * 200);
     WriteBlock(0, 0, 320, 200, 1, (addr)g_back_buf);
     for (i = 0; i < 5; i++)
    {
        PAL_LavaDrawSystemMenuItem(i);
    }
     lava_end_text_batch();
}

static void PAL_LavaDrawSystemMenuItem(int index)
{
   int color;
   char *label;

   color = (g_lava_menu_selection == index) ? MENUITEM_COLOR_SELECTED : MENUITEM_COLOR;
   label = PAL_LavaSystemMenuLabel(index);
        PAL_LavaDrawShadowText(63, 72 + index * 18, label, color);

}

static void PAL_LavaDrawTwoChoiceTexts(char *left, char *right, int current)
{
    PAL_LavaDrawShadowText(145, 110, left, current == 0 ? MENUITEM_COLOR_SELECTED : MENUITEM_COLOR);
    PAL_LavaDrawShadowText(220, 110, right, current == 1 ? MENUITEM_COLOR_SELECTED : MENUITEM_COLOR);
}

static int PAL_LavaTwoChoiceMenu(char *left, char *right, int selected)
{
   int current;
   int old_current;
   int dirty;

   current = selected;
   PAL_ClearKeyState();
   dirty = 1;
   while (TRUE)
   {
        if (dirty)
        {
            lava_begin_text_batch();
            PAL_LavaDrawSingleLineBox(130, 100, 3);
            PAL_LavaDrawSingleLineBox(205, 100, 3);
            PAL_LavaDrawTwoChoiceTexts(left, right, current);
            lava_end_text_batch();
           dirty = 0;
        }

      PAL_ProcessEvent();
      if (g_InputState.dwKeyPress & (kKeyLeft | kKeyUp))
      {
         old_current = current;
         current = 0;
         PAL_ClearKeyState();
         if (old_current != current)
         {
            dirty = 1;
         }
         continue;
      }
      if (g_InputState.dwKeyPress & (kKeyRight | kKeyDown))
      {
         old_current = current;
         current = 1;
         PAL_ClearKeyState();
         if (old_current != current)
         {
            dirty = 1;
         }
         continue;
      }
      if (PAL_LavaReadCancelKey())
      {
         PAL_ClearKeyState();
         return -1;
      }
      if (PAL_LavaReadConfirmKey())
      {
         PAL_ClearKeyState();
         return current;
      }
      Delay(50);
   }
}

static void PAL_LavaDrawSystemMenu(void)
{
   PAL_LavaDrawSystemMenuFrameOnly();
}

static void PAL_LavaRedrawSystemMenuFromScene(void)
{
   PAL_LavaDrawMainMenu();
   PAL_LavaDrawSystemMenu();
}

static int PAL_LavaSystemMenu(void)
{
   int running;
   int load_slot;
   int ignore_cancel;
   int choice;
   int dirty;
   int old_selection;

   g_lava_menu_selection = 0;
   printf("[LAVA][SYSMENU] open\n");
   PAL_ClearKeyState();
   running = 1;
   ignore_cancel = 1;
   dirty = 1;
   while (running)
   {
      if (dirty)
      {
         PAL_LavaDrawSystemMenu();
         dirty = 0;
      }
      PAL_ProcessEvent();
      if (ignore_cancel)
      {
         ignore_cancel = 0;
         PAL_ClearKeyState();
         Delay(50);
         continue;
      }
      if (g_InputState.dwKeyPress & kKeyUp)
      {
         old_selection = g_lava_menu_selection;
         if (g_lava_menu_selection > 0)
         {
            g_lava_menu_selection--;
         }
         PAL_ClearKeyState();
          if (old_selection != g_lava_menu_selection)
          {
             PAL_LavaDrawSystemMenuLabels();
          }
          continue;
       }
       if (g_InputState.dwKeyPress & kKeyDown)
       {
          old_selection = g_lava_menu_selection;
          if (g_lava_menu_selection < 4)
          {
             g_lava_menu_selection++;
          }
          PAL_ClearKeyState();
          if (old_selection != g_lava_menu_selection)
          {
             PAL_LavaDrawSystemMenuLabels();
          }
         continue;
      }
      if (PAL_LavaReadCancelKey())
      {
         printf("[LAVA][SYSMENU] close cancel\n");
         PAL_ClearKeyState();
         PAL_LavaDrawSceneFrame();
         return 0;
      }
      if (PAL_LavaReadConfirmKey())
      {
         int selected;

         selected = g_lava_menu_selection;
         PAL_ClearKeyState();
         if (selected == 0)
         {
            load_slot = PAL_LavaChooseSaveSlot();
            if (load_slot > 0)
            {
               if (PAL_LavaSaveGame(load_slot) == 0)
               {
                  printf("[LAVA][SYSMENU] save slot=%d\n", load_slot);
               }
               else
               {
                  printf("[LAVA][SYSMENU] save failed slot=%d\n", load_slot);
               }
            }
            PAL_LavaRedrawSystemMenuFromScene();
            dirty = 0;
            continue;
         }
         if (selected == 1)
         {
            if (!PAL_LavaAnySaveExists())
            {
               printf("[LAVA][SYSMENU] load requested but no save exists\n");
               dirty = 1;
               continue;
            }
            load_slot = PAL_LavaChooseLoadSlot(1);
            if (load_slot > 0)
            {
               printf("[LAVA][SYSMENU] load slot=%d\n", load_slot);
               PAL_FadeOut(1);
               PAL_ReloadInNextTick(load_slot);
               return 1;
            }
            PAL_LavaRedrawSystemMenuFromScene();
            dirty = 0;
            continue;
         }
          if (selected == 2)
          {
             choice = PAL_LavaTwoChoiceMenu("关", "开", AUDIO_MusicEnabled() ? 1 : 0);
             if (choice >= 0)
             {
                AUDIO_EnableMusic(choice == 1 ? TRUE : FALSE);
                printf("[LAVA][SYSMENU] music=%d\n", AUDIO_MusicEnabled());
             }
             PAL_LavaDrawSceneFrame();
             return 0;
          }
          if (selected == 3)
          {
             choice = PAL_LavaTwoChoiceMenu("关", "开", AUDIO_SoundEnabled() ? 1 : 0);
             if (choice >= 0)
             {
                AUDIO_EnableSound(choice == 1 ? TRUE : FALSE);
                printf("[LAVA][SYSMENU] sound=%d\n", AUDIO_SoundEnabled());
             }
             PAL_LavaDrawSceneFrame();
             return 0;
          }
         if (selected == 4)
         {
            choice = PAL_LavaTwoChoiceMenu("否", "是", 0);
            if (choice == 1)
            {
               printf("[LAVA][SYSMENU] quit\n");
               PAL_FadeOut(1);
               PAL_Shutdown(0);
               return 1;
            }
            dirty = 1;
            continue;
         }
      }
      Delay(50);
   }

   PAL_LavaDrawSceneFrame();
   return 0;
}

static void PAL_LavaDrawOpeningMenuBackground(void)
{
   FILE *fp_fbp;

   fp_fbp = UTIL_OpenFile("FBP.MKF");
   if (fp_fbp != 0 &&
       PAL_LavaDecompressOK(PAL_MKFDecompressChunk((addr)g_lava_fbp_buf, 64000, MAINMENU_BACKGROUND_FBPNUM, fp_fbp), 64000))
   {
      PAL_FBPBlitToSurface((addr)g_lava_fbp_buf, gpScreen);
   }
   if (fp_fbp != 0)
   {
      fclose(fp_fbp);
   }
}

static void PAL_LavaDrawOpeningMenu(int selected)
{
   PAL_LavaDrawOpeningMenuBackground();
   PAL_LavaCacheCurrentSurface();
   PAL_LavaDrawOpeningMenuLabels(selected);
   Refresh();
}

int PAL_OpeningMenu(void)
{
    int selection = 0;
    int load_slot;

       if (g_lava_autotest_search || g_lava_autotest_walk || g_lava_autotest_exits || g_lava_autotest_scene5 || g_lava_autotest_scene6 || g_lava_autotest_scene13 || g_lava_autotest_scene9 || g_lava_autotest_hooks || g_lava_autotest_door || g_lava_autotest_hall || g_lava_autotest_kitchen || g_lava_autotest_load || g_lava_autotest_menu_load || g_lava_autotest_status || g_lava_autotest_input || g_lava_autotest_intro || g_lava_autotest_xianling || g_lava_autotest_obj204 || g_lava_autotest_battle || g_lava_autotest_op48)
   {
      PAL_SetPalette(0, FALSE);
      PAL_LavaDrawOpeningMenuBackground();
      PAL_FadeIn(1);
      PAL_LavaDrawOpeningMenu(selection);
      UTIL_Delay(120);
      if (g_lava_autotest_menu_load)
      {
         printf("[LAVA][MENU] load slot=5\n");
         return 5;
      }
      return 0;
   }

   PAL_SetPalette(0, FALSE);
   PAL_LavaDrawOpeningMenuBackground();
   PAL_FadeIn(1);
   PAL_LavaDrawOpeningMenu(selection);
   while (TRUE)
   {
      PAL_ProcessEvent();
      if (g_InputState.dwKeyPress & kKeyDown)
      {
         selection = 1;
         PAL_LavaRedrawOpeningMenuSelection(selection);
         PAL_ClearKeyState();
      }
      else if (g_InputState.dwKeyPress & kKeyUp)
      {
         selection = 0;
         PAL_LavaRedrawOpeningMenuSelection(selection);
         PAL_ClearKeyState();
      }
      else if (PAL_LavaReadConfirmKey())
      {
          if (selection == 0)
          {
             printf("[LAVA][MENU] new game selected\n");
             PAL_ClearKeyState();
             break;
          }

          if (!PAL_LavaAnySaveExists())
          {
             printf("[LAVA][MENU] load requested but no save exists\n");
             selection = 0;
             PAL_LavaRedrawOpeningMenuSelection(selection);
             PAL_ClearKeyState();
             continue;
          }

           load_slot = PAL_LavaChooseLoadSlot(0);
          if (load_slot > 0)
          {
             printf("[LAVA][MENU] load slot=%d\n", load_slot);
             PAL_ClearKeyState();
             return load_slot;
          }

          PAL_LavaRedrawOpeningMenuSelection(selection);
          PAL_ClearKeyState();
      }
      Delay(50);
   }

   return 0;
}

void PAL_ReloadInNextTick(int iSaveSlot)
{
   g_lava_pending_load_slot = iSaveSlot;
}

void PAL_LoadResources(void)
{
   if (g_lava_pending_load_slot > 0)
   {
      PAL_LavaLoadSavedGame(g_lava_pending_load_slot);
      g_lava_pending_load_slot = 0;
   }
}

void PAL_StartFrame(void)
{
   int changed;
   int finishing_step;
   int run_logic;
   int move_dir;
   int dir_press;
   int dir_hold;

   if (!g_lava_scene_ready) return;

   if (g_lava_scene_enter_pending)
   {
      g_lava_walk_substep_pending = 0;
      PAL_LavaRunPendingSceneEnter();
      return;
   }

   run_logic = g_lava_run_logic_frame;

   dir_press = g_InputState.dwKeyPress & (kKeyDown | kKeyLeft | kKeyUp | kKeyRight);
   dir_hold = (g_lava_frame_hold | g_lava_key_hold) & (kKeyDown | kKeyLeft | kKeyUp | kKeyRight);
   move_dir = g_InputState.dir;

   if (move_dir == kDirUnknown)
   {
      if (dir_hold & kKeyDown) move_dir = kDirSouth;
      else if (dir_hold & kKeyLeft) move_dir = kDirWest;
      else if (dir_hold & kKeyUp) move_dir = kDirNorth;
      else if (dir_hold & kKeyRight) move_dir = kDirEast;
   }

   if (g_lava_autotest_input && (dir_press || dir_hold))
   {
      printf("[LAVA][FRAMEINPUT] hold=%d framehold=%d press=%d dir=%d pos=(%d,%d)\n",
         g_lava_key_hold & (kKeyDown | kKeyLeft | kKeyUp | kKeyRight),
         g_lava_frame_hold & (kKeyDown | kKeyLeft | kKeyUp | kKeyRight),
         dir_press, move_dir, g_lava_party_x, g_lava_party_y);
   }

   if (g_InputState.dwKeyPress & kKeySearch)
   {
      if (PAL_LavaSearchScene())
      {
         return;
      }
   }

    if (g_InputState.dwKeyPress & kKeyMenu)
    {
      if (PAL_LavaInGameMenu())
      {
         return;
      }
      PAL_ClearKeyState();
      PAL_LavaDrawSceneFrame();
      return;
   }

   finishing_step = g_lava_walk_substep_pending;
   if (finishing_step)
   {
      changed = PAL_LavaMovePartySubstep(g_lava_walk_substep_dir, 0);
      g_lava_walk_substep_pending = 0;
   }
   else if (move_dir != kDirUnknown)
   {
      changed = PAL_LavaMovePartySubstep(move_dir, 1);
      if (changed > 0)
      {
         g_lava_walk_substep_pending = 1;
         g_lava_walk_substep_dir = move_dir;
      }
   }
   else
   {
      changed = 0;
   }
   if (changed > 0)
   {
      if (g_lava_autotest_input)
      {
          printf("[LAVA][MOVE] pos=(%d,%d) view=(%d,%d) dir=%d frame=%d\n",
             g_lava_party_x, g_lava_party_y,
             g_lava_view_x, g_lava_view_y,
             g_lava_party_direction, g_lava_party_frame);
       }
   }
   else if (changed == 0)
   {
      g_lava_party_frame = 0;
   }
   else
   {
      return;
   }

    if (changed > 0 && finishing_step &&
        g_lava_logic_frame_num >= g_lava_touch_cooldown_until_frame &&
        PAL_LavaTouchScene())
    {
       return;
    }

   if (run_logic)
   {
      PAL_LavaTickSceneEventObjects();
      PAL_LavaRunSceneAutoScripts();
   }
   PAL_LavaUpdateViewport();
   PAL_LavaDrawSceneFrame();
}

#define PAL_DelayUntil(t) \
   while (SDL_GetTicks() < (t)) \
   { \
      PAL_ProcessEvent(); \
      SDL_Delay(1); \
   }

void PAL_GameMain(void)
{
   DWORD dwLogicTime;
   DWORD dwNow;
   DWORD dwTime;

   PAL_LavaInitAutotestFlags();
   g_lava_gpGlobals.bCurrentSaveSlot = PAL_OpeningMenu();
   g_lava_gpGlobals.fInMainGame = TRUE;
   PAL_ClearKeyState();

   PAL_ReloadInNextTick(g_lava_gpGlobals.bCurrentSaveSlot);
   if (g_lava_gpGlobals.bCurrentSaveSlot == 0)
   {
      PAL_LavaLoadPlayerRoles();
      PAL_LavaProbeNewGameData();
      PAL_LavaLoadScenePreview();
        if (g_lava_autotest_search || g_lava_autotest_walk || g_lava_autotest_exits || g_lava_autotest_scene5 || g_lava_autotest_scene6 || g_lava_autotest_scene13 || g_lava_autotest_scene9 || g_lava_autotest_hooks || g_lava_autotest_door || g_lava_autotest_hall || g_lava_autotest_kitchen || g_lava_autotest_load || g_lava_autotest_menu_load || g_lava_autotest_status || g_lava_autotest_input || g_lava_autotest_intro || g_lava_autotest_xianling || g_lava_autotest_obj204 || g_lava_autotest_battle || g_lava_autotest_op48)
      {
         g_lava_dialog_event_count = 0;
         PAL_LavaDrawSceneFrame();
       }
       else
      {
         PAL_LavaRunPendingDialog();
      }
      if (g_lava_autotest_status)
      {
         PAL_LavaShowStatusMenu();
      }
      else if (g_lava_autotest_battle)
      {
         PAL_LavaAutotestRunBattleSmoke();
      }
      else if (g_lava_autotest_op48)
      {
         PAL_LavaAutotestRunOp48();
      }
      else if (g_lava_autotest_search)
      {
         PAL_LavaAutotestRunSearchSequence();
       }
       else if (g_lava_autotest_load)
       {
          PAL_LavaAutotestRunLoadSlot(g_lava_autotest_load_params[0]);
       }
      else if (g_lava_autotest_exits)
      {
         PAL_LavaAutotestRunExitsSequence();
      }
      else if (g_lava_autotest_scene6)
      {
         PAL_LavaAutotestRunScene6Sequence();
      }
      else if (g_lava_autotest_scene5)
      {
         PAL_LavaAutotestRunScene5Sequence();
      }
      else if (g_lava_autotest_scene13)
      {
         PAL_LavaAutotestRunScene13Sequence();
      }
      else if (g_lava_autotest_scene9)
      {
         PAL_LavaAutotestRunScene9Sequence();
      }
      else if (g_lava_autotest_hooks)
      {
         PAL_LavaAutotestRunHooksSequence();
      }
      else if (g_lava_autotest_door)
      {
         PAL_LavaAutotestRunDoorSequence();
       }
       else if (g_lava_autotest_hall)
       {
         PAL_LavaAutotestRunHallSequence();
       }
      else if (g_lava_autotest_kitchen)
      {
         PAL_LavaAutotestRunKitchenSequence();
      }
       else if (g_lava_autotest_intro)
       {
          PAL_LavaAutotestRunIntroSequence();
       }
       else if (g_lava_autotest_xianling)
       {
          PAL_LavaAutotestRunXianlingSequence();
       }
       else if (g_lava_autotest_obj204)
       {
          PAL_LavaAutotestRunObj204Sequence();
       }
       else if (g_lava_autotest_walk)
       {
         PAL_LavaAutotestRunWalkSequence();
        }
      if (g_lava_shutdown_requested)
      {
         return;
      }
    }
    else
    {
       PAL_LavaLoadPlayerRoles();
       g_lava_pending_load_slot = 0;
       printf("[LAVA][LOAD] begin slot=%d\n", g_lava_gpGlobals.bCurrentSaveSlot);
       if (PAL_LavaLoadSavedGame(g_lava_gpGlobals.bCurrentSaveSlot) == 0)
       {
          int load_visible;
          int load_obj;

          load_visible = 0;
          for (load_obj = g_lava_scene_event_first; load_obj <= g_lava_scene_event_last; load_obj++)
          {
             addr evt;

             evt = PAL_LavaSceneEventData(load_obj);
             if (evt != 0 && PAL_LavaReadS16(evt, 12) > 0 && PAL_LavaReadU16(evt, 16) != 0)
             {
                load_visible++;
             }
          }
          printf("[LAVA][LOAD] slot=%d scene=%d view=(%d,%d) party=(%d,%d) dir=%d count=%d\n",
             g_lava_gpGlobals.bCurrentSaveSlot,
             g_lava_scene_num,
             g_lava_view_x, g_lava_view_y,
             g_lava_party_x, g_lava_party_y,
             g_lava_party_direction, g_lava_party_count);
          printf("[LAVA][LOAD] visible_objects=%d range=%d..%d\n",
             load_visible,
             g_lava_scene_event_first,
             g_lava_scene_event_last);
          if (g_lava_autotest_menu_load && g_lava_gpGlobals.bCurrentSaveSlot == 5)
          {
             int saved_search_log;

             saved_search_log = PAL_LavaAutotestEnableSearchLog();
             PAL_LavaDumpTouchCandidates();
             printf("[LAVA][LOADMENU] autotouch object=694 trigger=40164\n");
             PAL_LavaAutotestTouchObject(694);
             PAL_LavaAutotestRestoreSearchLog(saved_search_log);
          }
       }
       else
       {
          printf("[LAVA][LOAD] slot=%d load failed, fallback newgame\n",
             g_lava_gpGlobals.bCurrentSaveSlot);
          g_lava_gpGlobals.bCurrentSaveSlot = 0;
          PAL_LavaLoadPlayerRoles();
          PAL_LavaProbeNewGameData();
          PAL_LavaLoadScenePreview();
          PAL_LavaRunPendingDialog();
       }
    }

    dwTime = SDL_GetTicks();
    dwLogicTime = dwTime;
   while (TRUE)
   {
      if (g_lava_shutdown_requested)
      {
         return;
      }
      PAL_LoadResources();
      PAL_ClearKeyState();
       PAL_DelayUntil(dwTime);
       PAL_ProcessEvent();
        dwNow = SDL_GetTicks();
        dwTime = dwNow + RENDER_FRAME_TIME;
        g_lava_render_frame_num++;
        g_lava_run_logic_frame = (int)(dwNow - dwLogicTime) >= 0;
        if (g_lava_run_logic_frame)
        {
           g_lava_logic_frame_num++;
           dwLogicTime = dwNow + FRAME_TIME;
        }
        PAL_StartFrame();
        if (g_lava_autotest_input && g_lava_run_logic_frame)
      {
         g_lava_autotest_input_step++;
      }
   }
}

static int PAL_LavaAutotestTouchObject(int object_id)
{
   addr evt;
   int trigger_mode;
    long trigger_script;
   int ox;
   int oy;
   int trigger_range;
   int base_dist;
   int dist;

   evt = PAL_LavaSceneEventData(object_id);
   if (evt == 0)
   {
      return 0;
   }

   if (PAL_LavaReadS16(evt, 12) <= 0 || PAL_LavaReadU16(evt, 8) == 0)
   {
      return 0;
   }

   trigger_script = PAL_LavaReadU16(evt, 8);
   if (g_lava_autotouch_last_object == object_id &&
       g_lava_autotouch_last_trigger == trigger_script)
   {
      return 0;
   }
   if (PAL_LavaAutotouchSeen(object_id, trigger_script))
   {
      return 0;
   }

   trigger_mode = PAL_LavaReadU16(evt, 14);
   if (trigger_mode < 4)
   {
      return 0;
   }

   ox = PAL_LavaReadU16(evt, 2);
   oy = PAL_LavaReadU16(evt, 4);
   trigger_range = (trigger_mode - 4) * 32 + 16;
   base_dist = trigger_range - 4;
   if (base_dist < 0)
   {
      base_dist = 0;
   }

   for (dist = base_dist; dist >= 0; dist -= 4)
   {
      g_lava_party_x = ox + dist;
      g_lava_party_y = oy;
      g_lava_party_direction = 1;
      PAL_LavaUpdateViewport();
      PAL_LavaDrawSceneFrame();
      printf("[LAVA][AUTOTOUCH] try object=%d from pos=(%d,%d) mode=%d dist=%d\n",
         object_id, g_lava_party_x, g_lava_party_y, trigger_mode, dist);
      if (PAL_LavaTouchScene())
      {
         g_lava_autotouch_last_object = object_id;
         g_lava_autotouch_last_trigger = trigger_script;
         PAL_LavaAutotouchRemember(object_id, trigger_script);
         return 1;
      }
   }

   printf("[LAVA][AUTOTOUCH] no touch position for object=%d mode=%d\n",
      object_id, trigger_mode);
   return 0;
}

static void PAL_LavaDumpTouchCandidates(void)
{
   int object_id;

   printf("[LAVA][AUTOTOUCH] scan scene=%d events=%d..%d\n",
      g_lava_scene_num, g_lava_scene_event_first, g_lava_scene_event_last);
   for (object_id = g_lava_scene_event_first;
        object_id <= g_lava_scene_event_last;
        object_id++)
   {
      addr evt;
      int state;
      int trigger_mode;
      long trigger_script;

      evt = PAL_LavaSceneEventData(object_id);
      if (evt == 0)
      {
         continue;
      }

      state = PAL_LavaReadS16(evt, 12);
      trigger_mode = PAL_LavaReadU16(evt, 14);
      trigger_script = PAL_LavaReadU16(evt, 8);
      if (state > 0 && trigger_mode >= 4 && trigger_script != 0)
      {
         printf("[LAVA][AUTOTOUCH] candidate object=%d trigger=%d mode=%d state=%d pos=(%d,%d)\n",
            object_id, trigger_script, trigger_mode, state,
            PAL_LavaReadU16(evt, 2), PAL_LavaReadU16(evt, 4));
      }
   }
}

long Decompress(addr Source, addr Destination, long DestSize)
{
   PAL_TmpReset();
   if (gConfig.fIsWIN95)
   {
      return YJ2_Decompress(Source, Destination, DestSize);
   }
   return YJ1_Decompress(Source, Destination, DestSize);
}

LPCBITMAPRLE PAL_SpriteGetFrame(LPSPRITE lpSprite, int iFrameNum)
{
   int imagecount;
   long offset;

   if (lpSprite == 0)
   {
      return 0;
   }

   imagecount = (PAL_U8(lpSprite[0]) | (PAL_U8(lpSprite[1]) << 8));
   if (iFrameNum < 0 || iFrameNum >= imagecount)
   {
      return 0;
   }

   iFrameNum <<= 1;
   offset = ((long)(PAL_U8(lpSprite[iFrameNum]) | (PAL_U8(lpSprite[iFrameNum + 1]) << 8)) << 1);
   return (LPCBITMAPRLE)&lpSprite[offset];
}

int PAL_SpriteGetNumFrames(LPSPRITE lpSprite)
{
   if (lpSprite == 0)
   {
      return 0;
   }

   return (PAL_U8(lpSprite[0]) | (PAL_U8(lpSprite[1]) << 8)) - 1;
}

int PAL_RLEGetHeight(LPCBITMAPRLE lpBitmapRLE)
{
   if (lpBitmapRLE == 0)
   {
      return 0;
   }

   if (PAL_U8(lpBitmapRLE[0]) == 0x02 && PAL_U8(lpBitmapRLE[1]) == 0x00 &&
      PAL_U8(lpBitmapRLE[2]) == 0x00 && PAL_U8(lpBitmapRLE[3]) == 0x00)
   {
      lpBitmapRLE += 4;
   }

   return PAL_U8(lpBitmapRLE[2]) | (PAL_U8(lpBitmapRLE[3]) << 8);
}

int PAL_RLEGetWidth(LPCBITMAPRLE lpBitmapRLE)
{
   if (lpBitmapRLE == 0)
   {
      return 0;
   }

   if (PAL_U8(lpBitmapRLE[0]) == 0x02 && PAL_U8(lpBitmapRLE[1]) == 0x00 &&
      PAL_U8(lpBitmapRLE[2]) == 0x00 && PAL_U8(lpBitmapRLE[3]) == 0x00)
   {
      lpBitmapRLE += 4;
   }

   return PAL_U8(lpBitmapRLE[0]) | (PAL_U8(lpBitmapRLE[1]) << 8);
}

int PAL_RLEBlitToSurface(LPCBITMAPRLE lpBitmapRLE, addr lpDstSurface, PAL_POS pos)
{
   g_rle_dst = lpDstSurface;
   g_rle_i = 0;
   g_rle_j = 0;
   g_rle_k = 0;
   g_rle_sx = 0;
   g_rle_x = 0;
   g_rle_y = 0;
   g_rle_len = 0;
   g_rle_w = 0;
   g_rle_h = 0;
   g_rle_srcx = 0;
   g_rle_t = 0;
   g_rle_dx = PAL_X(pos);
   g_rle_dy = PAL_Y(pos);
   if (lpBitmapRLE == 0 || g_rle_dst == 0)
   {
      return -1;
   }

   if (PAL_U8(lpBitmapRLE[0]) == 0x02 && PAL_U8(lpBitmapRLE[1]) == 0x00 &&
      PAL_U8(lpBitmapRLE[2]) == 0x00 && PAL_U8(lpBitmapRLE[3]) == 0x00)
   {
      lpBitmapRLE += 4;
   }

   g_rle_w = PAL_U8(lpBitmapRLE[0]) | (PAL_U8(lpBitmapRLE[1]) << 8);
   g_rle_h = PAL_U8(lpBitmapRLE[2]) | (PAL_U8(lpBitmapRLE[3]) << 8);
   if (g_rle_w + g_rle_dx <= 0 || g_rle_dx >= g_lava_gpScreen.w ||
       g_rle_h + g_rle_dy <= 0 || g_rle_dy >= g_lava_gpScreen.h)
   {
      return 0;
   }

   g_rle_len = g_rle_w * g_rle_h;
   lpBitmapRLE += 4;
   for (g_rle_i = 0; g_rle_i < g_rle_len;)
   {
      g_rle_t = PAL_U8(*lpBitmapRLE++);
      if ((g_rle_t & 0x80) && g_rle_t <= 0x80 + g_rle_w)
      {
         g_rle_i += g_rle_t - 0x80;
         g_rle_srcx += g_rle_t - 0x80;
         if (g_rle_srcx >= g_rle_w)
         {
            g_rle_srcx -= g_rle_w;
            g_rle_dy++;
         }
      }
      else
      {
         g_rle_j = 0;
         g_rle_sx = g_rle_srcx;
         g_rle_x = g_rle_dx + g_rle_srcx;
         g_rle_y = g_rle_dy;
         if (g_rle_y < 0)
         {
            g_rle_j += -g_rle_y * g_rle_w;
            g_rle_y = 0;
         }
         else if (g_rle_y >= g_lava_gpScreen.h)
         {
            return 0;
         }
         while (g_rle_j < g_rle_t)
         {
            if (g_rle_x < 0)
            {
               g_rle_j += -g_rle_x;
               if (g_rle_j >= g_rle_t) break;
               g_rle_sx += -g_rle_x;
               g_rle_x = 0;
            }
            else if (g_rle_x >= g_lava_gpScreen.w)
            {
               g_rle_j += g_rle_w - g_rle_sx;
               g_rle_x -= g_rle_sx;
               g_rle_sx = 0;
               g_rle_y++;
               if (g_rle_y >= g_lava_gpScreen.h)
               {
                  return 0;
               }
               continue;
            }
              g_rle_k = g_rle_t - g_rle_j;
              if (g_lava_gpScreen.w - g_rle_x < g_rle_k) g_rle_k = g_lava_gpScreen.w - g_rle_x;
              if (g_rle_w - g_rle_sx < g_rle_k) g_rle_k = g_rle_w - g_rle_sx;
              g_rle_sx += g_rle_k;
             if (g_lava_direct_screen && g_rle_dst == (addr)gpScreen)
             {
                g_rle_cp = (char *)g_lava_gpScreen.pixels + g_rle_y * g_lava_gpScreen.pitch;
                WriteBlock(g_rle_x, g_rle_y, g_rle_k, 1, 1, (addr)(lpBitmapRLE + g_rle_j));
                for (; g_rle_k != 0; g_rle_k--)
                {
                   g_rle_cp[g_rle_x] = lpBitmapRLE[g_rle_j];
                   g_rle_j++;
                   g_rle_x++;
                }
             }
             else
             {
                g_rle_cp = (char *)g_lava_gpScreen.pixels + g_rle_y * g_lava_gpScreen.pitch;
                for (; g_rle_k != 0; g_rle_k--)
                {
                   g_rle_cp[g_rle_x] = lpBitmapRLE[g_rle_j];
                   g_rle_j++;
                   g_rle_x++;
                }
             }
            if (g_rle_sx >= g_rle_w)
            {
               g_rle_sx -= g_rle_w;
               g_rle_x -= g_rle_w;
               g_rle_y++;
               if (g_rle_y >= g_lava_gpScreen.h)
               {
                  return 0;
               }
            }
         }
         lpBitmapRLE += g_rle_t;
         g_rle_i += g_rle_t;
         g_rle_srcx += g_rle_t;
         while (g_rle_srcx >= g_rle_w)
         {
            g_rle_srcx -= g_rle_w;
            g_rle_dy++;
         }
      }
   }

   return 0;
}

int PAL_RLEBlitWithColorShift(LPCBITMAPRLE lpBitmapRLE, addr lpDstSurface, PAL_POS pos, int iColorShift)
{
   long i;
   long j;
   long k;
   int sx;
   int x;
   int y;
   long uiLen;
   int uiWidth;
   int uiHeight;
   int uiSrcX;
   int T;
   int dx;
   int dy;
   int color;
   char *p;
   SDL_Surface *dst;

   dst = lpDstSurface;
   i = 0;
   j = 0;
   k = 0;
   sx = 0;
   x = 0;
   y = 0;
   uiLen = 0;
   uiWidth = 0;
   uiHeight = 0;
   uiSrcX = 0;
   T = 0;
   dx = PAL_X(pos);
   dy = PAL_Y(pos);
   color = 0;
   if (lpBitmapRLE == 0 || dst == 0)
   {
      return -1;
   }

   if (PAL_U8(lpBitmapRLE[0]) == 0x02 && PAL_U8(lpBitmapRLE[1]) == 0x00 &&
      PAL_U8(lpBitmapRLE[2]) == 0x00 && PAL_U8(lpBitmapRLE[3]) == 0x00)
   {
      lpBitmapRLE += 4;
   }

   uiWidth = PAL_U8(lpBitmapRLE[0]) | (PAL_U8(lpBitmapRLE[1]) << 8);
   uiHeight = PAL_U8(lpBitmapRLE[2]) | (PAL_U8(lpBitmapRLE[3]) << 8);
   if (uiWidth + dx <= 0 || dx >= dst->w ||
       uiHeight + dy <= 0 || dy >= dst->h)
   {
      return 0;
   }

   uiLen = uiWidth * uiHeight;
   lpBitmapRLE += 4;
   for (i = 0; i < uiLen;)
   {
      T = PAL_U8(*lpBitmapRLE++);
      if ((T & 0x80) && T <= 0x80 + uiWidth)
      {
         i += T - 0x80;
         uiSrcX += T - 0x80;
         if (uiSrcX >= uiWidth)
         {
            uiSrcX -= uiWidth;
            dy++;
         }
      }
      else
      {
         j = 0;
         sx = uiSrcX;
         x = dx + uiSrcX;
         y = dy;
         if (y < 0)
         {
            j += -y * uiWidth;
            y = 0;
         }
         else if (y >= dst->h)
         {
            return 0;
         }
         while (j < T)
         {
            if (x < 0)
            {
               j += -x;
               if (j >= T) break;
               sx += -x;
               x = 0;
            }
            else if (x >= dst->w)
            {
               j += uiWidth - sx;
               x -= sx;
               sx = 0;
               y++;
               if (y >= dst->h)
               {
                  return 0;
               }
               continue;
            }
            k = T - j;
            if (dst->w - x < k) k = dst->w - x;
            if (uiWidth - sx < k) k = uiWidth - sx;
            sx += k;
            p = (char *)dst->pixels + y * dst->pitch;
            for (; k != 0; k--)
            {
               color = lpBitmapRLE[j] & 0x0F;
               color += iColorShift;
               if (color > 0x0F) color = 0x0F;
               if (color < 0) color = 0;
               p[x] = (PAL_U8(lpBitmapRLE[j]) & 0xF0) | color;
               j++;
               x++;
            }
            if (sx >= uiWidth)
            {
               sx -= uiWidth;
               x -= uiWidth;
               y++;
               if (y >= dst->h)
               {
                  return 0;
               }
            }
         }
         lpBitmapRLE += T;
         i += T;
         uiSrcX += T;
         while (uiSrcX >= uiWidth)
         {
            uiSrcX -= uiWidth;
            dy++;
         }
      }
   }

   return 0;
}

int PAL_RLEBlitMonoColor(LPCBITMAPRLE lpBitmapRLE, addr lpDstSurface, PAL_POS pos, BYTE bColor, int iColorShift)
{
   long i;
   long j;
   long k;
   int sx;
   int x;
   int y;
   long uiLen;
   int uiWidth;
   int uiHeight;
   int uiSrcX;
   int T;
   int dx;
   int dy;
   int color;
   int shade;
   char *p;
   SDL_Surface *dst;

   dst = lpDstSurface;
   i = 0;
   j = 0;
   k = 0;
   sx = 0;
   x = 0;
   y = 0;
   uiLen = 0;
   uiWidth = 0;
   uiHeight = 0;
   uiSrcX = 0;
   T = 0;
   dx = PAL_X(pos);
   dy = PAL_Y(pos);
   color = 0;
   shade = 0;
   if (lpBitmapRLE == 0 || dst == 0)
   {
      return -1;
   }

   if (PAL_U8(lpBitmapRLE[0]) == 0x02 && PAL_U8(lpBitmapRLE[1]) == 0x00 &&
      PAL_U8(lpBitmapRLE[2]) == 0x00 && PAL_U8(lpBitmapRLE[3]) == 0x00)
   {
      lpBitmapRLE += 4;
   }

   uiWidth = PAL_U8(lpBitmapRLE[0]) | (PAL_U8(lpBitmapRLE[1]) << 8);
   uiHeight = PAL_U8(lpBitmapRLE[2]) | (PAL_U8(lpBitmapRLE[3]) << 8);
   if (uiWidth + dx <= 0 || dx >= dst->w ||
       uiHeight + dy <= 0 || dy >= dst->h)
   {
      return 0;
   }

   uiLen = uiWidth * uiHeight;
   lpBitmapRLE += 4;
   for (i = 0; i < uiLen;)
   {
      T = PAL_U8(*lpBitmapRLE++);
      if ((T & 0x80) && T <= 0x80 + uiWidth)
      {
         i += T - 0x80;
         uiSrcX += T - 0x80;
         if (uiSrcX >= uiWidth)
         {
            uiSrcX -= uiWidth;
            dy++;
         }
      }
      else
      {
         j = 0;
         sx = uiSrcX;
         x = dx + uiSrcX;
         y = dy;
         if (y < 0)
         {
            j += -y * uiWidth;
            y = 0;
         }
         else if (y >= dst->h)
         {
            return 0;
         }
         while (j < T)
         {
            if (x < 0)
            {
               j += -x;
               if (j >= T) break;
               sx += -x;
               x = 0;
            }
            else if (x >= dst->w)
            {
               j += uiWidth - sx;
               x -= sx;
               sx = 0;
               y++;
               if (y >= dst->h)
               {
                  return 0;
               }
               continue;
            }
            k = T - j;
            if (dst->w - x < k) k = dst->w - x;
            if (uiWidth - sx < k) k = uiWidth - sx;
            sx += k;
             p = (char *)dst->pixels + y * dst->pitch;
             for (; k != 0; k--)
             {
                shade = (PAL_U8(lpBitmapRLE[j]) & 0x0F) + iColorShift;
                if (shade < 0)
                {
                   shade = 0;
                }
                else if (shade > 15)
                {
                   shade = 15;
                }
                if (bColor == 0)
                {
                   color = shade;
                }
                else
                {
                   color = (PAL_U8(bColor) & 0xF0) | shade;
                }
                p[x] = color;
                j++;
                x++;
             }
            if (sx >= uiWidth)
            {
               sx -= uiWidth;
               x -= uiWidth;
               y++;
               if (y >= dst->h)
               {
                  return 0;
               }
            }
         }
         lpBitmapRLE += T;
         i += T;
         uiSrcX += T;
         while (uiSrcX >= uiWidth)
         {
            uiSrcX -= uiWidth;
            dy++;
         }
      }
   }

   return 0;
}

int PAL_RLEBlitToSurfaceWithShadow(LPCBITMAPRLE lpBitmapRLE, addr lpDstSurface, PAL_POS pos, BOOL bShadow)
{
   long i;
   long j;
   long k;
   int sx;
   int x;
   int y;
   long uiLen;
   int uiWidth;
   int uiHeight;
   int uiSrcX;
   int T;
   int dx;
   int dy;
   char *p;
   SDL_Surface *dst;

   dst = lpDstSurface;
   i = 0;
   j = 0;
   k = 0;
   sx = 0;
   x = 0;
   y = 0;
   uiLen = 0;
   uiWidth = 0;
   uiHeight = 0;
   uiSrcX = 0;
   T = 0;
   dx = PAL_X(pos);
   dy = PAL_Y(pos);
   if (lpBitmapRLE == 0 || dst == 0)
   {
      return -1;
   }

   if (PAL_U8(lpBitmapRLE[0]) == 0x02 && PAL_U8(lpBitmapRLE[1]) == 0x00 &&
      PAL_U8(lpBitmapRLE[2]) == 0x00 && PAL_U8(lpBitmapRLE[3]) == 0x00)
   {
      lpBitmapRLE += 4;
   }

   uiWidth = PAL_U8(lpBitmapRLE[0]) | (PAL_U8(lpBitmapRLE[1]) << 8);
   uiHeight = PAL_U8(lpBitmapRLE[2]) | (PAL_U8(lpBitmapRLE[3]) << 8);
   if (uiWidth + dx <= 0 || dx >= dst->w ||
       uiHeight + dy <= 0 || dy >= dst->h)
   {
      return 0;
   }

   uiLen = uiWidth * uiHeight;
   lpBitmapRLE += 4;
   for (i = 0; i < uiLen;)
   {
      T = PAL_U8(*lpBitmapRLE++);
      if ((T & 0x80) && T <= 0x80 + uiWidth)
      {
         i += T - 0x80;
         uiSrcX += T - 0x80;
         if (uiSrcX >= uiWidth)
         {
            uiSrcX -= uiWidth;
            dy++;
         }
      }
      else
      {
         j = 0;
         sx = uiSrcX;
         x = dx + uiSrcX;
         y = dy;
         if (y < 0)
         {
            j += -y * uiWidth;
            y = 0;
         }
         else if (y >= dst->h)
         {
            return 0;
         }
         while (j < T)
         {
            if (x < 0)
            {
               j += -x;
               if (j >= T) break;
               sx += -x;
               x = 0;
            }
            else if (x >= dst->w)
            {
               j += uiWidth - sx;
               x -= sx;
               sx = 0;
               y++;
               if (y >= dst->h)
               {
                  return 0;
               }
               continue;
            }
            k = T - j;
            if (dst->w - x < k) k = dst->w - x;
            if (uiWidth - sx < k) k = uiWidth - sx;
            sx += k;
            p = (char *)dst->pixels + y * dst->pitch;
            if (bShadow)
            {
               j += k;
               for (; k != 0; k--)
               {
                  p[x] = (p[x] & 0xF0) | ((p[x] & 0x0F) >> 1);
                  x++;
               }
            }
            else
            {
               for (; k != 0; k--)
               {
                  p[x] = lpBitmapRLE[j];
                  j++;
                  x++;
               }
            }
            if (sx >= uiWidth)
            {
               sx -= uiWidth;
               x -= uiWidth;
               y++;
               if (y >= dst->h)
               {
                  return 0;
               }
            }
         }
         lpBitmapRLE += T;
         i += T;
         uiSrcX += T;
         while (uiSrcX >= uiWidth)
         {
            uiSrcX -= uiWidth;
            dy++;
         }
      }
   }

   return 0;
}

int PAL_FBPBlitToSurface(addr lpBitmapFBP, addr lpDstSurface)
{
   SDL_Surface *dst;
   int x, y;
   char *src;

   dst = lpDstSurface;
   src = lpBitmapFBP;

   if (!src || !dst) return -1;
   if (dst->w != 320 || dst->h != 200) return -1;

   if (g_lava_direct_screen && lpDstSurface == (addr)gpScreen)
   {
      WriteBlock(0, 0, 320, 200, 1, src);
      memcpy(dst->pixels, lpBitmapFBP, 320 * 200);
      return 0;
   }

   for (y = 0; y < 200; y++)
   {
      char *p;
      p = (char *)dst->pixels + y * dst->pitch;
      for (x = 0; x < 320; x++)
      {
         p[x] = src[y * 320 + x];
      }
   }
   return 0;
}

int PAL_FBPBlitRectToSurface(addr lpBitmapFBP, addr lpDstSurface, int src_y, int dst_y, int h)
{
   SDL_Surface *dst;
   char *src;
   int x;
   int y;

   dst = lpDstSurface;
   src = lpBitmapFBP;

   if (!src || !dst) return -1;
   if (dst->w != 320 || dst->h != 200) return -1;
   if (src_y < 0 || dst_y < 0 || h < 0) return -1;
   if (src_y + h > 200 || dst_y + h > 200) return -1;

   if (g_lava_direct_screen && lpDstSurface == (addr)gpScreen)
   {
      WriteBlock(0, dst_y, 320, h, 1, (addr)(src + src_y * 320));
      for (y = 0; y < h; y++)
      {
         memcpy((addr)((char *)dst->pixels + (dst_y + y) * dst->pitch),
            (addr)(src + (src_y + y) * 320), 320);
      }
      return 0;
   }

   for (y = 0; y < h; y++)
   {
      char *p;
      p = (char *)dst->pixels + (dst_y + y) * dst->pitch;
      for (x = 0; x < 320; x++)
      {
         p[x] = src[(src_y + y) * 320 + x];
      }
   }
   return 0;
}

#ifndef LAVA_NATIVE_COMPILED
#ifndef stderr
#define stderr 0
#endif

int fprintf(addr stream, char *fmt, ...)
{
   return 0;
}
#endif

#endif
