/* Lava battle UI bridge.
 * This file is the Lava-only counterpart of original uibattle.c. */

#include "lava_battle.h"

#define LAVA_BATTLE_ENEMY_X 70
#define LAVA_BATTLE_PARTY_X 91
#define LAVA_BATTLE_PARTY_Y 155
#define LAVA_BATTLE_PARTY_W 77
#define LAVA_BATTLE_MENU_X 8
#define LAVA_BATTLE_MENU_Y 146
#define LAVA_BATTLE_MSG_BOX_X 62
#define LAVA_BATTLE_MSG_BOX_Y 162
#define LAVA_BATTLE_MSG_BOX_W 186
#define LAVA_BATTLE_MSG_X 70
#define LAVA_BATTLE_MSG_Y 170
#define LAVA_BATTLE_VALUE_X 220
#define LAVA_BATTLE_DAMAGE_X 248
#define LAVA_BATTLE_MAGIC_BOX_LEFT 10
#define LAVA_BATTLE_MAGIC_BOX_TOP 42
#define LAVA_BATTLE_MAGIC_BOX_X 35
#define LAVA_BATTLE_MAGIC_BOX_Y 54
#define LAVA_BATTLE_MAGIC_ROW_H 18
/* Original DOS+desc.dat layout: iItemsPerLine = 32 / dwWordLength (10) = 3,
   iItemTextWidth = 8 * dwWordLength + 7 = 87. */
#define LAVA_BATTLE_MAGIC_ITEMS_PER_LINE 3
#define LAVA_BATTLE_MAGIC_ITEM_W 87
#define LAVA_BATTLE_MAGIC_LINES 5
#define LAVA_BATTLE_MAGIC_CURSOR_X_OFFSET 20
#define LAVA_BATTLE_KEY_PGUP 0x40
#define LAVA_BATTLE_KEY_PGDN 0x80
/* Original draws the MP single-line box at top-left when desc.dat is present:
   box (0,0) width 5, slash (45,14), needed (15,14), current (50,14). */
#define LAVA_BATTLE_MAGIC_MP_BOX_X 0
#define LAVA_BATTLE_MAGIC_MP_BOX_Y 0
#define LAVA_BATTLE_MAGIC_MP_SLASH_X 45
#define LAVA_BATTLE_MAGIC_MP_SLASH_Y 14
#define LAVA_BATTLE_MAGIC_MP_COST_X 15
#define LAVA_BATTLE_MAGIC_MP_CURRENT_X 50
#define LAVA_BATTLE_MAGIC_MP_Y 14
#define LAVA_BATTLE_MAGIC_DESC_X 102
#define LAVA_BATTLE_MAGIC_DESC_Y 3
/* Battle message label (e.g. 遭遇敌人) sits on the line below the magic
   description, right-aligned to the right screen edge. */
#define LAVA_BATTLE_MSG_RIGHT_X 316
#define LAVA_BATTLE_MSG_Y_TOP 2
#define LAVA_BATTLE_MSG_LINES 6
#define LAVA_BATTLE_MSG_LINE_H 16
#define LAVA_BATTLE_MSG_GRAY 192
#define LAVA_BATTLE_MSG_TEXT_MAX 96
#define LAVA_BATTLE_NUM_COLOR_YELLOW 0
#define LAVA_BATTLE_NUM_COLOR_BLUE 1
#define LAVA_BATTLE_NUM_COLOR_CYAN 2
#define LAVA_BATTLE_NUM_ALIGN_LEFT 0
#define LAVA_BATTLE_NUM_ALIGN_MID 1
#define LAVA_BATTLE_NUM_ALIGN_RIGHT 2
#define LAVA_BATTLE_LIST_COLOR_SELECTED 0xF9
#define LAVA_BATTLE_LIST_COLOR_SELECTED_INACTIVE MENUITEM_COLOR_SELECTED_INACTIVE
#define LAVA_BATTLE_SPRITENUM_SLASH 39
#define LAVA_BATTLE_SPRITENUM_ARROW 47
#define LAVA_BATTLE_SPRITENUM_PLAYERINFOBOX 18
#define LAVA_BATTLE_SPRITENUM_PLAYERFACE_FIRST 48
#define LAVA_BATTLE_SPRITENUM_ICON_ATTACK 40
#define LAVA_BATTLE_SPRITENUM_ICON_MAGIC 41
#define LAVA_BATTLE_SPRITENUM_ICON_COOPMAGIC 42
#define LAVA_BATTLE_SPRITENUM_ICON_MISC 43
#define LAVA_BATTLE_SPRITENUM_CURSOR 69
#define LAVA_BATTLE_SPRITENUM_ARROW_CURRENT 69
#define LAVA_BATTLE_SPRITENUM_ARROW_CURRENT_RED 68
#define LAVA_BATTLE_SPRITENUM_ARROW_SELECTED 67
#define LAVA_BATTLE_SPRITENUM_ARROW_SELECTED_RED 66
#define LAVA_BATTLE_TIMEMETER_COLOR_DEFAULT 0x1B
#define LAVA_BATTLE_UI_STATE_COMMAND 0
#define LAVA_BATTLE_UI_STATE_TARGET 1
#define LAVA_BATTLE_UI_STATE_MAGIC 2
#define LAVA_BATTLE_UI_STATE_PARTY_TARGET 3
#define LAVA_BATTLE_UI_STATE_ITEM 4
#define LAVA_BATTLE_UI_STATE_MISC 5
#define LAVA_BATTLE_UI_STATE_STATUS 6
#define LAVA_BATTLE_UI_STATE_TARGET_ALL 7
#define LAVA_BATTLE_UI_STATE_PARTY_TARGET_ALL 8
#define LAVA_BATTLE_UI_STATE_ITEM_SUBMENU 9
#define LAVA_BATTLE_MISC_ITEM 0
#define LAVA_BATTLE_MISC_DEFEND 1
#define LAVA_BATTLE_MISC_AUTO 2
#define LAVA_BATTLE_MISC_FLEE 3
#define LAVA_BATTLE_MISC_STATUS 4
#define LAVA_BATTLE_MISC_COUNT 5
#define LAVA_BATTLE_ITEM_SUBMENU_USE 0
#define LAVA_BATTLE_ITEM_SUBMENU_THROW 1
#define LAVA_BATTLE_FLOAT_NUM_COUNT 8
#define LAVA_BATTLE_FLOAT_NUM_FRAMES 6
#define LAVA_BATTLE_FLOAT_NUM_FRAME_MS 25

int PAL_BattleUIWaitForPlayerAction(LAVA_BATTLE_STATE *state);
void PAL_BattleUIDrawFrame(LAVA_BATTLE_STATE *state, int ui_state, int target_sel, char *message);
void PAL_BattleUIShowRoundResult(int *round_result);
void PAL_BattleUIShowText(char *text);
void PAL_BattleUIFinishBattle(int battle_result, char *message);
static void PAL_BattleUIApplyMagicHitFeedback(int target, int target_is_player, int from_enemy, int step);
static void PAL_BattleUIRedrawRoundFrame(char *message);
static void PAL_BattleUIClearMessageLog(void);
static void PAL_BattleUIClearFloatingNumbers(void);
static void PAL_BattleUIDrawFloatingNumbers(void);

int g_lava_battle_ui_last_enemy_count;
int g_lava_battle_ui_pending_fade_in;
int g_lava_battle_ui_autotest_turn;
int g_lava_battle_ui_autotest_magic_done;
int g_lava_battle_ui_enemy_name_logged;
int g_lava_battle_ui_magic_name_logged;
int g_lava_battle_misc_sel;
int g_lava_battle_item_submenu_sel;
int g_lava_battle_last_attack_target_sel;
int g_lava_battle_last_coop_target_sel;
int g_lava_battle_last_magic_sel[6];
int g_lava_battle_last_magic_log_role = -1;
int g_lava_battle_last_magic_log_mp = -1;
int g_lava_battle_last_magic_log_count = -1;
int g_lava_battle_last_magic_log_sel = -1;
int g_lava_battle_last_magic_log_obj = -1;
int g_lava_battle_last_magic_log_gb2312 = -1;
char g_lava_battle_magic_name_buf[32];
char g_lava_battle_log_text_buf[96];
char g_lava_battle_message_log[LAVA_BATTLE_MSG_LINES][LAVA_BATTLE_MSG_TEXT_MAX];
int g_lava_battle_message_log_count;
int g_lava_battle_float_num_value[LAVA_BATTLE_FLOAT_NUM_COUNT];
int g_lava_battle_float_num_x[LAVA_BATTLE_FLOAT_NUM_COUNT];
int g_lava_battle_float_num_y[LAVA_BATTLE_FLOAT_NUM_COUNT];
int g_lava_battle_float_num_color[LAVA_BATTLE_FLOAT_NUM_COUNT];
int g_lava_battle_float_num_frame[LAVA_BATTLE_FLOAT_NUM_COUNT];
int g_lava_battle_player_color_shift[3];
int g_lava_battle_player_offset_x[3];
int g_lava_battle_player_offset_y[3];
int g_lava_battle_enemy_color_shift[LAVA_BATTLE_MAX_ENEMIES];
int g_lava_battle_enemy_offset_x[LAVA_BATTLE_MAX_ENEMIES];
int g_lava_battle_enemy_offset_y[LAVA_BATTLE_MAX_ENEMIES];
int g_lava_battle_hide_players;
static int g_lava_battle_hide_cast_panels;
LAVA_BATTLE_STATE g_lava_battle_ui_state_copy;
int g_lava_battle_ui_state_valid;
char g_lava_battle_magic_sprite_buf[65536];

static char *PAL_BattleUILogText(char *text)
{
#ifdef LAVA_NATIVE_COMPILED
   if (text == 0)
   {
      g_lava_battle_log_text_buf[0] = 0;
      return g_lava_battle_log_text_buf;
   }
   return text;
#else
   return text;
#endif
}

static int PAL_BattleUIDebugMagicEffect(void)
{
   return g_lava_autotest_search || g_lava_autotest_load ||
      g_lava_autotest_bsilence || g_lava_autotest_bsleep ||
      g_lava_autotest_battle || g_lava_autotest_fengshen ||
      g_lava_battle_selected_magic_object_id == 0x145 ||
      PAL_LavaFightGetRoundEventArg0(TRUE) == 0x145 ||
      g_lava_battle_selected_magic_object_id == 0x156 ||
      PAL_LavaFightGetRoundEventArg0(TRUE) == 0x156 ||
      g_lava_battle_selected_magic_object_id == 0x15F ||
      PAL_LavaFightGetRoundEventArg0(TRUE) == 0x15F ||
      g_lava_battle_selected_magic_object_id == 0x143 ||
      PAL_LavaFightGetRoundEventArg0(TRUE) == 0x143;
}

static int PAL_BattleUICaptureMagicObject(int magic_object_id)
{
   return 0;
}

static int PAL_BattleUICaptureRawMagicObject(int magic_object_id, int from_enemy)
{
   return 0;
}

static void PAL_BattleUIClearSummonHold(void)
{
   PAL_LavaBattleClearSummonHold();
}

static void PAL_BattleUISetSummonHold(addr frame_rle, int cx, int cy)
{
   PAL_LavaBattleSetSummonHold(frame_rle, cx, cy);
}

static void PAL_BattleUIEndSummonCast(int old_hide_players)
{
   g_lava_battle_hide_players = old_hide_players;
   g_lava_battle_hide_cast_panels = 0;
   PAL_BattleUIClearSummonHold();
}

static void PAL_BattleUIRestoreSummonCast(int old_hide_players)
{
   g_lava_battle_hide_players = old_hide_players;
   g_lava_battle_hide_cast_panels = 0;
}

static int PAL_BattleUICaptureMagicStep(int magic_object_id, int step_index, int from_enemy)
{
   if (!PAL_BattleUICaptureMagicObject(magic_object_id) || from_enemy)
   {
      return 0;
   }
   if (magic_object_id == 82)
   {
      return step_index == 0 || step_index == 46 || step_index == 47;
   }
   return step_index == 0;
}

static void PAL_BattleUIDumpMagicScreenBMP(
   int magic_object_id,
   int step_index,
   int frame_index,
   char *stage,
   int from_enemy
);

static void PAL_BattleUIWriteU32LE(char *buf, int offset, long value)
{
   buf[offset] = (char)(value & 255);
   buf[offset + 1] = (char)((value >> 8) & 255);
   buf[offset + 2] = (char)((value >> 16) & 255);
   buf[offset + 3] = (char)((value >> 24) & 255);
}

static int PAL_BattleUIWritePalettedBMP(
   char *path,
   char *pixels,
   int width,
   int height,
   int pitch,
   int debug_log
)
{
   FILE *fp;
   char file_header[14];
   char dib_header[40];
   char pad[4];
   char row_buf[320 * 3 + 4];
   SDL_Color *color;
   long pixel_bytes;
   long file_size;
   int row_size;
   int padding;
   int x;
   int y;

   if (path == 0 || pixels == 0 || width <= 0 || height <= 0 || pitch < width)
   {
      return -1;
   }

   if (debug_log)
   {
      printf("[LAVA][MAGICCAP] bmp open begin path=%s w=%d h=%d pitch=%d\n",
         path, width, height, pitch);
      fflush(stdout);
   }

   fp = fopen(path, "wb");
   if (fp == 0)
   {
      if (debug_log)
      {
         printf("[LAVA][MAGICCAP] bmp open fail path=%s\n", path);
         fflush(stdout);
      }
      return -1;
   }

   if (debug_log)
   {
      printf("[LAVA][MAGICCAP] bmp open done path=%s\n", path);
      fflush(stdout);
   }

   row_size = ((width * 3 + 3) / 4) * 4;
   padding = row_size - width * 3;
   pixel_bytes = (long)row_size * height;
   file_size = 14 + 40 + pixel_bytes;

   PAL_LavaZeroBuffer(file_header, sizeof(file_header));
   PAL_LavaZeroBuffer(dib_header, sizeof(dib_header));
   PAL_LavaZeroBuffer(pad, sizeof(pad));
   file_header[0] = 'B';
   file_header[1] = 'M';
   PAL_BattleUIWriteU32LE(file_header, 2, file_size);
   PAL_BattleUIWriteU32LE(file_header, 10, 54);
   PAL_BattleUIWriteU32LE(dib_header, 0, 40);
   PAL_BattleUIWriteU32LE(dib_header, 4, width);
   PAL_BattleUIWriteU32LE(dib_header, 8, height);
   PAL_LavaWriteU16((addr)dib_header, 12, 1);
   PAL_LavaWriteU16((addr)dib_header, 14, 24);
   PAL_BattleUIWriteU32LE(dib_header, 20, pixel_bytes);

   if (fwrite((addr)file_header, 1, sizeof(file_header), fp) != sizeof(file_header) ||
       fwrite((addr)dib_header, 1, sizeof(dib_header), fp) != sizeof(dib_header))
   {
      if (debug_log)
      {
         printf("[LAVA][MAGICCAP] bmp header write fail path=%s\n", path);
         fflush(stdout);
      }
      fclose(fp);
      return -1;
   }

   if (debug_log)
   {
      printf("[LAVA][MAGICCAP] bmp pixels write begin path=%s rows=%d row_size=%d\n",
         path, height, row_size);
      fflush(stdout);
   }

   for (y = height - 1; y >= 0; y--)
   {
      for (x = 0; x < width; x++)
      {
         color = &g_current_palette[PAL_U8(pixels[y * pitch + x])];
         row_buf[x * 3] = (char)PAL_U8(color->b);
         row_buf[x * 3 + 1] = (char)PAL_U8(color->g);
         row_buf[x * 3 + 2] = (char)PAL_U8(color->r);
      }
      if (padding > 0)
      {
         row_buf[width * 3] = pad[0];
         row_buf[width * 3 + 1] = pad[1];
         row_buf[width * 3 + 2] = pad[2];
      }
      if (fwrite((addr)row_buf, 1, row_size, fp) != row_size)
      {
         if (debug_log)
         {
            printf("[LAVA][MAGICCAP] bmp row write fail path=%s y=%d\n", path, y);
            fflush(stdout);
         }
         fclose(fp);
         return -1;
      }
      if (debug_log && (y == height - 1 || y == height / 2 || y == 0))
      {
         printf("[LAVA][MAGICCAP] bmp row written path=%s y=%d\n", path, y);
         fflush(stdout);
      }
   }

   if (debug_log)
   {
      printf("[LAVA][MAGICCAP] bmp close begin path=%s\n", path);
      fflush(stdout);
   }
   fclose(fp);
   if (debug_log)
   {
      printf("[LAVA][MAGICCAP] bmp close done path=%s\n", path);
      fflush(stdout);
   }
   return 0;
}

static void PAL_BattleUIDumpRawMagicFrameBMP(
   int magic_object_id,
   int step_index,
   int frame_index,
   char *frame,
   int from_enemy
)
{
    int saved_direct_screen;
    long i;
    char *pixels;
    static char saved_pixels[320 * 200];

   if (!PAL_BattleUICaptureMagicObject(magic_object_id) || from_enemy || step_index != 0 ||
       frame == 0)
   {
      return;
   }

   if (g_screen_surface.pixels == 0)
    {
       return;
    }
    pixels = (char *)g_screen_surface.pixels;

    for (i = 0; i < 320 * 200; i++)
    {
      saved_pixels[i] = pixels[i];
      pixels[i] = 0;
   }
   saved_direct_screen = g_lava_direct_screen;
   g_lava_direct_screen = 0;
   PAL_RLEBlitToSurface((LPCBITMAPRLE)frame, gpScreen, PAL_XY(0, 0));
   PAL_BattleUIDumpMagicScreenBMP(magic_object_id, step_index, frame_index, "raw", from_enemy);
    g_lava_direct_screen = saved_direct_screen;
    for (i = 0; i < 320 * 200; i++)
    {
      pixels[i] = saved_pixels[i];
    }
}

static int PAL_BattleUIDecodeRLEToBuffer(char *frame, char *pixels, int width, int height, int pitch)
{
   long total_len;
   long pixel_index;
   int frame_w;
   int frame_h;
   int token;
   int x;
   int y;

   if (frame == 0 || pixels == 0 || width <= 0 || height <= 0 || pitch < width)
   {
      return -1;
   }
   if (PAL_U8(frame[0]) == 0x02 && PAL_U8(frame[1]) == 0x00 &&
       PAL_U8(frame[2]) == 0x00 && PAL_U8(frame[3]) == 0x00)
   {
      frame += 4;
   }
   frame_w = PAL_U8(frame[0]) | (PAL_U8(frame[1]) << 8);
   frame_h = PAL_U8(frame[2]) | (PAL_U8(frame[3]) << 8);
   if (frame_w <= 0 || frame_h <= 0 || frame_w > width || frame_h > height)
   {
      return -1;
   }

   frame += 4;
   total_len = (long)frame_w * frame_h;
   pixel_index = 0;
	while (pixel_index < total_len)
	{
	   token = PAL_U8(*frame++);
	   if (token == 0)
	   {
	      return -1;
	   }
	   if ((token & 0x80) && token <= 0x80 + frame_w)
	   {
	      pixel_index += token - 0x80;
	      if (pixel_index > total_len)
	      {
	         return -1;
	      }
	   }
      else
      {
         while (token-- > 0 && pixel_index < total_len)
         {
            x = (int)(pixel_index % frame_w);
            y = (int)(pixel_index / frame_w);
            pixels[y * pitch + x] = *frame++;
            pixel_index++;
         }
      }
   }
   return 0;
}

static void PAL_BattleUIDumpLastMagicFrameBMP(
   int magic_object_id,
   int magic_num,
   int chunk,
   int step_index,
   int total_frames,
   int frame_index,
   char *frame,
   int from_enemy
)
{
   char path[128];
   long i;
   static char decoded_pixels[320 * 200];

   if (!PAL_BattleUIDebugMagicEffect() || frame == 0 || step_index + 1 != total_frames)
   {
      return;
   }

   for (i = 0; i < 320 * 200; i++)
   {
      decoded_pixels[i] = 0;
   }
   if (PAL_BattleUIDecodeRLEToBuffer(frame, decoded_pixels, 320, 200, 320) != 0)
   {
      printf("[LAVA][MAGICCAP] dump-last decode-fail obj=%d magic=%d chunk=%d step=%d/%d frame=%d\n",
         magic_object_id, magic_num, chunk, step_index + 1, total_frames, frame_index);
      return;
   }
   sprintf(path,
      "magicfx_last_obj%03d_magic%03d_chunk%03d_%s_step%03d_frame%03d.bmp",
      magic_object_id, magic_num, chunk, from_enemy ? "enemy" : "player",
      step_index + 1, frame_index);
   if (PAL_BattleUIWritePalettedBMP(path, decoded_pixels, 320, 200, 320, 0) == 0)
   {
      printf("[LAVA][MAGICCAP] dumped-last obj=%d magic=%d chunk=%d side=%d step=%d/%d frame=%d path=%s\n",
         magic_object_id, magic_num, chunk, from_enemy, step_index + 1,
         total_frames, frame_index, path);
   }
}

static void PAL_BattleUIDumpPersistentRawMagicFrameBMP(
   int magic_object_id,
   int magic_num,
   int chunk,
   int step_index,
   int total_frames,
   int frame_index,
   char *frame,
   int from_enemy
)
{
   char path[160];
   long i;
   static char decoded_pixels[320 * 200];

   if (!PAL_BattleUICaptureRawMagicObject(magic_object_id, from_enemy) || frame == 0)
   {
      return;
   }

   printf("[LAVA][MAGICCAP] raw begin obj=%d magic=%d chunk=%d step=%d/%d frame=%d\n",
      magic_object_id, magic_num, chunk, step_index + 1, total_frames, frame_index);
   fflush(stdout);

   for (i = 0; i < 320 * 200; i++)
   {
      decoded_pixels[i] = 0;
   }
   printf("[LAVA][MAGICCAP] raw decode begin obj=%d magic=%d chunk=%d step=%d/%d frame=%d\n",
      magic_object_id, magic_num, chunk, step_index + 1, total_frames, frame_index);
   fflush(stdout);
   if (PAL_BattleUIDecodeRLEToBuffer(frame, decoded_pixels, 320, 200, 320) != 0)
   {
      printf("[LAVA][MAGICCAP] raw decode-fail obj=%d magic=%d chunk=%d step=%d/%d frame=%d\n",
         magic_object_id, magic_num, chunk, step_index + 1, total_frames, frame_index);
      fflush(stdout);
      return;
   }
   printf("[LAVA][MAGICCAP] raw decode done obj=%d magic=%d chunk=%d step=%d/%d frame=%d\n",
      magic_object_id, magic_num, chunk, step_index + 1, total_frames, frame_index);
   fflush(stdout);

   sprintf(path,
      "magicfx_dumps/raw_obj%03d_magic%03d_chunk%03d_step%03d_of%03d_frame%03d.bmp",
      magic_object_id, magic_num, chunk, step_index + 1, total_frames, frame_index);
   printf("[LAVA][MAGICCAP] raw write begin obj=%d magic=%d chunk=%d step=%d/%d frame=%d path=%s\n",
      magic_object_id, magic_num, chunk, step_index + 1, total_frames, frame_index, path);
   fflush(stdout);
   if (PAL_BattleUIWritePalettedBMP(path, decoded_pixels, 320, 200, 320, 1) == 0)
   {
      printf("[LAVA][MAGICCAP] raw dumped obj=%d magic=%d chunk=%d step=%d/%d frame=%d path=%s\n",
         magic_object_id, magic_num, chunk, step_index + 1, total_frames, frame_index, path);
      fflush(stdout);
   }
   else
   {
      printf("[LAVA][MAGICCAP] raw write fail obj=%d magic=%d chunk=%d step=%d/%d frame=%d path=%s\n",
         magic_object_id, magic_num, chunk, step_index + 1, total_frames, frame_index, path);
      fflush(stdout);
   }
}

static void PAL_BattleUIDumpMagicScreenBMP(
   int magic_object_id,
   int step_index,
   int frame_index,
   char *stage,
   int from_enemy
)
{
   FILE *fp;
   char path[256];
   char file_header[14];
   char dib_header[40];
   char pad[4];
   char bgr[3];
   char *pixels;
   SDL_Color *color;
   long pixel_bytes;
   long file_size;
   int screen_w;
   int screen_h;
   int pitch;
   int row_size;
   int padding;
   int x;
   int y;

   if (!PAL_BattleUICaptureMagicStep(magic_object_id, step_index, from_enemy) ||
       g_screen_surface.pixels == 0 || stage == 0)
   {
      return;
   }

   sprintf(path,
      "magicfx_dumps/screen_obj%03d_%s_%s_step%03d_frame%03d.bmp",
      magic_object_id, from_enemy ? "enemy" : "player",
      stage, step_index + 1, frame_index);
   screen_w = g_screen_surface.w;
   screen_h = g_screen_surface.h;
   pitch = g_screen_surface.pitch;
   pixels = (char *)g_screen_surface.pixels;
   if (PAL_BattleUIWritePalettedBMP(path, pixels, screen_w, screen_h, pitch, 0) == 0)
   {
      printf("[LAVA][MAGICCAP] dumped obj=%d side=%d stage=%s step=%d frame=%d path=%s\n",
         magic_object_id, from_enemy, stage, step_index + 1, frame_index, path);
   }
}

static void PAL_BattleUIDebugRLEStats(int magic_object_id, addr frame)
{
   int width;
   int height;
   long total_len;
   long pixel_index;
   long skip_runs;
   long draw_runs;
   long draw_pixels;
   long nonzero_pixels;
   int first_nonzero;
   int min_x;
   int min_y;
   int max_x;
   int max_y;
   int token;
   int pixel;
   int x;
   int y;

   if (!PAL_BattleUICaptureMagicObject(magic_object_id) || frame == 0)
   {
      return;
   }

   if (PAL_U8(frame[0]) == 0x02 && PAL_U8(frame[1]) == 0x00 &&
       PAL_U8(frame[2]) == 0x00 && PAL_U8(frame[3]) == 0x00)
   {
      frame += 4;
   }

   width = PAL_U8(frame[0]) | (PAL_U8(frame[1]) << 8);
   height = PAL_U8(frame[2]) | (PAL_U8(frame[3]) << 8);
   total_len = (long)width * height;
   pixel_index = 0;
   skip_runs = 0;
   draw_runs = 0;
   draw_pixels = 0;
   nonzero_pixels = 0;
   first_nonzero = -1;
   min_x = width;
   min_y = height;
   max_x = -1;
   max_y = -1;
   frame += 4;
   while (pixel_index < total_len)
   {
      token = PAL_U8(*frame++);
      if ((token & 0x80) && token <= 0x80 + width)
      {
         pixel_index += token - 0x80;
         skip_runs++;
      }
      else
      {
         draw_runs++;
         draw_pixels += token;
         while (token-- > 0)
         {
            pixel = PAL_U8(*frame++);
            x = (int)(pixel_index % width);
            y = (int)(pixel_index / width);
            if (pixel != 0)
            {
               nonzero_pixels++;
               if (first_nonzero < 0)
               {
                  first_nonzero = pixel;
               }
               if (x < min_x) min_x = x;
               if (y < min_y) min_y = y;
               if (x > max_x) max_x = x;
               if (y > max_y) max_y = y;
            }
            pixel_index++;
         }
      }
   }
   printf("[LAVA][MAGICRLE] obj=%d w=%d h=%d total=%ld skip_runs=%ld draw_runs=%ld draw_pixels=%ld nonzero=%ld first_nonzero=%d bbox=(%d,%d)-(%d,%d)\n",
      magic_object_id, width, height, total_len, skip_runs, draw_runs, draw_pixels,
      nonzero_pixels, first_nonzero, min_x, min_y, max_x, max_y);
}

static long PAL_BattleUIGetChunkDecompressedSize(FILE *fp, int chunk)
{
   char buf[8];
   long offset;

   if (fp == 0 || chunk < 0 || chunk >= PAL_MKFGetChunkCount(fp))
   {
      return -1;
   }

   fseek(fp, 4 * chunk, SEEK_SET);
   if (fread((addr)buf, 1, 4, fp) != 4)
   {
      return -1;
   }
   offset = PAL_U8(buf[0]) |
      (PAL_U8(buf[1]) << 8) |
      (PAL_U8(buf[2]) << 16) |
      (PAL_U8(buf[3]) << 24);
   fseek(fp, offset, SEEK_SET);

   if (gConfig.fIsWIN95)
   {
      if (fread((addr)buf, 1, 4, fp) != 4)
      {
         return -1;
      }
      return PAL_U8(buf[0]) |
         (PAL_U8(buf[1]) << 8) |
         (PAL_U8(buf[2]) << 16) |
         (PAL_U8(buf[3]) << 24);
   }

   if (fread((addr)buf, 1, 8, fp) != 8)
   {
      return -1;
   }
   if ((PAL_U8(buf[0]) | (PAL_U8(buf[1]) << 8) |
        (PAL_U8(buf[2]) << 16) | (PAL_U8(buf[3]) << 24)) != 0x315f4a59)
   {
      return -1;
   }
   return PAL_U8(buf[4]) |
      (PAL_U8(buf[5]) << 8) |
      (PAL_U8(buf[6]) << 16) |
      (PAL_U8(buf[7]) << 24);
}

static addr PAL_BattleUILoadMagicSpriteChunk(FILE *fp, int chunk, char *label)
{
   long chunk_size;
   long decompressed_size;
   long ret;

   if (fp == 0 || chunk < 0)
   {
      return 0;
   }

   chunk_size = PAL_MKFGetChunkSize((UINT)chunk, fp);
   decompressed_size = PAL_BattleUIGetChunkDecompressedSize(fp, chunk);
   if (chunk_size <= 0 || chunk_size > 65536 ||
       decompressed_size <= 0 || decompressed_size > sizeof(g_lava_battle_magic_sprite_buf))
   {
      if (PAL_BattleUIDebugMagicEffect())
      {
         printf("[LAVA][MAGICFX] %s bad size chunk=%d packed=%ld unpacked=%ld\n",
            label, chunk, chunk_size, decompressed_size);
      }
      return 0;
   }

   if (PAL_MKFReadChunk((addr)g_lava_mkf_buf, 65536, (UINT)chunk, fp) <= 0)
   {
      if (PAL_BattleUIDebugMagicEffect())
      {
         printf("[LAVA][MAGICFX] %s read fail chunk=%d\n", label, chunk);
      }
      return 0;
   }

   PAL_TmpReset();
   ret = Decompress((addr)g_lava_mkf_buf, (addr)g_lava_battle_magic_sprite_buf, sizeof(g_lava_battle_magic_sprite_buf));
   if (!PAL_LavaDecompressOK(ret, decompressed_size))
   {
      if (PAL_BattleUIDebugMagicEffect())
      {
         printf("[LAVA][MAGICFX] %s decompress fail chunk=%d ret=%ld expected=%ld\n",
            label, chunk, ret, decompressed_size);
      }
      return 0;
   }

   return (addr)g_lava_battle_magic_sprite_buf;
}

static int PAL_BattleUIEnemyBaseY(int enemy_count)
{
   if (enemy_count >= 5)
   {
      return 62;
   }
   if (enemy_count >= 4)
   {
      return 66;
   }
   if (enemy_count >= 3)
   {
      return 70;
   }
   return 74;
}

static void PAL_BattleUISyncCurrentState(LAVA_BATTLE_STATE *state)
{
   long i;

   if (state == 0)
   {
      g_lava_battle_ui_state_valid = 0;
      return;
   }

   g_lava_battle_ui_state_copy.battle_result = state->battle_result;
   g_lava_battle_ui_state_copy.flow_action = state->flow_action;
   g_lava_battle_ui_state_copy.enemy_team = state->enemy_team;
   g_lava_battle_ui_state_copy.command_sel = state->command_sel;
    g_lava_battle_ui_state_copy.magic_sel = state->magic_sel;
    g_lava_battle_ui_state_copy.magic_object_id = state->magic_object_id;
    g_lava_battle_ui_state_copy.item_sel = state->item_sel;
    g_lava_battle_ui_state_copy.item_object_id = state->item_object_id;
    g_lava_battle_ui_state_copy.round_command = state->round_command;
   g_lava_battle_ui_state_copy.target_sel = state->target_sel;
   g_lava_battle_ui_state_copy.acting_player_index = state->acting_player_index;
   g_lava_battle_ui_state_copy.enemy_count = state->enemy_count;
   g_lava_battle_ui_state_copy.turn = state->turn;
   for (i = 0; i < LAVA_BATTLE_MAX_ENEMIES; i++)
   {
      g_lava_battle_ui_state_copy.enemy_object_id[i] = state->enemy_object_id[i];
      g_lava_battle_ui_state_copy.enemy_hp[i] = state->enemy_hp[i];
      g_lava_battle_ui_state_copy.enemy_hp_max[i] = state->enemy_hp_max[i];
   }
   for (i = 0; i < 3; i++)
   {
      g_lava_battle_ui_state_copy.party_hp[i] = state->party_hp[i];
      g_lava_battle_ui_state_copy.party_hp_max[i] = state->party_hp_max[i];
      g_lava_battle_ui_state_copy.party_defending[i] = state->party_defending[i];
      g_lava_battle_ui_state_copy.party_pose[i] = state->party_pose[i];
   }
   strcpy(g_lava_battle_ui_state_copy.message, state->message);
   for (i = 0; i < LAVA_BATTLE_ROUND_SLOTS; i++)
   {
      g_lava_battle_ui_state_copy.round_result[i] = state->round_result[i];
   }
   g_lava_battle_ui_state_valid = 1;
}

static void PAL_BattleUIClearPartyPoses(void)
{
   int i;

   if (!g_lava_battle_ui_state_valid)
   {
      return;
   }

   for (i = 0; i < 3; i++)
   {
      g_lava_battle_ui_state_copy.party_pose[i] = 0;
   }
}

static void PAL_BattleUISetPartyPose(int party_index, int frame)
{
   if (!g_lava_battle_ui_state_valid)
   {
      return;
   }
   if (party_index < 0 || party_index >= 3)
   {
      return;
   }

   g_lava_battle_ui_state_copy.party_pose[party_index] = frame;
}

static void PAL_BattleUIClearMagicVisualState(void)
{
   int i;

   for (i = 0; i < 3; i++)
   {
      g_lava_battle_player_color_shift[i] = 0;
      g_lava_battle_player_offset_x[i] = 0;
      g_lava_battle_player_offset_y[i] = 0;
   }
   for (i = 0; i < LAVA_BATTLE_MAX_ENEMIES; i++)
   {
      g_lava_battle_enemy_color_shift[i] = 0;
      g_lava_battle_enemy_offset_x[i] = 0;
      g_lava_battle_enemy_offset_y[i] = 0;
   }
}

static void PAL_BattleUISetPartyOffset(int party_index, int offset_x, int offset_y)
{
   if (party_index < 0 || party_index >= 3)
   {
      return;
   }

   g_lava_battle_player_offset_x[party_index] = offset_x;
   g_lava_battle_player_offset_y[party_index] = offset_y;
}

static int PAL_BattleUIFindEnemyAttacker(void)
{
   int enemy_object_id;
   int i;

   enemy_object_id = PAL_LavaFightGetRoundEnemyObjectID();
   if (enemy_object_id <= 0 || !g_lava_battle_ui_state_valid)
   {
      return -1;
   }

   for (i = 0; i < g_lava_battle_ui_state_copy.enemy_count && i < LAVA_BATTLE_MAX_ENEMIES; i++)
   {
      if (g_lava_battle_ui_state_copy.enemy_object_id[i] == enemy_object_id)
      {
         return i;
      }
   }

   return -1;
}

static void PAL_BattleUIPlayPlayerAttackMotion(int player_index, int target)
{
   int dx;
   int dy;
   int enemy_x;
   int enemy_y;
   int player_x;
   int player_y;
   int step;
   int steps_x[5];
   int steps_y[5];

   if (!g_lava_battle_ui_state_valid || player_index < 0 || player_index >= 3)
   {
      return;
   }

   player_x = 0;
   player_y = 0;
   if (g_lava_party_count <= 1)
   {
      player_x = 240;
      player_y = 170;
   }
   else if (g_lava_party_count == 2)
   {
      player_x = player_index == 0 ? 200 : 256;
      player_y = player_index == 0 ? 176 : 152;
   }
   else
   {
      player_x = player_index == 0 ? 180 : (player_index == 1 ? 234 : 270);
      player_y = player_index == 0 ? 180 : (player_index == 1 ? 170 : 146);
   }

   if (target >= 0)
   {
      PAL_LavaBattleGetEnemyPos(target, &enemy_x, &enemy_y);
   }
   else
   {
      enemy_x = 150;
      enemy_y = 100;
   }
   if (enemy_x <= 0)
   {
      enemy_x = 150;
      enemy_y = 100;
   }

   dx = enemy_x + 58 - player_x;
   dy = enemy_y + 18 - player_y;
   steps_x[0] = dx / 3;
   steps_y[0] = dy / 3;
   steps_x[1] = dx * 2 / 3;
   steps_y[1] = dy * 2 / 3;
   steps_x[2] = dx;
   steps_y[2] = dy;
   steps_x[3] = dx * 2 / 3;
   steps_y[3] = dy * 2 / 3;
   steps_x[4] = 0;
   steps_y[4] = 0;

   for (step = 0; step < 5; step++)
   {
      PAL_BattleUISetPartyPose(player_index, step < 2 ? 8 : 9);
      PAL_BattleUISetPartyOffset(player_index, steps_x[step], steps_y[step]);
      if (step == 2)
      {
         PAL_BattleUIApplyMagicHitFeedback(target, FALSE, FALSE, 1);
      }
      PAL_BattleUIRedrawRoundFrame(0);
      Delay(step == 2 ? 40 : 25);
      if (step == 2)
      {
         PAL_BattleUIClearMagicVisualState();
      }
   }
   PAL_BattleUIClearPartyPoses();
   PAL_BattleUISetPartyOffset(player_index, 0, 0);
}

static void PAL_BattleUIPlayEnemyAttackMotion(int target)
{
   int enemy_index;
   int step;
   int steps_x[5];
   int steps_y[5];

   enemy_index = PAL_BattleUIFindEnemyAttacker();
   if (enemy_index < 0)
   {
      return;
   }

   steps_x[0] = 14;
   steps_y[0] = 2;
   steps_x[1] = 28;
   steps_y[1] = 5;
   steps_x[2] = 36;
   steps_y[2] = 7;
   steps_x[3] = 18;
   steps_y[3] = 3;
   steps_x[4] = 0;
   steps_y[4] = 0;

   for (step = 0; step < 5; step++)
   {
      g_lava_battle_enemy_offset_x[enemy_index] = steps_x[step];
      g_lava_battle_enemy_offset_y[enemy_index] = steps_y[step];
      if (step == 2)
      {
         g_lava_battle_enemy_color_shift[enemy_index] = 4;
         PAL_BattleUISetPartyPose(target, 4);
      }
      PAL_BattleUIRedrawRoundFrame(0);
      Delay(step == 2 ? 40 : 25);
      if (step == 2)
      {
         g_lava_battle_enemy_color_shift[enemy_index] = 0;
      }
   }
   g_lava_battle_enemy_offset_x[enemy_index] = 0;
   g_lava_battle_enemy_offset_y[enemy_index] = 0;
}

static void PAL_BattleUIPlayPlayerFleeSuccessMotion(void)
{
   int i;
   int offset_x;
   int offset_y;
   int step;

   if (!g_lava_battle_ui_state_valid)
   {
      return;
   }

   PAL_BattleUIClearPartyPoses();
   for (step = 1; step <= 16; step++)
   {
      for (i = 0; i < g_lava_party_count && i < 3; i++)
      {
         if (g_lava_battle_ui_state_copy.party_hp[i] <= 0)
         {
            continue;
         }
         if (i == 0 && g_lava_party_count > 1)
         {
            offset_x = step * 4;
            offset_y = step * 6;
         }
         else if (i == 2)
         {
            offset_x = step * 6;
            offset_y = step * 3;
         }
         else
         {
            offset_x = step * 4;
            offset_y = step * 4;
         }
         PAL_BattleUISetPartyPose(i, 0);
         PAL_BattleUISetPartyOffset(i, offset_x, offset_y);
      }
      PAL_BattleUIRedrawRoundFrame(0);
      Delay(10);
   }
   for (i = 0; i < g_lava_party_count && i < 3; i++)
   {
      PAL_BattleUISetPartyOffset(i, 9999, 9999);
   }
   PAL_BattleUIRedrawRoundFrame(0);
}

static void PAL_BattleUIPlayPlayerFleeFailMotion(int player_index)
{
   int step;

   if (!g_lava_battle_ui_state_valid || player_index < 0 || player_index >= 3)
   {
      return;
   }

   PAL_BattleUISetPartyPose(player_index, 0);
   for (step = 1; step <= 3; step++)
   {
      PAL_BattleUISetPartyOffset(player_index, step * 4, step * 2);
      PAL_BattleUIRedrawRoundFrame(0);
      Delay(45);
   }
   PAL_BattleUISetPartyPose(player_index, 1);
}

static void PAL_BattleUIPlayPlayerUseItemMotion(int player_index, int target, char *message)
{
   int color_shift;
   int i;

   if (!g_lava_battle_ui_state_valid || player_index < 0 || player_index >= 3)
   {
      return;
   }

   PAL_BattleUIRedrawRoundFrame(message);
   Delay(160);
   PAL_BattleUISetPartyOffset(player_index, -15, -7);
   PAL_BattleUISetPartyPose(player_index, 5);
   for (color_shift = 0; color_shift <= 6; color_shift++)
   {
      if (target < 0)
      {
         for (i = 0; i < g_lava_party_count && i < 3; i++)
         {
            g_lava_battle_player_color_shift[i] = color_shift;
         }
      }
      else if (target < 3)
      {
         g_lava_battle_player_color_shift[target] = color_shift;
      }
      PAL_BattleUIRedrawRoundFrame(message);
      Delay(45);
   }
   for (color_shift = 5; color_shift >= 0; color_shift--)
   {
      if (target < 0)
      {
         for (i = 0; i < g_lava_party_count && i < 3; i++)
         {
            g_lava_battle_player_color_shift[i] = color_shift;
         }
      }
      else if (target < 3)
      {
         g_lava_battle_player_color_shift[target] = color_shift;
      }
      PAL_BattleUIRedrawRoundFrame(message);
      Delay(45);
   }
   PAL_BattleUIClearMagicVisualState();
   PAL_BattleUIClearPartyPoses();
}

static void PAL_BattleUIPlayPlayerThrowItemMotion(int player_index, char *message)
{
   int offset_x;
   int offset_y;
   int step;

   if (!g_lava_battle_ui_state_valid || player_index < 0 || player_index >= 3)
   {
      return;
   }

   offset_x = 0;
   offset_y = 0;
   for (step = 0; step < 4; step++)
   {
      offset_x -= 4 - step;
      offset_y -= (4 - step) / 2;
      PAL_BattleUISetPartyOffset(player_index, offset_x, offset_y);
      PAL_BattleUIRedrawRoundFrame(0);
      Delay(45);
   }
   PAL_BattleUIRedrawRoundFrame(message);
   Delay(90);
   PAL_BattleUISetPartyPose(player_index, 5);
   PAL_BattleUIRedrawRoundFrame(message);
   Delay(320);
   PAL_BattleUISetPartyPose(player_index, 6);
   PAL_BattleUIRedrawRoundFrame(message);
   Delay(90);
   PAL_BattleUISetPartyOffset(player_index, 0, 0);
   PAL_BattleUIClearPartyPoses();
}

static void PAL_BattleUISetPartyColorShiftAll(int color_shift)
{
   int i;

   for (i = 0; i < g_lava_party_count && i < 3; i++)
   {
      if (!g_lava_battle_ui_state_valid || g_lava_battle_ui_state_copy.party_hp[i] > 0)
      {
         g_lava_battle_player_color_shift[i] = color_shift;
      }
   }
}

static void PAL_BattleUIRedrawRoundFrame(char *message)
{
   if (!g_lava_battle_ui_state_valid)
   {
      return;
   }

   PAL_BattleUIDrawFrame(&g_lava_battle_ui_state_copy,
      LAVA_BATTLE_UI_STATE_COMMAND, -1, message);
   VIDEO_UpdateScreen(0);
}

static int PAL_BattleUIEnemyLineH(int enemy_count)
{
   if (enemy_count >= 4)
   {
      return 12;
   }
   if (enemy_count >= 3)
   {
      return 14;
   }
   return 16;
}

static int PAL_BattleUIPartyBaseY(int enemy_count)
{
   if (enemy_count >= 4)
   {
      return 120;
   }
   if (enemy_count >= 3)
   {
      return 116;
   }
   return 110;
}

static int PAL_BattleUIPartyLineH(int enemy_count)
{
   if (enemy_count >= 4)
   {
      return 14;
   }
   return 16;
}

static void PAL_BattleUIDrawNumberSprite(addr sprite, int value, int digits, int x, int y, int color, int align);
static void PAL_BattleUIFormatEnemyName(char *enemy_name, int enemy_name_size, int enemy_object_id);

void PAL_BattleUIRunCommandState(LAVA_BATTLE_STATE *state)
{
   if (state == 0)
   {
      return;
   }

   PAL_BattleUISyncCurrentState(state);
   state->round_command = PAL_BattleUIWaitForPlayerAction(state);
}

void PAL_BattleUIPlayRoundState(LAVA_BATTLE_STATE *state)
{
    if (state == 0)
    {
      return;
   }

    PAL_BattleUISyncCurrentState(state);
    PAL_BattleUIShowRoundResult(state->round_result);
}

static void PAL_BattleUIDrawVictoryRewards(LAVA_BATTLE_STATE *state, char *message)
{
   if (state == 0)
   {
      return;
   }

   PAL_BattleUIDrawFrame(&g_lava_battle_ui_state_copy,
      LAVA_BATTLE_UI_STATE_COMMAND, -1, 0);
   PAL_LavaDrawSingleLineBox(83, 60, 8);
   PAL_LavaDrawSingleLineBox(65, 105, 10);
   PAL_LavaDrawShadowText(95, 70, "获得经验值", 0);
   PAL_LavaDrawShadowText(77, 115, "打败敌人得", 0);
   PAL_LavaDrawShadowText(197, 115, "文钱", 0);
   PAL_LavaDrawNumberText(197, 71, state->exp_gained, 0x2C);
   PAL_LavaDrawNumberText(177, 116, state->cash_gained, 0x2C);
   VIDEO_UpdateScreen(0);
   PAL_ClearKeyState();
   while (!PAL_LavaReadConfirmKey())
   {
      PAL_ProcessEvent();
      Delay(10);
   }
   PAL_ClearKeyState();
}

static void PAL_BattleUIDrawLevelUpPage(LAVA_BATTLE_STATE *state, int index)
{
   addr sprite;
   int j;
   int role;

   if (state == 0 || index < 0 || index >= state->level_up_role_count)
   {
      return;
   }

   role = state->level_up_role[index];
   sprite = PAL_LavaLoadUISprite();

   PAL_BattleUIDrawFrame(&g_lava_battle_ui_state_copy,
      LAVA_BATTLE_UI_STATE_COMMAND, -1, 0);
   PAL_LavaDrawSingleLineBox(80, 0, 10);
   if (!PAL_LavaDrawSpriteBoxAt(sprite, 82, 32, 7, 8, 1))
   {
      PAL_LavaDrawDirectSingleLineBox(82, 32, 176);
   }
   PAL_LavaDrawShadowText(100, 10, PAL_LavaRoleName(role), 0x2C);
   PAL_LavaDrawShadowText(164, 10, "等级提升", 0x2C);

   for (j = 0; j < 8; j++)
   {
      if (sprite != 0)
      {
         PAL_RLEBlitToSurface(PAL_SpriteGetFrame(sprite, LAVA_BATTLE_SPRITENUM_ARROW),
            gpScreen, PAL_XY(180, 48 + 18 * j));
      }
      else
      {
         PAL_LavaDrawShadowText(180, 48 + 18 * j, ">", 0x2C);
      }
   }

   PAL_LavaDrawShadowText(100, 44, "等级", 0x2C);
   PAL_LavaDrawShadowText(100, 62, "体力", 0x2C);
   PAL_LavaDrawShadowText(100, 80, "真气", 0x2C);
   PAL_LavaDrawShadowText(100, 98, "武术", 0x2C);
   PAL_LavaDrawShadowText(100, 116, "灵力", 0x2C);
   PAL_LavaDrawShadowText(100, 134, "防御", 0x2C);
   PAL_LavaDrawShadowText(100, 152, "身法", 0x2C);
   PAL_LavaDrawShadowText(100, 170, "吉运", 0x2C);

   PAL_BattleUIDrawNumberSprite(sprite, state->level_up_before[0 * 3 + index], 4, 133, 47, LAVA_BATTLE_NUM_COLOR_YELLOW, LAVA_BATTLE_NUM_ALIGN_RIGHT);
   PAL_BattleUIDrawNumberSprite(sprite, state->level_up_after[0 * 3 + index], 4, 195, 47, LAVA_BATTLE_NUM_COLOR_YELLOW, LAVA_BATTLE_NUM_ALIGN_RIGHT);

   PAL_BattleUIDrawNumberSprite(sprite, state->level_up_before[1 * 3 + index], 4, 133, 64, LAVA_BATTLE_NUM_COLOR_YELLOW, LAVA_BATTLE_NUM_ALIGN_RIGHT);
   PAL_BattleUIDrawNumberSprite(sprite, state->level_up_before[2 * 3 + index], 4, 154, 68, LAVA_BATTLE_NUM_COLOR_BLUE, LAVA_BATTLE_NUM_ALIGN_RIGHT);
   PAL_BattleUIDrawNumberSprite(sprite, state->level_up_after[1 * 3 + index], 4, 195, 64, LAVA_BATTLE_NUM_COLOR_YELLOW, LAVA_BATTLE_NUM_ALIGN_RIGHT);
   PAL_BattleUIDrawNumberSprite(sprite, state->level_up_after[2 * 3 + index], 4, 216, 68, LAVA_BATTLE_NUM_COLOR_BLUE, LAVA_BATTLE_NUM_ALIGN_RIGHT);

   PAL_BattleUIDrawNumberSprite(sprite, state->level_up_before[3 * 3 + index], 4, 133, 82, LAVA_BATTLE_NUM_COLOR_YELLOW, LAVA_BATTLE_NUM_ALIGN_RIGHT);
   PAL_BattleUIDrawNumberSprite(sprite, state->level_up_before[4 * 3 + index], 4, 154, 86, LAVA_BATTLE_NUM_COLOR_BLUE, LAVA_BATTLE_NUM_ALIGN_RIGHT);
   PAL_BattleUIDrawNumberSprite(sprite, state->level_up_after[3 * 3 + index], 4, 195, 82, LAVA_BATTLE_NUM_COLOR_YELLOW, LAVA_BATTLE_NUM_ALIGN_RIGHT);
   PAL_BattleUIDrawNumberSprite(sprite, state->level_up_after[4 * 3 + index], 4, 216, 86, LAVA_BATTLE_NUM_COLOR_BLUE, LAVA_BATTLE_NUM_ALIGN_RIGHT);

   PAL_BattleUIDrawNumberSprite(sprite, state->level_up_before[5 * 3 + index], 4, 133, 101, LAVA_BATTLE_NUM_COLOR_YELLOW, LAVA_BATTLE_NUM_ALIGN_RIGHT);
   PAL_BattleUIDrawNumberSprite(sprite, state->level_up_after[5 * 3 + index], 4, 195, 101, LAVA_BATTLE_NUM_COLOR_YELLOW, LAVA_BATTLE_NUM_ALIGN_RIGHT);
   PAL_BattleUIDrawNumberSprite(sprite, state->level_up_before[6 * 3 + index], 4, 133, 119, LAVA_BATTLE_NUM_COLOR_YELLOW, LAVA_BATTLE_NUM_ALIGN_RIGHT);
   PAL_BattleUIDrawNumberSprite(sprite, state->level_up_after[6 * 3 + index], 4, 195, 119, LAVA_BATTLE_NUM_COLOR_YELLOW, LAVA_BATTLE_NUM_ALIGN_RIGHT);
   PAL_BattleUIDrawNumberSprite(sprite, state->level_up_before[7 * 3 + index], 4, 133, 137, LAVA_BATTLE_NUM_COLOR_YELLOW, LAVA_BATTLE_NUM_ALIGN_RIGHT);
   PAL_BattleUIDrawNumberSprite(sprite, state->level_up_after[7 * 3 + index], 4, 195, 137, LAVA_BATTLE_NUM_COLOR_YELLOW, LAVA_BATTLE_NUM_ALIGN_RIGHT);
   PAL_BattleUIDrawNumberSprite(sprite, state->level_up_before[8 * 3 + index], 4, 133, 155, LAVA_BATTLE_NUM_COLOR_YELLOW, LAVA_BATTLE_NUM_ALIGN_RIGHT);
   PAL_BattleUIDrawNumberSprite(sprite, state->level_up_after[8 * 3 + index], 4, 195, 155, LAVA_BATTLE_NUM_COLOR_YELLOW, LAVA_BATTLE_NUM_ALIGN_RIGHT);
   PAL_BattleUIDrawNumberSprite(sprite, state->level_up_before[9 * 3 + index], 4, 133, 173, LAVA_BATTLE_NUM_COLOR_YELLOW, LAVA_BATTLE_NUM_ALIGN_RIGHT);
   PAL_BattleUIDrawNumberSprite(sprite, state->level_up_after[9 * 3 + index], 4, 195, 173, LAVA_BATTLE_NUM_COLOR_YELLOW, LAVA_BATTLE_NUM_ALIGN_RIGHT);

   if (sprite != 0)
   {
      PAL_RLEBlitToSurface(PAL_SpriteGetFrame(sprite, LAVA_BATTLE_SPRITENUM_SLASH), gpScreen, PAL_XY(156, 66));
      PAL_RLEBlitToSurface(PAL_SpriteGetFrame(sprite, LAVA_BATTLE_SPRITENUM_SLASH), gpScreen, PAL_XY(218, 66));
      PAL_RLEBlitToSurface(PAL_SpriteGetFrame(sprite, LAVA_BATTLE_SPRITENUM_SLASH), gpScreen, PAL_XY(156, 84));
      PAL_RLEBlitToSurface(PAL_SpriteGetFrame(sprite, LAVA_BATTLE_SPRITENUM_SLASH), gpScreen, PAL_XY(218, 84));
   }
   VIDEO_UpdateScreen(0);
   PAL_ClearKeyState();
   while (!PAL_LavaReadConfirmKey())
   {
      PAL_ProcessEvent();
      Delay(10);
   }
   PAL_ClearKeyState();
}


void PAL_BattleUIFinishBattleState(LAVA_BATTLE_STATE *state)
{
   int i;

   if (state == 0)
   {
      PAL_BattleUIFinishBattle(LAVA_BATTLE_RESULT_LOSE, 0);
      return;
   }

   PAL_BattleUISyncCurrentState(state);
   if (state->battle_result == LAVA_BATTLE_RESULT_WIN)
   {
      PAL_BattleUIDrawVictoryRewards(state, state->message);
      PAL_LavaFightApplyBattleRewards(state);
      for (i = 0; i < state->level_up_role_count; i++)
      {
         PAL_BattleUIDrawLevelUpPage(state, i);
      }
      PAL_ClearKeyState();
      return;
   }
   PAL_BattleUIFinishBattle(state->battle_result, state->message);
}

void PAL_BattleUIBeginBattle(void)
{
   VIDEO_UpdateScreen(0);
   PAL_BattleUIClearMessageLog();
   PAL_BattleUIClearFloatingNumbers();
   g_lava_battle_ui_pending_fade_in = 0;
   g_lava_battle_ui_autotest_turn = 0;
   g_lava_battle_ui_autotest_magic_done = 0;
   g_lava_battle_ui_enemy_name_logged = 0;
   g_lava_battle_ui_magic_name_logged = 0;
   PAL_ClearKeyState();
}

void PAL_BattleUIFinishBattle(int battle_result, char *message)
{
   if (battle_result == LAVA_BATTLE_RESULT_WIN &&
       message != 0 &&
       message[0] != 0 &&
       battle_result != LAVA_BATTLE_RESULT_CONTINUE &&
       battle_result != LAVA_BATTLE_RESULT_FLEE)
   {
      PAL_BattleUIDrawFrame(&g_lava_battle_ui_state_copy,
         LAVA_BATTLE_UI_STATE_COMMAND, -1, 0);
      PAL_LavaDrawDirectSingleLineBox(72, 72, 176);
      PAL_LavaDrawShadowText(104, 82, message, 0x1C);
      if (battle_result == LAVA_BATTLE_RESULT_WIN)
      {
         PAL_LavaDrawShadowText(90, 100, "获得战斗胜利", 0x2C);
      }
      VIDEO_UpdateScreen(0);
      Delay(900);
   }
   PAL_ClearKeyState();
}

static void PAL_BattleUIClearMessageLog(void)
{
   int i;

   g_lava_battle_message_log_count = 0;
   for (i = 0; i < LAVA_BATTLE_MSG_LINES; i++)
   {
      g_lava_battle_message_log[i][0] = 0;
   }
}

static void PAL_BattleUIAppendMessageLog(char *text)
{
   int i;
   int last;

   if (text == 0 || text[0] == 0)
   {
      return;
   }

   if (g_lava_battle_message_log_count > 0)
   {
      last = g_lava_battle_message_log_count - 1;
      if (strcmp(g_lava_battle_message_log[last], text) == 0)
      {
         return;
      }
   }

   if (g_lava_battle_message_log_count >= LAVA_BATTLE_MSG_LINES)
   {
      for (i = 1; i < LAVA_BATTLE_MSG_LINES; i++)
      {
         strcpy(g_lava_battle_message_log[i - 1], g_lava_battle_message_log[i]);
      }
      g_lava_battle_message_log_count = LAVA_BATTLE_MSG_LINES - 1;
   }

   for (i = 0; i < LAVA_BATTLE_MSG_TEXT_MAX - 1 && text[i] != 0; i++)
   {
      g_lava_battle_message_log[g_lava_battle_message_log_count][i] = text[i];
   }
   g_lava_battle_message_log[g_lava_battle_message_log_count][i] = 0;
   g_lava_battle_message_log_count++;
}

static void PAL_BattleUIDrawMessageLog(void)
{
   int i;
   int color;

   color = PAL_LavaNearestPaletteColor(LAVA_BATTLE_MSG_GRAY,
      LAVA_BATTLE_MSG_GRAY, LAVA_BATTLE_MSG_GRAY);

   for (i = 0; i < g_lava_battle_message_log_count; i++)
   {
      PAL_LavaDrawShadowTextSmallRight(LAVA_BATTLE_MSG_RIGHT_X,
         LAVA_BATTLE_MSG_Y_TOP + i * LAVA_BATTLE_MSG_LINE_H,
         g_lava_battle_message_log[i], color);
   }
}

void PAL_BattleUIShowText(char *text)
{
   PAL_BattleUIAppendMessageLog(text);
   PAL_BattleUIDrawMessageLog();
}


void PAL_BattleUIShowNum(int x, int y, int value, int color)
{
   char buf[16];

   sprintf(buf, "%d", value);
   PAL_LavaDrawShadowText(x, y, buf, color);
}

static void PAL_BattleUIClearFloatingNumbers(void)
{
   int i;

   for (i = 0; i < LAVA_BATTLE_FLOAT_NUM_COUNT; i++)
   {
      g_lava_battle_float_num_value[i] = 0;
      g_lava_battle_float_num_x[i] = 0;
      g_lava_battle_float_num_y[i] = 0;
      g_lava_battle_float_num_color[i] = 0;
      g_lava_battle_float_num_frame[i] = 0;
   }
}

static void PAL_BattleUIShowFloatingNumber(int x, int y, int value, int color)
{
   int i;

   if (value < 0)
   {
      value = -value;
   }
   if (value == 0)
   {
      return;
   }

   for (i = 0; i < LAVA_BATTLE_FLOAT_NUM_COUNT; i++)
   {
      if (g_lava_battle_float_num_value[i] == 0)
      {
         g_lava_battle_float_num_value[i] = value;
         g_lava_battle_float_num_x[i] = x - 15;
         g_lava_battle_float_num_y[i] = y;
         g_lava_battle_float_num_color[i] = color;
         g_lava_battle_float_num_frame[i] = 0;
         return;
      }
   }
}

static void PAL_BattleUIDrawFloatingNumbers(void)
{
   int i;

   for (i = 0; i < LAVA_BATTLE_FLOAT_NUM_COUNT; i++)
   {
      if (g_lava_battle_float_num_value[i] == 0)
      {
         continue;
      }
      if (g_lava_battle_float_num_frame[i] > LAVA_BATTLE_FLOAT_NUM_FRAMES)
      {
         g_lava_battle_float_num_value[i] = 0;
         continue;
      }
      PAL_BattleUIShowNum(g_lava_battle_float_num_x[i],
         g_lava_battle_float_num_y[i] - g_lava_battle_float_num_frame[i],
         g_lava_battle_float_num_value[i],
         g_lava_battle_float_num_color[i]);
   }
}

static void PAL_BattleUIAdvanceFloatingNumbers(void)
{
   int i;

   for (i = 0; i < LAVA_BATTLE_FLOAT_NUM_COUNT; i++)
   {
      if (g_lava_battle_float_num_value[i] != 0)
      {
         g_lava_battle_float_num_frame[i]++;
         if (g_lava_battle_float_num_frame[i] > LAVA_BATTLE_FLOAT_NUM_FRAMES)
         {
            g_lava_battle_float_num_value[i] = 0;
         }
      }
   }
}

static void PAL_BattleUIPlayFloatingNumber(int x, int y, int value, int color)
{
   int frame;

   PAL_BattleUIShowFloatingNumber(x, y, value, color);
   for (frame = 0; frame <= LAVA_BATTLE_FLOAT_NUM_FRAMES; frame++)
   {
      PAL_BattleUIRedrawRoundFrame(0);
      Delay(LAVA_BATTLE_FLOAT_NUM_FRAME_MS);
      PAL_BattleUIAdvanceFloatingNumbers();
   }
}

static void PAL_BattleUIDrawTargetMarker(int left_x, int y, int right_x, int selected, int alive)
{
   int color;

   if (!selected || !alive)
   {
      return;
   }

   color = MENUITEM_COLOR_SELECTED;
   PAL_LavaDrawShadowText(left_x, y, ">", color);
   PAL_LavaDrawShadowText(right_x, y, "<", color);
}

static void PAL_BattleUIBuildPlayerStatusTags(int role, char *buf, int buf_size)
{
   int i;
   int used;
   int tag_len;

   if (buf == 0 || buf_size <= 0)
   {
      return;
   }

   buf[0] = 0;
   used = 0;
   for (i = 0; i < LAVA_BATTLE_STATUS_COUNT; i++)
   {
      char *tag;

      if (PAL_LavaBattleGetPlayerStatus(role, i) <= 0)
      {
         continue;
      }
      tag = PAL_LavaBattleStatusShortName(i);
      if (tag == 0 || tag[0] == 0)
      {
         continue;
      }
      tag_len = strlen(tag);
      if (used + tag_len + 3 > buf_size)
      {
         break;
      }
      buf[used++] = '[';
      memcpy(buf + used, tag, tag_len);
      used += tag_len;
      buf[used++] = ']';
      buf[used] = 0;
   }
}

static void PAL_BattleUICopyText(char *dst, int dst_size, char *src)
{
   int copy_len;

   if (dst == 0 || dst_size <= 0)
   {
      return;
   }
   if (src == 0)
   {
      dst[0] = 0;
      return;
   }

   copy_len = strlen(src);
   if (copy_len >= dst_size)
   {
      copy_len = dst_size - 1;
   }
   memcpy(dst, src, copy_len);
   dst[copy_len] = 0;
}

static void PAL_BattleUIDrawNumberSprite(addr sprite, int value, int digits, int x, int y, int color, int align)
{
   int actual;
   int frame_base;
   int i;
   int n;
   int draw_x;

   if (sprite == 0)
   {
      return;
   }

   frame_base = (color == LAVA_BATTLE_NUM_COLOR_BLUE) ? 29 : ((color == LAVA_BATTLE_NUM_COLOR_CYAN) ? 56 : 19);
   n = value;
   actual = 0;
   while (n > 0)
   {
      n /= 10;
      actual++;
   }
   if (actual > digits)
   {
      actual = digits;
   }
   else if (actual == 0)
   {
      actual = 1;
   }

   draw_x = x - 6;
   if (align == LAVA_BATTLE_NUM_ALIGN_LEFT)
   {
      draw_x += 6 * actual;
   }
   else if (align == LAVA_BATTLE_NUM_ALIGN_MID)
   {
      draw_x += 3 * (digits + actual);
   }
   else
   {
      draw_x += 6 * digits;
   }

   for (i = 0; i < actual; i++)
   {
      PAL_RLEBlitToSurface(PAL_SpriteGetFrame(sprite, frame_base + value % 10),
         gpScreen, PAL_XY(draw_x, y));
      draw_x -= 6;
      value /= 10;
   }
}

static void PAL_BattleUIFillRect(int x, int y, int w, int h, int color)
{
   char *pixels;
   int draw_x;
   int draw_y;
   int end_x;
   int end_y;
   int pitch;

   if (w <= 0 || h <= 0)
   {
      return;
   }

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
   if (x >= g_lava_gpScreen.w || y >= g_lava_gpScreen.h || w <= 0 || h <= 0)
   {
      return;
   }
   if (x + w > g_lava_gpScreen.w)
   {
      w = g_lava_gpScreen.w - x;
   }
   if (y + h > g_lava_gpScreen.h)
   {
      h = g_lava_gpScreen.h - y;
   }

   pixels = (char *)g_lava_gpScreen.pixels;
   pitch = g_lava_gpScreen.pitch;
   end_x = x + w;
   end_y = y + h;
   for (draw_y = y; draw_y < end_y; draw_y++)
   {
      for (draw_x = x; draw_x < end_x; draw_x++)
      {
         pixels[draw_y * pitch + draw_x] = (char)color;
      }
   }
}

void PAL_PlayerInfoBox(int x, int y, int role, int hp, int hp_max, int selected, int acting, int trance_count)
{
   int meter;
   int mp;
   int mp_max;
   addr sprite;

   sprite = PAL_LavaLoadUISprite();
   if (sprite == 0)
   {
      char line[64];
      int color;

      color = selected ? MENUITEM_COLOR_SELECTED : (acting ? 0x2C : 0x4F);
      sprintf(line, "%s %d/%d", PAL_LavaRoleName(role), hp, hp_max);
      PAL_LavaDrawShadowText(x, y, line, color);
      return;
   }

   if (hp < 0)
   {
      hp = 0;
   }
   if (hp_max < 0)
   {
      hp_max = 0;
   }

   mp = PAL_LavaRoleWordByArray(10, role);
   mp_max = PAL_LavaRoleWordByArray(8, role);
   if (mp < 0)
   {
      mp = 0;
   }
   if (mp_max < 0)
   {
      mp_max = 0;
   }

   PAL_RLEBlitToSurface(PAL_SpriteGetFrame(sprite, LAVA_BATTLE_SPRITENUM_PLAYERINFOBOX),
      gpScreen, PAL_XY(x, y));
   if (hp <= 0)
   {
      PAL_RLEBlitMonoColor(PAL_SpriteGetFrame(sprite, LAVA_BATTLE_SPRITENUM_PLAYERFACE_FIRST + role),
         gpScreen, PAL_XY(x - 2, y - 4), 0, 0);
   }
   else
   {
      PAL_RLEBlitToSurface(PAL_SpriteGetFrame(sprite, LAVA_BATTLE_SPRITENUM_PLAYERFACE_FIRST + role),
         gpScreen, PAL_XY(x - 2, y - 4));
   }

   PAL_BattleUIFillRect(x + 31, y + 4, 1, 6, 0xBD);
   PAL_BattleUIFillRect(x + 70, y + 4, 1, 6, 0xBD);
   PAL_BattleUIFillRect(x + 32, y + 3, 38, 1, 0xBD);
   PAL_BattleUIFillRect(x + 32, y + 11, 38, 1, 0xBD);

   meter = 0;
   if (hp > 0 && hp_max > 0)
   {
      meter = hp * 100 / hp_max;
      if (meter <= 0)
      {
         meter = 1;
      }
      if (meter > 100)
      {
         meter = 100;
      }
   }
   if (meter > 0)
   {
      PAL_BattleUIFillRect(x + 33, y + 6, meter * 36 / 100, 2,
         (selected || acting) ? MENUITEM_COLOR_SELECTED : LAVA_BATTLE_TIMEMETER_COLOR_DEFAULT);
   }

   PAL_RLEBlitToSurface(PAL_SpriteGetFrame(sprite, LAVA_BATTLE_SPRITENUM_SLASH), gpScreen,
      PAL_XY(x + 49, y + 14));
   PAL_BattleUIDrawNumberSprite(sprite, hp_max, 4, x + 47, y + 16, LAVA_BATTLE_NUM_COLOR_YELLOW, LAVA_BATTLE_NUM_ALIGN_RIGHT);
   PAL_BattleUIDrawNumberSprite(sprite, hp, 4, x + 26, y + 13, LAVA_BATTLE_NUM_COLOR_YELLOW, LAVA_BATTLE_NUM_ALIGN_RIGHT);

   PAL_RLEBlitToSurface(PAL_SpriteGetFrame(sprite, LAVA_BATTLE_SPRITENUM_SLASH), gpScreen,
      PAL_XY(x + 49, y + 24));
   PAL_BattleUIDrawNumberSprite(sprite, mp_max, 4, x + 47, y + 26, LAVA_BATTLE_NUM_COLOR_CYAN, LAVA_BATTLE_NUM_ALIGN_RIGHT);
   PAL_BattleUIDrawNumberSprite(sprite, mp, 4, x + 26, y + 23, LAVA_BATTLE_NUM_COLOR_CYAN, LAVA_BATTLE_NUM_ALIGN_RIGHT);

   if (selected && hp > 0)
   {
      PAL_BattleUIDrawTargetMarker(x - 8, y + 12, x + 72, selected, hp > 0);
   }
}

void PAL_BattleUIDrawEnemyLine(int x, int y, int enemy_object_id, int enemy_index, int hp, int hp_max, int selected)
{
   char enemy_name[24];
   char line[48];
   int color;

   color = selected ? MENUITEM_COLOR_SELECTED : 0x4F;
   if (hp <= 0)
   {
      color = 0x1C;
   }
   PAL_BattleUIFormatEnemyName(enemy_name, sizeof(enemy_name), enemy_object_id);
   if (enemy_name[0] == 0)
   {
      sprintf(enemy_name, "敌人%d", enemy_index + 1);
   }
   if (!g_lava_battle_ui_enemy_name_logged &&
       (g_lava_autotest_search || g_lava_autotest_load))
   {
      printf("[LAVA][BATTLEUI] enemy name slot=%d obj=%d text=%s\n",
         enemy_index, enemy_object_id, enemy_name);
      g_lava_battle_ui_enemy_name_logged = 1;
   }
   sprintf(line, "%s HP %d/%d", enemy_name, hp, hp_max);
   PAL_BattleUIDrawTargetMarker(x - 12, y, LAVA_BATTLE_DAMAGE_X + 20, selected, hp > 0);
   PAL_LavaDrawShadowText(x, y, line, color);
   PAL_BattleUIShowNum(LAVA_BATTLE_VALUE_X, y, hp, color);
}

static void PAL_BattleUIFormatEnemyName(char *enemy_name, int enemy_name_size, int enemy_object_id)
{
   char *name;

   if (enemy_name == 0)
   {
      return;
   }
   if (enemy_name_size <= 0)
   {
      return;
   }

   name = PAL_LavaReadWord(enemy_object_id);
   if (name != 0 && name[0] != 0)
   {
      PAL_BattleUICopyText(enemy_name, enemy_name_size, name);
      return;
   }

   sprintf(enemy_name, "敌人");
}

static char *PAL_BattleUIFormatMagicName(int magic_object_id)
{
   char *name;

   name = PAL_LavaReadWord(magic_object_id);
   if (g_lava_autotest_search || g_lava_autotest_load)
   {
      printf("[LAVA][MAGICUI] FormatMagicName obj=%d word_ptr=%d bytes=%02x%02x%02x%02x%02x%02x\n",
         magic_object_id,
         name != 0 ? 1 : 0,
         name != 0 ? (PAL_U8(name[0])) : 0,
         name != 0 ? (PAL_U8(name[1])) : 0,
         name != 0 ? (PAL_U8(name[2])) : 0,
         name != 0 ? (PAL_U8(name[3])) : 0,
         name != 0 ? (PAL_U8(name[4])) : 0,
         name != 0 ? (PAL_U8(name[5])) : 0);
   }
   g_lava_battle_magic_name_buf[0] = 0;
   if (name != 0 && name[0] != 0)
   {
      PAL_BattleUICopyText(g_lava_battle_magic_name_buf, sizeof(g_lava_battle_magic_name_buf), name);
      if (g_lava_battle_magic_name_buf[0] != 0)
      {
         return g_lava_battle_magic_name_buf;
      }
   }

   sprintf(g_lava_battle_magic_name_buf, "仙术%d", magic_object_id);
   return g_lava_battle_magic_name_buf;
}

static int PAL_BattleUIActingRole(LAVA_BATTLE_STATE *state)
{
   if (state == 0)
   {
      return 0;
   }

   if (state->acting_player_index >= 0 &&
       state->acting_player_index < g_lava_party_count &&
       state->acting_player_index < 3)
   {
      return g_lava_party_role[state->acting_player_index];
   }

   return g_lava_party_role[PAL_LavaFightChooseActingPlayer(state->party_hp)];
}

static int PAL_BattleUICoopMagicAvailable(LAVA_BATTLE_STATE *state)
{
   int i;
   int max_hp;
   int role;

   if (state == 0 || g_lava_party_count <= 1)
   {
      return FALSE;
   }

   for (i = 0; i < g_lava_party_count && i < 3; i++)
   {
      role = g_lava_party_role[i];
      max_hp = PAL_LavaRoleWordByArray(7, role);
      if (state->party_hp[i] <= 0 ||
          state->party_hp[i] < max_hp / 5 ||
          PAL_LavaBattleGetPlayerStatus(role, LAVA_BATTLE_STATUS_SLEEP) > 0 ||
          PAL_LavaBattleGetPlayerStatus(role, LAVA_BATTLE_STATUS_CONFUSED) > 0 ||
          PAL_LavaBattleGetPlayerStatus(role, LAVA_BATTLE_STATUS_SILENCE) > 0)
      {
         return FALSE;
      }
   }

   return TRUE;
}

static int PAL_BattleUIClampMagicSel(LAVA_BATTLE_STATE *state)
{
   int magic_count;
   int player_role;

   if (state == 0)
   {
      return 0;
   }

   player_role = PAL_BattleUIActingRole(state);
   magic_count = PAL_LavaFightMagicCount(player_role);
   if (magic_count <= 0)
   {
      state->magic_sel = 0;
      state->magic_object_id = 0;
      return 0;
   }
   if (state->magic_sel < 0)
   {
      state->magic_sel = magic_count - 1;
   }
   if (state->magic_sel >= magic_count)
   {
      state->magic_sel = 0;
   }
      return magic_count;
}

static int PAL_BattleUIClampMagicMenuSel(LAVA_BATTLE_STATE *state)
{
   int magic_count;
   int player_role;

   if (state == 0)
   {
      return 0;
   }

   player_role = PAL_BattleUIActingRole(state);
   magic_count = PAL_LavaFightMagicCount(player_role);
   if (magic_count <= 0)
   {
      state->magic_sel = 0;
      state->magic_object_id = 0;
      return 0;
   }
   if (state->magic_sel < 0)
   {
      state->magic_sel = 0;
   }
   if (state->magic_sel >= magic_count)
   {
      state->magic_sel = magic_count - 1;
   }
   return magic_count;
}

static int PAL_BattleUIMagicPageStart(int magic_sel, int magic_count)
{
   int max_start_row;
   int rows_before;
   int start;
   int start_row;
   int total_rows;

   rows_before = 2;
   start_row = magic_sel / LAVA_BATTLE_MAGIC_ITEMS_PER_LINE - rows_before;
   if (start_row < 0)
   {
      start_row = 0;
   }

   total_rows = (magic_count + LAVA_BATTLE_MAGIC_ITEMS_PER_LINE - 1) /
      LAVA_BATTLE_MAGIC_ITEMS_PER_LINE;
   max_start_row = total_rows - LAVA_BATTLE_MAGIC_LINES;
   if (max_start_row < 0)
   {
      max_start_row = 0;
   }
   if (start_row > max_start_row)
   {
      start_row = max_start_row;
   }

   start = start_row * LAVA_BATTLE_MAGIC_ITEMS_PER_LINE;
   return start;
}

static void PAL_BattleUIMoveMagicSel(LAVA_BATTLE_STATE *state, int delta)
{
   int magic_count;
   int next_sel;

   magic_count = PAL_LavaFightMagicCount(PAL_BattleUIActingRole(state));
   if (magic_count <= 0)
   {
      state->magic_sel = 0;
      return;
   }

   next_sel = state->magic_sel + delta;
   if (next_sel < 0 || next_sel >= magic_count)
   {
      return;
   }
   state->magic_sel = next_sel;
}

static int PAL_BattleUIFindMagicSel(int player_role, int magic_object_id)
{
   int i;
   int magic_count;

   magic_count = PAL_LavaFightMagicCount(player_role);
   for (i = 0; i < magic_count; i++)
   {
      if (PAL_LavaFightMagicByIndex(player_role, i) == magic_object_id)
      {
         return i;
      }
   }
   return -1;
}

static int PAL_BattleUIBestMagicSel(LAVA_BATTLE_STATE *state)
{
   int best_score;
   int best_sel;
   int current_mp;
   int i;
   int magic_count;
   int magic_object_id;
   int mp_cost;
   int player_role;
   int score;

   if (state == 0)
   {
      return -1;
   }
   player_role = PAL_BattleUIActingRole(state);
   current_mp = PAL_LavaRoleWordByArray(10, player_role);
   magic_count = PAL_LavaFightMagicCount(player_role);
   best_sel = -1;
   best_score = -1;
   for (i = 0; i < magic_count; i++)
   {
      magic_object_id = PAL_LavaFightMagicByIndex(player_role, i);
      if (magic_object_id <= 0)
      {
         continue;
      }
      mp_cost = PAL_LavaFightMagicCost(magic_object_id);
      score = PAL_LavaFightMagicBaseDamage(magic_object_id);
      if (mp_cost == 1 || mp_cost > current_mp || score <= 0)
      {
         continue;
      }
      score += RandomLong(0, 60);
      if (score > best_score)
      {
         best_score = score;
         best_sel = i;
      }
   }
   return best_sel;
}

static int PAL_BattleUISelectMagicShortcut(LAVA_BATTLE_STATE *state, int use_last)
{
   int magic_object_id;
   int magic_sel;
   int magic_count;
   int player_role;

   if (state == 0)
   {
      return FALSE;
   }

   player_role = PAL_BattleUIActingRole(state);
   magic_count = PAL_LavaFightMagicCount(player_role);
   if (use_last)
   {
      magic_sel = state->acting_player_index >= 0 && state->acting_player_index < 6 ?
         g_lava_battle_last_magic_sel[state->acting_player_index] : state->magic_sel;
      if (magic_sel < 0 || magic_sel >= magic_count)
      {
         magic_sel = state->magic_sel;
      }
   }
   else
   {
      magic_sel = PAL_BattleUIBestMagicSel(state);
   }

   magic_object_id = magic_sel >= 0 ? PAL_LavaFightMagicByIndex(player_role, magic_sel) : 0;
   printf("[LAVA][HOTKEY] ui action=%s actor=%d role=%d mp=%d count=%d sel=%d obj=%d\n",
      use_last ? "last" : "best",
      state->acting_player_index,
      player_role,
      PAL_LavaRoleWordByArray(10, player_role),
      magic_count,
      magic_sel,
      magic_object_id);
   if (magic_object_id <= 0)
   {
      return FALSE;
   }

   state->command_sel = LAVA_BATTLE_COMMAND_MAGIC;
   state->magic_sel = magic_sel;
   state->magic_object_id = magic_object_id;
   if (state->acting_player_index >= 0 && state->acting_player_index < 6)
   {
      g_lava_battle_last_magic_sel[state->acting_player_index] = state->magic_sel;
   }
   return TRUE;
}

static int PAL_BattleUIStartMagicShortcutTarget(LAVA_BATTLE_STATE *state, int *ui_state)
{
   if (state == 0 || ui_state == 0)
   {
      return FALSE;
   }

   if (PAL_LavaFightMagicNeedsTarget(state->magic_object_id) &&
       PAL_LavaFightCountAlive(state->enemy_hp, state->enemy_count) > 1)
   {
      printf("[LAVA][HOTKEY] target enemy obj=%d alive=%d\n",
         state->magic_object_id,
         PAL_LavaFightCountAlive(state->enemy_hp, state->enemy_count));
      state->target_sel = PAL_LavaFightFixEnemyTarget(
         g_lava_battle_last_attack_target_sel, state->enemy_count, state->enemy_hp);
      *ui_state = LAVA_BATTLE_UI_STATE_TARGET;
      return TRUE;
   }
   if (PAL_LavaFightMagicNeedsPartyTarget(state->magic_object_id))
   {
      printf("[LAVA][HOTKEY] target party obj=%d\n", state->magic_object_id);
      state->target_sel = PAL_LavaFightFixLivingPartyTarget(
         state->target_sel, state->party_hp, state->party_hp_max);
      *ui_state = LAVA_BATTLE_UI_STATE_PARTY_TARGET;
      return TRUE;
   }
   return FALSE;
}

static int PAL_BattleUISelectAutoAction(LAVA_BATTLE_STATE *state, int *ui_state)
{
   if (state == 0 || ui_state == 0)
   {
      return FALSE;
   }
   if (PAL_BattleUISelectMagicShortcut(state, FALSE))
   {
      if (PAL_LavaFightMagicApplyToAll(state->magic_object_id))
      {
         state->target_sel = -1;
      }
      else if (PAL_LavaFightMagicNeedsPartyTarget(state->magic_object_id))
      {
         state->target_sel = PAL_LavaFightFixLivingPartyTarget(
            state->acting_player_index, state->party_hp, state->party_hp_max);
      }
      else
      {
         state->target_sel = PAL_LavaFightFixEnemyTarget(
            g_lava_battle_last_attack_target_sel, state->enemy_count, state->enemy_hp);
      }
      *ui_state = LAVA_BATTLE_UI_STATE_COMMAND;
      return TRUE;
   }
   state->command_sel = LAVA_BATTLE_COMMAND_ATTACK;
   state->target_sel = PAL_LavaFightFixEnemyTarget(
      g_lava_battle_last_attack_target_sel, state->enemy_count, state->enemy_hp);
   *ui_state = LAVA_BATTLE_UI_STATE_COMMAND;
   return TRUE;
}

static int PAL_BattleUIItemField(int item_object_id, int field_index)
{
   return PAL_LavaReadObjectField(item_object_id, field_index);
}

static int PAL_BattleUIItemFlags(int item_object_id)
{
   return PAL_BattleUIItemField(item_object_id, gConfig.fIsWIN95 ? 6 : 5);
}

static int PAL_BattleUIBuildItemList(int item_flag, int *item_ids, int *item_amounts)
{
   int count;
   int flags;
   int i;

   count = 0;
   for (i = 0; i < 256; i++)
   {
      if (g_lava_inventory_item[i] <= 0 || g_lava_inventory_amount[i] <= 0)
      {
         continue;
      }

      flags = PAL_BattleUIItemFlags(g_lava_inventory_item[i]);
      if ((flags & item_flag) == 0)
      {
         continue;
      }

      item_ids[count] = g_lava_inventory_item[i];
      item_amounts[count] = g_lava_inventory_amount[i];
      count++;
   }
   return count;
}

static int PAL_BattleUICurrentItemFlag(LAVA_BATTLE_STATE *state)
{
   if (state != 0 && state->command_sel == LAVA_BATTLE_COMMAND_THROW_ITEM)
   {
      return LAVA_ITEM_FLAG_THROWABLE;
   }
   return LAVA_ITEM_FLAG_USABLE;
}

static int PAL_BattleUIClampItemSel(LAVA_BATTLE_STATE *state)
{
   int item_amounts[256];
   int item_count;
   int item_ids[256];

   if (state == 0)
   {
      return 0;
   }

   item_count = PAL_BattleUIBuildItemList(PAL_BattleUICurrentItemFlag(state), item_ids, item_amounts);
   if (item_count <= 0)
   {
      state->item_sel = 0;
      state->item_object_id = 0;
      return 0;
   }
   if (state->item_sel < 0)
   {
      state->item_sel = item_count - 1;
   }
   if (state->item_sel >= item_count)
   {
      state->item_sel = 0;
   }
   state->item_object_id = item_ids[state->item_sel];
   return item_count;
}

static char *PAL_BattleUIFormatItemName(int item_object_id)
{
   static char buf[32];
   char *name;

   name = PAL_LavaReadWord(item_object_id);
   if (name != 0 && name[0] != 0)
   {
      return name;
   }

   sprintf(buf, "物品%d", item_object_id);
   return buf;
}

static char *PAL_BattleUIBuildItemPrompt(LAVA_BATTLE_STATE *state)
{
   static char buf[96];
   int item_object_id;

   buf[0] = 0;
   if (PAL_BattleUIClampItemSel(state) <= 0)
   {
      sprintf(buf, state != 0 && state->command_sel == LAVA_BATTLE_COMMAND_THROW_ITEM ?
         "暂无可投掷物品" : "暂无可用物品");
      return buf;
   }

   item_object_id = state->item_object_id;
   if (state->command_sel == LAVA_BATTLE_COMMAND_THROW_ITEM)
   {
      if (PAL_BattleUIItemFlags(item_object_id) & LAVA_ITEM_FLAG_APPLY_TO_ALL)
      {
         sprintf(buf, "投掷 %s 全体", PAL_BattleUIFormatItemName(item_object_id));
      }
      else
      {
         sprintf(buf, "投掷 %s 单体", PAL_BattleUIFormatItemName(item_object_id));
      }
   }
   else if (PAL_BattleUIItemFlags(item_object_id) & LAVA_ITEM_FLAG_APPLY_TO_ALL)
   {
      sprintf(buf, "物品 %s 全体", PAL_BattleUIFormatItemName(item_object_id));
   }
   else
   {
      sprintf(buf, "物品 %s 单人", PAL_BattleUIFormatItemName(item_object_id));
   }
   return buf;
}

static char *PAL_BattleUIBuildMagicPrompt(LAVA_BATTLE_STATE *state)
{
   static char buf[96];
   int magic_object_id;
   int mp_cost;

   buf[0] = 0;
   if (PAL_BattleUIClampMagicSel(state) <= 0)
   {
      sprintf(buf, "尚未习得仙术");
      return buf;
   }

   magic_object_id = PAL_LavaFightMagicByIndex(PAL_BattleUIActingRole(state), state->magic_sel);
   if (magic_object_id <= 0)
   {
      sprintf(buf, "尚未习得仙术");
      return buf;
   }

   mp_cost = PAL_LavaFightMagicCost(magic_object_id);
   if (!PAL_LavaFightMagicUsableToEnemy(magic_object_id))
   {
      if (PAL_LavaFightMagicType(magic_object_id) == 8)
      {
         sprintf(buf, "仙术 %s 耗气%d 入定", PAL_BattleUIFormatMagicName(magic_object_id), mp_cost);
      }
      else
      {
         sprintf(buf, "仙术 %s 耗气%d 辅助", PAL_BattleUIFormatMagicName(magic_object_id), mp_cost);
      }
   }
   else if (PAL_LavaFightMagicApplyToAll(magic_object_id))
   {
      sprintf(buf, "仙术 %s 耗气%d 全体", PAL_BattleUIFormatMagicName(magic_object_id), mp_cost);
   }
   else
   {
      sprintf(buf, "仙术 %s 耗气%d 单体", PAL_BattleUIFormatMagicName(magic_object_id), mp_cost);
   }
    return buf;
}

static char *PAL_BattleUIBuildMiscPrompt(void)
{
   static char buf[64];

   if (g_lava_battle_misc_sel == LAVA_BATTLE_MISC_ITEM)
   {
      sprintf(buf, "选择物品");
   }
   else if (g_lava_battle_misc_sel == LAVA_BATTLE_MISC_DEFEND)
   {
      sprintf(buf, "防御待机");
   }
   else if (g_lava_battle_misc_sel == LAVA_BATTLE_MISC_AUTO)
   {
      sprintf(buf, "自动行动");
   }
   else if (g_lava_battle_misc_sel == LAVA_BATTLE_MISC_FLEE)
   {
      sprintf(buf, "尝试撤退");
   }
   else
   {
      sprintf(buf, "查看状态");
   }
   return buf;
}

static char *PAL_BattleUIBuildPartyTargetPrompt(LAVA_BATTLE_STATE *state)
{
   static char buf[96];
   int party_index;

   buf[0] = 0;
   if (state == 0)
   {
      return buf;
   }

   party_index = PAL_LavaFightFixLivingPartyTarget(state->target_sel, state->party_hp, state->party_hp_max);
   sprintf(buf, "同伴 %s %d/%d", PAL_LavaRoleName(g_lava_party_role[party_index]),
      state->party_hp[party_index], state->party_hp_max[party_index]);
   return buf;
}

static char *PAL_BattleUIBuildAllTargetPrompt(int party_target)
{
   return party_target ? "目标 我方全体" : "目标 敌方全体";
}

static char *PAL_BattleUIBuildEnemyTargetPrompt(LAVA_BATTLE_STATE *state)
{
   static char buf[96];
   char enemy_name[64];
   int target_sel;

   buf[0] = 0;
   if (state == 0)
   {
      return buf;
   }

   target_sel = PAL_LavaFightFixEnemyTarget(state->target_sel, state->enemy_count, state->enemy_hp);
   PAL_BattleUIFormatEnemyName(enemy_name, sizeof(enemy_name), state->enemy_object_id[target_sel]);
   if (enemy_name[0] == 0)
   {
      sprintf(enemy_name, "敌人%d", target_sel + 1);
   }
   sprintf(buf, "目标 %s %d/%d", enemy_name,
      state->enemy_hp[target_sel], state->enemy_hp_max[target_sel]);
   return buf;
}

void PAL_BattleUIDrawCommandMenu(LAVA_BATTLE_STATE *state, int x, int y, int command_sel)
{
   int frame[4];
   int icon_x[4];
   int icon_y[4];
   int i;
   int selected_icon;
   int valid[4];
   addr sprite;

   frame[0] = LAVA_BATTLE_SPRITENUM_ICON_ATTACK;
   frame[1] = LAVA_BATTLE_SPRITENUM_ICON_MAGIC;
   frame[2] = LAVA_BATTLE_SPRITENUM_ICON_COOPMAGIC;
   frame[3] = LAVA_BATTLE_SPRITENUM_ICON_MISC;
   icon_x[0] = 27;
   icon_y[0] = 135;
   icon_x[1] = 0;
   icon_y[1] = 150;
   icon_x[2] = 54;
   icon_y[2] = 150;
   icon_x[3] = 27;
   icon_y[3] = 165;
   selected_icon = 3;
   if (command_sel == LAVA_BATTLE_COMMAND_ATTACK)
   {
      selected_icon = 0;
   }
   else if (command_sel == LAVA_BATTLE_COMMAND_DEFEND)
   {
      selected_icon = 3;
   }
     else if (command_sel == LAVA_BATTLE_COMMAND_MAGIC)
     {
        selected_icon = 1;
     }
     else if (command_sel == LAVA_BATTLE_COMMAND_USE_ITEM)
     {
        selected_icon = 3;
     }
      else if (command_sel == LAVA_BATTLE_COMMAND_COOP_MAGIC)
      {
         selected_icon = 2;
      }

   valid[0] = TRUE;
   valid[1] = state != 0 && PAL_LavaFightMagicCount(PAL_BattleUIActingRole(state)) > 0;
   valid[2] = PAL_BattleUICoopMagicAvailable(state);
   valid[3] = TRUE;

   sprite = PAL_LavaLoadUISprite();
   if (sprite == 0)
   {
        PAL_LavaDrawShadowText(x, y, "攻击", command_sel == 0 ? MENUITEM_COLOR_SELECTED : 0x4F);
        PAL_LavaDrawShadowText(x, y + 14, "仙术", command_sel == 2 ? MENUITEM_COLOR_SELECTED : 0x4F);
        PAL_LavaDrawShadowText(x + 40, y + 14, "协力", command_sel == 5 ? MENUITEM_COLOR_SELECTED : 0x4F);
         PAL_LavaDrawShadowText(x + 20, y + 28, "杂项", command_sel == 4 ? MENUITEM_COLOR_SELECTED : 0x4F);
       return;
    }

   for (i = 0; i < 4; i++)
   {
      if (i == selected_icon && valid[i])
      {
         PAL_RLEBlitToSurface(PAL_SpriteGetFrame(sprite, frame[i]),
            gpScreen, PAL_XY(icon_x[i], icon_y[i]));
      }
      else if (valid[i])
      {
         PAL_RLEBlitMonoColor(PAL_SpriteGetFrame(sprite, frame[i]),
            gpScreen, PAL_XY(icon_x[i], icon_y[i]), 0, -4);
      }
      else
      {
         PAL_RLEBlitMonoColor(PAL_SpriteGetFrame(sprite, frame[i]),
            gpScreen, PAL_XY(icon_x[i], icon_y[i]), 0x10, -4);
      }
   }
}

static void PAL_BattleUIDrawCurrentPlayerArrow(LAVA_BATTLE_STATE *state)
{
   int arrow_frame;
   int party_count;
   int player_index;
   int x;
   int y;
   addr sprite;

   if (state == 0)
   {
      return;
   }
   player_index = state->acting_player_index;
   party_count = g_lava_party_count;
   if (party_count > 3)
   {
      party_count = 3;
   }
   if (party_count <= 0 || player_index < 0 || player_index >= party_count)
   {
      return;
   }

   x = 0;
   y = 0;
   if (party_count == 1)
   {
      x = 240;
      y = 170;
   }
   else if (party_count == 2)
   {
      if (player_index == 0)
      {
         x = 200;
         y = 176;
      }
      else
      {
         x = 256;
         y = 152;
      }
   }
   else
   {
      if (player_index == 0)
      {
         x = 180;
         y = 180;
      }
      else if (player_index == 1)
      {
         x = 234;
         y = 170;
      }
      else
      {
         x = 270;
         y = 146;
      }
   }

   sprite = PAL_LavaLoadUISprite();
   if (sprite == 0)
   {
      return;
   }
   arrow_frame = LAVA_BATTLE_SPRITENUM_ARROW_CURRENT_RED;
   if (g_lava_logic_frame_num & 1)
   {
      arrow_frame = LAVA_BATTLE_SPRITENUM_ARROW_CURRENT;
   }
   PAL_RLEBlitToSurface(PAL_SpriteGetFrame(sprite, arrow_frame),
      gpScreen, PAL_XY(x - 8, y - 74));
}

static void PAL_BattleUIDrawPartyTargetArrow(int player_index)
{
   int arrow_frame;
   int party_count;
   int x;
   int y;
   addr sprite;

   party_count = g_lava_party_count;
   if (party_count > 3)
   {
      party_count = 3;
   }
   if (party_count <= 0 || player_index < 0 || player_index >= party_count)
   {
      return;
   }

   x = 0;
   y = 0;
   if (party_count == 1)
   {
      x = 240;
      y = 170;
   }
   else if (party_count == 2)
   {
      if (player_index == 0)
      {
         x = 200;
         y = 176;
      }
      else
      {
         x = 256;
         y = 152;
      }
   }
   else
   {
      if (player_index == 0)
      {
         x = 180;
         y = 180;
      }
      else if (player_index == 1)
      {
         x = 234;
         y = 170;
      }
      else
      {
         x = 270;
         y = 146;
      }
   }

   sprite = PAL_LavaLoadUISprite();
   if (sprite == 0)
   {
      return;
   }
   arrow_frame = LAVA_BATTLE_SPRITENUM_ARROW_SELECTED;
   if (g_lava_logic_frame_num & 1)
   {
      arrow_frame = LAVA_BATTLE_SPRITENUM_ARROW_SELECTED_RED;
   }
   PAL_RLEBlitToSurface(PAL_SpriteGetFrame(sprite, arrow_frame),
      gpScreen, PAL_XY(x - 8, y - 67));
}

static void PAL_BattleUIDrawMagicMenu(LAVA_BATTLE_STATE *state)
{
   int current_mp;
   int i;
   int item_x;
   int item_y;
   int items_per_line;
   int magic_count;
   int magic_object_id;
   int mp_cost;
   int page_line_offset;
   int player_role;
   int selected_magic_object_id;
   int start;
   char caster_name[32];
   char *desc;
   char *name;
   char selected_magic_name[32];
    addr sprite;

   if (state == 0)
   {
      return;
   }

   sprite = PAL_LavaLoadUISprite();
   player_role = PAL_BattleUIActingRole(state);
   current_mp = PAL_LavaRoleWordByArray(10, player_role);
   magic_count = PAL_BattleUIClampMagicSel(state);
   selected_magic_object_id = 0;
   selected_magic_name[0] = 0;
   PAL_BattleUICopyText(caster_name, sizeof(caster_name), PAL_LavaRoleNameForLog(player_role));
   if (magic_count > 0)
   {
      selected_magic_object_id = PAL_LavaFightMagicByIndex(player_role, state->magic_sel);
      PAL_BattleUICopyText(selected_magic_name, sizeof(selected_magic_name),
         PAL_BattleUIFormatMagicName(selected_magic_object_id));
   }
   if (player_role != g_lava_battle_last_magic_log_role ||
       current_mp != g_lava_battle_last_magic_log_mp ||
       magic_count != g_lava_battle_last_magic_log_count ||
       state->magic_sel != g_lava_battle_last_magic_log_sel ||
       selected_magic_object_id != g_lava_battle_last_magic_log_obj ||
       g_lava_word_file_is_gb2312 != g_lava_battle_last_magic_log_gb2312)
   {
      printf("[LAVA][MAGICUI] enter role=%d caster='%s' mp=%d count=%d sel=%d obj=%d name='%s' gb2312=%d\n",
         player_role, PAL_BattleUILogText(caster_name), current_mp, magic_count,
         state->magic_sel, selected_magic_object_id,
         PAL_BattleUILogText(selected_magic_name), g_lava_word_file_is_gb2312);
      g_lava_battle_last_magic_log_role = player_role;
      g_lava_battle_last_magic_log_mp = current_mp;
      g_lava_battle_last_magic_log_count = magic_count;
      g_lava_battle_last_magic_log_sel = state->magic_sel;
      g_lava_battle_last_magic_log_obj = selected_magic_object_id;
      g_lava_battle_last_magic_log_gb2312 = g_lava_word_file_is_gb2312;
   }
    if (!PAL_LavaDrawSpriteBoxAt(sprite,
        LAVA_BATTLE_MAGIC_BOX_LEFT, LAVA_BATTLE_MAGIC_BOX_TOP, 4, 16, 1))
   {
      SetFgColor(0x21);
      Box(10, 42, 310, 136, 1, 1);
   }
   PAL_LavaDrawSingleLineBox(LAVA_BATTLE_MAGIC_MP_BOX_X, LAVA_BATTLE_MAGIC_MP_BOX_Y, 5);

    if (magic_count <= 0)
    {
        PAL_LavaDrawShadowText(LAVA_BATTLE_MAGIC_BOX_X, LAVA_BATTLE_MAGIC_BOX_Y, "尚未习得仙术", 0x1C);
        return;
    }

    magic_object_id = PAL_LavaFightMagicByIndex(player_role, state->magic_sel);
    mp_cost = PAL_LavaFightMagicCost(magic_object_id);
    /* Right-side description comes straight from desc.dat (e.g. "我方單人HP+220"),
       matching the original SDLPal magic-select description pane. Fall back to
       the synthesized prompt only when no description is available. */
    desc = PAL_LavaReadObjectDesc(magic_object_id);
    if (desc == 0 || desc[0] == 0)
    {
       desc = PAL_BattleUIBuildMagicPrompt(state);
    }
    if (desc != 0 && desc[0] != 0)
    {
       PAL_LavaDrawShadowTextSmall(LAVA_BATTLE_MAGIC_DESC_X, LAVA_BATTLE_MAGIC_DESC_Y, desc, 0x2C);
    }
   if (sprite != 0)
   {
      PAL_RLEBlitToSurface(PAL_SpriteGetFrame(sprite, LAVA_BATTLE_SPRITENUM_SLASH),
         gpScreen, PAL_XY(LAVA_BATTLE_MAGIC_MP_SLASH_X, LAVA_BATTLE_MAGIC_MP_SLASH_Y));
      PAL_BattleUIDrawNumberSprite(sprite, mp_cost, 4,
         LAVA_BATTLE_MAGIC_MP_COST_X, LAVA_BATTLE_MAGIC_MP_Y,
         LAVA_BATTLE_NUM_COLOR_YELLOW, LAVA_BATTLE_NUM_ALIGN_RIGHT);
      PAL_BattleUIDrawNumberSprite(sprite, current_mp, 4,
         LAVA_BATTLE_MAGIC_MP_CURRENT_X, LAVA_BATTLE_MAGIC_MP_Y,
         LAVA_BATTLE_NUM_COLOR_CYAN, LAVA_BATTLE_NUM_ALIGN_RIGHT);
    }

    items_per_line = LAVA_BATTLE_MAGIC_ITEMS_PER_LINE;
    start = PAL_BattleUIMagicPageStart(state->magic_sel, magic_count);

    for (i = start; i < magic_count && i < start + items_per_line * LAVA_BATTLE_MAGIC_LINES; i++)
    {
       char fallback_name[24];
       int color;

       magic_object_id = PAL_LavaFightMagicByIndex(player_role, i);
       mp_cost = PAL_LavaFightMagicCost(magic_object_id);
       name = PAL_BattleUIFormatMagicName(magic_object_id);
       if (name == 0 || name[0] == 0)
       {
          sprintf(fallback_name, "仙术%d", magic_object_id);
          name = fallback_name;
       }
       item_x = LAVA_BATTLE_MAGIC_BOX_X + (i - start) % items_per_line * LAVA_BATTLE_MAGIC_ITEM_W;
       item_y = LAVA_BATTLE_MAGIC_BOX_Y + (i - start) / items_per_line * LAVA_BATTLE_MAGIC_ROW_H;
        if (g_lava_autotest_search || g_lava_autotest_load)
        {
           printf("[LAVA][MAGICUI] item i=%d obj=%d cost=%d name='%s' bytes=%02x%02x%02x%02x pos=(%d,%d)\n",
              i, magic_object_id, mp_cost,
              PAL_BattleUILogText(name),
              name != 0 ? (PAL_U8(name[0])) : 0,
              name != 0 ? (PAL_U8(name[1])) : 0,
              name != 0 ? (PAL_U8(name[2])) : 0,
              name != 0 ? (PAL_U8(name[3])) : 0,
              item_x, item_y);
        }
      if (!g_lava_battle_ui_magic_name_logged &&
          i == state->magic_sel &&
          (g_lava_autotest_search || g_lava_autotest_load))
      {
         printf("[LAVA][BATTLEUI] magic name role=%d obj=%d text=%s\n",
            player_role, magic_object_id, PAL_BattleUILogText(name));
         g_lava_battle_ui_magic_name_logged = 1;
      }
      if (mp_cost > current_mp)
      {
          color = (i == state->magic_sel) ? LAVA_BATTLE_LIST_COLOR_SELECTED_INACTIVE : MENUITEM_COLOR_INACTIVE;
      }
      else
      {
          color = (i == state->magic_sel) ? LAVA_BATTLE_LIST_COLOR_SELECTED : MENUITEM_COLOR;
      }

       PAL_LavaDrawShadowText(item_x, item_y, name, color);
       if (g_lava_autotest_search || g_lava_autotest_load)
       {
          printf("[LAVA][MAGICUI] draw i=%d color=0x%02X text='%s' at (%d,%d)\n",
             i, color, PAL_BattleUILogText(name), item_x, item_y);
       }
      if (i == state->magic_sel && sprite != 0)
      {
         PAL_RLEBlitToSurface(PAL_SpriteGetFrame(sprite, LAVA_BATTLE_SPRITENUM_CURSOR),
            gpScreen, PAL_XY(item_x + LAVA_BATTLE_MAGIC_CURSOR_X_OFFSET, item_y + 10));
      }
    }
}

static void PAL_BattleUIDrawItemMenu(LAVA_BATTLE_STATE *state)
{
   int color;
   char *desc;
   int i;
   int item_amounts[256];
   int item_count;
   int item_ids[256];
   int item_object_id;
   int item_x;
   int item_y;
   int page_line_offset;
   int start;
   char amount_buf[8];
   char *name;
   addr sprite;

   if (state == 0)
   {
      return;
   }

   sprite = PAL_LavaLoadUISprite();
   if (!PAL_LavaDrawSpriteBoxAt(sprite,
       LAVA_BATTLE_MAGIC_BOX_LEFT, LAVA_BATTLE_MAGIC_BOX_TOP, 4, 16, 1))
   {
      SetFgColor(0x21);
      Box(10, 42, 310, 136, 1, 1);
   }

   item_count = PAL_BattleUIBuildItemList(PAL_BattleUICurrentItemFlag(state), item_ids, item_amounts);
   if (item_count <= 0)
   {
      PAL_LavaDrawShadowText(LAVA_BATTLE_MAGIC_BOX_X, LAVA_BATTLE_MAGIC_BOX_Y,
         state->command_sel == LAVA_BATTLE_COMMAND_THROW_ITEM ? "暂无可投掷物品" : "暂无可用物品", 0x1C);
      return;
   }
   PAL_BattleUIClampItemSel(state);

   item_object_id = state->item_object_id;
   desc = PAL_LavaReadObjectDesc(item_object_id);
   if (desc == 0 || desc[0] == 0)
   {
      desc = PAL_BattleUIBuildItemPrompt(state);
   }
   if (desc != 0 && desc[0] != 0)
   {
      PAL_LavaDrawShadowTextSmall(LAVA_BATTLE_MAGIC_DESC_X, LAVA_BATTLE_MAGIC_DESC_Y, desc, 0x2C);
   }

   page_line_offset = 2;
   start = state->item_sel - LAVA_BATTLE_MAGIC_ITEMS_PER_LINE * page_line_offset;
   if (start < 0)
   {
      start = 0;
   }
   if (start > item_count - LAVA_BATTLE_MAGIC_ITEMS_PER_LINE * LAVA_BATTLE_MAGIC_LINES)
   {
      start = item_count - LAVA_BATTLE_MAGIC_ITEMS_PER_LINE * LAVA_BATTLE_MAGIC_LINES;
   }
   if (start < 0)
   {
      start = 0;
   }

   for (i = start; i < item_count && i < start + LAVA_BATTLE_MAGIC_ITEMS_PER_LINE * LAVA_BATTLE_MAGIC_LINES; i++)
   {
      name = PAL_BattleUIFormatItemName(item_ids[i]);
      item_x = LAVA_BATTLE_MAGIC_BOX_X + (i - start) % LAVA_BATTLE_MAGIC_ITEMS_PER_LINE * LAVA_BATTLE_MAGIC_ITEM_W;
      item_y = LAVA_BATTLE_MAGIC_BOX_Y + (i - start) / LAVA_BATTLE_MAGIC_ITEMS_PER_LINE * LAVA_BATTLE_MAGIC_ROW_H;
      color = (i == state->item_sel) ? LAVA_BATTLE_LIST_COLOR_SELECTED : MENUITEM_COLOR;
      PAL_LavaDrawShadowText(item_x, item_y, name, color);
      if (item_amounts[i] > 1)
      {
         sprintf(amount_buf, "%d", item_amounts[i]);
         PAL_LavaDrawShadowText(item_x + 62, item_y, amount_buf, color);
      }
      if (i == state->item_sel && sprite != 0)
      {
         PAL_RLEBlitToSurface(PAL_SpriteGetFrame(sprite, LAVA_BATTLE_SPRITENUM_CURSOR),
            gpScreen, PAL_XY(item_x + LAVA_BATTLE_MAGIC_CURSOR_X_OFFSET, item_y + 10));
      }
   }
}

static void PAL_BattleUIDrawMiscMenu(void)
{
   char *labels[5];
   int i;
   int y;

   labels[0] = "物品";
   labels[1] = "防御";
   labels[2] = "自动";
   labels[3] = "逃跑";
   labels[4] = "状态";
    PAL_LavaDrawSingleLineBox(2, 20, 5);
    for (i = 0; i < LAVA_BATTLE_MISC_COUNT; i++)
    {
       y = 32 + i * 18;
       PAL_LavaDrawShadowText(16, y, labels[i],
          i == g_lava_battle_misc_sel ? MENUITEM_COLOR_SELECTED : MENUITEM_COLOR);
     }
}

static void PAL_BattleUIDrawItemSubMenu(void)
{
   PAL_BattleUIDrawMiscMenu();
   PAL_LavaDrawSingleLineBox(30, 50, 1);
   PAL_LavaDrawShadowText(44, 62, "使用",
      g_lava_battle_item_submenu_sel == LAVA_BATTLE_ITEM_SUBMENU_USE ?
         MENUITEM_COLOR_SELECTED : MENUITEM_COLOR);
   PAL_LavaDrawShadowText(44, 80, "投掷",
      g_lava_battle_item_submenu_sel == LAVA_BATTLE_ITEM_SUBMENU_THROW ?
         MENUITEM_COLOR_SELECTED : MENUITEM_COLOR);
}

static void PAL_BattleUIDrawStatusPage(LAVA_BATTLE_STATE *state)
{
   char buf[64];
   char tags[32];
   int current_mp;
   int i;
   int level;
   int max_mp;
   int role;
   int y;

   if (state == 0)
   {
      return;
   }

   PAL_LavaDrawSingleLineBox(16, 36, 17);
   PAL_LavaDrawShadowText(34, 46, "队伍状态", MENUITEM_COLOR_SELECTED);
   for (i = 0; i < g_lava_party_count && i < 3; i++)
   {
      role = g_lava_party_role[i];
      level = PAL_LavaRoleWordByArray(6, role);
      current_mp = PAL_LavaRoleWordByArray(10, role);
      max_mp = PAL_LavaRoleWordByArray(8, role);
      PAL_BattleUIBuildPlayerStatusTags(role, tags, sizeof(tags));
      if (tags[0] == 0)
      {
         sprintf(tags, "正常");
      }
      y = 64 + i * 36;
      sprintf(buf, "%s 等级%d", PAL_LavaRoleName(role), level);
      PAL_LavaDrawShadowText(34, y, buf, MENUITEM_COLOR);
      sprintf(buf, "体力 %d/%d", state->party_hp[i], state->party_hp_max[i]);
      PAL_LavaDrawShadowText(132, y, buf, MENUITEM_COLOR);
      sprintf(buf, "真气 %d/%d", current_mp, max_mp);
      PAL_LavaDrawShadowText(132, y + 14, buf, MENUITEM_COLOR);
      PAL_LavaDrawShadowText(34, y + 14, tags, 0x2C);
      PAL_LavaDrawShadowText(34, y + 26, "攻  灵  防  身  吉", MENUITEM_COLOR_SELECTED);
      sprintf(buf, "%3d %3d %3d %3d %3d",
         PAL_LavaRoleWordByArray(17, role),
         PAL_LavaRoleWordByArray(18, role),
         PAL_LavaRoleWordByArray(19, role),
         PAL_LavaRoleWordByArray(20, role),
         PAL_LavaRoleWordByArray(21, role));
      PAL_LavaDrawShadowText(132, y + 26, buf, MENUITEM_COLOR);
   }
   PAL_LavaDrawShadowText(34, 178, "确定/取消返回", MENUITEM_COLOR_SELECTED);
}

void PAL_BattleUIDrawFrame(LAVA_BATTLE_STATE *state, int ui_state, int target_sel, char *message)
{
   int i;
   int selected;

   if (state == 0)
   {
      return;
   }

    lava_begin_text_batch();
    PAL_LavaBattleDrawSceneFrame(state, -1);
    if (ui_state == LAVA_BATTLE_UI_STATE_TARGET && target_sel >= 0)
    {
       PAL_LavaBattleDrawEnemyTargetOverlay(state, target_sel);
   }
   else if (ui_state == LAVA_BATTLE_UI_STATE_TARGET_ALL)
   {
      for (i = state->enemy_count - 1; i >= 0; i--)
      {
         if (state->enemy_hp[i] > 0)
         {
            PAL_LavaBattleDrawEnemyTargetOverlay(state, i);
         }
      }
   }

    g_lava_battle_ui_last_enemy_count = state->enemy_count;

    if (!g_lava_battle_hide_cast_panels)
    {
       for (i = 0; i < g_lava_party_count && i < 3; i++)
       {
          selected = target_sel == -100 - i || ui_state == LAVA_BATTLE_UI_STATE_PARTY_TARGET_ALL;
          PAL_PlayerInfoBox(LAVA_BATTLE_PARTY_X + LAVA_BATTLE_PARTY_W * i,
             LAVA_BATTLE_PARTY_Y, g_lava_party_role[i], state->party_hp[i], state->party_hp_max[i],
             selected, i == state->acting_player_index, PAL_LavaFightGetPartyTrance(i));
       }
       PAL_BattleUIDrawCurrentPlayerArrow(state);
       if (ui_state == LAVA_BATTLE_UI_STATE_PARTY_TARGET && target_sel <= -100)
       {
          PAL_BattleUIDrawPartyTargetArrow(-100 - target_sel);
       }
       else if (ui_state == LAVA_BATTLE_UI_STATE_PARTY_TARGET_ALL)
       {
          for (i = 0; i < g_lava_party_count && i < 3; i++)
          {
             PAL_BattleUIDrawPartyTargetArrow(i);
          }
       }

       PAL_BattleUIDrawCommandMenu(state, LAVA_BATTLE_MENU_X, LAVA_BATTLE_MENU_Y, state->command_sel);
       if (ui_state == LAVA_BATTLE_UI_STATE_MAGIC)
       {
          PAL_BattleUIDrawMagicMenu(state);
       }
       else if (ui_state == LAVA_BATTLE_UI_STATE_ITEM)
       {
          PAL_BattleUIDrawItemMenu(state);
       }
       else if (ui_state == LAVA_BATTLE_UI_STATE_MISC)
       {
          PAL_BattleUIDrawMiscMenu();
       }
       else if (ui_state == LAVA_BATTLE_UI_STATE_ITEM_SUBMENU)
       {
          PAL_BattleUIDrawItemSubMenu();
       }
       else if (ui_state == LAVA_BATTLE_UI_STATE_STATUS)
       {
          PAL_BattleUIDrawStatusPage(state);
       }
    }
      PAL_BattleUIShowText(message);
      PAL_BattleUIDrawFloatingNumbers();
     lava_end_text_batch();
}

void PAL_BattleUIShowEnemyDamage(int enemy_index, int damage)
{
   int digit_count;
   int draw_x;
   int x;
   int y;

   if (damage < 0)
   {
      damage = -damage;
   }

   PAL_LavaBattleGetEnemyPos(enemy_index, &x, &y);
   y -= 115;
   if (y < 10)
   {
      y = 10;
   }

   digit_count = 1;
   if (damage >= 10000)
   {
      digit_count = 5;
   }
   else if (damage >= 1000)
   {
      digit_count = 4;
   }
   else if (damage >= 100)
   {
      digit_count = 3;
   }
   else if (damage >= 10)
   {
      digit_count = 2;
   }
   draw_x = x - 24 - digit_count * 8;

    PAL_BattleUIPlayFloatingNumber(draw_x, y, damage, 0xF9);
}

static int PAL_BattleUIReadMagicSignedField(int magic_num, int field_index)
{
   int value;

   value = PAL_LavaFightReadMagicField(magic_num, field_index);
   if (value >= 0x8000)
   {
      value -= 0x10000;
   }
   return value;
}

static void PAL_BattleUIBlitMagicFrameAt(LPCBITMAPRLE frame, int cx, int cy)
{
   if (PAL_BattleUIDebugMagicEffect())
   {
      printf("[LAVA][MAGICDRAW] scene-frame cx=%d cy=%d w=%d h=%d direct=%d\n",
         cx, cy,
         PAL_RLEGetWidth(frame),
         PAL_RLEGetHeight(frame),
         g_lava_direct_screen);
   }
   PAL_LavaBattleAddCurrentEffect((addr)frame,
      cx - PAL_RLEGetWidth(frame) / 2, cy - PAL_RLEGetHeight(frame));
}

static void PAL_BattleUIBlitMagicFrame(
   LPCBITMAPRLE frame,
   int magic_type,
   int target,
   int x_offset,
   int y_offset,
   int target_is_player,
   int from_enemy
)
{
   int cx;
   int cy;
   int i;
   int party_base_y;
   int party_line_h;
   int effect_x[3];
   int effect_y[3];

   if (frame == 0)
   {
      return;
   }

   if (magic_type == 1)
   {
      if (target_is_player && from_enemy)
      {
         effect_x[0] = 180;
         effect_y[0] = 180;
         effect_x[1] = 234;
         effect_y[1] = 170;
         effect_x[2] = 270;
         effect_y[2] = 146;
      }
      else
      {
         effect_x[0] = 70;
         effect_y[0] = 140;
         effect_x[1] = 100;
         effect_y[1] = 110;
         effect_x[2] = 160;
         effect_y[2] = 100;
      }
      for (i = 0; i < 3; i++)
      {
         PAL_BattleUIBlitMagicFrameAt(frame,
            effect_x[i] + x_offset, effect_y[i] + y_offset);
      }
      return;
   }

   if (target_is_player && (target < 0 || magic_type == 5))
   {
      party_base_y = PAL_BattleUIPartyBaseY(g_lava_battle_ui_last_enemy_count);
      party_line_h = PAL_BattleUIPartyLineH(g_lava_battle_ui_last_enemy_count);
      for (i = 0; i < g_lava_party_count && i < 3; i++)
      {
         if (!from_enemy && g_lava_battle_ui_state_valid && g_lava_battle_ui_state_copy.party_hp[i] <= 0)
         {
            continue;
         }
         PAL_BattleUIBlitMagicFrameAt(frame, 238 + x_offset,
            party_base_y + i * party_line_h + 10 + y_offset);
      }
      return;
   }

   if (magic_type == 2)
   {
      cx = (target_is_player && from_enemy ? 240 : 120) + x_offset;
      cy = (target_is_player && from_enemy ? 150 : 100) + y_offset;
   }
   else if (magic_type == 3)
   {
      cx = 160 + x_offset;
      cy = 200 + y_offset;
   }
   else if (!target_is_player && (target < 0 || magic_type == 1))
   {
      for (i = 0; i < g_lava_battle_ui_last_enemy_count && i < LAVA_BATTLE_MAX_ENEMIES; i++)
      {
         if (g_lava_battle_ui_state_valid && g_lava_battle_ui_state_copy.enemy_hp[i] <= 0)
         {
            continue;
         }
         PAL_LavaBattleGetEnemyPos(i, &cx, &cy);
         if (cx <= 0)
         {
            continue;
         }
         PAL_BattleUIBlitMagicFrameAt(frame, cx + x_offset, cy + y_offset);
      }
      return;
   }
   else if (target_is_player)
   {
      if (target < 0 || target >= g_lava_party_count || target >= 3)
      {
         target = 0;
      }
      party_base_y = PAL_BattleUIPartyBaseY(g_lava_battle_ui_last_enemy_count);
      party_line_h = PAL_BattleUIPartyLineH(g_lava_battle_ui_last_enemy_count);
      cx = 238 + x_offset;
      cy = party_base_y + target * party_line_h + 10 + y_offset;
   }
   else
   {
      PAL_LavaBattleGetEnemyPos(target, &cx, &cy);
      if (cx <= 0)
      {
         return;
      }
      cx += x_offset;
      cy += y_offset;
   }

   PAL_BattleUIBlitMagicFrameAt(frame, cx, cy);
}

static int PAL_BattleUIFindMagicObjectByMagicNum(int magic_num)
{
   int object_count;
   int object_id;

   if (g_lava_object_data_size <= 0)
   {
      PAL_LavaReadObjectField(1, 0);
   }
   object_count = (int)(g_lava_object_data_size / (gConfig.fIsWIN95 ? 14 : 12));
   if (object_count <= 0)
   {
      object_count = 600;
   }

   for (object_id = 1; object_id < object_count; object_id++)
   {
      if (PAL_LavaReadObjectField(object_id, 0) == magic_num ||
          PAL_LavaFightResolveMagicIndex(object_id) == magic_num)
      {
         return object_id;
      }
   }
   return 0;
}

static void PAL_BattleUIPlaySummonSprite(int magic_object_id, int magic_num)
{
   FILE *fp;
   long decompressed_size;
   long decompress_ret;
   int chunk;
   int frame_delay;
   int frame_index;
   int n;
   int play_frames;
   int speed;
   int step;
   int dump_step;
   int saved_direct_screen;
   int x;
   int y;
   addr frame;
   addr sprite;

   chunk = PAL_LavaFightReadMagicField(magic_num, 4) + 10;
   if (chunk < 10)
   {
      if (PAL_BattleUIDebugMagicEffect())
      {
         printf("[LAVA][MAGICFX] summon skip magic=%d summon=%d\n",
            magic_num, chunk - 10);
      }
      return;
   }

   fp = (FILE *)PAL_LavaBattleGetPlayerAssetFile();
   if (fp == 0)
   {
      if (PAL_BattleUIDebugMagicEffect())
      {
         printf("[LAVA][MAGICFX] summon open F.MKF fail magic=%d chunk=%d\n",
            magic_num, chunk);
      }
      return;
   }

   decompressed_size = PAL_BattleUIGetChunkDecompressedSize(fp, chunk);
   if (decompressed_size <= 0)
   {
      if (PAL_BattleUIDebugMagicEffect())
      {
         printf("[LAVA][MAGICFX] summon bad size magic=%d chunk=%d unpacked=%ld\n",
            magic_num, chunk, decompressed_size);
      }
      return;
   }
   sprite = (addr)malloc(decompressed_size);
   if (sprite == 0)
   {
      printf("[LAVA][MAGICFX] summon alloc fail magic=%d chunk=%d bytes=%ld\n",
         magic_num, chunk, decompressed_size);
      return;
   }
   decompress_ret = PAL_MKFDecompressChunk(sprite, decompressed_size, chunk, fp);
   if (!PAL_LavaDecompressOK(decompress_ret, decompressed_size))
   {
      printf("[LAVA][MAGICFX] summon decompress fail magic=%d chunk=%d ret=%ld expected=%ld\n",
         magic_num, chunk, decompress_ret, decompressed_size);
      free((void *)sprite);
      return;
   }

   n = PAL_SpriteGetNumFrames(sprite);
   if (n <= 0)
   {
      if (PAL_BattleUIDebugMagicEffect())
      {
         printf("[LAVA][MAGICFX] summon no frames magic=%d chunk=%d\n",
             magic_num, chunk);
      }
      free((void *)sprite);
      return;
   }

   x = 240 + PAL_BattleUIReadMagicSignedField(magic_num, 2);
   y = 165 + PAL_BattleUIReadMagicSignedField(magic_num, 3);
   speed = PAL_BattleUIReadMagicSignedField(magic_num, 5);
   frame_delay = (speed + 5) * 5;
   if (frame_delay < 10)
   {
      frame_delay = 10;
   }
   if (frame_delay > 60)
   {
      frame_delay = 60;
   }
   play_frames = n - 1;
   if (play_frames <= 0)
   {
      play_frames = n;
   }
   if (play_frames > 80)
   {
      play_frames = 80;
   }
   if (g_lava_autotest_fengshen || PAL_BattleUIDebugMagicEffect())
   {
      printf("[LAVA][MAGICFX] summon play magic=%d chunk=%d frames=%d play=%d delay=%d x=%d y=%d\n",
         magic_num, chunk, n, play_frames, frame_delay, x, y);
   }

   saved_direct_screen = g_lava_direct_screen;
   g_lava_direct_screen = 1;

   for (step = 1; step <= 10; step++)
   {
      PAL_BattleUISetPartyColorShiftAll(step);
      PAL_BattleUIRedrawRoundFrame(0);
      Delay(20);
   }

   frame = (addr)PAL_SpriteGetFrame(sprite, 0);
   g_lava_battle_hide_players = 1;
   for (step = 8; step >= 0; step -= 2)
   {
      PAL_BattleUIRedrawRoundFrame(0);
      if (frame != 0)
      {
         PAL_RLEBlitWithColorShift((LPCBITMAPRLE)frame, gpScreen,
            PAL_XY(x - PAL_RLEGetWidth((LPCBITMAPRLE)frame) / 2,
               y - PAL_RLEGetHeight((LPCBITMAPRLE)frame)), step);
      }
      VIDEO_UpdateScreen(0);
      Delay(25);
   }

   PAL_BattleUISetPartyColorShiftAll(0);
   dump_step = 0;
   for (frame_index = 0; frame_index < play_frames; frame_index++)
   {
      PAL_BattleUIRedrawRoundFrame(0);
      frame = (addr)PAL_SpriteGetFrame(sprite, frame_index);
      if (frame != 0)
      {
         PAL_BattleUIDumpPersistentRawMagicFrameBMP(magic_object_id, magic_num,
            chunk, dump_step, play_frames, frame_index, frame, 0);
         PAL_BattleUISetSummonHold(frame, x, y);
         PAL_BattleUIBlitMagicFrameAt((LPCBITMAPRLE)frame, x, y);
      }
      VIDEO_UpdateScreen(0);
      Delay(frame_delay);
      dump_step++;
   }
   frame = (addr)PAL_SpriteGetFrame(sprite, n - 1);
   if (frame != 0)
   {
      PAL_BattleUISetSummonHold(frame, x, y);
   }
   g_lava_direct_screen = saved_direct_screen;
   free((void *)sprite);
   if (g_lava_autotest_fengshen || PAL_BattleUIDebugMagicEffect())
   {
      printf("[LAVA][MAGICFX] summon done magic=%d chunk=%d\n", magic_num, chunk);
   }
}

static void PAL_BattleUIApplyMagicHitFeedback(int target, int target_is_player, int from_enemy, int step)
{
   int i;
   int flash;
   int offset;

   flash = step == 1 ? 6 : 0;
   offset = step == 0 ? -8 : (step == 1 ? 4 : 0);
   if (target_is_player)
   {
      if (target < 0)
      {
         for (i = 0; i < g_lava_party_count && i < 3; i++)
         {
            if (!g_lava_battle_ui_state_valid || g_lava_battle_ui_state_copy.party_hp[i] > 0)
            {
               g_lava_battle_player_color_shift[i] = flash;
            }
         }
      }
      else if (target < 3)
      {
         g_lava_battle_player_color_shift[target] = flash;
      }
      return;
   }

   if (target < 0 || from_enemy)
   {
      for (i = 0; i < g_lava_battle_ui_last_enemy_count && i < LAVA_BATTLE_MAX_ENEMIES; i++)
      {
         if (!g_lava_battle_ui_state_valid || g_lava_battle_ui_state_copy.enemy_hp[i] > 0)
         {
            g_lava_battle_enemy_color_shift[i] = flash;
            g_lava_battle_enemy_offset_x[i] = offset;
         }
      }
   }
   else if (target < LAVA_BATTLE_MAX_ENEMIES)
   {
      g_lava_battle_enemy_color_shift[target] = flash;
      g_lava_battle_enemy_offset_x[target] = offset;
   }
}

static void PAL_BattleUIPlayMagicHitFeedback(int target, int target_is_player, int from_enemy)
{
   int step;

   for (step = 0; step < 3; step++)
   {
      PAL_BattleUIApplyMagicHitFeedback(target, target_is_player, from_enemy, step);
      PAL_BattleUIRedrawRoundFrame(0);
      Delay(25);
   }
   PAL_BattleUIClearMagicVisualState();
   PAL_BattleUIRedrawRoundFrame(0);
}

/* Play a FIRE.MKF magic effect sequence against enemies or party targets. */
static void PAL_BattleUIPlayMagicSprite(int magic_object_id, int target, int target_is_player, int from_enemy)
{
   int effect_times;
   int magic_num;
   int effect_chunk;
   int fire_delay;
   int frame_index;
   int chain_magic_num;
   int effect_magic_object_id;
   int magic_type;
   int n;
    int i;
    int loop_frames;
    int shake;
    int speed;
    int total_frames;
    int hide_players_for_summon;
    int old_hide_players;
    int saved_direct_screen;
    char caster_name[32];
    char magic_name[32];
     int x_offset;
     int y_offset;
    FILE *fp_fire;
     addr sprite;
     addr frame;
     addr keep_frame;
    int keep_effect;
    int keep_cx;
    int keep_cy;

   hide_players_for_summon = 0;
   old_hide_players = 0;
   saved_direct_screen = 0;
   caster_name[0] = 0;
   magic_name[0] = 0;

   if (magic_object_id <= 0)
   {
      if (PAL_BattleUIDebugMagicEffect())
      {
         printf("[LAVA][MAGICFX] fire skip obj=%d fp=0\n",
            magic_object_id);
      }
      return;
   }
   PAL_BattleUICopyText(magic_name, sizeof(magic_name),
      PAL_BattleUIFormatMagicName(magic_object_id));
   if (from_enemy)
   {
      PAL_BattleUIFormatEnemyName(caster_name, sizeof(caster_name),
         PAL_LavaFightGetRoundEnemyObjectID());
   }
   else if (g_lava_battle_ui_state_valid &&
            g_lava_battle_ui_state_copy.acting_player_index >= 0 &&
            g_lava_battle_ui_state_copy.acting_player_index < 3)
   {
      PAL_BattleUICopyText(caster_name, sizeof(caster_name),
         PAL_LavaRoleNameForLog(g_lava_party_role[g_lava_battle_ui_state_copy.acting_player_index]));
   }

   magic_num = PAL_LavaFightResolveMagicIndex(magic_object_id);
   if (magic_num < 0)
   {
      if (PAL_BattleUIDebugMagicEffect())
      {
         printf("[LAVA][MAGICFX] fire bad object obj=%d name='%s' magic=%d\n",
            magic_object_id, PAL_BattleUILogText(magic_name), magic_num);
      }
      return;
   }

   magic_type = PAL_LavaFightReadMagicField(magic_num, 1);
   if (magic_type == 9)
   {
      int summon_bg_shift;

      summon_bg_shift = PAL_BattleUIReadMagicSignedField(magic_num, 8);
      if (g_lava_autotest_fengshen || PAL_BattleUIDebugMagicEffect())
      {
         printf("[LAVA][MAGICFX] summon begin obj=%d name='%s' magic=%d\n",
            magic_object_id, PAL_BattleUILogText(magic_name), magic_num);
      }
      hide_players_for_summon = 1;
      old_hide_players = g_lava_battle_hide_players;
      g_lava_battle_hide_cast_panels = 1;
      PAL_LavaBattleApplyBackgroundShift(summon_bg_shift);
      PAL_BattleUIPlaySummonSprite(magic_object_id, magic_num);
      if (g_lava_autotest_fengshen || PAL_BattleUIDebugMagicEffect())
      {
         printf("[LAVA][MAGICFX] summon return obj=%d name='%s' magic=%d\n",
            magic_object_id, PAL_BattleUILogText(magic_name), magic_num);
      }
      printf("[LAVA][MAGICFX] summon bg-shift=%d obj=%d name='%s' magic=%d\n",
         summon_bg_shift, magic_object_id, PAL_BattleUILogText(magic_name), magic_num);
      chain_magic_num = PAL_LavaFightReadMagicField(magic_num, 0);
      effect_magic_object_id = PAL_BattleUIFindMagicObjectByMagicNum(chain_magic_num);
      if (PAL_BattleUIDebugMagicEffect())
      {
         printf("[LAVA][MAGICFX] summon chain obj=%d magic=%d effect_magic=%d effect_obj=%d\n",
            magic_object_id, magic_num, chain_magic_num, effect_magic_object_id);
      }
      if (effect_magic_object_id > 0 && effect_magic_object_id != magic_object_id)
      {
         magic_object_id = effect_magic_object_id;
         PAL_BattleUICopyText(magic_name, sizeof(magic_name),
            PAL_BattleUIFormatMagicName(magic_object_id));
         magic_num = PAL_LavaFightResolveMagicIndex(magic_object_id);
      }
      else
      {
         magic_num = chain_magic_num;
      }
      if (magic_num < 0)
      {
          if (hide_players_for_summon)
          {
             PAL_BattleUIEndSummonCast(old_hide_players);
          }
         return;
      }
      magic_type = PAL_LavaFightReadMagicField(magic_num, 1);
   }

   effect_chunk = PAL_LavaFightReadMagicField(magic_num, 0); /* wEffect / FIRE chunk */
   if (effect_chunk <= 0)
   {
      if (PAL_BattleUIDebugMagicEffect())
      {
         printf("[LAVA][MAGICFX] fire no effect obj=%d name='%s' magic=%d effect=%d\n",
            magic_object_id, PAL_BattleUILogText(magic_name), magic_num, effect_chunk);
      }
      if (hide_players_for_summon)
      {
         PAL_BattleUIEndSummonCast(old_hide_players);
      }
      return;
   }
   x_offset = PAL_BattleUIReadMagicSignedField(magic_num, 2);
   y_offset = PAL_BattleUIReadMagicSignedField(magic_num, 3);
   speed = PAL_BattleUIReadMagicSignedField(magic_num, 5);
   fire_delay = PAL_LavaFightReadMagicField(magic_num, 7);
   effect_times = PAL_LavaFightReadMagicField(magic_num, 8);
   shake = PAL_LavaFightReadMagicField(magic_num, 9);
   if (PAL_BattleUIDebugMagicEffect())
   {
      printf("[LAVA][MAGICFX] resolve caster='%s' obj=%d name='%s' magic=%d type=%d effect=%d x=%d y=%d speed=%d delay=%d times=%d shake=%d target=%d player=%d enemy=%d\n",
         PAL_BattleUILogText(caster_name), magic_object_id,
         PAL_BattleUILogText(magic_name), magic_num, magic_type,
         effect_chunk, x_offset, y_offset, speed, fire_delay, effect_times,
         shake, target, target_is_player, from_enemy);
   }
   if (g_lava_autotest_fengshen || magic_object_id == 0x13B)
   {
      printf("[LAVA][FENGSHEN] fields obj=%d name='%s' magic=%d type=%d effect=%d x=%d y=%d speed=%d delay=%d times=%d shake=%d\n",
         magic_object_id, PAL_BattleUILogText(magic_name), magic_num, magic_type, effect_chunk,
         x_offset, y_offset, speed, fire_delay, effect_times, shake);
   }

     fp_fire = UTIL_OpenFile("FIRE.MKF");
      if (fp_fire == 0)
      {
       if (PAL_BattleUIDebugMagicEffect())
       {
          printf("[LAVA][MAGICFX] fire open fail obj=%d name='%s' magic=%d chunk=%d\n",
              magic_object_id, PAL_BattleUILogText(magic_name), magic_num, effect_chunk);
        }
         if (hide_players_for_summon)
         {
            PAL_BattleUIEndSummonCast(old_hide_players);
         }
        return;
      }
     sprite = PAL_BattleUILoadMagicSpriteChunk(fp_fire,
        effect_chunk, "fire");
     fclose(fp_fire);
      if (sprite == 0)
      {
       if (PAL_BattleUIDebugMagicEffect())
      {
          printf("[LAVA][MAGICFX] fire bad chunk obj=%d name='%s' magic=%d type=%d chunk=%d\n",
              magic_object_id, PAL_BattleUILogText(magic_name), magic_num, magic_type, effect_chunk);
       }
        if (hide_players_for_summon)
        {
           PAL_BattleUIEndSummonCast(old_hide_players);
        }
       return;
     }

    n = PAL_SpriteGetNumFrames(sprite);
   if (n <= 0)
   {
      if (PAL_BattleUIDebugMagicEffect())
      {
         printf("[LAVA][MAGICFX] fire no frames obj=%d name='%s' magic=%d chunk=%d\n",
            magic_object_id, PAL_BattleUILogText(magic_name), magic_num, effect_chunk);
      }
      if (hide_players_for_summon)
      {
         PAL_BattleUIEndSummonCast(old_hide_players);
      }
      return;
   }
   if (PAL_BattleUIDebugMagicEffect())
   {
      printf("[LAVA][MAGICFX] fire play caster='%s' obj=%d name='%s' magic=%d type=%d chunk=%d frames=%d target=%d player=%d enemy=%d\n",
         PAL_BattleUILogText(caster_name), magic_object_id,
         PAL_BattleUILogText(magic_name), magic_num, magic_type, effect_chunk,
         n, target, target_is_player, from_enemy);
   }
   if (g_lava_autotest_battle && !target_is_player && !from_enemy)
   {
      g_lava_autotest_battle_magic_seen++;
      g_lava_autotest_battle_magic_object_id = magic_object_id;
      printf("[LAVA][BATTLESMOKE] magic-effect seen=%d obj=%d name='%s' magic=%d\n",
         g_lava_autotest_battle_magic_seen, magic_object_id,
         PAL_BattleUILogText(magic_name), magic_num);
   }
   if (g_lava_autotest_battle && target_is_player && from_enemy)
   {
      g_lava_autotest_battle_magic_seen++;
      printf("[LAVA][BATTLESMOKE] enemy-magic-effect seen=%d obj=%d name='%s' magic=%d\n",
         g_lava_autotest_battle_magic_seen, magic_object_id,
         PAL_BattleUILogText(magic_name), magic_num);
   }
   if (fire_delay < 0 || fire_delay >= n)
   {
      fire_delay = 0;
   }
   if (effect_times <= 0)
   {
      effect_times = 1;
   }
   if (shake < 0)
   {
      shake = 0;
   }
     loop_frames = n - fire_delay;
     if (loop_frames <= 0)
     {
        loop_frames = n;
     }
     total_frames = n + loop_frames * effect_times + shake;
     if (total_frames <= 0 || total_frames > 240)
     {
        if (g_lava_autotest_fengshen || magic_object_id == 0x13B || PAL_BattleUIDebugMagicEffect())
        {
           printf("[LAVA][MAGICFX] clamp frames obj=%d magic=%d total=%d n=%d loop=%d times=%d shake=%d\n",
              magic_object_id, magic_num, total_frames, n, loop_frames,
              effect_times, shake);
        }
        total_frames = 240;
     }
     if (g_lava_autotest_fengshen || magic_object_id == 0x13B)
     {
        printf("[LAVA][FENGSHEN] play obj=%d name='%s' total=%d frames=%d loop=%d delay=%d\n",
           magic_object_id, PAL_BattleUILogText(magic_name), total_frames, n,
           loop_frames, fire_delay);
     }
     if (PAL_BattleUIDebugMagicEffect())
     {
        printf("[LAVA][MAGICFX] frame-plan caster='%s' obj=%d name='%s' magic=%d chunk=%d frames=%d loop=%d total=%d delay=%d times=%d shake=%d target=%d player=%d enemy=%d\n",
           PAL_BattleUILogText(caster_name), magic_object_id,
           PAL_BattleUILogText(magic_name), magic_num,
           effect_chunk, n, loop_frames, total_frames, fire_delay, effect_times,
           shake, target, target_is_player, from_enemy);
     }

     keep_frame = 0;
     saved_direct_screen = g_lava_direct_screen;
     g_lava_direct_screen = 1;
   for (i = 0; i < total_frames; i++)
   {
      if (i < n)
      {
         frame_index = i;
      }
      else if (i < total_frames - shake)
      {
         frame_index = (i - fire_delay) % loop_frames + fire_delay;
      }
      else
      {
         frame_index = (total_frames - shake - 1) % n;
      }

      frame = (addr)PAL_SpriteGetFrame(sprite, frame_index);
      if (frame != 0)
      {
         PAL_BattleUIDumpPersistentRawMagicFrameBMP(magic_object_id, magic_num,
            effect_chunk, i, total_frames, frame_index, frame, from_enemy);
      }
      if (frame != 0 && i + 1 == total_frames && PAL_BattleUICaptureMagicObject(magic_object_id))
      {
         PAL_BattleUIDebugRLEStats(magic_object_id, frame);
         PAL_BattleUIDumpLastMagicFrameBMP(magic_object_id, magic_num,
            effect_chunk, i, total_frames, frame_index, frame, from_enemy);
      }
      if (PAL_BattleUIDebugMagicEffect() &&
          (!g_lava_autotest_battle || g_lava_autotest_fengshen))
      {
         printf("[LAVA][MAGICFX] frame caster='%s' obj=%d name='%s' magic=%d chunk=%d step=%d/%d frame=%d delay=%d speed=%d target=%d player=%d enemy=%d has_frame=%d\n",
            PAL_BattleUILogText(caster_name), magic_object_id,
            PAL_BattleUILogText(magic_name), magic_num,
            effect_chunk, i + 1, total_frames, frame_index, fire_delay, speed,
            target, target_is_player, from_enemy, frame != 0 ? 1 : 0);
      }
      PAL_LavaBattleClearCurrentEffect();
      if (frame != 0)
      {
         PAL_BattleUIDumpMagicScreenBMP(magic_object_id, i, frame_index, "pre", from_enemy);
         keep_frame = frame;
         PAL_BattleUIBlitMagicFrame((LPCBITMAPRLE)frame, magic_type, target, x_offset, y_offset,
            target_is_player, from_enemy);
      }
      PAL_BattleUIRedrawRoundFrame(0);
      PAL_BattleUIDumpMagicScreenBMP(magic_object_id, i, frame_index, "post", from_enemy);
      VIDEO_UpdateScreen(0);
      Delay((speed + 5) * 10);
   }
   g_lava_direct_screen = saved_direct_screen;
   PAL_LavaBattleClearCurrentEffect();

   keep_effect = PAL_LavaFightReadMagicField(magic_num, 6);
   if (keep_effect == 0xFFFF && keep_frame != 0)
   {
      if (magic_type == 2)
      {
         keep_cx = 120 + x_offset;
         keep_cy = 100 + y_offset;
      }
      else if (magic_type == 3)
      {
         keep_cx = 160 + x_offset;
         keep_cy = 200 + y_offset;
      }
      else if (magic_type == 0)
      {
         PAL_LavaBattleGetEnemyPos(target >= 0 ? target : 0, &keep_cx, &keep_cy);
         keep_cx += x_offset;
         keep_cy += y_offset;
      }
      else
      {
         keep_cx = 110 + x_offset;
         keep_cy = 120 + y_offset;
      }
      PAL_LavaBattleSetKeepEffect(keep_frame,
         keep_cx - PAL_RLEGetWidth((LPCBITMAPRLE)keep_frame) / 2,
         keep_cy - PAL_RLEGetHeight((LPCBITMAPRLE)keep_frame));
      printf("[LAVA][MAGICFX] keep-effect baked obj=%d name='%s' magic=%d type=%d cx=%d cy=%d\n",
         magic_object_id, PAL_BattleUILogText(magic_name), magic_num, magic_type,
         keep_cx, keep_cy);
   }

    if (!target_is_player)
    {
       PAL_BattleUIPlayMagicHitFeedback(target, target_is_player, from_enemy);
    }
     if (hide_players_for_summon)
     {
        PAL_BattleUIRestoreSummonCast(old_hide_players);
     }
    PAL_BattleUIRedrawRoundFrame(0);
}

static void PAL_BattleUIPlayPlayerMagicEffectOnce(int event_type, int target)
{
   if (event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_HIT ||
       event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_HIT_KO ||
       event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_ALL ||
       event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_ALL_KO ||
       event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_HIT ||
       event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_HIT_KO ||
       event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_ALL ||
       event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_ALL_KO)
   {
      if (event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_ALL ||
          event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_ALL_KO ||
          event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_ALL ||
          event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_ALL_KO)
      {
         target = -1;
      }
      PAL_BattleUIPlayMagicSprite(PAL_LavaFightGetRoundEventArg0(TRUE), target, FALSE, FALSE);
   }
}

static void PAL_BattleUIPlayPlayerSupportMagicEffectOnce(int event_type, int target)
{
   if (event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_HEAL ||
       event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_HEAL_ALL ||
       event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_MP ||
       event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_MP_ALL ||
       event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_HELPER ||
       event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_TRANCE ||
       event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_HEAL ||
       event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_HEAL_ALL ||
       event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_MP ||
       event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_MP_ALL ||
       event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_HELPER ||
       event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_TRANCE)
   {
      int target_is_player;

      target_is_player = TRUE;
      if (event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_HEAL_ALL ||
          event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_MP_ALL ||
          event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_HEAL_ALL ||
          event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_MP_ALL ||
          PAL_LavaFightGetRoundEventArg1(TRUE) == LAVA_BATTLE_HELPER_STATUS_MULTI)
      {
         target = -1;
      }
      if (event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_TRANCE ||
          event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_TRANCE ||
          event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_HELPER ||
          event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_HELPER)
      {
         target = target >= 0 ? target : -1;
      }
      PAL_BattleUIPlayMagicSprite(PAL_LavaFightGetRoundEventArg0(TRUE), target,
         target_is_player, FALSE);
   }
}

static void PAL_BattleUIPlayEnemyMagicEffectOnce(int event_type, int target)
{
   if (event_type == LAVA_BATTLE_EVENT_ENEMY_MAGIC_HIT ||
       event_type == LAVA_BATTLE_EVENT_ENEMY_MAGIC_HIT_KO ||
       event_type == LAVA_BATTLE_EVENT_ENEMY_MAGIC_ALL ||
       event_type == LAVA_BATTLE_EVENT_ENEMY_MAGIC_ALL_KO)
   {
      if (event_type == LAVA_BATTLE_EVENT_ENEMY_MAGIC_ALL ||
          event_type == LAVA_BATTLE_EVENT_ENEMY_MAGIC_ALL_KO)
      {
         target = -1;
      }
      PAL_BattleUIPlayMagicSprite(PAL_LavaFightGetRoundEnemyMagicObjectID(), target, TRUE, TRUE);
   }
}

static void PAL_BattleUIKeepEnemyVisibleForMagic(int *round_result, int hit_count, int *saved_hp)
{
   int base;
   int i;
   int target;

   if (!g_lava_battle_ui_state_valid || round_result == 0 || saved_hp == 0)
   {
      return;
   }

   for (i = 0; i < LAVA_BATTLE_MAX_ENEMIES; i++)
   {
      saved_hp[i] = g_lava_battle_ui_state_copy.enemy_hp[i];
   }

   for (i = 0; i < hit_count; i++)
   {
      if (i == 0)
      {
         target = round_result[LAVA_BATTLE_ROUND_PLAYER_TARGET];
      }
      else
      {
         base = LAVA_BATTLE_ROUND_PLAYER_TARGET1 + (i - 1) * 2;
         target = round_result[base];
      }
      if (target >= 0 && target < LAVA_BATTLE_MAX_ENEMIES &&
          target < g_lava_battle_ui_state_copy.enemy_count &&
          g_lava_battle_ui_state_copy.enemy_hp[target] <= 0 &&
          g_lava_battle_ui_state_copy.enemy_hp_max[target] > 0)
      {
         g_lava_battle_ui_state_copy.enemy_hp[target] = 1;
      }
   }
}

static void PAL_BattleUIRestoreEnemyVisibleForMagic(int *saved_hp)
{
   int i;

   if (!g_lava_battle_ui_state_valid || saved_hp == 0)
   {
      return;
   }

   for (i = 0; i < LAVA_BATTLE_MAX_ENEMIES; i++)
   {
      g_lava_battle_ui_state_copy.enemy_hp[i] = saved_hp[i];
   }
}

static int PAL_BattleUIIsPlayerAttackEvent(int event_type)
{
   return event_type == LAVA_BATTLE_EVENT_PLAYER_HIT ||
      event_type == LAVA_BATTLE_EVENT_PLAYER_HIT_KO ||
      event_type == LAVA_BATTLE_EVENT_PLAYER_STRONG_HIT ||
      event_type == LAVA_BATTLE_EVENT_PLAYER_STRONG_HIT_KO ||
      event_type == LAVA_BATTLE_EVENT_PLAYER_HIT_ALL ||
      event_type == LAVA_BATTLE_EVENT_PLAYER_HIT_ALL_KO;
}

static int PAL_BattleUIIsPlayerMagicDamageEvent(int event_type)
{
   return event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_HIT ||
      event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_HIT_KO ||
      event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_ALL ||
      event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_ALL_KO ||
      event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_HIT ||
      event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_HIT_KO ||
      event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_ALL ||
      event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_ALL_KO;
}

static int PAL_BattleUIIsPlayerCoopEvent(int event_type)
{
   return event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_HIT ||
      event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_HIT_KO ||
      event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_ALL ||
      event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_ALL_KO ||
      event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_HELPER ||
      event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_HEAL ||
      event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_HEAL_ALL ||
      event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_MP ||
      event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_MP_ALL ||
      event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_TRANCE;
}

static void PAL_BattleUISetCoopPartyPose(void)
{
   int i;

   for (i = 0; i < g_lava_party_count && i < 3; i++)
   {
      if (g_lava_battle_ui_state_copy.party_hp[i] > 0)
      {
         PAL_BattleUISetPartyPose(i, 5);
      }
   }
}

static void PAL_BattleUIPlayCoopCastPose(int delay_ms)
{
   int i;

   PAL_BattleUIClearPartyPoses();
   for (i = 0; i < g_lava_party_count && i < 3; i++)
   {
      if (g_lava_battle_ui_state_copy.party_hp[i] <= 0)
      {
         continue;
      }
      PAL_BattleUISetPartyPose(i, 5);
      PAL_BattleUIRedrawRoundFrame(0);
      Delay(60);
   }
   PAL_BattleUISetCoopPartyPose();
   PAL_BattleUIRedrawRoundFrame(0);
   Delay(delay_ms);
}

void PAL_BattleUIShowPlayerDamage(int player_index, int damage)
{
   int party_base_y;
   int party_line_h;

   party_base_y = PAL_BattleUIPartyBaseY(g_lava_battle_ui_last_enemy_count);
   party_line_h = PAL_BattleUIPartyLineH(g_lava_battle_ui_last_enemy_count);
    PAL_BattleUIPlayFloatingNumber(
       LAVA_BATTLE_DAMAGE_X,
       party_base_y + player_index * party_line_h,
       -damage,
       0x1C);
}

void PAL_BattleUIShowPlayerHeal(int player_index, int amount)
{
   int party_base_y;
   int party_line_h;

   party_base_y = PAL_BattleUIPartyBaseY(g_lava_battle_ui_last_enemy_count);
   party_line_h = PAL_BattleUIPartyLineH(g_lava_battle_ui_last_enemy_count);
    PAL_BattleUIPlayFloatingNumber(
       LAVA_BATTLE_DAMAGE_X,
       party_base_y + player_index * party_line_h,
       amount,
       0x2C);
}

void PAL_BattleUIShowPlayerMPHeal(int player_index, int amount)
{
   char buf[24];
   int party_base_y;
   int party_line_h;

   party_base_y = PAL_BattleUIPartyBaseY(g_lava_battle_ui_last_enemy_count);
   party_line_h = PAL_BattleUIPartyLineH(g_lava_battle_ui_last_enemy_count);
   sprintf(buf, "气+%d", amount);
   PAL_LavaDrawShadowText(
      LAVA_BATTLE_DAMAGE_X - 18,
      party_base_y + player_index * party_line_h,
      buf,
      0xCF);
   VIDEO_UpdateScreen(0);
   Delay(120);
}

static char *PAL_BattleUIBuildRoundMessage(int player_side)
{
   static char buf[96];
   char base[96];
   int arg0;
   int arg1;
   char enemy_name[16];
   int event_type;

   buf[0] = 0;
   base[0] = 0;
   if (player_side)
   {
      event_type = PAL_LavaFightGetRoundEventType(TRUE);
      arg0 = PAL_LavaFightGetRoundEventArg0(TRUE);
      arg1 = PAL_LavaFightGetRoundEventArg1(TRUE);
   }
   else
   {
      event_type = PAL_LavaFightGetRoundEventType(FALSE);
      arg0 = PAL_LavaFightGetRoundEventArg0(FALSE);
      arg1 = PAL_LavaFightGetRoundEventArg1(FALSE);
   }

   if (event_type == LAVA_BATTLE_EVENT_PLAYER_FLEE_OK)
   {
      sprintf(base, "撤退成功");
   }
   else if (event_type == LAVA_BATTLE_EVENT_PLAYER_FLEE_FAIL)
   {
      sprintf(base, "撤退失败");
   }
   else if (event_type == LAVA_BATTLE_EVENT_PLAYER_DEFEND)
   {
      sprintf(base, "摆好架势");
   }
   else if (event_type == LAVA_BATTLE_EVENT_PLAYER_HIT)
   {
      sprintf(base, "命中目标-%d", arg1);
   }
   else if (event_type == LAVA_BATTLE_EVENT_PLAYER_HIT_KO)
   {
      sprintf(base, "命中目标-%d，敌人倒下", arg1);
   }
   else if (event_type == LAVA_BATTLE_EVENT_PLAYER_STRONG_HIT)
   {
      sprintf(base, "重击命中-%d", arg1);
   }
   else if (event_type == LAVA_BATTLE_EVENT_PLAYER_STRONG_HIT_KO)
   {
      sprintf(base, "重击命中-%d，敌人倒下", arg1);
   }
   else if (event_type == LAVA_BATTLE_EVENT_PLAYER_HIT_ALL)
   {
      sprintf(base, "横扫全体");
   }
   else if (event_type == LAVA_BATTLE_EVENT_PLAYER_HIT_ALL_KO)
   {
      sprintf(base, "横扫全体，敌阵大乱");
   }
   else if (event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_HIT)
   {
      sprintf(base, "%s命中-%d", PAL_BattleUIFormatMagicName(arg0), arg1);
   }
   else if (event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_HIT_KO)
   {
      sprintf(base, "%s命中-%d，敌人倒下", PAL_BattleUIFormatMagicName(arg0), arg1);
   }
   else if (event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_EMPTY)
   {
      sprintf(base, "尚未习得仙术");
   }
   else if (event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_NOMP)
   {
      sprintf(base, "%s耗气%d，真气不足", PAL_BattleUIFormatMagicName(arg0), arg1);
   }
   else if (event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_ALL)
   {
      sprintf(base, "%s席卷全场", PAL_BattleUIFormatMagicName(arg0));
   }
    else if (event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_ALL_KO)
    {
       sprintf(base, "%s席卷全场，敌阵溃散", PAL_BattleUIFormatMagicName(arg0));
    }
    else if (event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_HIT)
    {
       sprintf(base, "%s协力命中-%d", PAL_BattleUIFormatMagicName(arg0), arg1);
    }
    else if (event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_HIT_KO)
    {
       sprintf(base, "%s协力命中-%d，敌人倒下", PAL_BattleUIFormatMagicName(arg0), arg1);
    }
    else if (event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_ALL)
    {
       sprintf(base, "%s协力发动", PAL_BattleUIFormatMagicName(arg0));
    }
    else if (event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_ALL_KO)
    {
       sprintf(base, "%s协力破敌", PAL_BattleUIFormatMagicName(arg0));
    }
    else if (event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_HELPER)
    {
       if (arg1 == LAVA_BATTLE_HELPER_STATUS_MULTI)
       {
          sprintf(base, "%s协力调整全体状态", PAL_BattleUIFormatMagicName(arg0));
       }
       else if (arg1 > 0 && arg1 <= LAVA_BATTLE_STATUS_COUNT)
       {
          sprintf(base, "%s协力加持%s", PAL_BattleUIFormatMagicName(arg0),
             PAL_LavaBattleStatusName(arg1 - 1));
       }
       else if (arg1 < 0 && -arg1 <= LAVA_BATTLE_STATUS_COUNT)
       {
          sprintf(base, "%s协力解除%s", PAL_BattleUIFormatMagicName(arg0),
             PAL_LavaBattleStatusName((-arg1) - 1));
       }
       else
       {
          sprintf(base, "%s协力施展完毕", PAL_BattleUIFormatMagicName(arg0));
       }
    }
    else if (event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_HEAL)
    {
       sprintf(base, "%s协力回复-%d", PAL_BattleUIFormatMagicName(arg0), arg1);
    }
    else if (event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_HEAL_ALL)
    {
       sprintf(base, "%s协力回复全体", PAL_BattleUIFormatMagicName(arg0));
    }
    else if (event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_MP)
    {
       sprintf(base, "%s协力回复真气-%d", PAL_BattleUIFormatMagicName(arg0), arg1);
    }
    else if (event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_MP_ALL)
    {
       sprintf(base, "%s协力回复真气全体", PAL_BattleUIFormatMagicName(arg0));
    }
    else if (event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_TRANCE)
    {
       sprintf(base, "%s协力令灵力高涨", PAL_BattleUIFormatMagicName(arg0));
    }
   else if (event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_HELPER)
   {
      if (arg1 == LAVA_BATTLE_HELPER_STATUS_MULTI)
      {
         sprintf(base, "%s调整全体状态", PAL_BattleUIFormatMagicName(arg0));
      }
      else if (arg1 > 0 && arg1 <= LAVA_BATTLE_STATUS_COUNT)
      {
         sprintf(base, "%s加持%s", PAL_BattleUIFormatMagicName(arg0),
            PAL_LavaBattleStatusName(arg1 - 1));
      }
      else if (arg1 < 0 && -arg1 <= LAVA_BATTLE_STATUS_COUNT)
      {
         sprintf(base, "%s解除%s", PAL_BattleUIFormatMagicName(arg0),
            PAL_LavaBattleStatusName((-arg1) - 1));
      }
      else
      {
         sprintf(base, "%s施展完毕", PAL_BattleUIFormatMagicName(arg0));
      }
   }
   else if (event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_NO_TARGET)
   {
      sprintf(base, "%s无人可疗", PAL_BattleUIFormatMagicName(arg0));
   }
   else if (event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_HEAL)
   {
      sprintf(base, "%s回复-%d", PAL_BattleUIFormatMagicName(arg0), arg1);
   }
   else if (event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_HEAL_ALL)
   {
      sprintf(base, "%s回复全体", PAL_BattleUIFormatMagicName(arg0));
   }
   else if (event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_TRANCE)
   {
      sprintf(base, "%s令灵力高涨(%d)", PAL_BattleUIFormatMagicName(arg0), arg1);
   }
   else if (event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_MP)
   {
      sprintf(base, "%s回复真气-%d", PAL_BattleUIFormatMagicName(arg0), arg1);
   }
   else if (event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_MP_ALL)
   {
      sprintf(base, "%s回复真气全体", PAL_BattleUIFormatMagicName(arg0));
   }
   else if (event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_SILENCE)
   {
      sprintf(base, "%s被封，无法施术", PAL_LavaRoleName(g_lava_party_role[g_lava_battle_ui_state_copy.acting_player_index]));
   }
    else if (event_type == LAVA_BATTLE_EVENT_PLAYER_SLEEP)
    {
       sprintf(base, "%s沉睡未醒", PAL_LavaRoleName(g_lava_party_role[g_lava_battle_ui_state_copy.acting_player_index]));
    }
    else if (event_type == LAVA_BATTLE_EVENT_PLAYER_ITEM_EMPTY)
    {
       sprintf(base, "暂无可用物品");
    }
     else if (event_type == LAVA_BATTLE_EVENT_PLAYER_ITEM_USE)
     {
        if (arg1 > 0)
       {
          sprintf(base, "%s回复-%d", PAL_BattleUIFormatItemName(arg0), arg1);
       }
       else
       {
           sprintf(base, "使用%s", PAL_BattleUIFormatItemName(arg0));
        }
     }
     else if (event_type == LAVA_BATTLE_EVENT_PLAYER_ITEM_THROW_EMPTY)
     {
        sprintf(base, "暂无可投掷物品");
     }
     else if (event_type == LAVA_BATTLE_EVENT_PLAYER_ITEM_THROW)
     {
        sprintf(base, "投掷%s", PAL_BattleUIFormatItemName(arg0));
     }
   else if (event_type == LAVA_BATTLE_EVENT_ENEMY_IDLE)
   {
      sprintf(buf, "敌方逼近");
   }
   else if (event_type == LAVA_BATTLE_EVENT_ENEMY_HIT)
   {
      PAL_BattleUIFormatEnemyName(enemy_name, sizeof(enemy_name), PAL_LavaFightGetRoundEnemyObjectID());
      sprintf(buf, "%s扑来，%s受击-%d", enemy_name, PAL_LavaRoleName(g_lava_party_role[arg0]), arg1);
   }
   else if (event_type == LAVA_BATTLE_EVENT_ENEMY_HIT_KO)
   {
      PAL_BattleUIFormatEnemyName(enemy_name, sizeof(enemy_name), PAL_LavaFightGetRoundEnemyObjectID());
      sprintf(buf, "%s扑来，%s倒下", enemy_name, PAL_LavaRoleName(g_lava_party_role[arg0]));
   }
   else if (event_type == LAVA_BATTLE_EVENT_ENEMY_FLEE_FAIL_HIT)
   {
      PAL_BattleUIFormatEnemyName(enemy_name, sizeof(enemy_name), PAL_LavaFightGetRoundEnemyObjectID());
      sprintf(buf, "撤退失败，%s追击，%s受击-%d", enemy_name, PAL_LavaRoleName(g_lava_party_role[arg0]), arg1);
   }
   else if (event_type == LAVA_BATTLE_EVENT_ENEMY_FLEE_FAIL_KO)
   {
      PAL_BattleUIFormatEnemyName(enemy_name, sizeof(enemy_name), PAL_LavaFightGetRoundEnemyObjectID());
      sprintf(buf, "撤退失败，%s追击，%s倒下", enemy_name, PAL_LavaRoleName(g_lava_party_role[arg0]));
   }
   else if (event_type == LAVA_BATTLE_EVENT_ENEMY_STRONG_HIT)
   {
      PAL_BattleUIFormatEnemyName(enemy_name, sizeof(enemy_name), PAL_LavaFightGetRoundEnemyObjectID());
      sprintf(buf, "%s重击，%s受击-%d", enemy_name, PAL_LavaRoleName(g_lava_party_role[arg0]), arg1);
   }
   else if (event_type == LAVA_BATTLE_EVENT_ENEMY_STRONG_HIT_KO)
   {
      PAL_BattleUIFormatEnemyName(enemy_name, sizeof(enemy_name), PAL_LavaFightGetRoundEnemyObjectID());
      sprintf(buf, "%s重创%s", enemy_name, PAL_LavaRoleName(g_lava_party_role[arg0]));
   }
   else if (event_type == LAVA_BATTLE_EVENT_ENEMY_MAGIC_HIT)
   {
      PAL_BattleUIFormatEnemyName(enemy_name, sizeof(enemy_name), PAL_LavaFightGetRoundEnemyObjectID());
      sprintf(buf, "%s施展妖术，%s受击-%d", enemy_name, PAL_LavaRoleName(g_lava_party_role[arg0]), arg1);
   }
   else if (event_type == LAVA_BATTLE_EVENT_ENEMY_MAGIC_HIT_KO)
   {
      PAL_BattleUIFormatEnemyName(enemy_name, sizeof(enemy_name), PAL_LavaFightGetRoundEnemyObjectID());
      sprintf(buf, "%s妖术重创%s", enemy_name, PAL_LavaRoleName(g_lava_party_role[arg0]));
   }
   else if (event_type == LAVA_BATTLE_EVENT_ENEMY_MAGIC_ALL)
   {
      PAL_BattleUIFormatEnemyName(enemy_name, sizeof(enemy_name), PAL_LavaFightGetRoundEnemyObjectID());
      sprintf(buf, "%s施展妖术，全体受击-%d", enemy_name, arg1);
   }
   else if (event_type == LAVA_BATTLE_EVENT_ENEMY_MAGIC_ALL_KO)
   {
      PAL_BattleUIFormatEnemyName(enemy_name, sizeof(enemy_name), PAL_LavaFightGetRoundEnemyObjectID());
      sprintf(buf, "%s妖术袭来，有人倒下", enemy_name);
   }
   else if (event_type == LAVA_BATTLE_EVENT_ENEMY_POWER_HIT)
   {
      PAL_BattleUIFormatEnemyName(enemy_name, sizeof(enemy_name), PAL_LavaFightGetRoundEnemyObjectID());
      sprintf(buf, "%s猛攻，%s受击-%d", enemy_name, PAL_LavaRoleName(g_lava_party_role[arg0]), arg1);
   }
   else if (event_type == LAVA_BATTLE_EVENT_ENEMY_POWER_HIT_KO)
   {
      PAL_BattleUIFormatEnemyName(enemy_name, sizeof(enemy_name), PAL_LavaFightGetRoundEnemyObjectID());
      sprintf(buf, "%s猛攻击倒%s", enemy_name, PAL_LavaRoleName(g_lava_party_role[arg0]));
   }
   else if (event_type == LAVA_BATTLE_EVENT_ENEMY_COVER_HIT)
   {
      PAL_BattleUIFormatEnemyName(enemy_name, sizeof(enemy_name), PAL_LavaFightGetRoundEnemyObjectID());
      sprintf(buf, "%s扑来，%s护住同伴，受击-%d", enemy_name, PAL_LavaRoleName(g_lava_party_role[arg0]), arg1);
   }
   else if (event_type == LAVA_BATTLE_EVENT_ENEMY_COVER_KO)
   {
      PAL_BattleUIFormatEnemyName(enemy_name, sizeof(enemy_name), PAL_LavaFightGetRoundEnemyObjectID());
      sprintf(base, "%s扑来，%s护住同伴后倒下", enemy_name, PAL_LavaRoleName(g_lava_party_role[arg0]));
   }
   else if (event_type == LAVA_BATTLE_EVENT_ENEMY_MISS)
   {
      PAL_BattleUIFormatEnemyName(enemy_name, sizeof(enemy_name), PAL_LavaFightGetRoundEnemyObjectID());
      if (arg1 != 0)
      {
         sprintf(buf, "%s扑来，%s护住同伴后闪开", enemy_name, PAL_LavaRoleName(g_lava_party_role[arg0]));
      }
      else
      {
         sprintf(buf, "%s扑来，%s闪开", enemy_name, PAL_LavaRoleName(g_lava_party_role[arg0]));
      }
   }

   if (player_side &&
       PAL_LavaFightGetRoundPlayerTranceBoost() &&
       (event_type == LAVA_BATTLE_EVENT_PLAYER_HIT ||
        event_type == LAVA_BATTLE_EVENT_PLAYER_HIT_KO ||
        event_type == LAVA_BATTLE_EVENT_PLAYER_STRONG_HIT ||
        event_type == LAVA_BATTLE_EVENT_PLAYER_STRONG_HIT_KO ||
        event_type == LAVA_BATTLE_EVENT_PLAYER_HIT_ALL ||
        event_type == LAVA_BATTLE_EVENT_PLAYER_HIT_ALL_KO ||
        event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_HIT ||
         event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_HIT_KO ||
         event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_ALL ||
          event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_ALL_KO ||
          event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_HIT ||
          event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_HIT_KO ||
          event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_ALL ||
          event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_ALL_KO ||
          event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_HEAL ||
          event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_HEAL_ALL ||
          event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_HEAL ||
          event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_HEAL_ALL ||
          event_type == LAVA_BATTLE_EVENT_PLAYER_ITEM_USE ||
          event_type == LAVA_BATTLE_EVENT_PLAYER_ITEM_THROW))
   {
      sprintf(buf, "灵力加持，%s", base);
   }
   else if (base[0] != 0)
   {
      sprintf(buf, "%s", base);
   }

   return buf;
}

void PAL_BattleUIShowRoundResult(int *round_result)
{
   int acting_player_index;
   int enemy_event_type;
   int player_event_type;
   int flags;
   int phase;
   int player_magic_effect_played;
   int step;
   int enemy_visibility_active;
   int enemy_hp_before_player_action[LAVA_BATTLE_MAX_ENEMIES];

   if (round_result == 0)
   {
      return;
   }

   flags = round_result[LAVA_BATTLE_ROUND_FLAGS];
   enemy_event_type = PAL_LavaFightGetRoundEventType(FALSE);
   player_event_type = PAL_LavaFightGetRoundEventType(TRUE);
   acting_player_index = g_lava_battle_ui_state_valid ?
      g_lava_battle_ui_state_copy.acting_player_index : 0;
   player_magic_effect_played = FALSE;
   enemy_visibility_active = FALSE;
   PAL_BattleUIClearPartyPoses();
   if ((flags & LAVA_BATTLE_ROUND_FLAG_PLAYER_DAMAGE) &&
       (PAL_BattleUIIsPlayerMagicDamageEvent(player_event_type) ||
        PAL_BattleUIIsPlayerAttackEvent(player_event_type)))
   {
      int hit_count;

      hit_count = round_result[LAVA_BATTLE_ROUND_PLAYER_HIT_COUNT];
      if (hit_count <= 0)
      {
         hit_count = 1;
      }
      PAL_BattleUIKeepEnemyVisibleForMagic(round_result, hit_count,
         enemy_hp_before_player_action);
      enemy_visibility_active = TRUE;
   }
   if (PAL_LavaFightGetRoundEventArg0(TRUE) == 0x143 ||
       PAL_LavaFightGetRoundEventArg0(TRUE) == 0x145 ||
       PAL_LavaFightGetRoundEventArg0(TRUE) == 0x156 ||
       g_lava_battle_selected_magic_object_id == 0x143 ||
       g_lava_battle_selected_magic_object_id == 0x145 ||
       g_lava_battle_selected_magic_object_id == 0x156)
   {
      printf("[LAVA][MAGICROUND] show actor=%d flags=%d event=%d arg0=%d arg1=%d target=%d damage=%d hits=%d steps=%d,%d,%d,%d\n",
         acting_player_index, flags, player_event_type,
         PAL_LavaFightGetRoundEventArg0(TRUE), PAL_LavaFightGetRoundEventArg1(TRUE),
         round_result[LAVA_BATTLE_ROUND_PLAYER_TARGET],
         round_result[LAVA_BATTLE_ROUND_PLAYER_DAMAGE],
         round_result[LAVA_BATTLE_ROUND_PLAYER_HIT_COUNT],
         round_result[LAVA_BATTLE_ROUND_STEP0],
         round_result[LAVA_BATTLE_ROUND_STEP1],
         round_result[LAVA_BATTLE_ROUND_STEP2],
         round_result[LAVA_BATTLE_ROUND_STEP3]);
   }
   if (g_lava_autotest_fengshen)
   {
      printf("[LAVA][FENGSHEN] show-round flags=%d player_event=%d enemy_event=%d steps=%d,%d,%d,%d\n",
         flags, player_event_type, enemy_event_type,
         round_result[LAVA_BATTLE_ROUND_STEP0],
         round_result[LAVA_BATTLE_ROUND_STEP1],
         round_result[LAVA_BATTLE_ROUND_STEP2],
         round_result[LAVA_BATTLE_ROUND_STEP3]);
   }

   for (step = 0; step < 4; step++)
   {
      phase = round_result[LAVA_BATTLE_ROUND_STEP0 + step];
      if (g_lava_autotest_fengshen ||
          PAL_LavaFightGetRoundEventArg0(TRUE) == 0x145 ||
          PAL_LavaFightGetRoundEventArg0(TRUE) == 0x156 ||
          g_lava_battle_selected_magic_object_id == 0x145 ||
          g_lava_battle_selected_magic_object_id == 0x156)
      {
         printf("[LAVA][MAGICPHASE] step=%d phase=%d event=%d arg0=%d target=%d flags=%d\n",
            step, phase, player_event_type, PAL_LavaFightGetRoundEventArg0(TRUE),
            round_result[LAVA_BATTLE_ROUND_PLAYER_TARGET], flags);
      }
      if (phase == LAVA_BATTLE_PHASE_PLAYER_MESSAGE)
      {
         char *player_message;

         player_message = PAL_BattleUIBuildRoundMessage(TRUE);
             if ((flags & LAVA_BATTLE_ROUND_FLAG_PLAYER_MESSAGE) &&
                 player_message != 0 && player_message[0] != 0)
             {
            if (player_event_type == LAVA_BATTLE_EVENT_PLAYER_HIT ||
                player_event_type == LAVA_BATTLE_EVENT_PLAYER_HIT_KO ||
                player_event_type == LAVA_BATTLE_EVENT_PLAYER_STRONG_HIT ||
                player_event_type == LAVA_BATTLE_EVENT_PLAYER_STRONG_HIT_KO ||
                player_event_type == LAVA_BATTLE_EVENT_PLAYER_HIT_ALL ||
                player_event_type == LAVA_BATTLE_EVENT_PLAYER_HIT_ALL_KO)
             {
                PAL_BattleUISetPartyPose(acting_player_index, 8);
             }
             else if (player_event_type == LAVA_BATTLE_EVENT_PLAYER_FLEE_OK)
             {
                PAL_BattleUIPlayPlayerFleeSuccessMotion();
             }
             else if (player_event_type == LAVA_BATTLE_EVENT_PLAYER_FLEE_FAIL)
             {
                PAL_BattleUIPlayPlayerFleeFailMotion(acting_player_index);
             }
             else if (player_event_type == LAVA_BATTLE_EVENT_PLAYER_ITEM_USE)
             {
                PAL_BattleUIPlayPlayerUseItemMotion(acting_player_index,
                   round_result[LAVA_BATTLE_ROUND_PLAYER_TARGET], player_message);
             }
             else if (player_event_type == LAVA_BATTLE_EVENT_PLAYER_ITEM_THROW)
             {
                PAL_BattleUIPlayPlayerThrowItemMotion(acting_player_index, player_message);
             }
             else if (PAL_BattleUIIsPlayerCoopEvent(player_event_type))
             {
                PAL_BattleUISetCoopPartyPose();
            }
            else if (player_event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_HIT ||
                      player_event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_HIT_KO ||
                      player_event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_ALL ||
                      player_event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_ALL_KO ||
                      player_event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_HELPER ||
                      player_event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_HEAL ||
                     player_event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_HEAL_ALL ||
                     player_event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_MP ||
                      player_event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_MP_ALL ||
                      player_event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_SILENCE ||
                      player_event_type == LAVA_BATTLE_EVENT_PLAYER_SLEEP ||
                      player_event_type == LAVA_BATTLE_EVENT_PLAYER_ITEM_EMPTY ||
                      player_event_type == LAVA_BATTLE_EVENT_PLAYER_ITEM_THROW ||
                      player_event_type == LAVA_BATTLE_EVENT_PLAYER_ITEM_THROW_EMPTY ||
                      player_event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_TRANCE ||
                      player_event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_NOMP ||
                     player_event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_NO_TARGET)
            {
               PAL_BattleUISetPartyPose(acting_player_index, 5);
            }
            else if (player_event_type == LAVA_BATTLE_EVENT_PLAYER_DEFEND)
            {
               PAL_BattleUISetPartyPose(acting_player_index, 3);
            }
            if ((g_lava_autotest_search || g_lava_autotest_load) &&
                g_lava_battle_ui_state_valid &&
                g_lava_battle_ui_state_copy.party_pose[acting_player_index] > 0)
            {
               printf("[LAVA][BATTLEUI] round pose player=%d frame=%d event=%d\n",
                  acting_player_index,
                  g_lava_battle_ui_state_copy.party_pose[acting_player_index],
                  player_event_type);
                 }
	                 PAL_BattleUIRedrawRoundFrame(player_message);
	                 if (!PAL_BattleUIIsPlayerMagicDamageEvent(player_event_type))
	                 {
	                    PAL_BattleUIPlayPlayerSupportMagicEffectOnce(player_event_type,
	                       round_result[LAVA_BATTLE_ROUND_PLAYER_TARGET]);
	                 }
	                 else
	                 {
	                    PAL_BattleUIPlayPlayerMagicEffectOnce(player_event_type,
	                       round_result[LAVA_BATTLE_ROUND_PLAYER_TARGET]);
	                    player_magic_effect_played = TRUE;
	                 }
	                 Delay(round_result[LAVA_BATTLE_ROUND_PLAYER_DELAY]);
	                 if (player_event_type == LAVA_BATTLE_EVENT_PLAYER_FLEE_FAIL)
	                 {
	                    PAL_BattleUIClearPartyPoses();
	                    PAL_BattleUISetPartyOffset(acting_player_index, 0, 0);
	                 }
	              }
      }
      else if (phase == LAVA_BATTLE_PHASE_PLAYER_DAMAGE)
      {
         if (flags & LAVA_BATTLE_ROUND_FLAG_PLAYER_DAMAGE)
         {
            int effect_played;
            int hit_count;
            int i;
            int target;
            int damage;

            effect_played = FALSE;
            hit_count = round_result[LAVA_BATTLE_ROUND_PLAYER_HIT_COUNT];
            if (hit_count <= 0)
            {
               hit_count = 1;
            }

            if (PAL_BattleUIIsPlayerMagicDamageEvent(player_event_type) &&
                !effect_played && !player_magic_effect_played)
            {
               if (g_lava_autotest_fengshen)
               {
                  printf("[LAVA][FENGSHEN] before-player-magic-effect event=%d target=%d hits=%d\n",
                     player_event_type,
                     round_result[LAVA_BATTLE_ROUND_PLAYER_TARGET], hit_count);
               }
               PAL_BattleUIPlayPlayerMagicEffectOnce(player_event_type,
                  round_result[LAVA_BATTLE_ROUND_PLAYER_TARGET]);
               if (g_lava_autotest_fengshen)
               {
                  printf("[LAVA][FENGSHEN] after-player-magic-effect\n");
               }
               effect_played = TRUE;
            }

            if (PAL_BattleUIIsPlayerCoopEvent(player_event_type))
            {
               PAL_BattleUIPlayCoopCastPose(round_result[LAVA_BATTLE_ROUND_PLAYER_DELAY]);
            }
            else if (PAL_BattleUIIsPlayerAttackEvent(player_event_type))
            {
               PAL_BattleUIPlayPlayerAttackMotion(acting_player_index,
                  round_result[LAVA_BATTLE_ROUND_PLAYER_TARGET]);
            }

            for (i = 0; i < hit_count; i++)
            {
               if (i == 0)
               {
                  target = round_result[LAVA_BATTLE_ROUND_PLAYER_TARGET];
                  damage = round_result[LAVA_BATTLE_ROUND_PLAYER_DAMAGE];
               }
               else
               {
                  int base;

                  base = LAVA_BATTLE_ROUND_PLAYER_TARGET1 + (i - 1) * 2;
                  target = round_result[base];
                  damage = round_result[base + 1];
               }

               if (target >= 0 && damage > 0)
               {
                  if (player_event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_HEAL ||
                      player_event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_HEAL_ALL ||
                      player_event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_HEAL ||
                      player_event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_HEAL_ALL ||
                      player_event_type == LAVA_BATTLE_EVENT_PLAYER_ITEM_USE)
                  {
                     PAL_BattleUIShowPlayerHeal(target, damage);
                  }
                  else if (player_event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_MP ||
                           player_event_type == LAVA_BATTLE_EVENT_PLAYER_MAGIC_MP_ALL ||
                           player_event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_MP ||
                           player_event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_MP_ALL)
                  {
                     PAL_BattleUIShowPlayerMPHeal(target, damage);
                  }
                  else
                  {
                     PAL_BattleUIShowEnemyDamage(target, damage);
                  }
               }
            }
            if (enemy_visibility_active)
            {
               PAL_BattleUIRestoreEnemyVisibleForMagic(enemy_hp_before_player_action);
               enemy_visibility_active = FALSE;
            }
            PAL_BattleUIClearSummonHold();
            PAL_BattleUIClearPartyPoses();
         }
      }
      else if (phase == LAVA_BATTLE_PHASE_ENEMY_MESSAGE)
      {
         char *enemy_message;

         enemy_message = PAL_BattleUIBuildRoundMessage(FALSE);
         if ((flags & LAVA_BATTLE_ROUND_FLAG_ENEMY_MESSAGE) &&
             enemy_message != 0 && enemy_message[0] != 0)
         {
            PAL_BattleUIClearPartyPoses();
            PAL_BattleUIRedrawRoundFrame(enemy_message);
            Delay(round_result[LAVA_BATTLE_ROUND_ENEMY_DELAY]);
         }
      }
       else if (phase == LAVA_BATTLE_PHASE_ENEMY_DAMAGE)
       {
          if (flags & LAVA_BATTLE_ROUND_FLAG_ENEMY_DAMAGE)
          {
             int enemy_damage;
             int enemy_hit_count;
             int enemy_hit_index;
             int enemy_target;

             enemy_hit_count = round_result[LAVA_BATTLE_ROUND_ENEMY_HIT_COUNT];
             if (enemy_hit_count <= 0)
             {
                enemy_hit_count = 1;
             }
             for (enemy_hit_index = 0; enemy_hit_index < enemy_hit_count; enemy_hit_index++)
             {
                if (enemy_hit_index == 0)
                {
                   enemy_target = round_result[LAVA_BATTLE_ROUND_ENEMY_TARGET];
                   enemy_damage = round_result[LAVA_BATTLE_ROUND_ENEMY_DAMAGE];
                }
                else
                {
                   int enemy_base;

                   enemy_base = LAVA_BATTLE_ROUND_ENEMY_TARGET1 + enemy_hit_index * 2;
                   enemy_target = round_result[enemy_base];
                   enemy_damage = round_result[enemy_base + 1];
                }
                if (enemy_target < 0 || enemy_damage <= 0)
                {
                   continue;
                }
                 if (enemy_hit_index == 0)
                 {
                    PAL_BattleUIPlayEnemyMagicEffectOnce(enemy_event_type, enemy_target);
                    if (enemy_event_type == LAVA_BATTLE_EVENT_ENEMY_HIT ||
                        enemy_event_type == LAVA_BATTLE_EVENT_ENEMY_HIT_KO ||
                        enemy_event_type == LAVA_BATTLE_EVENT_ENEMY_POWER_HIT ||
                        enemy_event_type == LAVA_BATTLE_EVENT_ENEMY_POWER_HIT_KO ||
                        enemy_event_type == LAVA_BATTLE_EVENT_ENEMY_COVER_HIT ||
                        enemy_event_type == LAVA_BATTLE_EVENT_ENEMY_COVER_KO ||
                        enemy_event_type == LAVA_BATTLE_EVENT_ENEMY_FLEE_FAIL_HIT ||
                        enemy_event_type == LAVA_BATTLE_EVENT_ENEMY_FLEE_FAIL_KO)
                    {
                       PAL_BattleUIPlayEnemyAttackMotion(enemy_target);
                    }
                 }
                PAL_BattleUISetPartyPose(enemy_target, 4);
                if (g_lava_autotest_search || g_lava_autotest_load)
                {
                   printf("[LAVA][BATTLEUI] round pose player=%d frame=4 event=%d\n",
                      enemy_target, enemy_event_type);
                }
                PAL_BattleUIRedrawRoundFrame(PAL_BattleUIBuildRoundMessage(FALSE));
                PAL_BattleUIShowPlayerDamage(enemy_target, enemy_damage);
             }
             PAL_BattleUIClearPartyPoses();
          }
       }
      else
      {
         break;
      }
   }

   if (enemy_visibility_active)
   {
      PAL_BattleUIRestoreEnemyVisibleForMagic(enemy_hp_before_player_action);
   }
}

int PAL_BattleUIWaitForPlayerAction(LAVA_BATTLE_STATE *state)
{
   char *display_message;
   int next_actor;
   int need_redraw;
   int ui_state;
   int magic_count;

    if (state == 0)
    {
      return LAVA_BATTLE_COMMAND_FLEE;
   }

   next_actor = state->acting_player_index;
   if (next_actor < 0 || next_actor >= 3 || state->party_hp[next_actor] <= 0)
   {
      next_actor = PAL_LavaFightPickTurnActor(state->party_hp, state->turn);
   }
   state->acting_player_index = next_actor;
   state->command_sel = g_lava_battle_pending_command[next_actor];
   state->target_sel = g_lava_battle_pending_target[next_actor];
   state->magic_sel = g_lava_battle_pending_magic_sel[next_actor];
   state->magic_object_id = g_lava_battle_pending_magic_object_id[next_actor];
   state->item_sel = g_lava_battle_pending_item_sel[next_actor];
   state->item_object_id = g_lava_battle_pending_item_object_id[next_actor];

   need_redraw = 1;
   ui_state = LAVA_BATTLE_UI_STATE_COMMAND;
   if (state->acting_player_index >= 0 && state->acting_player_index < 6)
   {
      state->magic_sel = g_lava_battle_last_magic_sel[state->acting_player_index];
      PAL_BattleUIClampMagicSel(state);
   }
   if (g_lava_autotest_search || g_lava_autotest_load)
   {
      printf("[LAVA][BATTLEUI] wait actor=%d enemy_count=%d command=%d\n",
         state->acting_player_index, state->enemy_count, state->command_sel);
   }

   while (TRUE)
   {
      if (need_redraw)
      {
         need_redraw = 0;
         display_message = state->message;
          if (ui_state == LAVA_BATTLE_UI_STATE_TARGET)
          {
             display_message = PAL_BattleUIBuildEnemyTargetPrompt(state);
          }
          else if (ui_state == LAVA_BATTLE_UI_STATE_TARGET_ALL)
          {
             display_message = PAL_BattleUIBuildAllTargetPrompt(FALSE);
          }
           else if (ui_state == LAVA_BATTLE_UI_STATE_PARTY_TARGET)
           {
              display_message = PAL_BattleUIBuildPartyTargetPrompt(state);
           }
           else if (ui_state == LAVA_BATTLE_UI_STATE_PARTY_TARGET_ALL)
           {
              display_message = PAL_BattleUIBuildAllTargetPrompt(TRUE);
           }
          else if (ui_state == LAVA_BATTLE_UI_STATE_ITEM)
          {
             display_message = PAL_BattleUIBuildItemPrompt(state);
          }
           else if (ui_state == LAVA_BATTLE_UI_STATE_MISC)
           {
              display_message = PAL_BattleUIBuildMiscPrompt();
           }
           else if (ui_state == LAVA_BATTLE_UI_STATE_ITEM_SUBMENU)
           {
              display_message = "选择物品用法";
           }
           else if (ui_state == LAVA_BATTLE_UI_STATE_STATUS)
           {
              display_message = "队伍状态";
          }
         PAL_BattleUIDrawFrame(state,
            ui_state,
            ui_state == LAVA_BATTLE_UI_STATE_TARGET ? state->target_sel :
             (ui_state == LAVA_BATTLE_UI_STATE_PARTY_TARGET ? (-100 - state->target_sel) : -1),
             display_message);
         VIDEO_UpdateScreen(0);
         if (g_lava_battle_ui_pending_fade_in)
         {
            PAL_FadeIn(1);
            g_lava_battle_ui_pending_fade_in = 0;
            if (g_lava_autotest_search || g_lava_autotest_load)
            {
               printf("[LAVA][BATTLEUI] first-frame-visible\n");
            }
         }
         if (g_lava_autotest_battle)
         {
            int smoke_magic_sel;

            smoke_magic_sel = PAL_BattleUIBestMagicSel(state);
            if (g_lava_autotest_fengshen && g_lava_battle_ui_autotest_turn < 2)
            {
               state->command_sel = LAVA_BATTLE_COMMAND_MAGIC;
               state->magic_object_id = 0x13B;
               state->target_sel = PAL_LavaFightFixEnemyTarget(
                  state->target_sel, state->enemy_count, state->enemy_hp);
               g_lava_battle_ui_autotest_magic_done++;
            }
            else if (g_lava_autotest_xueyao && g_lava_battle_ui_autotest_turn == 0)
            {
               state->command_sel = LAVA_BATTLE_COMMAND_MAGIC;
               state->magic_object_id = 0x145;
               state->target_sel = PAL_LavaFightFixEnemyTarget(
                  state->target_sel, state->enemy_count, state->enemy_hp);
               g_lava_battle_ui_autotest_magic_done++;
            }
            else if (g_lava_autotest_xueyao && g_lava_battle_ui_autotest_turn == 1)
            {
               state->command_sel = LAVA_BATTLE_COMMAND_MAGIC;
               state->magic_object_id = 0x156;
               state->target_sel = PAL_LavaFightFixEnemyTarget(
                  state->target_sel, state->enemy_count, state->enemy_hp);
               g_lava_battle_ui_autotest_magic_done++;
            }
            else if (g_lava_battle_ui_autotest_turn < 2 && smoke_magic_sel >= 0)
            {
               state->command_sel = LAVA_BATTLE_COMMAND_MAGIC;
               state->magic_sel = smoke_magic_sel;
               state->magic_object_id = PAL_LavaFightMagicByIndex(
                  PAL_BattleUIActingRole(state), smoke_magic_sel);
               state->target_sel = PAL_LavaFightFixEnemyTarget(
                  state->target_sel, state->enemy_count, state->enemy_hp);
               g_lava_battle_ui_autotest_magic_done++;
            }
            else if (g_lava_autotest_battle_magic_seen < 3)
            {
               state->command_sel = LAVA_BATTLE_COMMAND_DEFEND;
               state->magic_object_id = 0;
            }
            else
            {
               state->command_sel = LAVA_BATTLE_COMMAND_FLEE;
               state->magic_object_id = 0;
            }
            printf("[LAVA][BATTLESMOKE] command actor=%d command=%d magic=%d selected=%d\n",
               state->acting_player_index, state->command_sel,
               state->magic_object_id, g_lava_battle_ui_autotest_magic_done);
            g_lava_battle_ui_autotest_turn++;
            break;
         }
         if (g_lava_autotest_load)
         {
            if (g_lava_battle_ui_autotest_turn == 0)
            {
               state->command_sel = LAVA_BATTLE_COMMAND_ATTACK;
               g_lava_battle_ui_autotest_turn = 1;
            }
            else if (g_lava_battle_ui_autotest_turn == 1)
            {
               state->magic_object_id = PAL_LavaFightMagicByIndex(
                  PAL_BattleUIActingRole(state), 0);
               if (state->magic_object_id > 0)
               {
                  state->command_sel = LAVA_BATTLE_COMMAND_MAGIC;
                  state->target_sel = PAL_LavaFightFixEnemyTarget(
                     state->target_sel, state->enemy_count, state->enemy_hp);
                  g_lava_battle_ui_autotest_magic_done = 1;
               }
               else
               {
                  state->command_sel = LAVA_BATTLE_COMMAND_DEFEND;
               }
               g_lava_battle_ui_autotest_turn = 2;
            }
            else if (g_lava_battle_ui_autotest_turn == 2)
            {
               state->magic_object_id = 0;
               if (!g_lava_battle_ui_autotest_magic_done)
               {
                  state->magic_object_id = PAL_LavaFightMagicByIndex(
                     PAL_BattleUIActingRole(state), 0);
                  if (state->magic_object_id > 0)
                  {
                     state->command_sel = LAVA_BATTLE_COMMAND_MAGIC;
                     state->target_sel = PAL_LavaFightFixEnemyTarget(
                        state->target_sel, state->enemy_count, state->enemy_hp);
                     g_lava_battle_ui_autotest_magic_done = 1;
                  }
                  else
                  {
                     state->command_sel = LAVA_BATTLE_COMMAND_DEFEND;
                  }
               }
               else
               {
                  state->command_sel = LAVA_BATTLE_COMMAND_DEFEND;
               }
               g_lava_battle_ui_autotest_turn = 3;
            }
            else
            {
               state->command_sel = LAVA_BATTLE_COMMAND_FLEE;
               state->magic_object_id = 0;
            }
            if (g_lava_autotest_search || g_lava_autotest_load)
            {
               printf("[LAVA][BATTLEUI] autotest command=%d magic=%d done=%d\n",
                  state->command_sel, state->magic_object_id,
                  g_lava_battle_ui_autotest_magic_done);
            }
            break;
          }
      }

      PAL_ProcessEvent();

      if (PAL_LavaReadCancelKey())
      {
         PAL_ClearKeyState();
          if (ui_state == LAVA_BATTLE_UI_STATE_TARGET ||
              ui_state == LAVA_BATTLE_UI_STATE_TARGET_ALL ||
                ui_state == LAVA_BATTLE_UI_STATE_MAGIC ||
                ui_state == LAVA_BATTLE_UI_STATE_ITEM ||
                ui_state == LAVA_BATTLE_UI_STATE_ITEM_SUBMENU ||
                ui_state == LAVA_BATTLE_UI_STATE_MISC ||
               ui_state == LAVA_BATTLE_UI_STATE_STATUS)
           {
             if (ui_state == LAVA_BATTLE_UI_STATE_STATUS)
             {
                ui_state = LAVA_BATTLE_UI_STATE_MISC;
                need_redraw = 1;
                continue;
             }
              if (ui_state == LAVA_BATTLE_UI_STATE_MISC)
              {
                 ui_state = LAVA_BATTLE_UI_STATE_COMMAND;
                 need_redraw = 1;
                 continue;
              }
               if (ui_state == LAVA_BATTLE_UI_STATE_ITEM_SUBMENU)
               {
                  ui_state = LAVA_BATTLE_UI_STATE_MISC;
                  need_redraw = 1;
                  continue;
               }
               if ((ui_state == LAVA_BATTLE_UI_STATE_TARGET ||
                    ui_state == LAVA_BATTLE_UI_STATE_TARGET_ALL) &&
                   state->command_sel == LAVA_BATTLE_COMMAND_THROW_ITEM)
               {
                  ui_state = LAVA_BATTLE_UI_STATE_ITEM;
                  need_redraw = 1;
                  continue;
               }
               ui_state = (state->command_sel == LAVA_BATTLE_COMMAND_MAGIC) ?
                 LAVA_BATTLE_UI_STATE_MAGIC :
                 (state->command_sel == LAVA_BATTLE_COMMAND_USE_ITEM ||
                  state->command_sel == LAVA_BATTLE_COMMAND_THROW_ITEM ?
                    LAVA_BATTLE_UI_STATE_ITEM_SUBMENU : LAVA_BATTLE_UI_STATE_COMMAND);
             if (ui_state == LAVA_BATTLE_UI_STATE_MAGIC && state->command_sel != LAVA_BATTLE_COMMAND_MAGIC)
             {
                ui_state = LAVA_BATTLE_UI_STATE_COMMAND;
             }
             if (ui_state == LAVA_BATTLE_UI_STATE_ITEM && state->command_sel != LAVA_BATTLE_COMMAND_USE_ITEM)
             {
                ui_state = LAVA_BATTLE_UI_STATE_COMMAND;
             }
             if (ui_state == LAVA_BATTLE_UI_STATE_MISC && state->command_sel != LAVA_BATTLE_COMMAND_USE_ITEM)
             {
                ui_state = LAVA_BATTLE_UI_STATE_COMMAND;
             }
             if (ui_state == LAVA_BATTLE_UI_STATE_MAGIC &&
                 state->magic_object_id <= 0)
             {
                ui_state = LAVA_BATTLE_UI_STATE_COMMAND;
             }
            need_redraw = 1;
            continue;
          }
           if (ui_state == LAVA_BATTLE_UI_STATE_PARTY_TARGET)
           {
              ui_state = (state->command_sel == LAVA_BATTLE_COMMAND_USE_ITEM) ?
                 LAVA_BATTLE_UI_STATE_ITEM : LAVA_BATTLE_UI_STATE_MAGIC;
              if (state->command_sel == LAVA_BATTLE_COMMAND_THROW_ITEM)
              {
                 ui_state = LAVA_BATTLE_UI_STATE_ITEM;
              }
              need_redraw = 1;
              continue;
           }
           if (ui_state == LAVA_BATTLE_UI_STATE_PARTY_TARGET_ALL)
           {
              ui_state = (state->command_sel == LAVA_BATTLE_COMMAND_USE_ITEM) ?
                 LAVA_BATTLE_UI_STATE_ITEM : LAVA_BATTLE_UI_STATE_MAGIC;
              if (state->command_sel == LAVA_BATTLE_COMMAND_THROW_ITEM)
              {
                 ui_state = LAVA_BATTLE_UI_STATE_ITEM;
              }
              need_redraw = 1;
              continue;
           }

         return LAVA_BATTLE_COMMAND_FLEE;
      }

      if (ui_state == LAVA_BATTLE_UI_STATE_COMMAND &&
          (g_InputState.dwKeyPress & (kKeyUp | kKeyDown | kKeyLeft | kKeyRight)))
      {
         if (g_InputState.dwKeyPress & kKeyUp)
         {
            state->command_sel = LAVA_BATTLE_COMMAND_ATTACK;
         }
         else if (g_InputState.dwKeyPress & kKeyLeft)
         {
            state->command_sel = LAVA_BATTLE_COMMAND_MAGIC;
         }
          else if (g_InputState.dwKeyPress & kKeyRight)
          {
             state->command_sel = LAVA_BATTLE_COMMAND_COOP_MAGIC;
          }
          else if (g_InputState.dwKeyPress & kKeyDown)
          {
             state->command_sel = LAVA_BATTLE_COMMAND_USE_ITEM;
            }

         need_redraw = 1;
         PAL_ClearKeyState();
           continue;
         }

         if (ui_state == LAVA_BATTLE_UI_STATE_COMMAND &&
             (g_InputState.dwKeyPress & kKeyDefend))
         {
            state->command_sel = LAVA_BATTLE_COMMAND_DEFEND;
            PAL_ClearKeyState();
            break;
         }

         if (ui_state == LAVA_BATTLE_UI_STATE_COMMAND &&
             (g_InputState.dwKeyPress & kKeyFlee))
         {
            state->command_sel = LAVA_BATTLE_COMMAND_FLEE;
            PAL_ClearKeyState();
            break;
         }

         if (ui_state == LAVA_BATTLE_UI_STATE_COMMAND &&
             (g_InputState.dwKeyPress & kKeyUseItem))
         {
            state->command_sel = LAVA_BATTLE_COMMAND_USE_ITEM;
            if (PAL_BattleUIClampItemSel(state) > 0)
            {
               ui_state = LAVA_BATTLE_UI_STATE_ITEM;
            }
            else
            {
               sprintf(state->message, "暂无可用物品");
            }
            need_redraw = 1;
            PAL_ClearKeyState();
            continue;
         }

         if (ui_state == LAVA_BATTLE_UI_STATE_COMMAND &&
             (g_InputState.dwKeyPress & kKeyThrowItem))
         {
            state->command_sel = LAVA_BATTLE_COMMAND_THROW_ITEM;
            if (PAL_BattleUIClampItemSel(state) > 0)
            {
               ui_state = LAVA_BATTLE_UI_STATE_ITEM;
            }
            else
            {
               sprintf(state->message, "暂无可投掷物品");
            }
            need_redraw = 1;
            PAL_ClearKeyState();
            continue;
         }

         if (ui_state == LAVA_BATTLE_UI_STATE_COMMAND &&
              (g_InputState.dwKeyPress & kKeyForce))
          {
             if (PAL_BattleUISelectMagicShortcut(state,
                TRUE))
             {
                printf("[LAVA][HOTKEY] command accepted action=last obj=%d ui=%d\n",
                   state->magic_object_id, ui_state);
                if (PAL_BattleUIStartMagicShortcutTarget(state, &ui_state))
                {
                    need_redraw = 1;
                    PAL_ClearKeyState();
                    continue;
                }
                 break;
               }
              printf("[LAVA][HOTKEY] command failed action=last ui=%d\n", ui_state);
              sprintf(state->message, "当前无法施术");
             need_redraw = 1;
            PAL_ClearKeyState();
            continue;
         }

         if (ui_state == LAVA_BATTLE_UI_STATE_MISC &&
             (g_InputState.dwKeyPress & (kKeyUp | kKeyDown | kKeyLeft | kKeyRight)))
        {
           if (g_InputState.dwKeyPress & (kKeyUp | kKeyLeft))
           {
              g_lava_battle_misc_sel--;
              if (g_lava_battle_misc_sel < 0)
              {
                 g_lava_battle_misc_sel = LAVA_BATTLE_MISC_COUNT - 1;
              }
           }
           else
           {
              g_lava_battle_misc_sel++;
              if (g_lava_battle_misc_sel >= LAVA_BATTLE_MISC_COUNT)
              {
                 g_lava_battle_misc_sel = 0;
              }
           }
           need_redraw = 1;
           PAL_ClearKeyState();
            continue;
         }

         if (ui_state == LAVA_BATTLE_UI_STATE_ITEM_SUBMENU &&
             (g_InputState.dwKeyPress & (kKeyUp | kKeyDown | kKeyLeft | kKeyRight)))
         {
            if (g_InputState.dwKeyPress & (kKeyUp | kKeyLeft))
            {
               g_lava_battle_item_submenu_sel = LAVA_BATTLE_ITEM_SUBMENU_USE;
            }
            else
            {
               g_lava_battle_item_submenu_sel = LAVA_BATTLE_ITEM_SUBMENU_THROW;
            }
            need_redraw = 1;
            PAL_ClearKeyState();
            continue;
         }

      if (ui_state == LAVA_BATTLE_UI_STATE_MAGIC &&
          ((g_InputState.dwKeyPress & (kKeyUp | kKeyDown | kKeyLeft | kKeyRight | kKeyForce | kKeyUseItem)) ||
           (g_InputState.dwKeyPress & (LAVA_BATTLE_KEY_PGUP | LAVA_BATTLE_KEY_PGDN))))
      {
         if (g_InputState.dwKeyPress & kKeyForce)
         {
            if (!PAL_BattleUISelectMagicShortcut(state, TRUE))
            {
               printf("[LAVA][HOTKEY] magic-menu failed action=last\n");
               sprintf(state->message, "当前无法施术");
            }
            else
            {
               printf("[LAVA][HOTKEY] magic-menu accepted action=last obj=%d\n",
                  state->magic_object_id);
            }
         }
         else if (g_InputState.dwKeyPress & kKeyUseItem)
         {
            if (!PAL_BattleUISelectMagicShortcut(state, FALSE))
            {
               printf("[LAVA][HOTKEY] magic-menu failed action=best\n");
               sprintf(state->message, "当前无法施术");
            }
            else
            {
               printf("[LAVA][HOTKEY] magic-menu accepted action=best obj=%d\n",
                  state->magic_object_id);
            }
         }
         else if (g_InputState.dwKeyPress & kKeyUp)
         {
            PAL_BattleUIMoveMagicSel(state, -LAVA_BATTLE_MAGIC_ITEMS_PER_LINE);
         }
         else if (g_InputState.dwKeyPress & kKeyDown)
         {
            PAL_BattleUIMoveMagicSel(state, LAVA_BATTLE_MAGIC_ITEMS_PER_LINE);
         }
         else if (g_InputState.dwKeyPress & kKeyLeft)
         {
            PAL_BattleUIMoveMagicSel(state, -1);
         }
         else if (g_InputState.dwKeyPress & kKeyRight)
         {
            PAL_BattleUIMoveMagicSel(state, 1);
         }
         else if (g_InputState.dwKeyPress & LAVA_BATTLE_KEY_PGUP)
         {
            state->magic_sel -= LAVA_BATTLE_MAGIC_ITEMS_PER_LINE * LAVA_BATTLE_MAGIC_LINES;
            PAL_BattleUIClampMagicMenuSel(state);
         }
         else if (g_InputState.dwKeyPress & LAVA_BATTLE_KEY_PGDN)
         {
            state->magic_sel += LAVA_BATTLE_MAGIC_ITEMS_PER_LINE * LAVA_BATTLE_MAGIC_LINES;
            PAL_BattleUIClampMagicMenuSel(state);
         }
         else
         {
            magic_count = PAL_LavaFightMagicCount(PAL_BattleUIActingRole(state));
            state->magic_sel = magic_count - 1;
         }
          need_redraw = 1;
         PAL_ClearKeyState();
           continue;
       }

       if (ui_state == LAVA_BATTLE_UI_STATE_ITEM &&
           (g_InputState.dwKeyPress & (kKeyUp | kKeyDown | kKeyLeft | kKeyRight)))
       {
          if (g_InputState.dwKeyPress & kKeyUp)
          {
             state->item_sel -= LAVA_BATTLE_MAGIC_ITEMS_PER_LINE;
          }
          else if (g_InputState.dwKeyPress & kKeyDown)
          {
             state->item_sel += LAVA_BATTLE_MAGIC_ITEMS_PER_LINE;
          }
          else if (g_InputState.dwKeyPress & kKeyLeft)
          {
             state->item_sel--;
          }
          else if (g_InputState.dwKeyPress & kKeyRight)
          {
             state->item_sel++;
          }
          PAL_BattleUIClampItemSel(state);
          need_redraw = 1;
          PAL_ClearKeyState();
          continue;
       }

      if (g_InputState.dwKeyPress & (kKeyUp | kKeyLeft))
      {
         if (ui_state == LAVA_BATTLE_UI_STATE_TARGET)
         {
            state->target_sel = PAL_LavaFightPrevEnemyTarget(state->target_sel, state->enemy_count, state->enemy_hp);
         }
         else if (ui_state == LAVA_BATTLE_UI_STATE_PARTY_TARGET)
         {
            state->target_sel = PAL_LavaFightPrevLivingPartyTarget(state->target_sel, state->party_hp, state->party_hp_max);
         }
          else if (ui_state == LAVA_BATTLE_UI_STATE_MAGIC)
          {
             state->magic_sel--;
             PAL_BattleUIClampMagicSel(state);
          }
          else if (ui_state == LAVA_BATTLE_UI_STATE_ITEM)
          {
             state->item_sel--;
             PAL_BattleUIClampItemSel(state);
          }
          else if (state->command_sel > 0)
          {
             state->command_sel--;
          }

         need_redraw = 1;
         PAL_ClearKeyState();
         continue;
      }

      if (g_InputState.dwKeyPress & (kKeyDown | kKeyRight))
      {
         if (ui_state == LAVA_BATTLE_UI_STATE_TARGET)
         {
            state->target_sel = PAL_LavaFightNextEnemyTarget(state->target_sel, state->enemy_count, state->enemy_hp);
         }
         else if (ui_state == LAVA_BATTLE_UI_STATE_PARTY_TARGET)
         {
            state->target_sel = PAL_LavaFightNextLivingPartyTarget(state->target_sel, state->party_hp, state->party_hp_max);
         }
          else if (ui_state == LAVA_BATTLE_UI_STATE_MAGIC)
          {
             state->magic_sel++;
             PAL_BattleUIClampMagicSel(state);
          }
          else if (ui_state == LAVA_BATTLE_UI_STATE_ITEM)
          {
             state->item_sel++;
             PAL_BattleUIClampItemSel(state);
          }
           else if (state->command_sel < LAVA_BATTLE_COMMAND_USE_ITEM)
            {
               state->command_sel++;
            }

         need_redraw = 1;
         PAL_ClearKeyState();
         continue;
      }

      if (PAL_LavaReadConfirmKey())
      {
         PAL_ClearKeyState();
         if (ui_state == LAVA_BATTLE_UI_STATE_STATUS)
         {
            ui_state = LAVA_BATTLE_UI_STATE_MISC;
            need_redraw = 1;
            continue;
         }
         if (ui_state == LAVA_BATTLE_UI_STATE_COMMAND &&
             state->command_sel == LAVA_BATTLE_COMMAND_MAGIC &&
             PAL_BattleUIClampMagicSel(state) > 0)
         {
            ui_state = LAVA_BATTLE_UI_STATE_MAGIC;
            need_redraw = 1;
             continue;
          }
           if (ui_state == LAVA_BATTLE_UI_STATE_COMMAND &&
               state->command_sel == LAVA_BATTLE_COMMAND_USE_ITEM &&
               PAL_BattleUIClampItemSel(state) > 0)
           {
              ui_state = LAVA_BATTLE_UI_STATE_MISC;
              need_redraw = 1;
              continue;
           }
           if (ui_state == LAVA_BATTLE_UI_STATE_COMMAND &&
               state->command_sel == LAVA_BATTLE_COMMAND_USE_ITEM)
           {
              ui_state = LAVA_BATTLE_UI_STATE_MISC;
              need_redraw = 1;
              continue;
           }
           if (ui_state == LAVA_BATTLE_UI_STATE_COMMAND &&
               state->command_sel == LAVA_BATTLE_COMMAND_COOP_MAGIC)
           {
              if (!PAL_BattleUICoopMagicAvailable(state))
              {
                 sprintf(state->message, "当前无法协力");
                 need_redraw = 1;
                 continue;
              }
           }
           if (ui_state == LAVA_BATTLE_UI_STATE_MISC)
           {
               if (g_lava_battle_misc_sel == LAVA_BATTLE_MISC_ITEM)
               {
                  g_lava_battle_item_submenu_sel = LAVA_BATTLE_ITEM_SUBMENU_USE;
                  ui_state = LAVA_BATTLE_UI_STATE_ITEM_SUBMENU;
                  need_redraw = 1;
                  continue;
               }
               if (g_lava_battle_misc_sel == LAVA_BATTLE_MISC_DEFEND)
                {
                   state->command_sel = LAVA_BATTLE_COMMAND_DEFEND;
                   break;
                }
                if (g_lava_battle_misc_sel == LAVA_BATTLE_MISC_AUTO)
                {
                   if (PAL_BattleUISelectAutoAction(state, &ui_state))
                   {
                      if (ui_state == LAVA_BATTLE_UI_STATE_TARGET ||
                          ui_state == LAVA_BATTLE_UI_STATE_TARGET_ALL ||
                          ui_state == LAVA_BATTLE_UI_STATE_PARTY_TARGET ||
                          ui_state == LAVA_BATTLE_UI_STATE_PARTY_TARGET_ALL)
                      {
                         need_redraw = 1;
                         continue;
                      }
                      break;
                   }
                   sprintf(state->message, "当前无法自动行动");
                   need_redraw = 1;
                   continue;
                }
                if (g_lava_battle_misc_sel == LAVA_BATTLE_MISC_FLEE)
                 {
                    state->command_sel = LAVA_BATTLE_COMMAND_FLEE;
                   break;
                }
               ui_state = LAVA_BATTLE_UI_STATE_STATUS;
               need_redraw = 1;
               continue;
            }
            if (ui_state == LAVA_BATTLE_UI_STATE_ITEM_SUBMENU)
            {
               if (g_lava_battle_item_submenu_sel == LAVA_BATTLE_ITEM_SUBMENU_USE)
               {
                  state->command_sel = LAVA_BATTLE_COMMAND_USE_ITEM;
                  if (PAL_BattleUIClampItemSel(state) > 0)
                  {
                     ui_state = LAVA_BATTLE_UI_STATE_ITEM;
                  }
                  else
                  {
                     sprintf(state->message, "暂无可用物品");
                  }
               }
               else
               {
                  state->command_sel = LAVA_BATTLE_COMMAND_THROW_ITEM;
                  if (PAL_BattleUIClampItemSel(state) > 0)
                  {
                     ui_state = LAVA_BATTLE_UI_STATE_ITEM;
                  }
                  else
                  {
                     sprintf(state->message, "暂无可投掷物品");
                  }
               }
               need_redraw = 1;
               continue;
            }
            if (ui_state == LAVA_BATTLE_UI_STATE_MAGIC)
            {
              state->magic_object_id = PAL_LavaFightMagicByIndex(
                 PAL_BattleUIActingRole(state), state->magic_sel);
            if (state->acting_player_index >= 0 && state->acting_player_index < 6)
            {
               g_lava_battle_last_magic_sel[state->acting_player_index] = state->magic_sel;
            }
              if (PAL_LavaFightMagicApplyToAll(state->magic_object_id))
              {
                 state->target_sel = -1;
                 ui_state = PAL_LavaFightMagicUsableToEnemy(state->magic_object_id) ?
                    LAVA_BATTLE_UI_STATE_TARGET_ALL : LAVA_BATTLE_UI_STATE_PARTY_TARGET_ALL;
                 need_redraw = 1;
                 continue;
              }
              if (PAL_LavaFightMagicNeedsTarget(state->magic_object_id) &&
                  PAL_LavaFightCountAlive(state->enemy_hp, state->enemy_count) > 1)
             {
                state->target_sel = PAL_LavaFightFixEnemyTarget(g_lava_battle_last_attack_target_sel, state->enemy_count, state->enemy_hp);
                ui_state = LAVA_BATTLE_UI_STATE_TARGET;
                need_redraw = 1;
                continue;
             }
             if (PAL_LavaFightMagicNeedsPartyTarget(state->magic_object_id))
             {
                state->target_sel = PAL_LavaFightFixLivingPartyTarget(state->target_sel, state->party_hp, state->party_hp_max);
               ui_state = LAVA_BATTLE_UI_STATE_PARTY_TARGET;
                need_redraw = 1;
                continue;
             }
             break;
           }
            if (ui_state == LAVA_BATTLE_UI_STATE_ITEM)
            {
              PAL_BattleUIClampItemSel(state);
              if (state->command_sel == LAVA_BATTLE_COMMAND_THROW_ITEM)
              {
                 if ((PAL_BattleUIItemFlags(state->item_object_id) & LAVA_ITEM_FLAG_APPLY_TO_ALL) != 0)
                 {
                    state->target_sel = -1;
                    ui_state = LAVA_BATTLE_UI_STATE_TARGET_ALL;
                 }
                 else
                 {
                    state->target_sel = PAL_LavaFightFixEnemyTarget(
                       g_lava_battle_last_attack_target_sel, state->enemy_count, state->enemy_hp);
                    ui_state = LAVA_BATTLE_UI_STATE_TARGET;
                 }
                 need_redraw = 1;
                 continue;
              }
              if ((PAL_BattleUIItemFlags(state->item_object_id) & LAVA_ITEM_FLAG_APPLY_TO_ALL) == 0)
              {
                state->target_sel = PAL_LavaFightFixLivingPartyTarget(state->target_sel, state->party_hp, state->party_hp_max);
                ui_state = LAVA_BATTLE_UI_STATE_PARTY_TARGET;
                need_redraw = 1;
                continue;
             }
              state->target_sel = -1;
              ui_state = LAVA_BATTLE_UI_STATE_PARTY_TARGET_ALL;
              need_redraw = 1;
              continue;
            }
            if (ui_state == LAVA_BATTLE_UI_STATE_TARGET_ALL ||
                ui_state == LAVA_BATTLE_UI_STATE_PARTY_TARGET_ALL)
            {
               state->target_sel = -1;
               break;
            }
            if (ui_state == LAVA_BATTLE_UI_STATE_PARTY_TARGET)
           {
             break;
           }
           if (ui_state == LAVA_BATTLE_UI_STATE_TARGET)
           {
             if (state->command_sel == LAVA_BATTLE_COMMAND_COOP_MAGIC)
             {
                g_lava_battle_last_coop_target_sel = state->target_sel;
             }
              else
              {
                 g_lava_battle_last_attack_target_sel = state->target_sel;
              }
              break;
           }
          if (ui_state == LAVA_BATTLE_UI_STATE_COMMAND &&
              PAL_LavaFightPlayerCommandNeedsTarget(state->command_sel, state->enemy_count,
                 state->enemy_hp, PAL_BattleUIActingRole(state)))
          {
              state->target_sel = PAL_LavaFightFixEnemyTarget(
                 state->command_sel == LAVA_BATTLE_COMMAND_COOP_MAGIC ?
                    g_lava_battle_last_coop_target_sel : g_lava_battle_last_attack_target_sel,
                state->enemy_count, state->enemy_hp);
              ui_state = LAVA_BATTLE_UI_STATE_TARGET;
              need_redraw = 1;
              continue;
           }

         break;
      }

      if (ui_state == LAVA_BATTLE_UI_STATE_COMMAND ||
          ui_state == LAVA_BATTLE_UI_STATE_TARGET ||
          ui_state == LAVA_BATTLE_UI_STATE_TARGET_ALL ||
          ui_state == LAVA_BATTLE_UI_STATE_PARTY_TARGET ||
          ui_state == LAVA_BATTLE_UI_STATE_PARTY_TARGET_ALL)
      {
         g_lava_logic_frame_num++;
         need_redraw = 1;
      }
      Delay(50);
   }

   next_actor = state->acting_player_index;
   if (next_actor >= 0 && next_actor < 3)
   {
      if (state->command_sel == LAVA_BATTLE_COMMAND_MAGIC &&
          state->magic_object_id > 0)
      {
         int caster_role;
         char caster_name[32];
         char magic_name[32];

         caster_role = PAL_BattleUIActingRole(state);
         PAL_BattleUICopyText(caster_name, sizeof(caster_name),
            PAL_LavaRoleNameForLog(caster_role));
         PAL_BattleUICopyText(magic_name, sizeof(magic_name),
            PAL_BattleUIFormatMagicName(state->magic_object_id));
         printf("[LAVA][MAGICCAST] commit actor=%d role=%d caster='%s' command=%d obj=%d name='%s' target=%d target_player=%d enemy_count=%d\n",
            next_actor, caster_role, PAL_BattleUILogText(caster_name),
            state->command_sel, state->magic_object_id,
            PAL_BattleUILogText(magic_name), state->target_sel,
            PAL_LavaFightMagicNeedsPartyTarget(state->magic_object_id) ? 1 : 0,
            state->enemy_count);
      }
      g_lava_battle_pending_command[next_actor] = state->command_sel;
      g_lava_battle_pending_target[next_actor] = state->target_sel;
      g_lava_battle_pending_magic_sel[next_actor] = state->magic_sel;
      g_lava_battle_pending_magic_object_id[next_actor] = state->magic_object_id;
      g_lava_battle_pending_item_sel[next_actor] = state->item_sel;
      g_lava_battle_pending_item_object_id[next_actor] = state->item_object_id;
      g_lava_battle_pending_ready[next_actor] = 1;
      if (g_lava_autotest_fengshen)
      {
         printf("[LAVA][FENGSHEN] pending actor=%d cmd=%d magic=%d target=%d ready=%d\n",
            next_actor, state->command_sel, state->magic_object_id,
            state->target_sel, g_lava_battle_pending_ready[next_actor]);
      }
   }

   if (state->command_sel == LAVA_BATTLE_COMMAND_FLEE)
   {
      PAL_ClearKeyState();
      return state->command_sel;
   }

   for (next_actor = 0; next_actor < g_lava_party_count && next_actor < 3; next_actor++)
   {
      if (state->party_hp[next_actor] > 0 && !g_lava_battle_pending_ready[next_actor])
      {
         state->acting_player_index = next_actor;
         state->command_sel = g_lava_battle_pending_command[next_actor];
         state->target_sel = g_lava_battle_pending_target[next_actor];
         state->magic_sel = g_lava_battle_pending_magic_sel[next_actor];
         state->magic_object_id = g_lava_battle_pending_magic_object_id[next_actor];
         state->item_sel = g_lava_battle_pending_item_sel[next_actor];
         state->item_object_id = g_lava_battle_pending_item_object_id[next_actor];
         return PAL_BattleUIWaitForPlayerAction(state);
      }
   }

   state->acting_player_index = PAL_LavaFightPickTurnActor(state->party_hp, state->turn);
   state->command_sel = g_lava_battle_pending_command[state->acting_player_index];
   state->target_sel = g_lava_battle_pending_target[state->acting_player_index];
   state->magic_sel = g_lava_battle_pending_magic_sel[state->acting_player_index];
   state->magic_object_id = g_lava_battle_pending_magic_object_id[state->acting_player_index];
   state->item_sel = g_lava_battle_pending_item_sel[state->acting_player_index];
   state->item_object_id = g_lava_battle_pending_item_object_id[state->acting_player_index];
   PAL_ClearKeyState();
   return state->command_sel;
}
