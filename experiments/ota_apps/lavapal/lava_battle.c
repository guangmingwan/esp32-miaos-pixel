/* Lava battle bridge copied for Lava-only adaptation.
 * Do not modify original battle.c for Lava behavior. */

#include "lava_battle.h"

#define LAVA_ENEMY_RECORD_SIZE 70
#define LAVA_ENEMY_FIELD_IDLE_FRAMES 0
#define LAVA_ENEMY_FIELD_IDLE_ANIM_SPEED 3
#define LAVA_BATTLE_SUMMON_HOLD_BUF_SIZE 65536L
#define LAVA_BATTLE_SPRITE_CACHE_SLOTS 10
#define LAVA_BATTLE_SPRITE_SOURCE_PLAYER 1
#define LAVA_BATTLE_SPRITE_SOURCE_ENEMY 2

static int g_lava_battle_background_ready;
static int g_lava_battle_enemy_pos_x[LAVA_BATTLE_MAX_ENEMIES];
static int g_lava_battle_enemy_pos_y[LAVA_BATTLE_MAX_ENEMIES];
static int g_lava_battle_enemy_idle_frame[LAVA_BATTLE_MAX_ENEMIES];
static int g_lava_battle_enemy_idle_counter[LAVA_BATTLE_MAX_ENEMIES];
static int g_lava_battle_enemy_idle_object_id[LAVA_BATTLE_MAX_ENEMIES];
static int g_lava_battle_enemy_idle_frames[LAVA_BATTLE_MAX_ENEMIES];
static int g_lava_battle_enemy_idle_speed[LAVA_BATTLE_MAX_ENEMIES];
static int g_lava_battle_visual_draw_logged;
static int g_lava_battle_enemy_draw_logged;
static int g_lava_battle_player_draw_logged[3];
static int g_lava_battle_player_pose_logged;
static int g_lava_battle_player_pos_x[3][3];
static int g_lava_battle_player_pos_y[3][3];
static int g_bde_i;
static int g_bde_id;
static int g_bde_frames;
static int g_bde_frame_idx;
static int g_bde_x;
static int g_bde_y;
static LPCBITMAPRLE g_bde_frame;
LAVA_BATTLE_STATE g_lava_battle_state;

static FILE *g_lava_battle_fp_players;
static FILE *g_lava_battle_fp_enemies;
static char g_lava_battle_sprite_cache[LAVA_BATTLE_SPRITE_CACHE_SLOTS][65536];
static int g_lava_battle_sprite_cache_source[LAVA_BATTLE_SPRITE_CACHE_SLOTS];
static int g_lava_battle_sprite_cache_chunk[LAVA_BATTLE_SPRITE_CACHE_SLOTS];
static DWORD g_lava_battle_sprite_cache_stamp[LAVA_BATTLE_SPRITE_CACHE_SLOTS];
static DWORD g_lava_battle_sprite_cache_clock;

static int g_lava_battle_keep_active;
static int g_lava_battle_keep_x;
static int g_lava_battle_keep_y;
static int g_lava_battle_keep_size;
static char g_lava_battle_keep_buf[8192];
static addr g_lava_battle_current_effect_frame[3];
static int g_lava_battle_current_effect_x[3];
static int g_lava_battle_current_effect_y[3];
static int g_lava_battle_current_effect_count;
static int g_lava_battle_summon_hold_active;
static int g_lava_battle_summon_hold_x;
static int g_lava_battle_summon_hold_y;
static int g_lava_battle_summon_hold_size;
static char g_lava_battle_summon_hold_buf[LAVA_BATTLE_SUMMON_HOLD_BUF_SIZE];

void PAL_LavaBattleClearSummonHold(void);

/* Must walk the same RLE format as PAL_RLEBlitWithColorShift so the copied
 * byte range stays parse-correct for re-blit. */
static long PAL_LavaBattleRLEByteSize(addr frame_rle)
{
   char *p;
   char *start;
   long width;
   long height;
   long total;
   long i;
   long T;

   if (frame_rle == 0)
   {
      return 0;
   }
   start = (char *)frame_rle;
   p = start;
   if ((PAL_U8(p[0]) == 0x02 && PAL_U8(p[1]) == 0x00 &&
        PAL_U8(p[2]) == 0x00 && PAL_U8(p[3]) == 0x00))
   {
      p += 4;
   }
   width = PAL_U8(p[0]) | (PAL_U8(p[1]) << 8);
   height = PAL_U8(p[2]) | (PAL_U8(p[3]) << 8);
   p += 4;
   total = width * height;
   if (total < 0)
   {
      total = 0;
   }
   i = 0;
   while (i < total)
   {
      T = PAL_U8(*p++);
      if ((T & 0x80) && T <= 0x80 + width)
      {
         i += T - 0x80;
      }
      else
      {
         p += T;
         i += T;
      }
   }
   return (long)(p - start);
}

static int PAL_LavaBattleReadEnemyField(int enemy_id, int field_index)
{
   long chunk_size;
   long enemy_offset;
   FILE *fp;
   char buf[2];

   if (enemy_id < 0 || field_index < 0)
   {
      return 0;
   }

   fp = UTIL_OpenRequiredFile("DATA.MKF");
   if (fp == 0)
   {
      return 0;
   }

   chunk_size = PAL_MKFGetChunkSize(1, fp);
   enemy_offset = (long)enemy_id * LAVA_ENEMY_RECORD_SIZE + field_index * 2;
   if (enemy_offset + 2 > chunk_size)
   {
      fclose(fp);
      return 0;
   }

   if (!PAL_LavaFseekOK(fp, PAL_LavaMKFChunkOffset(fp, 1) + enemy_offset, SEEK_SET))
   {
      fclose(fp);
      return 0;
   }
   if (fread((addr)buf, 1, 2, fp) != 2)
   {
      fclose(fp);
      return 0;
   }
   fclose(fp);

   return PAL_LavaReadU16((addr)buf, 0);
}

static void PAL_LavaBattleResetEnemyIdleState(int slot)
{
   if (slot < 0 || slot >= LAVA_BATTLE_MAX_ENEMIES)
   {
      return;
   }

   g_lava_battle_enemy_idle_frame[slot] = 0;
   g_lava_battle_enemy_idle_counter[slot] = 0;
   g_lava_battle_enemy_idle_object_id[slot] = 0;
}

static int PAL_LavaBattleEnemyIdleFrameIndex(int slot, int object_id, int enemy_id, int frame_count)
{
   int idle_frames;
   int idle_speed;

   if (slot < 0 || slot >= LAVA_BATTLE_MAX_ENEMIES || frame_count <= 0)
   {
      return 0;
   }

   if (g_lava_battle_enemy_idle_object_id[slot] != object_id)
   {
      PAL_LavaBattleResetEnemyIdleState(slot);
      g_lava_battle_enemy_idle_object_id[slot] = object_id;
   }

   idle_frames = g_lava_battle_enemy_idle_frames[slot];
   if (idle_frames <= 0)
   {
      idle_frames = 1;
   }
   if (idle_frames > frame_count)
   {
      idle_frames = frame_count;
   }

   idle_speed = g_lava_battle_enemy_idle_speed[slot];
   if (idle_speed <= 0)
   {
      idle_speed = 4;
   }

   if (g_lava_battle_enemy_idle_frame[slot] >= idle_frames)
   {
      g_lava_battle_enemy_idle_frame[slot] = 0;
   }
   if (g_lava_battle_enemy_idle_counter[slot] <= 0)
   {
      g_lava_battle_enemy_idle_counter[slot] = idle_speed;
   }

   g_lava_battle_enemy_idle_counter[slot]--;
   if (g_lava_battle_enemy_idle_counter[slot] <= 0)
   {
      g_lava_battle_enemy_idle_frame[slot]++;
      if (g_lava_battle_enemy_idle_frame[slot] >= idle_frames)
      {
         g_lava_battle_enemy_idle_frame[slot] = 0;
      }
      g_lava_battle_enemy_idle_counter[slot] = idle_speed;
   }

   return g_lava_battle_enemy_idle_frame[slot];
}

static int PAL_LavaBattleEnemyCurrentFrameIndex(int slot, int object_id, int frame_count)
{
   if (slot < 0 || slot >= LAVA_BATTLE_MAX_ENEMIES || frame_count <= 0)
   {
      return 0;
   }
   if (g_lava_battle_enemy_idle_object_id[slot] != object_id)
   {
      return 0;
   }
   if (g_lava_battle_enemy_idle_frame[slot] >= frame_count)
   {
      return 0;
   }
   return g_lava_battle_enemy_idle_frame[slot];
}

static int PAL_LavaBattleReadEnemyPosWord(int slot, int max_enemy_index, int coord)
{
   FILE *fp;
   long chunk_size;
   long pos_offset;
   char buf[2];

   if (slot < 0 || slot >= LAVA_BATTLE_MAX_ENEMIES ||
       max_enemy_index < 0 || max_enemy_index >= LAVA_BATTLE_MAX_ENEMIES)
   {
      return 0;
   }

   fp = UTIL_OpenRequiredFile("DATA.MKF");
   if (fp == 0)
   {
      return 0;
   }

   chunk_size = PAL_MKFGetChunkSize(13, fp);
   pos_offset = (long)(slot * LAVA_BATTLE_MAX_ENEMIES + max_enemy_index) * 4 + coord * 2;
   if (pos_offset + 2 > chunk_size)
   {
      fclose(fp);
      return 0;
   }

   if (!PAL_LavaFseekOK(fp, PAL_LavaMKFChunkOffset(fp, 13) + pos_offset, SEEK_SET))
   {
      fclose(fp);
      return 0;
   }
   if (fread((addr)buf, 1, 2, fp) != 2)
   {
      fclose(fp);
      return 0;
   }

   fclose(fp);
   return PAL_LavaReadU16((addr)buf, 0);
}

static void PAL_LavaBattleInitPlayerLayout(void)
{
   g_lava_battle_player_pos_x[0][0] = 240;
   g_lava_battle_player_pos_y[0][0] = 170;
   g_lava_battle_player_pos_x[0][1] = 0;
   g_lava_battle_player_pos_y[0][1] = 0;
   g_lava_battle_player_pos_x[0][2] = 0;
   g_lava_battle_player_pos_y[0][2] = 0;

   g_lava_battle_player_pos_x[1][0] = 200;
   g_lava_battle_player_pos_y[1][0] = 176;
   g_lava_battle_player_pos_x[1][1] = 256;
   g_lava_battle_player_pos_y[1][1] = 152;
   g_lava_battle_player_pos_x[1][2] = 0;
   g_lava_battle_player_pos_y[1][2] = 0;

   g_lava_battle_player_pos_x[2][0] = 180;
   g_lava_battle_player_pos_y[2][0] = 180;
   g_lava_battle_player_pos_x[2][1] = 234;
   g_lava_battle_player_pos_y[2][1] = 170;
   g_lava_battle_player_pos_x[2][2] = 270;
   g_lava_battle_player_pos_y[2][2] = 146;
}

static int PAL_LavaBattleGetDecompressedSize(int chunk_num, FILE *fp)
{
   char buf[8];
   char raw_offset[4];
   long offset;
   int chunk_count;

   if (fp == 0 || chunk_num < 0)
   {
      return -1;
   }

   chunk_count = PAL_MKFGetChunkCount(fp);
   if (chunk_num >= chunk_count)
   {
      return -1;
   }

   fseek(fp, 4 * chunk_num, SEEK_SET);
   if (fread((addr)raw_offset, 1, 4, fp) != 4)
   {
      return -1;
   }
   offset = PAL_U8(raw_offset[0]) |
      (PAL_U8(raw_offset[1]) << 8) |
      (PAL_U8(raw_offset[2]) << 16) |
      (PAL_U8(raw_offset[3]) << 24);
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
   if ((PAL_U8(buf[0]) | (PAL_U8(buf[1]) << 8) | (PAL_U8(buf[2]) << 16) | (PAL_U8(buf[3]) << 24))
      != 0x315f4a59)
   {
      return -1;
   }

   return PAL_U8(buf[4]) |
      (PAL_U8(buf[5]) << 8) |
      (PAL_U8(buf[6]) << 16) |
      (PAL_U8(buf[7]) << 24);
}

static void PAL_LavaBattleOpenAssetFiles(void)
{
   if (g_lava_battle_fp_players == 0)
   {
      g_lava_battle_fp_players = UTIL_OpenRequiredFile("F.MKF");
   }
   if (g_lava_battle_fp_enemies == 0)
   {
      g_lava_battle_fp_enemies = UTIL_OpenRequiredFile("ABC.MKF");
   }
}

addr PAL_LavaBattleGetPlayerAssetFile(void)
{
   PAL_LavaBattleOpenAssetFiles();
   return (addr)g_lava_battle_fp_players;
}

static addr PAL_LavaBattleGetCachedSprite(int source, int chunk, FILE *fp)
{
   int i;
   int slot;

   if (chunk < 0)
   {
      return 0;
   }

   g_lava_battle_sprite_cache_clock++;
   if (g_lava_battle_sprite_cache_clock == 0)
   {
      g_lava_battle_sprite_cache_clock = 1;
   }

   slot = -1;
   for (i = 0; i < LAVA_BATTLE_SPRITE_CACHE_SLOTS; i++)
   {
      if (g_lava_battle_sprite_cache_source[i] == source &&
          g_lava_battle_sprite_cache_chunk[i] == chunk)
      {
         g_lava_battle_sprite_cache_stamp[i] = g_lava_battle_sprite_cache_clock;
         return (addr)g_lava_battle_sprite_cache[i];
      }
      if (slot < 0 || g_lava_battle_sprite_cache_source[i] == 0 ||
          g_lava_battle_sprite_cache_stamp[i] < g_lava_battle_sprite_cache_stamp[slot])
      {
         slot = i;
         if (g_lava_battle_sprite_cache_source[i] == 0)
         {
            break;
         }
      }
   }

   if (slot < 0 || fp == 0 ||
       PAL_MKFDecompressChunk((addr)g_lava_battle_sprite_cache[slot],
          sizeof(g_lava_battle_sprite_cache[slot]), chunk, fp) <= 0)
   {
      return 0;
   }

   g_lava_battle_sprite_cache_source[slot] = source;
   g_lava_battle_sprite_cache_chunk[slot] = chunk;
   g_lava_battle_sprite_cache_stamp[slot] = g_lava_battle_sprite_cache_clock;
   return (addr)g_lava_battle_sprite_cache[slot];
}

static void PAL_LavaBattlePreloadSprites(LAVA_BATTLE_STATE *state)
{
   int i;
   int role;
   int sprite_num;

   PAL_LavaBattleOpenAssetFiles();
   for (i = 0; i < g_lava_party_count && i < 3; i++)
   {
      role = g_lava_party_role[i];
      sprite_num = PAL_LavaBattleGetPlayerTransformSprite(role);
      if (sprite_num <= 0)
      {
         sprite_num = PAL_LavaRoleWordByArray(1, role);
      }
      PAL_LavaBattleGetCachedSprite(LAVA_BATTLE_SPRITE_SOURCE_PLAYER,
         sprite_num, g_lava_battle_fp_players);
   }

   if (state != 0)
   {
      for (i = 0; i < state->enemy_count && i < LAVA_BATTLE_MAX_ENEMIES; i++)
      {
         sprite_num = PAL_LavaReadObjectField(state->enemy_object_id[i], 0);
         PAL_LavaBattleGetCachedSprite(LAVA_BATTLE_SPRITE_SOURCE_ENEMY,
            sprite_num, g_lava_battle_fp_enemies);
      }
   }
   PAL_LavaLoadUISprite();
}

static void PAL_LavaBattleLayoutEnemies(LAVA_BATTLE_STATE *state)
{
   int enemy_count;
   int enemy_id;
   int i;
   int max_enemy_index;
   int x;
   int y;
   int y_offset;

   enemy_count = state != 0 ? state->enemy_count : 0;

   for (i = 0; i < LAVA_BATTLE_MAX_ENEMIES; i++)
   {
      g_lava_battle_enemy_pos_x[i] = 0;
      g_lava_battle_enemy_pos_y[i] = 0;
      PAL_LavaBattleResetEnemyIdleState(i);
      g_lava_battle_enemy_idle_frames[i] = 0;
      g_lava_battle_enemy_idle_speed[i] = 0;
   }

   if (enemy_count <= 1)
   {
      g_lava_battle_enemy_pos_x[0] = 228;
      g_lava_battle_enemy_pos_y[0] = 108;
   }
   else if (enemy_count == 2)
   {
      g_lava_battle_enemy_pos_x[0] = 206;
      g_lava_battle_enemy_pos_y[0] = 96;
      g_lava_battle_enemy_pos_x[1] = 248;
      g_lava_battle_enemy_pos_y[1] = 124;
   }
   else if (enemy_count == 3)
   {
      g_lava_battle_enemy_pos_x[0] = 226;
      g_lava_battle_enemy_pos_y[0] = 82;
      g_lava_battle_enemy_pos_x[1] = 194;
      g_lava_battle_enemy_pos_y[1] = 114;
      g_lava_battle_enemy_pos_x[2] = 252;
      g_lava_battle_enemy_pos_y[2] = 126;
   }
   else if (enemy_count == 4)
   {
      g_lava_battle_enemy_pos_x[0] = 206;
      g_lava_battle_enemy_pos_y[0] = 80;
      g_lava_battle_enemy_pos_x[1] = 248;
      g_lava_battle_enemy_pos_y[1] = 88;
      g_lava_battle_enemy_pos_x[2] = 188;
      g_lava_battle_enemy_pos_y[2] = 120;
      g_lava_battle_enemy_pos_x[3] = 236;
      g_lava_battle_enemy_pos_y[3] = 132;
   }
   else
   {
      g_lava_battle_enemy_pos_x[0] = 224;
      g_lava_battle_enemy_pos_y[0] = 72;
      g_lava_battle_enemy_pos_x[1] = 194;
      g_lava_battle_enemy_pos_y[1] = 92;
      g_lava_battle_enemy_pos_x[2] = 252;
      g_lava_battle_enemy_pos_y[2] = 100;
      g_lava_battle_enemy_pos_x[3] = 184;
      g_lava_battle_enemy_pos_y[3] = 126;
      g_lava_battle_enemy_pos_x[4] = 236;
      g_lava_battle_enemy_pos_y[4] = 136;
   }

   if (state == 0 || enemy_count <= 0)
   {
      return;
   }

   max_enemy_index = enemy_count - 1;
   if (max_enemy_index < 0)
   {
      max_enemy_index = 0;
   }
   if (max_enemy_index >= LAVA_BATTLE_MAX_ENEMIES)
   {
      max_enemy_index = LAVA_BATTLE_MAX_ENEMIES - 1;
   }

   for (i = 0; i < enemy_count && i < LAVA_BATTLE_MAX_ENEMIES; i++)
   {
      x = PAL_LavaBattleReadEnemyPosWord(i, max_enemy_index, 0);
      y = PAL_LavaBattleReadEnemyPosWord(i, max_enemy_index, 1);
      if (x == 0 && y == 0)
      {
         continue;
      }

      y_offset = 0;
      if (state->enemy_object_id[i] > 0)
      {
         enemy_id = PAL_LavaReadObjectField(state->enemy_object_id[i], 0);
         if (enemy_id >= 0)
         {
            y_offset = PAL_LavaBattleReadEnemyField(enemy_id, 5);
            g_lava_battle_enemy_idle_frames[i] =
               PAL_LavaBattleReadEnemyField(enemy_id, LAVA_ENEMY_FIELD_IDLE_FRAMES);
            g_lava_battle_enemy_idle_speed[i] =
               PAL_LavaBattleReadEnemyField(enemy_id, LAVA_ENEMY_FIELD_IDLE_ANIM_SPEED);
            g_lava_battle_enemy_idle_object_id[i] = state->enemy_object_id[i];
         }
      }
      g_lava_battle_enemy_pos_x[i] = x;
      g_lava_battle_enemy_pos_y[i] = y + y_offset;
   }
}

void PAL_LavaBattleGetEnemyPos(int enemy_index, int *x, int *y)
{
   if (x != 0)
   {
      *x = 0;
   }
   if (y != 0)
   {
      *y = 0;
   }
   if (enemy_index < 0 || enemy_index >= LAVA_BATTLE_MAX_ENEMIES)
   {
      return;
   }
   if (x != 0)
   {
      *x = g_lava_battle_enemy_pos_x[enemy_index];
   }
   if (y != 0)
   {
      *y = g_lava_battle_enemy_pos_y[enemy_index];
   }
}

static void PAL_LavaBattleLoadBackground(void)
{
    long ret;
    FILE *fp;

    g_lava_battle_background_ready = 0;
    fp = UTIL_OpenFile("FBP.MKF");
    if (fp == 0)
    {
       return;
    }

    ret = PAL_MKFDecompressChunk((addr)g_lava_fbp_buf, 64000,
       g_lava_num_battle_field, fp);
    fclose(fp);
    if (PAL_LavaDecompressOK(ret, 64000))
    {
       g_lava_battle_background_ready = 1;
      return;
   }

   if (g_lava_autotest_search || g_lava_autotest_load)
   {
      printf("[LAVA][BATTLEVIS] bg load fail field=%d ret=%ld\n",
         g_lava_num_battle_field, ret);
   }
}

void PAL_LavaBattlePrepareVisuals(LAVA_BATTLE_STATE *state)
{
   if (g_lava_fpMGO != 0)
   {
      fclose((FILE *)g_lava_fpMGO);
      g_lava_fpMGO = 0;
   }
   PAL_LavaBattleClearKeepEffect();
   PAL_LavaBattleClearCurrentEffect();
   PAL_LavaBattleClearSummonHold();
   PAL_LavaBattleLoadBackground();
   PAL_LavaBattleInitPlayerLayout();
   PAL_LavaBattleLayoutEnemies(state);
   PAL_LavaBattlePreloadSprites(state);
   if (g_lava_autotest_search || g_lava_autotest_load)
   {
      printf("[LAVA][BATTLEVIS] prepared bg=%d field=%d enemies=%d\n",
         g_lava_battle_background_ready,
         g_lava_num_battle_field,
         state != 0 ? state->enemy_count : 0);
   }
}

void PAL_LavaBattleFreeVisuals(void)
{
   int i;

   if (g_lava_battle_fp_players != 0)
   {
      fclose(g_lava_battle_fp_players);
      g_lava_battle_fp_players = 0;
   }
   if (g_lava_battle_fp_enemies != 0)
   {
      fclose(g_lava_battle_fp_enemies);
      g_lava_battle_fp_enemies = 0;
   }

   g_lava_battle_background_ready = 0;
   g_lava_battle_visual_draw_logged = 0;
   g_lava_battle_enemy_draw_logged = 0;
   for (i = 0; i < 3; i++)
   {
      g_lava_battle_player_draw_logged[i] = 0;
   }
   g_lava_battle_player_pose_logged = 0;
   for (i = 0; i < LAVA_BATTLE_MAX_ENEMIES; i++)
   {
      g_lava_battle_enemy_pos_x[i] = 0;
      g_lava_battle_enemy_pos_y[i] = 0;
      PAL_LavaBattleResetEnemyIdleState(i);
   }
   PAL_LavaBattleClearKeepEffect();
   PAL_LavaBattleClearCurrentEffect();
   PAL_LavaBattleClearSummonHold();
}

void PAL_LavaBattleClearKeepEffect(void)
{
   g_lava_battle_keep_active = 0;
   g_lava_battle_keep_x = 0;
   g_lava_battle_keep_y = 0;
   g_lava_battle_keep_size = 0;
}

void PAL_LavaBattleClearCurrentEffect(void)
{
   int i;

   for (i = 0; i < 3; i++)
   {
      g_lava_battle_current_effect_frame[i] = 0;
      g_lava_battle_current_effect_x[i] = 0;
      g_lava_battle_current_effect_y[i] = 0;
   }
   g_lava_battle_current_effect_count = 0;
}

void PAL_LavaBattleSetCurrentEffect(addr frame_rle, int x, int y)
{
   if (frame_rle != 0)
   {
      printf("[LAVA][MAGICDRAW] set-current-effect x=%d y=%d w=%d h=%d direct=%d\n",
         x, y,
         PAL_RLEGetWidth((LPCBITMAPRLE)frame_rle),
         PAL_RLEGetHeight((LPCBITMAPRLE)frame_rle),
         g_lava_direct_screen);
   }
   g_lava_battle_current_effect_count = 1;
   g_lava_battle_current_effect_frame[0] = frame_rle;
   g_lava_battle_current_effect_x[0] = x;
   g_lava_battle_current_effect_y[0] = y;
}

void PAL_LavaBattleAddCurrentEffect(addr frame_rle, int x, int y)
{
   int index;

   if (g_lava_battle_current_effect_count >= 3)
   {
      return;
   }
   index = g_lava_battle_current_effect_count;
   g_lava_battle_current_effect_count++;
   g_lava_battle_current_effect_frame[index] = frame_rle;
   g_lava_battle_current_effect_x[index] = x;
   g_lava_battle_current_effect_y[index] = y;
}

void PAL_LavaBattleClearSummonHold(void)
{
   g_lava_battle_summon_hold_active = 0;
   g_lava_battle_summon_hold_size = 0;
}

void PAL_LavaBattleSetSummonHold(addr frame_rle, int cx, int cy)
{
   char *src;
   long frame_size;
   int i;

   frame_size = PAL_LavaBattleRLEByteSize(frame_rle);
   if (frame_size <= 0 || frame_size > LAVA_BATTLE_SUMMON_HOLD_BUF_SIZE)
   {
      return;
   }
   g_lava_battle_summon_hold_active = 0;
   g_lava_battle_summon_hold_size = 0;
   src = (char *)frame_rle;
   for (i = 0; i < frame_size; i++)
   {
      g_lava_battle_summon_hold_buf[i] = src[i];
   }
   g_lava_battle_summon_hold_x = cx - PAL_RLEGetWidth((LPCBITMAPRLE)frame_rle) / 2;
   g_lava_battle_summon_hold_y = cy - PAL_RLEGetHeight((LPCBITMAPRLE)frame_rle);
   g_lava_battle_summon_hold_size = frame_size;
   g_lava_battle_summon_hold_active = 1;
}

void PAL_LavaBattleSetKeepEffect(addr frame_rle, int x, int y)
{
   int i;
   long frame_size;
   char *src;

   g_lava_battle_keep_active = 0;
   g_lava_battle_keep_size = 0;
   frame_size = PAL_LavaBattleRLEByteSize(frame_rle);
   if (frame_size <= 0 || frame_size > (long)sizeof(g_lava_battle_keep_buf))
   {
      return;
   }

   src = (char *)frame_rle;
   for (i = 0; i < frame_size; i++)
   {
      g_lava_battle_keep_buf[i] = src[i];
   }
   g_lava_battle_keep_x = x;
   g_lava_battle_keep_y = y;
   g_lava_battle_keep_size = frame_size;
   g_lava_battle_keep_active = 1;
}

void PAL_LavaBattleDrawKeepEffect(void)
{
   if (!g_lava_battle_keep_active)
   {
      return;
   }
   PAL_RLEBlitToSurface((LPCBITMAPRLE)g_lava_battle_keep_buf, gpScreen,
      PAL_XY(g_lava_battle_keep_x, g_lava_battle_keep_y));
}

void PAL_LavaBattleDrawCurrentEffect(void)
{
   int saved_direct_screen;
   int i;

   if (g_lava_battle_current_effect_count <= 0)
   {
      return;
   }
   saved_direct_screen = g_lava_direct_screen;
   g_lava_direct_screen = 1;
   for (i = 0; i < g_lava_battle_current_effect_count && i < 3; i++)
   {
      if (g_lava_battle_current_effect_frame[i] == 0)
      {
         continue;
      }
      if (g_lava_autotest_fengshen || g_lava_autotest_battle)
      {
         printf("[LAVA][MAGICDRAW] draw-current-effect x=%d y=%d w=%d h=%d direct=%d\n",
            g_lava_battle_current_effect_x[i],
            g_lava_battle_current_effect_y[i],
            PAL_RLEGetWidth((LPCBITMAPRLE)g_lava_battle_current_effect_frame[i]),
            PAL_RLEGetHeight((LPCBITMAPRLE)g_lava_battle_current_effect_frame[i]),
            g_lava_direct_screen);
      }
      PAL_RLEBlitToSurface((LPCBITMAPRLE)g_lava_battle_current_effect_frame[i], gpScreen,
         PAL_XY(g_lava_battle_current_effect_x[i], g_lava_battle_current_effect_y[i]));
   }
   g_lava_direct_screen = saved_direct_screen;
}

static void PAL_LavaBattleDrawSummonHold(void)
{
   int saved_direct_screen;

   if (!g_lava_battle_summon_hold_active)
   {
      return;
   }
   saved_direct_screen = g_lava_direct_screen;
   g_lava_direct_screen = 1;
   PAL_RLEBlitToSurface((LPCBITMAPRLE)g_lava_battle_summon_hold_buf, gpScreen,
      PAL_XY(g_lava_battle_summon_hold_x, g_lava_battle_summon_hold_y));
   g_lava_direct_screen = saved_direct_screen;
}

/* One-time brightness shift of the FBP background buffer, mirroring the
 * original PAL sBackgroundColorShift for summon magics (e.g. snow demon
 * brightening the whole battlefield for the rest of the battle). */
void PAL_LavaBattleApplyBackgroundShift(int shift)
{
   char *p;
   int b;
   long i;

   if (shift == 0 || !g_lava_battle_background_ready)
   {
      return;
   }
   p = (char *)g_lava_fbp_buf;
   for (i = 0; i < 64000; i++)
   {
      b = (PAL_U8(p[i]) & 0x0F) + shift;
      if (b < 0)
      {
         b = 0;
      }
      if (b > 15)
      {
         b = 15;
      }
      p[i] = (PAL_U8(p[i]) & 0xF0) | b;
   }
}

static int PAL_LavaBattlePlayerFrameIndex(LAVA_BATTLE_STATE *state, int player_index, int frame_count)
{
   int pose;

   if (state == 0 || player_index < 0 || player_index >= 3)
   {
      return 0;
   }
   pose = state->party_pose[player_index];
   if (pose > 0 && pose < frame_count)
   {
      return pose;
   }
   if (state->party_hp[player_index] <= 0)
   {
      if (frame_count > 2)
      {
         return 2;
      }
      return 0;
   }
   if (state->party_defending[player_index] != 0)
   {
      if (frame_count > 3)
      {
         return 3;
      }
      return 0;
   }

   return 0;
}

static void PAL_LavaBattleDrawPlayers(LAVA_BATTLE_STATE *state)
{
   int i;
   int party_count;

    if (state == 0 || g_lava_party_count <= 0)
    {
       return;
    }

    if (g_lava_battle_hide_players)
    {
       return;
    }

   PAL_LavaBattleOpenAssetFiles();

   party_count = g_lava_party_count;
   if (party_count > 3)
   {
      party_count = 3;
   }
   if (party_count <= 0)
   {
      return;
   }

   for (i = 0; i < party_count; i++)
   {
      LPCBITMAPRLE frame;
      int frame_count;
      int frame_index;
      int frame_x;
      int frame_y;
      int player_role;
      int sprite_num;

      if (state->party_hp_max[i] <= 0)
      {
         continue;
      }

      player_role = g_lava_party_role[i];
      sprite_num = PAL_LavaBattleGetPlayerTransformSprite(player_role);
      if (sprite_num <= 0)
      {
         sprite_num = PAL_LavaRoleWordByArray(1, player_role);
      }
      if (sprite_num < 0)
      {
         if ((g_lava_autotest_search || g_lava_autotest_load) &&
             !g_lava_battle_player_draw_logged[i])
         {
            printf("[LAVA][BATTLEVIS] player bad sprite slot=%d role=%d sprite=%d\n",
               i, player_role, sprite_num);
            g_lava_battle_player_draw_logged[i] = 1;
         }
         continue;
      }

      {
         addr cached_sprite;

         cached_sprite = PAL_LavaBattleGetCachedSprite(
            LAVA_BATTLE_SPRITE_SOURCE_PLAYER, sprite_num,
            g_lava_battle_fp_players);
         if (cached_sprite == 0)
         {
            continue;
         }
         frame_count = PAL_SpriteGetNumFrames((LPSPRITE)cached_sprite);
         frame_index = PAL_LavaBattlePlayerFrameIndex(state, i, frame_count);
         frame = PAL_SpriteGetFrame((LPSPRITE)cached_sprite, frame_index);
      }
      if (frame == 0)
      {
         if ((g_lava_autotest_search || g_lava_autotest_load) &&
             !g_lava_battle_player_draw_logged[i])
         {
            printf("[LAVA][BATTLEVIS] player frame fail slot=%d role=%d sprite=%d frame=%d frames=%d\n",
               i, player_role, sprite_num, frame_index, frame_count);
            g_lava_battle_player_draw_logged[i] = 1;
         }
         continue;
      }

      if (!g_lava_battle_player_draw_logged[i] &&
           (g_lava_autotest_search || g_lava_autotest_load))
      {
         printf("[LAVA][BATTLEVIS] player ok slot=%d role=%d sprite=%d frames=%d frame0=%dx%d\n",
            i,
            player_role,
            sprite_num,
             frame_count,
             PAL_RLEGetWidth(frame),
             PAL_RLEGetHeight(frame));
         g_lava_battle_player_draw_logged[i] = 1;
      }
      if (!g_lava_battle_player_pose_logged &&
          frame_index == 3 &&
          (g_lava_autotest_search || g_lava_autotest_load))
      {
         printf("[LAVA][BATTLEVIS] player pose slot=%d frame=%d defend=%d hp=%d/%d\n",
            i,
            frame_index,
            state->party_defending[i],
            state->party_hp[i],
            state->party_hp_max[i]);
         g_lava_battle_player_pose_logged = 1;
      }

      frame_x = g_lava_battle_player_pos_x[party_count - 1][i] +
         g_lava_battle_player_offset_x[i] - PAL_RLEGetWidth(frame) / 2;
      frame_y = g_lava_battle_player_pos_y[party_count - 1][i] +
         g_lava_battle_player_offset_y[i] - PAL_RLEGetHeight(frame);
      if (g_lava_battle_player_color_shift[i] != 0)
      {
         PAL_RLEBlitWithColorShift(frame, gpScreen, PAL_XY(frame_x, frame_y),
            g_lava_battle_player_color_shift[i]);
      }
      else
      {
         PAL_RLEBlitToSurface(frame, gpScreen, PAL_XY(frame_x, frame_y));
      }
   }

}

static void PAL_LavaBattleDrawEnemies(LAVA_BATTLE_STATE *state)
{
   if (state == 0)
   {
      return;
   }

   PAL_LavaBattleOpenAssetFiles();

    for (g_bde_i = 0;
         g_bde_i < state->enemy_count && g_bde_i < LAVA_BATTLE_MAX_ENEMIES;
         g_bde_i++)
    {
       if (state->enemy_hp[g_bde_i] <= 0 || state->enemy_object_id[g_bde_i] <= 0)
       {
          PAL_LavaBattleResetEnemyIdleState(g_bde_i);
          continue;
       }

       g_bde_id = PAL_LavaReadObjectField(state->enemy_object_id[g_bde_i], 0);
       if (g_bde_id <= 0)
       {
          PAL_LavaBattleResetEnemyIdleState(g_bde_i);
          continue;
       }

       {
          addr cached_sprite;

          cached_sprite = PAL_LavaBattleGetCachedSprite(
             LAVA_BATTLE_SPRITE_SOURCE_ENEMY, g_bde_id,
             g_lava_battle_fp_enemies);
          if (cached_sprite == 0)
          {
             continue;
          }
          g_bde_frames = PAL_SpriteGetNumFrames((LPSPRITE)cached_sprite);
          g_bde_frame_idx = PAL_LavaBattleEnemyIdleFrameIndex(g_bde_i,
             state->enemy_object_id[g_bde_i], g_bde_id, g_bde_frames);
          g_bde_frame = PAL_SpriteGetFrame((LPSPRITE)cached_sprite, g_bde_frame_idx);
          if (g_bde_frame == 0)
          {
             g_bde_frame = PAL_SpriteGetFrame((LPSPRITE)cached_sprite, 0);
          }
       }
       if (g_bde_frame == 0)
       {
          continue;
       }

      if (!g_lava_battle_enemy_draw_logged &&
          (g_lava_autotest_search || g_lava_autotest_load))
      {
          printf("[LAVA][BATTLEVIS] enemy ok slot=%d obj=%d enemy=%d size=%d frames=%d frame=%dx%d\n",
             g_bde_i,
             state->enemy_object_id[g_bde_i],
             g_bde_id,
             PAL_LavaBattleGetDecompressedSize(g_bde_id, g_lava_battle_fp_enemies),
             g_bde_frames,
             PAL_RLEGetWidth(g_bde_frame),
             PAL_RLEGetHeight(g_bde_frame));
          g_lava_battle_enemy_draw_logged = 1;
       }

       g_bde_x = g_lava_battle_enemy_pos_x[g_bde_i] +
          g_lava_battle_enemy_offset_x[g_bde_i] - PAL_RLEGetWidth(g_bde_frame) / 2;
       g_bde_y = g_lava_battle_enemy_pos_y[g_bde_i] +
          g_lava_battle_enemy_offset_y[g_bde_i] - PAL_RLEGetHeight(g_bde_frame);
       if (g_lava_battle_enemy_color_shift[g_bde_i] != 0)
       {
          PAL_RLEBlitWithColorShift(g_bde_frame, gpScreen, PAL_XY(g_bde_x, g_bde_y),
             g_lava_battle_enemy_color_shift[g_bde_i]);
       }
        else
        {
           PAL_RLEBlitToSurface(g_bde_frame, gpScreen, PAL_XY(g_bde_x, g_bde_y));
        }
     }

}

void PAL_LavaBattleDrawEnemyTargetOverlay(LAVA_BATTLE_STATE *state, int target_sel)
{
   int enemy_id;
   int frame_count;
   int frame_index;
   int frame_x;
   int frame_y;
   int object_id;
   int sprite_id;
   addr cached_sprite;
   LPCBITMAPRLE frame;

   if ((g_lava_logic_frame_num & 1) == 0 || state == 0 ||
       target_sel < 0 || target_sel >= state->enemy_count ||
       target_sel >= LAVA_BATTLE_MAX_ENEMIES ||
       state->enemy_hp[target_sel] <= 0)
   {
      return;
   }

   object_id = state->enemy_object_id[target_sel];
   if (object_id <= 0)
   {
      return;
   }
   enemy_id = PAL_LavaReadObjectField(object_id, 0);
   if (enemy_id <= 0)
   {
      return;
   }

   PAL_LavaBattleOpenAssetFiles();
   sprite_id = enemy_id;
   cached_sprite = PAL_LavaBattleGetCachedSprite(
      LAVA_BATTLE_SPRITE_SOURCE_ENEMY, sprite_id,
      g_lava_battle_fp_enemies);
   if (cached_sprite == 0)
   {
      return;
   }

   frame_count = PAL_SpriteGetNumFrames((LPSPRITE)cached_sprite);
   frame_index = PAL_LavaBattleEnemyCurrentFrameIndex(target_sel,
      object_id, frame_count);
   frame = PAL_SpriteGetFrame((LPSPRITE)cached_sprite, frame_index);
   if (frame == 0)
   {
      frame = PAL_SpriteGetFrame((LPSPRITE)cached_sprite, 0);
   }
   if (frame == 0)
   {
      return;
   }

   frame_x = g_lava_battle_enemy_pos_x[target_sel] +
      g_lava_battle_enemy_offset_x[target_sel] - PAL_RLEGetWidth(frame) / 2;
   frame_y = g_lava_battle_enemy_pos_y[target_sel] +
      g_lava_battle_enemy_offset_y[target_sel] - PAL_RLEGetHeight(frame);
   PAL_RLEBlitWithColorShift(frame, gpScreen, PAL_XY(frame_x, frame_y), 7);
}

void PAL_LavaBattleDrawSceneFrame(LAVA_BATTLE_STATE *state, int target_sel)
{
   if (!g_lava_battle_visual_draw_logged &&
       (g_lava_autotest_search || g_lava_autotest_load))
   {
      printf("[LAVA][BATTLEVIS] draw bg=%d target=%d count=%d\n",
         g_lava_battle_background_ready,
         target_sel,
         state != 0 ? state->enemy_count : 0);
      g_lava_battle_visual_draw_logged = 1;
   }

   if (!g_lava_battle_background_ready)
   {
      PAL_LavaDrawSceneFrame();
   }
   else
   {
      PAL_FBPBlitToSurface((addr)g_lava_fbp_buf, gpScreen);
   }

   PAL_LavaBattleDrawKeepEffect();

   PAL_LavaBattleDrawEnemies(state);
   if (g_lava_battle_summon_hold_active)
   {
      PAL_LavaBattleDrawSummonHold();
   }
   else
   {
      PAL_LavaBattleDrawPlayers(state);
   }
   PAL_LavaBattleDrawCurrentEffect();
}

static void PAL_LavaBattleRunRoundState(LAVA_BATTLE_STATE *state)
{
   int replay_pending;

   if (state == 0)
   {
      return;
   }

   replay_pending = (state->flow_action == LAVA_BATTLE_FLOW_RECHECK);
   state->battle_result = PAL_LavaFightBeginRoundState(state);
   if (state->battle_result != LAVA_BATTLE_RESULT_CONTINUE)
   {
      state->flow_action = LAVA_BATTLE_FLOW_END;
      return;
   }

   if (!replay_pending)
   {
      PAL_BattleUIRunCommandState(state);
   }
   state->battle_result = PAL_LavaFightCommitRoundState(state);
   PAL_BattleUIPlayRoundState(state);
   PAL_LavaFightAdvanceBattleState(state);
}

int
PAL_StartBattle(
   WORD        wEnemyTeam,
   BOOL        fIsBoss
)
{
   LAVA_BATTLE_STATE *state;

   state = &g_lava_battle_state;
   PAL_LavaClearRoleTempStats();
   if (g_lava_autotest_search || g_lava_autotest_load)
   {
      printf("[LAVA][BATTLE] begin team=%d boss=%d\n", (int)wEnemyTeam, (int)fIsBoss);
   }
   PAL_LavaFightPrepareBattleState((int)wEnemyTeam, state);
   PAL_LavaBattlePrepareVisuals(state);
   if (g_lava_autotest_search || g_lava_autotest_load)
   {
      int i;

      printf("[LAVA][BATTLE] prepared enemies=%d actor=%d party_hp=%d/%d,%d/%d,%d/%d\n",
         state->enemy_count,
         state->acting_player_index,
         state->party_hp[0], state->party_hp_max[0],
         state->party_hp[1], state->party_hp_max[1],
         state->party_hp[2], state->party_hp_max[2]);
      for (i = 0; i < g_lava_party_count && i < 3; i++)
      {
         printf("[LAVA][BATTLE] party slot=%d role=%d name=%s battle_sprite=%d hp=%d/%d\n",
            i,
            g_lava_party_role[i],
             PAL_LavaRoleNameForLog(g_lava_party_role[i]),
            PAL_LavaRoleWordByArray(1, g_lava_party_role[i]),
             state->party_hp[i],
             state->party_hp_max[i]);
      }
   }
   PAL_BattleUIBeginBattle();
   if (g_lava_autotest_search || g_lava_autotest_load)
   {
      printf("[LAVA][BATTLE] ui-begin\n");
   }

   while (TRUE)
   {
      PAL_LavaBattleRunRoundState(state);
      if (state->flow_action == LAVA_BATTLE_FLOW_END)
      {
         break;
      }
   }

   PAL_BattleUIFinishBattleState(state);
   PAL_LavaClearRoleTempStats();
   PAL_LavaBattleFreeVisuals();
   PAL_LavaReturnFromBattle();
   if (g_lava_autotest_search || g_lava_autotest_load)
   {
      printf("[LAVA][BATTLE] end result=%d\n", state->battle_result);
   }
   return state->battle_result;
}
