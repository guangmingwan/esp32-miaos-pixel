/* Lava fight bridge.
 * This file is the Lava-only counterpart of original fight.c. */

#include "lava_battle.h"

#define LAVA_ENEMY_TEAM_SLOT_COUNT LAVA_BATTLE_MAX_ENEMIES
#define LAVA_ENEMY_TEAM_RECORD_SIZE 10
#define LAVA_ENEMY_RECORD_SIZE 70
#define LAVA_ENEMY_FIELD_HEALTH 11
#define LAVA_ENEMY_FIELD_EXP 12
#define LAVA_ENEMY_FIELD_CASH 13
#define LAVA_ENEMY_FIELD_LEVEL 14
#define LAVA_ENEMY_FIELD_MAGIC 15
#define LAVA_ENEMY_FIELD_MAGIC_RATE 16
#define LAVA_ENEMY_FIELD_ATTACK_EQUIV_ITEM 17
#define LAVA_ENEMY_FIELD_ATTACK_EQUIV_RATE 18
#define LAVA_ENEMY_FIELD_ATTACK_STRENGTH 21
#define LAVA_ENEMY_FIELD_MAGIC_STRENGTH 22
#define LAVA_ENEMY_FIELD_DEFENSE 23
#define LAVA_ENEMY_FIELD_DEXTERITY 24
#define LAVA_ENEMY_FIELD_FLEE_RATE 25
#define LAVA_ENEMY_FIELD_DUAL_MOVE 33
#define LAVA_MAGIC_FLAG_USABLE_TO_ENEMY (1 << 3)
#define LAVA_MAGIC_FLAG_APPLY_TO_ALL    (1 << 4)
#define LAVA_MAGIC_FIELD_TYPE 1
#define LAVA_MAGIC_FIELD_BASE_DAMAGE 13
#define LAVA_MAGIC_RECORD_SIZE 32
#define LAVA_MAGIC_TABLE_MAX 128
#define LAVA_MAGIC_HIGH_ID_BASE          104
#define LAVA_MAGIC_HIGH_ID_TIER2_BASE    125
#define LAVA_MAGIC_HIGH_ID_TIER3_BASE    130
#define LAVA_MAGIC_TYPE_ATTACK_ALL       1
#define LAVA_MAGIC_TYPE_ATTACK_WHOLE     2
#define LAVA_MAGIC_TYPE_ATTACK_FIELD     3
#define LAVA_PLAYER_ROLE_COUNT 6
#define LAVA_PLAYER_MAGIC_COUNT 32
#define LAVA_PLAYER_MAGIC_ARRAY_INDEX 32

static char g_lava_fight_magic_table[LAVA_MAGIC_TABLE_MAX * LAVA_MAGIC_RECORD_SIZE];
static int g_lava_fight_magic_table_count;
static int g_lava_fight_magic_table_loaded;

static void PAL_LavaFightEnsureMagicTable(void)
{
   FILE *fp;
   long chunk_size;
   long table_bytes;

   if (g_lava_fight_magic_table_loaded != 0)
   {
      return;
   }

   g_lava_fight_magic_table_count = 0;
   fp = UTIL_OpenRequiredFile("DATA.MKF");
   if (fp == 0)
   {
      g_lava_fight_magic_table_loaded = 1;
      return;
   }

   chunk_size = PAL_MKFGetChunkSize(4, fp);
   if (chunk_size <= 0)
   {
      fclose(fp);
      g_lava_fight_magic_table_loaded = 1;
      return;
   }

   table_bytes = chunk_size;
   if (table_bytes > (long)sizeof(g_lava_fight_magic_table))
   {
      table_bytes = (long)sizeof(g_lava_fight_magic_table);
   }

   if (PAL_MKFReadChunk((addr)g_lava_fight_magic_table, (UINT)table_bytes, 4,
          fp) > 0)
   {
      g_lava_fight_magic_table_count =
         (int)(table_bytes / LAVA_MAGIC_RECORD_SIZE);
   }
   fclose(fp);
   if (g_lava_autotest_battle)
   {
      printf("[LAVA][MAGICDATA] table count=%d bytes=%ld\n",
         g_lava_fight_magic_table_count, table_bytes);
   }

   g_lava_fight_magic_table_loaded = 1;
}

int PAL_LavaFightResolveMagicIndex(int magic_object_id)
{
   int magic_num;
   int resolved;

   if (magic_object_id <= 0)
   {
      return -1;
   }

   PAL_LavaFightEnsureMagicTable();

   magic_num = PAL_LavaReadObjectField(magic_object_id, 0);
   if (magic_num < 0)
   {
      return -1;
   }

   if (g_lava_fight_magic_table_count > 0 &&
       magic_num < g_lava_fight_magic_table_count)
   {
      return magic_num;
   }

   if (magic_num >= LAVA_MAGIC_HIGH_ID_BASE)
   {
      if (magic_num >= LAVA_MAGIC_HIGH_ID_TIER3_BASE)
      {
         resolved = magic_num - 105;
      }
      else if (magic_num >= LAVA_MAGIC_HIGH_ID_TIER2_BASE)
      {
         resolved = magic_num - 100;
      }
      else
      {
         resolved = magic_num - 95;
      }
      if (resolved >= 0 && resolved < g_lava_fight_magic_table_count)
      {
         return resolved;
      }
   }

   return magic_num;
}

int PAL_LavaFightReadMagicField(int magic_index, int field_index)
{
   long magic_offset;

   if (magic_index < 0 || field_index < 0)
   {
      return 0;
   }

   PAL_LavaFightEnsureMagicTable();
   if (g_lava_fight_magic_table_count <= 0)
   {
      return PAL_LavaReadMagicField(magic_index, field_index);
   }

   magic_offset = (long)magic_index * LAVA_MAGIC_RECORD_SIZE + field_index * 2;
   if (magic_offset + 2 >
       (long)g_lava_fight_magic_table_count * LAVA_MAGIC_RECORD_SIZE)
   {
      return 0;
   }

   return PAL_LavaReadU16((addr)g_lava_fight_magic_table, (int)magic_offset);
}

int PAL_LavaFightCheckResult(int enemy_count, int *enemy_hp, int *party_hp, char *message);
int PAL_LavaFightFixEnemyTarget(int target_sel, int enemy_count, int *enemy_hp);
int PAL_LavaFightFixPartyTarget(int target_sel, int *party_hp_max);
int PAL_LavaFightFixLivingPartyTarget(int target_sel, int *party_hp, int *party_hp_max);
int PAL_LavaFightNextLivingPartyTarget(int target_sel, int *party_hp, int *party_hp_max);
int PAL_LavaFightPrevLivingPartyTarget(int target_sel, int *party_hp, int *party_hp_max);
int PAL_LavaFightEnemyCount(int enemy_team);
void PAL_LavaFightInitEnemies(int enemy_team, int *enemy_hp, int *enemy_hp_max);
int PAL_LavaFightMagicByIndex(int player_role, int magic_sel);
int PAL_LavaFightMagicBaseDamage(int magic_object_id);
void PAL_LavaFightInitParty(int *party_hp, int *party_hp_max);

int g_lava_battle_round_event_arg0[2];
int g_lava_battle_round_event_arg1[2];
int g_lava_battle_round_event_type[2];
int g_lava_battle_round_enemy_object_id;
int g_lava_battle_selected_magic_object_id;
int g_lava_battle_selected_item_object_id;
int g_lava_battle_party_trance[3];
int g_lava_battle_party_defending[3];
int g_lava_battle_round_player_trance_boost;
int g_lava_battle_enemy_object_id[LAVA_BATTLE_MAX_ENEMIES];
int g_lava_battle_acting_player_index;
int g_lava_battle_enemy_attack_strength[LAVA_BATTLE_MAX_ENEMIES];
int g_lava_battle_enemy_magic_strength[LAVA_BATTLE_MAX_ENEMIES];
int g_lava_battle_enemy_defense[LAVA_BATTLE_MAX_ENEMIES];
int g_lava_battle_enemy_dexterity[LAVA_BATTLE_MAX_ENEMIES];
int g_lava_battle_enemy_flee_rate[LAVA_BATTLE_MAX_ENEMIES];
int g_lava_battle_enemy_magic_object_id[LAVA_BATTLE_MAX_ENEMIES];
int g_lava_battle_enemy_equiv_item[LAVA_BATTLE_MAX_ENEMIES];
int g_lava_battle_enemy_magic_rate[LAVA_BATTLE_MAX_ENEMIES];
int g_lava_battle_enemy_equiv_rate[LAVA_BATTLE_MAX_ENEMIES];
int g_lava_battle_enemy_level[LAVA_BATTLE_MAX_ENEMIES];
int g_lava_battle_enemy_dual_move[LAVA_BATTLE_MAX_ENEMIES];
int g_lava_battle_enemy_exp[LAVA_BATTLE_MAX_ENEMIES];
int g_lava_battle_enemy_cash[LAVA_BATTLE_MAX_ENEMIES];
int g_lava_battle_defer_enemy_turn;
int g_lava_battle_round_enemy_magic_object_id;
int g_lava_battle_pending_command[3];
int g_lava_battle_pending_target[3];
int g_lava_battle_pending_magic_sel[3];
int g_lava_battle_pending_magic_object_id[3];
int g_lava_battle_pending_item_sel[3];
int g_lava_battle_pending_item_object_id[3];
int g_lava_battle_pending_ready[3];
int g_lava_battle_defer_enemy_until_party_done;
int g_lava_battle_enemy_phase_index;
int g_lava_battle_forced_enemy_index;


static void PAL_LavaFightSetRoundEvent(int player_side, int event_type, int arg0, int arg1)
{
   int idx;

   idx = player_side ? 1 : 0;
   g_lava_battle_round_event_type[idx] = event_type;
   g_lava_battle_round_event_arg0[idx] = arg0;
   g_lava_battle_round_event_arg1[idx] = arg1;
}

static int PAL_LavaFightPlayerEventNeedsDamagePhase(int event_type)
{
   return event_type == LAVA_BATTLE_EVENT_PLAYER_HIT ||
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
      event_type == LAVA_BATTLE_EVENT_PLAYER_COOP_ALL_KO;
}

static void PAL_LavaFightStorePlayerHit(int *round_result, int hit_index, int target, int damage);

static void PAL_LavaFightSetPartyTrance(int party_index, int rounds)
{
   if (party_index < 0 || party_index >= 3)
   {
      return;
   }

   g_lava_battle_party_trance[party_index] = rounds;
   if (rounds <= 0 && party_index < g_lava_party_count)
   {
      PAL_LavaBattleSetPlayerTransformSprite(g_lava_party_role[party_index], 0);
   }
}

static void PAL_LavaFightConsumePartyTrance(int party_index)
{
   if (party_index < 0 || party_index >= 3)
   {
      return;
   }
   if (g_lava_battle_party_trance[party_index] <= 0)
   {
      return;
   }

   g_lava_battle_party_trance[party_index]--;
   if (g_lava_battle_party_trance[party_index] <= 0 && party_index < g_lava_party_count)
   {
      PAL_LavaBattleSetPlayerTransformSprite(g_lava_party_role[party_index], 0);
   }
}

static void PAL_LavaFightApplyAutotestBattleState(void)
{
   if (g_lava_autotest_bsilence && g_lava_party_count > 1)
   {
      PAL_LavaBattleSetPlayerStatus(g_lava_party_role[1], LAVA_BATTLE_STATUS_SILENCE, 2);
      printf("[LAVA][BATTLETEST] inject silence role=%d rounds=2\n", g_lava_party_role[1]);
   }
   if (g_lava_autotest_bsleep && g_lava_party_count > 1)
   {
      PAL_LavaBattleSetPlayerStatus(g_lava_party_role[1], LAVA_BATTLE_STATUS_SLEEP, 2);
      printf("[LAVA][BATTLETEST] inject sleep role=%d rounds=2\n", g_lava_party_role[1]);
   }
}

static void PAL_LavaFightSyncPartyStateFromRoles(int *party_hp, int *party_hp_max)
{
   int hp;
   int i;
   int max_hp;
   int role;

   for (i = 0; i < 3; i++)
   {
      if (party_hp != 0)
      {
         party_hp[i] = 0;
      }
      if (party_hp_max != 0)
      {
         party_hp_max[i] = 0;
      }
   }

   for (i = 0; i < g_lava_party_count && i < 3; i++)
   {
      role = g_lava_party_role[i];
      max_hp = PAL_LavaRoleWordByArray(7, role);
      hp = PAL_LavaRoleWordByArray(9, role);
      if (max_hp < 0)
      {
         max_hp = 0;
      }
      if (hp < 0)
      {
         hp = 0;
      }
      if (max_hp > 0 && hp > max_hp)
      {
         hp = max_hp;
      }
      if (party_hp != 0)
      {
         party_hp[i] = hp;
      }
      if (party_hp_max != 0)
      {
         party_hp_max[i] = max_hp;
      }
   }
}

static void PAL_LavaFightReadPartyMPFromRoles(int *party_mp)
{
   int i;

   if (party_mp == 0)
   {
      return;
   }

   for (i = 0; i < 3; i++)
   {
      party_mp[i] = 0;
   }

   for (i = 0; i < g_lava_party_count && i < 3; i++)
   {
      int mp;

      mp = PAL_LavaRoleWordByArray(10, g_lava_party_role[i]);
      if (mp < 0)
      {
         mp = 0;
      }
      party_mp[i] = mp;
   }
}

static int PAL_LavaFightSupportMagicTargetRole(int acting_player, int magic_type, int target_sel)
{
   if (magic_type == 8)
   {
      return g_lava_party_role[acting_player];
   }

   if (magic_type == 4 &&
       target_sel >= 0 &&
       target_sel < g_lava_party_count &&
       target_sel < 3)
   {
      return g_lava_party_role[target_sel];
   }

   return 0;
}

static long PAL_LavaFightReadObjectU16Field(int object_id, int field_index)
{
   int value;

   value = PAL_LavaReadObjectField(object_id, field_index);
   if (value < 0)
   {
      return ((long)value) + 65536;
   }

   return (long)value;
}

static int PAL_LavaFightReadItemFlags(int item_object_id)
{
   return PAL_LavaReadObjectField(item_object_id, gConfig.fIsWIN95 ? 6 : 5);
}

static int PAL_LavaFightReadRoleMagic(int player_role, int magic_slot)
{
   int offset;

   if (player_role < 0 || player_role >= LAVA_PLAYER_ROLE_COUNT ||
       magic_slot < 0 || magic_slot >= LAVA_PLAYER_MAGIC_COUNT)
   {
      return 0;
   }

   offset = LAVA_PLAYER_MAGIC_ARRAY_INDEX * LAVA_PLAYER_ROLE_COUNT * 2;
   offset += magic_slot * LAVA_PLAYER_ROLE_COUNT * 2;
   offset += player_role * 2;
   return PAL_LavaReadU16((addr)g_lava_data_buf, offset);
}

static int PAL_LavaFightReadRoleCoopMagic(int player_role)
{
   return PAL_LavaRoleWordByArray(65, player_role);
}

static int PAL_LavaFightRunSupportMagicScripts(
   int magic_object_id,
   int player_role,
   int acting_player,
   int magic_type,
   int target_sel,
   int *party_hp,
   int *party_hp_max
)
{
   long script_on_success;
   long script_on_use;
   int success_target_role;

   script_on_success = PAL_LavaFightReadObjectU16Field(magic_object_id, 2);
   script_on_use = PAL_LavaFightReadObjectU16Field(magic_object_id, 3);
   success_target_role = PAL_LavaFightSupportMagicTargetRole(
      acting_player, magic_type, target_sel);

   g_lava_script_success = 1;
   if (script_on_use > 0)
   {
      PAL_LavaRunRoleTriggerScript((long)script_on_use, player_role);
   }

   if (g_lava_script_success && script_on_success > 0)
   {
      PAL_LavaRunRoleTriggerScript((long)script_on_success, success_target_role);
   }

   PAL_LavaFightSyncPartyStateFromRoles(party_hp, party_hp_max);
   return g_lava_script_success;
}

static int PAL_LavaFightDetectSupportMagicRecovery(
   int *before_party_hp,
   int *party_hp,
   int *round_result
)
{
   int hit_count;
   int i;
   int recovered;

   hit_count = 0;
   for (i = 0; i < g_lava_party_count && i < 3; i++)
   {
      recovered = party_hp[i] - before_party_hp[i];
      if (recovered <= 0)
      {
         continue;
      }
      PAL_LavaFightStorePlayerHit(round_result, hit_count, i, recovered);
      hit_count++;
   }

   return hit_count;
}

static int PAL_LavaFightReadEnemyTeamMember(int enemy_team, int slot)
{
   FILE *fp;
   long chunk_base;
   long chunk_end;
    long team_offset;
   char buf[8];

   if (enemy_team < 0 || slot < 0 || slot >= LAVA_ENEMY_TEAM_SLOT_COUNT)
   {
      return 0;
   }

   fp = UTIL_OpenRequiredFile("DATA.MKF");
   if (fp == 0)
   {
      return 0;
   }

   if (!PAL_LavaFseekOK(fp, 8, SEEK_SET))
   {
      fclose(fp);
      return 0;
   }
   if (fread((addr)buf, 1, 8, fp) != 8)
   {
      fclose(fp);
      return 0;
   }
   chunk_base = PAL_LavaReadU32((addr)buf, 0);
   chunk_end = PAL_LavaReadU32((addr)buf, 4);
   team_offset = (long)enemy_team * LAVA_ENEMY_TEAM_RECORD_SIZE + slot * 2;
   if (team_offset + 2 > chunk_end - chunk_base)
   {
      fclose(fp);
      return 0;
   }

   if (!PAL_LavaFseekOK(fp, chunk_base + team_offset, SEEK_SET))
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

static int PAL_LavaFightReadEnemyField(int enemy_id, int field_index)
{
   long chunk_base;
   long chunk_end;
    long enemy_offset;
    FILE *fp;
   char buf[8];

   if (enemy_id < 0 || field_index < 0)
   {
      return 0;
   }

   fp = UTIL_OpenRequiredFile("DATA.MKF");
   if (fp == 0)
   {
      return 0;
   }

   if (!PAL_LavaFseekOK(fp, 4, SEEK_SET))
   {
      fclose(fp);
      return 0;
   }
   if (fread((addr)buf, 1, 8, fp) != 8)
   {
      fclose(fp);
      return 0;
   }
   chunk_base = PAL_LavaReadU32((addr)buf, 0);
   chunk_end = PAL_LavaReadU32((addr)buf, 4);
   enemy_offset = (long)enemy_id * LAVA_ENEMY_RECORD_SIZE + field_index * 2;
   if (enemy_offset + 2 > chunk_end - chunk_base)
   {
      fclose(fp);
      return 0;
   }

   if (!PAL_LavaFseekOK(fp, chunk_base + enemy_offset, SEEK_SET))
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

static void PAL_LavaFightLoadEnemyTeamState(int enemy_team, LAVA_BATTLE_STATE *state)
{
   int count;
   int enemy_id;
   int object_id;
   int slot;

   if (state == 0)
   {
      return;
   }
   if (enemy_team < 0)
   {
      return;
   }

   for (slot = 0; slot < LAVA_BATTLE_MAX_ENEMIES; slot++)
   {
      g_lava_battle_enemy_object_id[slot] = 0;
      state->enemy_object_id[slot] = 0;
      state->enemy_hp[slot] = 0;
      state->enemy_hp_max[slot] = 0;
      g_lava_battle_enemy_attack_strength[slot] = 18;
      g_lava_battle_enemy_magic_strength[slot] = 14;
      g_lava_battle_enemy_defense[slot] = 8;
      g_lava_battle_enemy_dexterity[slot] = 10;
      g_lava_battle_enemy_flee_rate[slot] = 40;
      g_lava_battle_enemy_magic_object_id[slot] = 0;
      g_lava_battle_enemy_equiv_item[slot] = 0;
      g_lava_battle_enemy_magic_rate[slot] = 0;
      g_lava_battle_enemy_equiv_rate[slot] = 0;
      g_lava_battle_enemy_level[slot] = 1;
      g_lava_battle_enemy_dual_move[slot] = 0;
      g_lava_battle_enemy_exp[slot] = 0;
      g_lava_battle_enemy_cash[slot] = 0;
   }
   count = 0;

   for (slot = 0; slot < LAVA_ENEMY_TEAM_SLOT_COUNT && count < LAVA_BATTLE_MAX_ENEMIES; slot++)
   {
      object_id = PAL_LavaFightReadEnemyTeamMember(enemy_team, slot);
      if (object_id == 0 || object_id == 0xFFFF)
      {
         continue;
      }

      enemy_id = PAL_LavaReadObjectField(object_id, 0);
      if (enemy_id < 0)
      {
         continue;
      }

      g_lava_battle_enemy_object_id[count] = object_id;
      state->enemy_object_id[count] = object_id;
      state->enemy_hp_max[count] = PAL_LavaFightReadEnemyField(enemy_id, LAVA_ENEMY_FIELD_HEALTH);
      state->enemy_hp[count] = state->enemy_hp_max[count];
      g_lava_battle_enemy_exp[count] = PAL_LavaFightReadEnemyField(enemy_id, LAVA_ENEMY_FIELD_EXP);
      g_lava_battle_enemy_cash[count] = PAL_LavaFightReadEnemyField(enemy_id, LAVA_ENEMY_FIELD_CASH);
      g_lava_battle_enemy_level[count] = PAL_LavaFightReadEnemyField(enemy_id, LAVA_ENEMY_FIELD_LEVEL);
      g_lava_battle_enemy_attack_strength[count] =
         PAL_LavaFightReadEnemyField(enemy_id, LAVA_ENEMY_FIELD_ATTACK_STRENGTH);
      g_lava_battle_enemy_magic_strength[count] =
         PAL_LavaFightReadEnemyField(enemy_id, LAVA_ENEMY_FIELD_MAGIC_STRENGTH);
      g_lava_battle_enemy_defense[count] =
         PAL_LavaFightReadEnemyField(enemy_id, LAVA_ENEMY_FIELD_DEFENSE);
      g_lava_battle_enemy_dexterity[count] =
         PAL_LavaFightReadEnemyField(enemy_id, LAVA_ENEMY_FIELD_DEXTERITY);
      g_lava_battle_enemy_flee_rate[count] =
         PAL_LavaFightReadEnemyField(enemy_id, LAVA_ENEMY_FIELD_FLEE_RATE);
      g_lava_battle_enemy_magic_object_id[count] =
         PAL_LavaFightReadEnemyField(enemy_id, LAVA_ENEMY_FIELD_MAGIC);
      g_lava_battle_enemy_equiv_item[count] =
         PAL_LavaFightReadEnemyField(enemy_id, LAVA_ENEMY_FIELD_ATTACK_EQUIV_ITEM);
      g_lava_battle_enemy_magic_rate[count] =
         PAL_LavaFightReadEnemyField(enemy_id, LAVA_ENEMY_FIELD_MAGIC_RATE);
      g_lava_battle_enemy_equiv_rate[count] =
         PAL_LavaFightReadEnemyField(enemy_id, LAVA_ENEMY_FIELD_ATTACK_EQUIV_RATE);
      g_lava_battle_enemy_dual_move[count] =
         PAL_LavaFightReadEnemyField(enemy_id, LAVA_ENEMY_FIELD_DUAL_MOVE);
      if (state->enemy_hp[count] <= 0)
      {
         state->enemy_hp[count] = 1;
      }
      if (g_lava_battle_enemy_level[count] <= 0)
      {
         g_lava_battle_enemy_level[count] = 1;
      }
      if (g_lava_battle_enemy_attack_strength[count] <= 0)
      {
         g_lava_battle_enemy_attack_strength[count] = 18;
      }
      if (g_lava_battle_enemy_magic_strength[count] <= 0)
      {
         g_lava_battle_enemy_magic_strength[count] = g_lava_battle_enemy_attack_strength[count];
      }
      if (g_lava_battle_enemy_defense[count] < 0)
      {
         g_lava_battle_enemy_defense[count] = 0;
      }
      if (g_lava_battle_enemy_dexterity[count] <= 0)
      {
         g_lava_battle_enemy_dexterity[count] = 10;
      }
      if (g_lava_battle_enemy_flee_rate[count] <= 0)
      {
         g_lava_battle_enemy_flee_rate[count] = 40;
      }
      count++;
   }

   if (count > 0)
   {
      state->enemy_count = count;
   }
}

static void PAL_LavaFightUpdateBattleRewards(LAVA_BATTLE_STATE *state)
{
   int i;

   if (state == 0)
   {
      return;
   }

   state->exp_gained = 0;
   state->cash_gained = 0;
   for (i = 0; i < state->enemy_count && i < LAVA_BATTLE_MAX_ENEMIES; i++)
   {
      state->exp_gained += g_lava_battle_enemy_exp[i];
      state->cash_gained += g_lava_battle_enemy_cash[i];
   }
}

void PAL_LavaFightApplyBattleRewards(LAVA_BATTLE_STATE *state)
{
   int exp;
   int i;

   if (state == 0 || state->rewards_applied)
   {
      return;
   }

   state->level_up_role_count = 0;
   if (state->battle_result == LAVA_BATTLE_RESULT_WIN)
   {
      g_lava_cash += state->cash_gained;
      exp = state->exp_gained;
      for (i = 0; i < g_lava_party_count && i < 3; i++)
      {
         int after_attack;
         int after_defense;
         int after_dexterity;
          int after_flee;
          int after_hp;
          int after_hp_max;
          int after_level;
          int after_magic;
          int after_mp;
          int after_mp_max;
          int before_attack;
          int before_defense;
          int before_dexterity;
          int before_flee;
          int before_hp;
          int before_hp_max;
          int before_level;
          int before_magic;
          int before_mp;
          int before_mp_max;
          int level_gain;
          int role;

          role = g_lava_party_role[i];
          before_level = PAL_LavaRoleWordByArray(6, role);
          before_hp = PAL_LavaRoleWordByArray(9, role);
          before_hp_max = PAL_LavaRoleWordByArray(7, role);
          before_mp = PAL_LavaRoleWordByArray(10, role);
          before_mp_max = PAL_LavaRoleWordByArray(8, role);
          before_attack = PAL_LavaRoleWordByArray(17, role);
          before_magic = PAL_LavaRoleWordByArray(18, role);
          before_defense = PAL_LavaRoleWordByArray(19, role);
         before_dexterity = PAL_LavaRoleWordByArray(20, role);
         before_flee = PAL_LavaRoleWordByArray(21, role);

         if (state->party_hp[i] <= 0)
         {
            continue;
         }

         level_gain = exp / 100;
         if (level_gain <= 0 && exp > 0)
         {
            level_gain = 1;
         }
         if (level_gain > 3)
         {
            level_gain = 3;
         }
         if (level_gain > 0)
         {
            PAL_LavaWriteU16((addr)g_lava_data_buf, 6 * 6 * 2 + role * 2, before_level + level_gain);
            PAL_LavaWriteU16((addr)g_lava_data_buf, 7 * 6 * 2 + role * 2, before_hp_max + level_gain * 8);
            PAL_LavaWriteU16((addr)g_lava_data_buf, 8 * 6 * 2 + role * 2, before_mp_max + level_gain * 6);
            PAL_LavaWriteU16((addr)g_lava_data_buf, 17 * 6 * 2 + role * 2, before_attack + level_gain * 2);
            PAL_LavaWriteU16((addr)g_lava_data_buf, 18 * 6 * 2 + role * 2, before_magic + level_gain * 2);
            PAL_LavaWriteU16((addr)g_lava_data_buf, 19 * 6 * 2 + role * 2, before_defense + level_gain * 2);
            PAL_LavaWriteU16((addr)g_lava_data_buf, 20 * 6 * 2 + role * 2, before_dexterity + level_gain * 2);
            PAL_LavaWriteU16((addr)g_lava_data_buf, 21 * 6 * 2 + role * 2, before_flee + level_gain * 2);
            after_level = PAL_LavaRoleWordByArray(6, role);
            after_hp_max = PAL_LavaRoleWordByArray(7, role);
            after_mp_max = PAL_LavaRoleWordByArray(8, role);
            PAL_LavaWriteU16((addr)g_lava_data_buf, 9 * 6 * 2 + role * 2, after_hp_max);
            PAL_LavaWriteU16((addr)g_lava_data_buf, 10 * 6 * 2 + role * 2, after_mp_max);
            after_hp = PAL_LavaRoleWordByArray(9, role);
            after_mp = PAL_LavaRoleWordByArray(10, role);
            after_attack = PAL_LavaRoleWordByArray(17, role);
            after_magic = PAL_LavaRoleWordByArray(18, role);
            after_defense = PAL_LavaRoleWordByArray(19, role);
            after_dexterity = PAL_LavaRoleWordByArray(20, role);
            after_flee = PAL_LavaRoleWordByArray(21, role);
            if (after_level > before_level && state->level_up_role_count < 3)
            {
               int idx;

               idx = state->level_up_role_count;
               state->level_up_role[idx] = role;
               state->level_up_before[0 * 3 + idx] = before_level;
               state->level_up_before[1 * 3 + idx] = before_hp;
               state->level_up_before[2 * 3 + idx] = before_hp_max;
               state->level_up_before[3 * 3 + idx] = before_mp;
               state->level_up_before[4 * 3 + idx] = before_mp_max;
               state->level_up_before[5 * 3 + idx] = before_attack;
               state->level_up_before[6 * 3 + idx] = before_magic;
               state->level_up_before[7 * 3 + idx] = before_defense;
               state->level_up_before[8 * 3 + idx] = before_dexterity;
               state->level_up_before[9 * 3 + idx] = before_flee;
               state->level_up_after[0 * 3 + idx] = after_level;
               state->level_up_after[1 * 3 + idx] = after_hp;
               state->level_up_after[2 * 3 + idx] = after_hp_max;
               state->level_up_after[3 * 3 + idx] = after_mp;
               state->level_up_after[4 * 3 + idx] = after_mp_max;
               state->level_up_after[5 * 3 + idx] = after_attack;
               state->level_up_after[6 * 3 + idx] = after_magic;
               state->level_up_after[7 * 3 + idx] = after_defense;
               state->level_up_after[8 * 3 + idx] = after_dexterity;
               state->level_up_after[9 * 3 + idx] = after_flee;
               if (i >= 0 && i < 3)
               {
                  state->party_hp[i] = after_hp;
                  state->party_hp_max[i] = after_hp_max;
               }
               state->level_up_role_count++;
            }
         }
      }
   }
   state->rewards_applied = 1;
}

static void PAL_LavaFightClearRoundScript(int *round_result)
{
   if (round_result == 0)
   {
      return;
   }

   round_result[LAVA_BATTLE_ROUND_STEP0] = LAVA_BATTLE_PHASE_NONE;
   round_result[LAVA_BATTLE_ROUND_STEP1] = LAVA_BATTLE_PHASE_NONE;
   round_result[LAVA_BATTLE_ROUND_STEP2] = LAVA_BATTLE_PHASE_NONE;
   round_result[LAVA_BATTLE_ROUND_STEP3] = LAVA_BATTLE_PHASE_NONE;
}

static void PAL_LavaFightClearPlayerHitList(int *round_result)
{
   if (round_result == 0)
   {
      return;
   }

   round_result[LAVA_BATTLE_ROUND_PLAYER_HIT_COUNT] = 0;
   round_result[LAVA_BATTLE_ROUND_PLAYER_TARGET1] = -1;
   round_result[LAVA_BATTLE_ROUND_PLAYER_DAMAGE1] = 0;
   round_result[LAVA_BATTLE_ROUND_PLAYER_TARGET2] = -1;
   round_result[LAVA_BATTLE_ROUND_PLAYER_DAMAGE2] = 0;
   round_result[LAVA_BATTLE_ROUND_PLAYER_TARGET3] = -1;
   round_result[LAVA_BATTLE_ROUND_PLAYER_DAMAGE3] = 0;
   round_result[LAVA_BATTLE_ROUND_PLAYER_TARGET4] = -1;
   round_result[LAVA_BATTLE_ROUND_PLAYER_DAMAGE4] = 0;
}

static void PAL_LavaFightClearEnemyHitList(int *round_result)
{
   if (round_result == 0)
   {
      return;
   }

   round_result[LAVA_BATTLE_ROUND_ENEMY_HIT_COUNT] = 0;
   round_result[LAVA_BATTLE_ROUND_ENEMY_TARGET1] = -1;
   round_result[LAVA_BATTLE_ROUND_ENEMY_DAMAGE1] = 0;
   round_result[LAVA_BATTLE_ROUND_ENEMY_TARGET2] = -1;
   round_result[LAVA_BATTLE_ROUND_ENEMY_DAMAGE2] = 0;
   round_result[LAVA_BATTLE_ROUND_ENEMY_TARGET3] = -1;
   round_result[LAVA_BATTLE_ROUND_ENEMY_DAMAGE3] = 0;
}

static void PAL_LavaFightStorePlayerHit(int *round_result, int hit_index, int target, int damage)
{
   int base;

   if (round_result == 0 || hit_index < 0 || hit_index >= LAVA_BATTLE_MAX_ENEMIES)
   {
      return;
   }

   if (hit_index == 0)
   {
      round_result[LAVA_BATTLE_ROUND_PLAYER_TARGET] = target;
      round_result[LAVA_BATTLE_ROUND_PLAYER_DAMAGE] = damage;
   }
   else
   {
      base = LAVA_BATTLE_ROUND_PLAYER_TARGET1 + (hit_index - 1) * 2;
      round_result[base] = target;
      round_result[base + 1] = damage;
   }

   if (round_result[LAVA_BATTLE_ROUND_PLAYER_HIT_COUNT] < hit_index + 1)
   {
      round_result[LAVA_BATTLE_ROUND_PLAYER_HIT_COUNT] = hit_index + 1;
   }
}

static void PAL_LavaFightStoreEnemyHit(int *round_result, int hit_index, int target, int damage)
{
   int base;

   if (round_result == 0 || hit_index < 0 || hit_index >= 3)
   {
      return;
   }

   if (hit_index == 0)
   {
      round_result[LAVA_BATTLE_ROUND_ENEMY_TARGET] = target;
      round_result[LAVA_BATTLE_ROUND_ENEMY_DAMAGE] = damage;
   }
   base = LAVA_BATTLE_ROUND_ENEMY_TARGET1 + hit_index * 2;
   round_result[base] = target;
   round_result[base + 1] = damage;

   if (round_result[LAVA_BATTLE_ROUND_ENEMY_HIT_COUNT] < hit_index + 1)
   {
      round_result[LAVA_BATTLE_ROUND_ENEMY_HIT_COUNT] = hit_index + 1;
   }
}

static void PAL_LavaFightAppendRoundPhase(int *round_result, int phase)
{
   if (round_result == 0 || phase == LAVA_BATTLE_PHASE_NONE)
   {
      return;
   }

   if (round_result[LAVA_BATTLE_ROUND_STEP0] == LAVA_BATTLE_PHASE_NONE)
   {
      round_result[LAVA_BATTLE_ROUND_STEP0] = phase;
   }
   else if (round_result[LAVA_BATTLE_ROUND_STEP1] == LAVA_BATTLE_PHASE_NONE)
   {
      round_result[LAVA_BATTLE_ROUND_STEP1] = phase;
   }
   else if (round_result[LAVA_BATTLE_ROUND_STEP2] == LAVA_BATTLE_PHASE_NONE)
   {
      round_result[LAVA_BATTLE_ROUND_STEP2] = phase;
   }
   else if (round_result[LAVA_BATTLE_ROUND_STEP3] == LAVA_BATTLE_PHASE_NONE)
   {
      round_result[LAVA_BATTLE_ROUND_STEP3] = phase;
   }
}

void PAL_LavaFightClearRoundMessages(void)
{
   g_lava_battle_round_event_type[0] = LAVA_BATTLE_EVENT_NONE;
   g_lava_battle_round_event_arg0[0] = 0;
   g_lava_battle_round_event_arg1[0] = 0;
   g_lava_battle_round_event_type[1] = LAVA_BATTLE_EVENT_NONE;
   g_lava_battle_round_event_arg0[1] = 0;
   g_lava_battle_round_event_arg1[1] = 0;
   g_lava_battle_round_enemy_object_id = 0;
   g_lava_battle_round_enemy_magic_object_id = 0;
   g_lava_battle_round_player_trance_boost = 0;
}

int PAL_LavaFightGetRoundEventType(int player_side)
{
   return g_lava_battle_round_event_type[player_side ? 1 : 0];
}

int PAL_LavaFightGetRoundEventArg0(int player_side)
{
   return g_lava_battle_round_event_arg0[player_side ? 1 : 0];
}

int PAL_LavaFightGetRoundEventArg1(int player_side)
{
   return g_lava_battle_round_event_arg1[player_side ? 1 : 0];
}

int PAL_LavaFightGetPartyTrance(int party_index)
{
   if (party_index < 0 || party_index >= 3)
   {
      return 0;
   }

   return g_lava_battle_party_trance[party_index];
}

int PAL_LavaFightGetRoundPlayerTranceBoost(void)
{
   return g_lava_battle_round_player_trance_boost;
}

int PAL_LavaFightGetRoundEnemyObjectID(void)
{
   return g_lava_battle_round_enemy_object_id;
}

int PAL_LavaFightGetRoundEnemyMagicObjectID(void)
{
   return g_lava_battle_round_enemy_magic_object_id;
}

int PAL_LavaFightPickTurnActor(int *party_hp, int turn)
{
   int alive_index;
   int alive_total;
   int i;
   int max_party;

   max_party = g_lava_party_count < 3 ? g_lava_party_count : 3;
   if (max_party <= 0)
   {
      return 0;
   }

   alive_total = 0;
   for (i = 0; i < max_party; i++)
   {
      if (party_hp[i] > 0)
      {
         alive_total++;
      }
   }
   if (alive_total <= 0)
   {
      return 0;
   }

   alive_index = turn % alive_total;
   for (i = 0; i < max_party; i++)
   {
      if (party_hp[i] <= 0)
      {
         continue;
      }
      if (alive_index == 0)
      {
         return i;
      }
      alive_index--;
   }

   return 0;
}

int PAL_LavaFightFirstLivingActor(int *party_hp)
{
   int i;
   int max_party;

   max_party = g_lava_party_count < 3 ? g_lava_party_count : 3;
   for (i = 0; i < max_party; i++)
   {
      if (party_hp[i] > 0)
      {
         return i;
      }
   }
   return 0;
}

int PAL_LavaFightBeginRound(int enemy_count, int *enemy_hp, int *party_hp, int *target_sel, char *message)
{
   if (target_sel != 0)
   {
      *target_sel = PAL_LavaFightFixEnemyTarget(*target_sel, enemy_count, enemy_hp);
   }

   return PAL_LavaFightCheckResult(enemy_count, enemy_hp, party_hp, message);
}

void PAL_LavaFightResetBattleState(
   int *command_sel,
   int *target_sel,
   int *turn,
   char *message
)
{
   if (command_sel != 0)
   {
      *command_sel = 0;
   }
   if (target_sel != 0)
   {
      *target_sel = 0;
   }
   if (turn != 0)
   {
      *turn = 0;
   }
   if (message != 0)
   {
      sprintf(message, "遭遇敌人");
   }
}

void PAL_LavaFightPrepareBattle(
   int enemy_team,
   int *enemy_count,
   int *enemy_hp,
   int *enemy_hp_max,
   int *party_hp,
   int *party_hp_max
)
{
   if (enemy_count != 0)
   {
      *enemy_count = PAL_LavaFightEnemyCount(enemy_team);
   }
   if (enemy_hp != 0 && enemy_hp_max != 0)
   {
      PAL_LavaFightInitEnemies(enemy_team, enemy_hp, enemy_hp_max);
   }
   if (party_hp != 0 && party_hp_max != 0)
   {
      PAL_LavaFightInitParty(party_hp, party_hp_max);
   }
}

void PAL_LavaFightPrepareBattleState(int enemy_team, LAVA_BATTLE_STATE *state)
{
   int i;

   if (state == 0)
   {
      return;
   }

   state->battle_result = LAVA_BATTLE_RESULT_WIN;
   state->flow_action = LAVA_BATTLE_FLOW_NEXT_ROUND;
   state->enemy_team = enemy_team;
    state->magic_sel = 0;
    state->magic_object_id = 0;
    state->item_sel = 0;
    state->item_object_id = 0;
    state->round_command = LAVA_BATTLE_COMMAND_ATTACK;
   state->acting_player_index = 0;
   state->exp_gained = 0;
   state->cash_gained = 0;
   state->rewards_applied = 0;
   PAL_LavaBattleClearPlayerStatuses();
   PAL_LavaBattleClearPlayerTransformSprites();
   for (i = 0; i < 3; i++)
   {
      g_lava_battle_party_trance[i] = 0;
      g_lava_battle_party_defending[i] = 0;
      state->party_defending[i] = 0;
      state->party_pose[i] = 0;
      g_lava_battle_pending_command[i] = LAVA_BATTLE_COMMAND_ATTACK;
      g_lava_battle_pending_target[i] = 0;
      g_lava_battle_pending_magic_sel[i] = 0;
      g_lava_battle_pending_magic_object_id[i] = 0;
      g_lava_battle_pending_item_sel[i] = 0;
      g_lava_battle_pending_item_object_id[i] = 0;
      g_lava_battle_pending_ready[i] = 0;
   }
   g_lava_battle_defer_enemy_until_party_done = 0;
   g_lava_battle_enemy_phase_index = -1;
   g_lava_battle_forced_enemy_index = -1;
   PAL_LavaFightResetBattleState(&state->command_sel, &state->target_sel,
      &state->turn, state->message);
   PAL_LavaFightPrepareBattle(enemy_team, &state->enemy_count, state->enemy_hp,
      state->enemy_hp_max, state->party_hp, state->party_hp_max);
   PAL_LavaFightApplyAutotestBattleState();
   PAL_LavaFightLoadEnemyTeamState(enemy_team, state);
   PAL_LavaFightUpdateBattleRewards(state);
}

void PAL_LavaFightAdvanceBattleState(LAVA_BATTLE_STATE *state)
{
   if (state == 0)
   {
      return;
   }

   if (state->battle_result == LAVA_BATTLE_RESULT_CONTINUE)
   {
      int i;
      int j;

      if (state->flow_action == LAVA_BATTLE_FLOW_RECHECK)
      {
         return;
      }

      for (i = 0; i < g_lava_party_count && i < 3; i++)
      {
         int role;

         role = g_lava_party_role[i];
         for (j = 0; j < LAVA_BATTLE_STATUS_COUNT; j++)
         {
            int status_value;

            status_value = PAL_LavaBattleGetPlayerStatus(role, j);
            if (status_value > 0 && status_value <= 999)
            {
               PAL_LavaBattleRemovePlayerStatus(role, j);
               if (status_value - 1 > 0)
               {
                  PAL_LavaBattleSetPlayerStatus(role, j, status_value - 1);
               }
            }
         }
      }
      state->turn++;
      state->magic_object_id = 0;
      state->item_object_id = 0;
      state->flow_action = LAVA_BATTLE_FLOW_NEXT_ROUND;
      return;
   }

   if (state->battle_result == LAVA_BATTLE_RESULT_WIN)
   {
      state->flow_action = LAVA_BATTLE_FLOW_END;
      return;
   }

   state->flow_action = LAVA_BATTLE_FLOW_END;
}

int PAL_LavaFightBeginRoundState(LAVA_BATTLE_STATE *state)
{
   int acting_player;
   int i;
   int result;

   if (state == 0)
   {
      return LAVA_BATTLE_RESULT_LOSE;
   }

    if (state->flow_action == LAVA_BATTLE_FLOW_RECHECK)
    {
       state->flow_action = LAVA_BATTLE_FLOW_NEXT_ROUND;
       return LAVA_BATTLE_RESULT_CONTINUE;
    }

   g_lava_battle_enemy_phase_index = -1;
   g_lava_battle_forced_enemy_index = -1;

    result = PAL_LavaFightBeginRound(state->enemy_count, state->enemy_hp,
       state->party_hp, &state->target_sel, state->message);
   if (result == LAVA_BATTLE_RESULT_CONTINUE)
   {
      int party_slots;

      state->command_sel = LAVA_BATTLE_COMMAND_ATTACK;
      acting_player = PAL_LavaFightFirstLivingActor(state->party_hp);
      if (acting_player != state->acting_player_index)
      {
         state->magic_sel = 0;
         state->magic_object_id = 0;
         state->item_sel = 0;
         state->item_object_id = 0;
      }
      state->acting_player_index = acting_player;
      party_slots = g_lava_party_count < 3 ? g_lava_party_count : 3;
      for (i = 0; i < party_slots; i++)
      {
         g_lava_battle_pending_command[i] = LAVA_BATTLE_COMMAND_ATTACK;
         g_lava_battle_pending_target[i] = PAL_LavaFightFixEnemyTarget(state->target_sel,
            state->enemy_count, state->enemy_hp);
         g_lava_battle_pending_magic_sel[i] = 0;
         g_lava_battle_pending_magic_object_id[i] = 0;
         g_lava_battle_pending_item_sel[i] = 0;
         g_lava_battle_pending_item_object_id[i] = 0;
         g_lava_battle_pending_ready[i] = (state->party_hp[i] <= 0);
      }
      for (; i < 3; i++)
      {
         g_lava_battle_pending_command[i] = LAVA_BATTLE_COMMAND_ATTACK;
         g_lava_battle_pending_target[i] = 0;
         g_lava_battle_pending_magic_sel[i] = 0;
         g_lava_battle_pending_magic_object_id[i] = 0;
         g_lava_battle_pending_item_sel[i] = 0;
         g_lava_battle_pending_item_object_id[i] = 0;
         g_lava_battle_pending_ready[i] = 1;
      }
      if (acting_player >= 0 && acting_player < 3)
      {
         g_lava_battle_party_defending[acting_player] = 0;
         state->party_defending[acting_player] = 0;
      }
      for (i = 0; i < 3; i++)
      {
         state->party_defending[i] = g_lava_battle_party_defending[i];
         state->party_pose[i] = 0;
      }
      g_lava_battle_acting_player_index = state->acting_player_index;
   }

   return result;
}

int PAL_LavaFightEnemyCount(int enemy_team)
{
   int enemy_count;
   int i;

   enemy_count = 0;
   if (enemy_team >= 0)
   {
      for (i = 0; i < LAVA_BATTLE_MAX_ENEMIES; i++)
      {
         if (PAL_LavaFightReadEnemyTeamMember(enemy_team, i) != 0)
         {
            enemy_count++;
         }
      }
   }
   if (enemy_count <= 0)
   {
      enemy_count = (enemy_team == 0 || enemy_team == 1) ? 1 : 2;
   }
   if (enemy_count > LAVA_BATTLE_MAX_ENEMIES) enemy_count = LAVA_BATTLE_MAX_ENEMIES;
   return enemy_count;
}

void PAL_LavaFightInitEnemies(int enemy_team, int *enemy_hp, int *enemy_hp_max)
{
   int i;

   for (i = 0; i < LAVA_BATTLE_MAX_ENEMIES; i++)
   {
      enemy_hp_max[i] = 60 + enemy_team % 15 + i * 8;
      enemy_hp[i] = enemy_hp_max[i];
   }
}

void PAL_LavaFightInitParty(int *party_hp, int *party_hp_max)
{
   int i;
   int role;

   for (i = 0; i < 3; i++)
   {
      party_hp[i] = 0;
      party_hp_max[i] = 0;
   }

   for (i = 0; i < g_lava_party_count && i < 3; i++)
   {
      role = g_lava_party_role[i];
      party_hp_max[i] = PAL_LavaRoleWordByArray(7, role);
      if (party_hp_max[i] <= 0)
      {
         party_hp[i] = 0;
         continue;
      }

      party_hp[i] = PAL_LavaRoleWordByArray(9, role);
      if (party_hp[i] < 0)
      {
         party_hp[i] = 0;
      }
      else if (party_hp[i] > party_hp_max[i])
      {
         party_hp[i] = party_hp_max[i];
      }
   }
}

int PAL_LavaFightCountAlive(int *hp_list, int count)
{
   int i;
   int alive;

   alive = 0;
   for (i = 0; i < count; i++)
   {
      if (hp_list[i] > 0)
      {
         alive++;
      }
   }
   return alive;
}

int PAL_LavaFightCheckResult(int enemy_count, int *enemy_hp, int *party_hp, char *message)
{
   int alive_enemy;
   int alive_party;
   int party_count;

   party_count = g_lava_party_count < 3 ? g_lava_party_count : 3;
   alive_party = PAL_LavaFightCountAlive(party_hp, party_count);
   alive_enemy = PAL_LavaFightCountAlive(enemy_hp, enemy_count);

   if (alive_enemy <= 0)
   {
      sprintf(message, "战斗胜利");
      return LAVA_BATTLE_RESULT_WIN;
   }
   if (alive_party <= 0)
   {
      sprintf(message, "全员败退");
      return LAVA_BATTLE_RESULT_LOSE;
   }

   return LAVA_BATTLE_RESULT_CONTINUE;
}

int PAL_LavaFightFixEnemyTarget(int target_sel, int enemy_count, int *enemy_hp)
{
   int i;

   if (enemy_count <= 0)
   {
      return 0;
   }

   if (target_sel < 0 || target_sel >= enemy_count)
   {
      target_sel = 0;
   }

   if (enemy_hp[target_sel] > 0)
   {
      return target_sel;
   }

   for (i = 0; i < enemy_count; i++)
   {
      if (enemy_hp[i] > 0)
      {
         return i;
      }
   }

   return 0;
}

int PAL_LavaFightNextEnemyTarget(int target_sel, int enemy_count, int *enemy_hp)
{
   int i;

   if (enemy_count <= 0)
   {
      return 0;
   }

   target_sel = PAL_LavaFightFixEnemyTarget(target_sel, enemy_count, enemy_hp);
   for (i = 0; i < enemy_count; i++)
   {
      target_sel++;
      if (target_sel >= enemy_count)
      {
         target_sel = 0;
      }
      if (enemy_hp[target_sel] > 0)
      {
         return target_sel;
      }
   }

   return PAL_LavaFightFixEnemyTarget(0, enemy_count, enemy_hp);
}

int PAL_LavaFightPrevEnemyTarget(int target_sel, int enemy_count, int *enemy_hp)
{
   int i;

   if (enemy_count <= 0)
   {
      return 0;
   }

   target_sel = PAL_LavaFightFixEnemyTarget(target_sel, enemy_count, enemy_hp);
   for (i = 0; i < enemy_count; i++)
   {
      target_sel--;
      if (target_sel < 0)
      {
         target_sel = enemy_count - 1;
      }
      if (enemy_hp[target_sel] > 0)
      {
         return target_sel;
      }
   }

   return PAL_LavaFightFixEnemyTarget(0, enemy_count, enemy_hp);
}

int PAL_LavaFightFixPartyTarget(int target_sel, int *party_hp_max)
{
   int i;
   int max_party;

   max_party = g_lava_party_count < 3 ? g_lava_party_count : 3;
   if (max_party <= 0)
   {
      return 0;
   }

   if (target_sel < 0 || target_sel >= max_party)
   {
      target_sel = 0;
   }

   if (party_hp_max != 0 && party_hp_max[target_sel] > 0)
   {
      return target_sel;
   }

   for (i = 0; i < max_party; i++)
   {
      if (party_hp_max == 0 || party_hp_max[i] > 0)
      {
         return i;
      }
   }

   return 0;
}

int PAL_LavaFightNextPartyTarget(int target_sel, int *party_hp_max)
{
   int i;
   int max_party;

   max_party = g_lava_party_count < 3 ? g_lava_party_count : 3;
   if (max_party <= 0)
   {
      return 0;
   }

   target_sel = PAL_LavaFightFixPartyTarget(target_sel, party_hp_max);
   for (i = 0; i < max_party; i++)
   {
      target_sel++;
      if (target_sel >= max_party)
      {
         target_sel = 0;
      }
      if (party_hp_max == 0 || party_hp_max[target_sel] > 0)
      {
         return target_sel;
      }
   }

   return PAL_LavaFightFixPartyTarget(0, party_hp_max);
}

int PAL_LavaFightPrevPartyTarget(int target_sel, int *party_hp_max)
{
   int i;
   int max_party;

   max_party = g_lava_party_count < 3 ? g_lava_party_count : 3;
   if (max_party <= 0)
   {
      return 0;
   }

   target_sel = PAL_LavaFightFixPartyTarget(target_sel, party_hp_max);
   for (i = 0; i < max_party; i++)
   {
      target_sel--;
      if (target_sel < 0)
      {
         target_sel = max_party - 1;
      }
      if (party_hp_max == 0 || party_hp_max[target_sel] > 0)
      {
         return target_sel;
      }
   }

   return PAL_LavaFightFixPartyTarget(0, party_hp_max);
}

int PAL_LavaFightFixLivingPartyTarget(int target_sel, int *party_hp, int *party_hp_max)
{
   int i;
   int max_party;

   max_party = g_lava_party_count < 3 ? g_lava_party_count : 3;
   if (max_party <= 0)
   {
      return 0;
   }

   target_sel = PAL_LavaFightFixPartyTarget(target_sel, party_hp_max);
   if (party_hp != 0 && party_hp[target_sel] > 0)
   {
      return target_sel;
   }

   for (i = 0; i < max_party; i++)
   {
      if (party_hp == 0 || party_hp[i] > 0)
      {
         return i;
      }
   }

   return target_sel;
}

int PAL_LavaFightNextLivingPartyTarget(int target_sel, int *party_hp, int *party_hp_max)
{
   int i;
   int max_party;

   max_party = g_lava_party_count < 3 ? g_lava_party_count : 3;
   if (max_party <= 0)
   {
      return 0;
   }

   target_sel = PAL_LavaFightFixLivingPartyTarget(target_sel, party_hp, party_hp_max);
   for (i = 0; i < max_party; i++)
   {
      target_sel++;
      if (target_sel >= max_party)
      {
         target_sel = 0;
      }
      if (party_hp == 0 || party_hp[target_sel] > 0)
      {
         return target_sel;
      }
   }

   return PAL_LavaFightFixLivingPartyTarget(0, party_hp, party_hp_max);
}

int PAL_LavaFightPrevLivingPartyTarget(int target_sel, int *party_hp, int *party_hp_max)
{
   int i;
   int max_party;

   max_party = g_lava_party_count < 3 ? g_lava_party_count : 3;
   if (max_party <= 0)
   {
      return 0;
   }

   target_sel = PAL_LavaFightFixLivingPartyTarget(target_sel, party_hp, party_hp_max);
   for (i = 0; i < max_party; i++)
   {
      target_sel--;
      if (target_sel < 0)
      {
         target_sel = max_party - 1;
      }
      if (party_hp == 0 || party_hp[target_sel] > 0)
      {
         return target_sel;
      }
   }

   return PAL_LavaFightFixLivingPartyTarget(0, party_hp, party_hp_max);
}

int PAL_LavaFightChoosePartyTarget(int *party_hp)
{
   int alive_count;
   int chosen_alive;
   int i;
   int max_party;
   int target_role;

   max_party = g_lava_party_count < 3 ? g_lava_party_count : 3;
   if (max_party <= 0)
   {
      return 0;
   }

   alive_count = 0;
   for (i = 0; i < max_party; i++)
   {
      if (party_hp[i] > 0)
      {
         alive_count++;
      }
   }
   if (alive_count <= 0)
   {
      return 0;
   }

   chosen_alive = RandomLong(0, alive_count - 1);
   target_role = 0;
   for (i = 0; i < max_party; i++)
   {
      if (party_hp[i] > 0)
      {
         if (chosen_alive == 0)
         {
            target_role = i;
            break;
         }
         chosen_alive--;
      }
   }

   return target_role;
}

int PAL_LavaFightChooseActingEnemy(int enemy_count, int *enemy_hp)
{
   int i;
   int total_weight;
   int weight;
   int roll;

   if (enemy_count <= 0 || enemy_hp == 0)
   {
      return 0;
   }

   if (g_lava_battle_forced_enemy_index >= 0 &&
       g_lava_battle_forced_enemy_index < enemy_count &&
       g_lava_battle_forced_enemy_index < LAVA_BATTLE_MAX_ENEMIES &&
       enemy_hp[g_lava_battle_forced_enemy_index] > 0)
   {
      return g_lava_battle_forced_enemy_index;
   }

   total_weight = 0;
   for (i = 0; i < enemy_count && i < LAVA_BATTLE_MAX_ENEMIES; i++)
   {
      if (enemy_hp[i] <= 0)
      {
         continue;
      }

      weight = g_lava_battle_enemy_attack_strength[i] +
         g_lava_battle_enemy_dexterity[i] / 2 +
         g_lava_battle_enemy_level[i] * 2;
      if (weight <= 0)
      {
         weight = 1;
      }
      total_weight += weight;
   }

   if (total_weight <= 0)
   {
      return PAL_LavaFightFixEnemyTarget(0, enemy_count, enemy_hp);
   }

   roll = RandomLong(0, total_weight - 1);
   for (i = 0; i < enemy_count && i < LAVA_BATTLE_MAX_ENEMIES; i++)
   {
      if (enemy_hp[i] <= 0)
      {
         continue;
      }

      weight = g_lava_battle_enemy_attack_strength[i] +
         g_lava_battle_enemy_dexterity[i] / 2 +
         g_lava_battle_enemy_level[i] * 2;
      if (weight <= 0)
      {
         weight = 1;
      }
      if (roll < weight)
      {
         return i;
      }
      roll -= weight;
   }

   return PAL_LavaFightFixEnemyTarget(0, enemy_count, enemy_hp);
}

static int PAL_LavaFightNextEnemyPhaseIndex(int start, int enemy_count, int *enemy_hp)
{
   int i;

   for (i = start; i < enemy_count && i < LAVA_BATTLE_MAX_ENEMIES; i++)
   {
      if (enemy_hp[i] > 0)
      {
         return i;
      }
   }
   return -1;
}

int PAL_LavaFightFindPartyIndexByRole(int role)
{
   int i;
   int max_party;

   max_party = g_lava_party_count < 3 ? g_lava_party_count : 3;
   for (i = 0; i < max_party; i++)
   {
      if (g_lava_party_role[i] == role)
      {
         return i;
      }
   }

   return -1;
}

int PAL_LavaFightApplyCoverTarget(int target_role, int *party_hp, int *party_hp_max)
{
   int cover_index;
   int cover_role;

   if (target_role < 0 || target_role >= 3 || party_hp == 0 || party_hp_max == 0)
   {
      return target_role;
   }

   if (party_hp[target_role] <= 0 || party_hp_max[target_role] <= 0)
   {
      return target_role;
   }

   if (party_hp[target_role] > party_hp_max[target_role] / 5)
   {
      return target_role;
   }

   cover_role = PAL_LavaRoleWordByArray(31, g_lava_party_role[target_role]);
   if (cover_role <= 0)
   {
      return target_role;
   }

   cover_index = PAL_LavaFightFindPartyIndexByRole(cover_role);
   if (cover_index < 0 || cover_index == target_role)
   {
      return target_role;
   }
   if (party_hp[cover_index] <= 0)
   {
      return target_role;
   }

   return cover_index;
}

int PAL_LavaFightChooseActingPlayer(int *party_hp)
{
   int i;
   int max_party;

   max_party = g_lava_party_count < 3 ? g_lava_party_count : 3;
   for (i = 0; i < max_party; i++)
   {
      if (party_hp[i] > 0)
      {
         return i;
      }
   }
   return 0;
}

int PAL_LavaFightPlayerDamage(int player_role, int enemy_index, int turn)
{
   int attack_strength;
   int damage;
   int enemy_defense;
   int party_index;
   int level;

   attack_strength = PAL_LavaRoleWordByArray(17, player_role);
   level = PAL_LavaRoleWordByArray(6, player_role);
   enemy_defense = g_lava_battle_enemy_defense[enemy_index];
   damage = attack_strength / 2 + level + 4 + turn - enemy_defense / 4;
   party_index = PAL_LavaFightFindPartyIndexByRole(player_role);
   if (party_index >= 0 && g_lava_battle_party_trance[party_index] > 0)
   {
      damage += damage / 2 + 2;
   }
   if (PAL_LavaBattleGetPlayerStatus(player_role, LAVA_BATTLE_STATUS_BRAVERY) > 0)
   {
      damage += damage / 2 + 1;
   }
   if (damage < 1)
   {
      damage = 1;
   }
   return damage;
}

int PAL_LavaFightPlayerStrongDamage(int player_role, int enemy_index, int turn)
{
   int damage;
   int dexterity;

   damage = PAL_LavaFightPlayerDamage(player_role, enemy_index, turn);
   dexterity = PAL_LavaRoleWordByArray(20, player_role);
   damage += damage / 2 + dexterity / 10 + 2;
   if (damage < 1)
   {
      damage = 1;
   }
   return damage;
}

int PAL_LavaFightPlayerStrongAttack(int player_role, int enemy_index, int turn)
{
   int chance;
   int dexterity;

   dexterity = PAL_LavaRoleWordByArray(20, player_role);
   chance = 8 + dexterity / 6 + turn;
   if (g_lava_battle_enemy_dexterity[enemy_index] > 0)
   {
      chance -= g_lava_battle_enemy_dexterity[enemy_index] / 12;
   }
   if (chance < 5)
   {
      chance = 5;
   }
   if (chance > 40)
   {
      chance = 40;
   }

   return RandomLong(0, 99) < chance;
}

int PAL_LavaFightEnemyDamage(int enemy_index, int player_role, int alive_enemy, int turn, int defending)
{
   int defense;
   int enemy_attack;
   int enemy_damage;
   int enemy_dexterity;

   enemy_attack = g_lava_battle_enemy_attack_strength[enemy_index];
   enemy_dexterity = g_lava_battle_enemy_dexterity[enemy_index];
   defense = PAL_LavaRoleWordByArray(19, player_role);
   enemy_damage = enemy_attack / 2 + enemy_dexterity / 8 + alive_enemy + turn + 2 - defense / 4;
   if (enemy_damage < 1)
   {
      enemy_damage = 1;
   }
   if (defending)
   {
      enemy_damage /= 2;
      if (enemy_damage < 1)
      {
         enemy_damage = 1;
      }
   }
   if (PAL_LavaBattleGetPlayerStatus(player_role, LAVA_BATTLE_STATUS_PROTECT) > 0)
   {
      enemy_damage /= 2;
      if (enemy_damage < 1)
      {
         enemy_damage = 1;
      }
   }
   return enemy_damage;
}

int PAL_LavaFightEnemyStrongDamage(int enemy_index, int player_role, int alive_enemy, int turn, int defending)
{
   int defense;
   int enemy_damage;
   int enemy_dexterity;
   int magic_strength;

   magic_strength = g_lava_battle_enemy_magic_strength[enemy_index];
   enemy_dexterity = g_lava_battle_enemy_dexterity[enemy_index];
   defense = PAL_LavaRoleWordByArray(19, player_role);
   enemy_damage = magic_strength / 2 + enemy_dexterity / 6 +
      g_lava_battle_enemy_level[enemy_index] + alive_enemy + turn + 4 - defense / 5;
   if (enemy_damage < 1)
   {
      enemy_damage = 1;
   }
   if (defending)
   {
      enemy_damage /= 2;
      if (enemy_damage < 1)
      {
         enemy_damage = 1;
      }
   }
   if (PAL_LavaBattleGetPlayerStatus(player_role, LAVA_BATTLE_STATUS_PROTECT) > 0)
   {
      enemy_damage /= 2;
      if (enemy_damage < 1)
      {
         enemy_damage = 1;
      }
   }
   return enemy_damage;
}

int PAL_LavaFightEnemyMagicDamage(int enemy_index, int player_role, int magic_object_id, int alive_enemy, int turn, int defending)
{
   int base_damage;
   int defense;
   int enemy_damage;
   int enemy_level;
   int magic_strength;

   base_damage = PAL_LavaFightMagicBaseDamage(magic_object_id);
   magic_strength = g_lava_battle_enemy_magic_strength[enemy_index];
   enemy_level = g_lava_battle_enemy_level[enemy_index];
   defense = PAL_LavaRoleWordByArray(19, player_role);
   enemy_damage = base_damage + magic_strength / 2 + enemy_level + alive_enemy + turn + 4 - defense / 5;
   if (enemy_damage < 1)
   {
      enemy_damage = 1;
   }
   if (defending)
   {
      enemy_damage /= 2;
      if (enemy_damage < 1)
      {
         enemy_damage = 1;
      }
   }
   if (PAL_LavaBattleGetPlayerStatus(player_role, LAVA_BATTLE_STATUS_PROTECT) > 0)
   {
      enemy_damage /= 2;
      if (enemy_damage < 1)
      {
         enemy_damage = 1;
      }
   }
   return enemy_damage;
}

int PAL_LavaFightEnemyHitPlayer(int enemy_index, int player_role, int turn)
{
   int chance;
   int enemy_dexterity;
   int enemy_level;
   int player_dexterity;
   int player_flee_rate;

   enemy_dexterity = g_lava_battle_enemy_dexterity[enemy_index];
   enemy_level = g_lava_battle_enemy_level[enemy_index];
   player_dexterity = PAL_LavaRoleWordByArray(20, player_role);
   player_flee_rate = PAL_LavaRoleWordByArray(21, player_role);
   chance = 82 + enemy_dexterity / 5 + enemy_level / 2 + turn / 3 -
      player_dexterity / 6 - player_flee_rate / 8;
   if (chance < 45)
   {
      chance = 45;
   }
   if (chance > 96)
   {
      chance = 96;
   }

   return RandomLong(0, 99) < chance;
}

int PAL_LavaFightChooseFirstMagic(int player_role)
{
   return PAL_LavaFightMagicByIndex(player_role, 0);
}

int PAL_LavaFightMagicCount(int player_role)
{
   int i;
   int magic_count;

   magic_count = 0;
   for (i = 0; i < LAVA_PLAYER_MAGIC_COUNT; i++)
   {
      if (PAL_LavaFightReadRoleMagic(player_role, i) > 0)
      {
         magic_count++;
      }
   }

   return magic_count;
}

int PAL_LavaFightMagicByIndex(int player_role, int magic_sel)
{
   int i;
   int magic_count;
   int magic_object_id;

   if (magic_sel < 0)
   {
      return 0;
   }

   magic_count = 0;
   for (i = 0; i < LAVA_PLAYER_MAGIC_COUNT; i++)
   {
      magic_object_id = PAL_LavaFightReadRoleMagic(player_role, i);
      if (magic_object_id <= 0)
      {
         continue;
      }
      if (magic_count == magic_sel)
      {
         return magic_object_id;
      }
      magic_count++;
   }

   return 0;
}

int PAL_LavaFightMagicCost(int magic_object_id)
{
   int magic_num;
   int mp_cost;

   if (magic_object_id <= 0)
   {
      return 0;
   }

   magic_num = PAL_LavaFightResolveMagicIndex(magic_object_id);
   if (magic_num < 0)
   {
      return 0;
   }

   mp_cost = PAL_LavaFightReadMagicField(magic_num, 12);
   if (mp_cost < 0)
   {
      mp_cost = 0;
   }
   return mp_cost;
}

int PAL_LavaFightMagicFlags(int magic_object_id)
{
   if (magic_object_id <= 0)
   {
      return 0;
   }

   return PAL_LavaReadObjectField(magic_object_id, gConfig.fIsWIN95 ? 6 : 5);
}

int PAL_LavaFightMagicType(int magic_object_id)
{
   int magic_num;

   if (magic_object_id <= 0)
   {
      return -1;
   }

   magic_num = PAL_LavaFightResolveMagicIndex(magic_object_id);
   if (magic_num < 0)
   {
      return -1;
   }

   return PAL_LavaFightReadMagicField(magic_num, LAVA_MAGIC_FIELD_TYPE);
}

int PAL_LavaFightMagicUsableToEnemy(int magic_object_id)
{
   return (PAL_LavaFightMagicFlags(magic_object_id) & LAVA_MAGIC_FLAG_USABLE_TO_ENEMY) != 0;
}

int PAL_LavaFightMagicApplyToAll(int magic_object_id)
{
   return (PAL_LavaFightMagicFlags(magic_object_id) & LAVA_MAGIC_FLAG_APPLY_TO_ALL) != 0;
}

static int PAL_LavaFightMagicHitsAllEnemies(int magic_object_id)
{
   int magic_type;

   if (magic_object_id <= 0)
   {
      return FALSE;
   }

   if (PAL_LavaFightMagicApplyToAll(magic_object_id))
   {
      return TRUE;
   }

   magic_type = PAL_LavaFightMagicType(magic_object_id);
   return magic_type == LAVA_MAGIC_TYPE_ATTACK_ALL ||
      magic_type == LAVA_MAGIC_TYPE_ATTACK_WHOLE ||
      magic_type == LAVA_MAGIC_TYPE_ATTACK_FIELD;
}

int PAL_LavaFightMagicNeedsTarget(int magic_object_id)
{
   int magic_type;

   if (magic_object_id <= 0)
   {
      return FALSE;
   }

   if (PAL_LavaFightMagicUsableToEnemy(magic_object_id))
   {
      return !PAL_LavaFightMagicHitsAllEnemies(magic_object_id);
   }

   magic_type = PAL_LavaFightMagicType(magic_object_id);
   return magic_type == 4;
}

int PAL_LavaFightMagicNeedsPartyTarget(int magic_object_id)
{
   return magic_object_id > 0 &&
      !PAL_LavaFightMagicUsableToEnemy(magic_object_id) &&
      PAL_LavaFightMagicType(magic_object_id) == 4;
}

int PAL_LavaFightMagicBaseDamage(int magic_object_id)
{
   int magic_num;
   int base_damage;

   if (magic_object_id <= 0)
   {
      return 0;
   }

   magic_num = PAL_LavaFightResolveMagicIndex(magic_object_id);
   if (magic_num < 0)
   {
      return 0;
   }

   base_damage = PAL_LavaFightReadMagicField(magic_num, LAVA_MAGIC_FIELD_BASE_DAMAGE);
   if (base_damage < 0)
   {
      base_damage = 0;
   }
   return base_damage;
}

int PAL_LavaFightMagicDamage(int player_role, int enemy_index, int magic_object_id, int turn)
{
   int damage;
   int enemy_defense;
   int party_index;
   int level;
   int magic_strength;
   int mp_cost;

   magic_strength = PAL_LavaRoleWordByArray(18, player_role);
   level = PAL_LavaRoleWordByArray(6, player_role);
   mp_cost = PAL_LavaFightMagicCost(magic_object_id);
   enemy_defense = g_lava_battle_enemy_defense[enemy_index];
   damage = magic_strength / 2 + level + mp_cost * 2 + turn + 6 - enemy_defense / 6;
   party_index = PAL_LavaFightFindPartyIndexByRole(player_role);
   if (party_index >= 0 && g_lava_battle_party_trance[party_index] > 0)
   {
      damage += damage / 2 + 3;
   }
   if (damage < 1)
   {
      damage = 1;
   }
   return damage;
}

int PAL_LavaFightMagicHealAmount(int player_role, int magic_object_id, int turn)
{
   int amount;
   int base_damage;
   int level;
   int magic_strength;
   int mp_cost;
   int party_index;

   magic_strength = PAL_LavaRoleWordByArray(18, player_role);
   level = PAL_LavaRoleWordByArray(6, player_role);
   mp_cost = PAL_LavaFightMagicCost(magic_object_id);
   base_damage = PAL_LavaFightMagicBaseDamage(magic_object_id);
   amount = base_damage + magic_strength / 3 + level + mp_cost + turn + 4;
   party_index = PAL_LavaFightFindPartyIndexByRole(player_role);
   if (party_index >= 0 && g_lava_battle_party_trance[party_index] > 0)
   {
      amount += amount / 2 + 2;
   }
   if (amount < 1)
   {
      amount = 1;
   }
   return amount;
}

void PAL_LavaFightConsumeRoleMP(int player_role, int mp_cost)
{
   int current_mp;

   current_mp = PAL_LavaRoleWordByArray(10, player_role);
   current_mp -= mp_cost;
   if (current_mp < 0)
   {
      current_mp = 0;
   }
   PAL_LavaWriteU16((addr)g_lava_data_buf, 10 * 6 * 2 + player_role * 2, current_mp);
}

void PAL_LavaFightWriteRoleHP(int player_role, int hp)
{
   if (hp < 0)
   {
      hp = 0;
   }
   PAL_LavaWriteU16((addr)g_lava_data_buf, 9 * 6 * 2 + player_role * 2, hp);
}

int PAL_LavaFightPlayerCommandNeedsTarget(int command_sel, int enemy_count, int *enemy_hp, int player_role)
{
   int magic_object_id;

   if (command_sel != LAVA_BATTLE_COMMAND_ATTACK &&
       command_sel != LAVA_BATTLE_COMMAND_MAGIC &&
       command_sel != LAVA_BATTLE_COMMAND_COOP_MAGIC)
   {
      return FALSE;
   }

   if (command_sel == LAVA_BATTLE_COMMAND_ATTACK &&
       PAL_LavaRoleWordByArray(4, player_role) != 0)
   {
      return FALSE;
   }
   if (command_sel == LAVA_BATTLE_COMMAND_MAGIC ||
       command_sel == LAVA_BATTLE_COMMAND_COOP_MAGIC)
   {
      magic_object_id = command_sel == LAVA_BATTLE_COMMAND_COOP_MAGIC ?
         PAL_LavaFightReadRoleCoopMagic(player_role) : PAL_LavaFightChooseFirstMagic(player_role);
      if (magic_object_id <= 0)
      {
         return FALSE;
      }
      return PAL_LavaFightCountAlive(enemy_hp, enemy_count) > 1 &&
         PAL_LavaFightMagicNeedsTarget(magic_object_id);
   }

   return PAL_LavaFightCountAlive(enemy_hp, enemy_count) > 1;
}

int PAL_LavaFightPlayerCommandDefends(int command_sel)
{
   return command_sel == LAVA_BATTLE_COMMAND_DEFEND;
}

int PAL_LavaFightTryFlee(int player_role, int target_enemy, int turn, int enemy_count)
{
   int enemy_flee_rate;
   int flee_rate;
   int player_flee_rate;

   if (g_lava_autotest_load)
   {
      return TRUE;
   }

   player_flee_rate = PAL_LavaRoleWordByArray(21, player_role);
   enemy_flee_rate = g_lava_battle_enemy_flee_rate[target_enemy];
   flee_rate = 40 + player_flee_rate / 2 + turn * 4 - enemy_count * 5 - enemy_flee_rate / 3;
   if (flee_rate < 10)
   {
      flee_rate = 10;
   }
   if (flee_rate > 95)
   {
      flee_rate = 95;
   }

   return RandomLong(0, 99) < flee_rate;
}

static int PAL_LavaFightCanUseCoopMagic(int *party_hp)
{
   int i;
   int max_hp;
   int role;

   if (party_hp == 0 || g_lava_party_count <= 1)
   {
      return FALSE;
   }

   for (i = 0; i < g_lava_party_count && i < 3; i++)
   {
      role = g_lava_party_role[i];
      max_hp = PAL_LavaRoleWordByArray(7, role);
      if (party_hp[i] <= 0 ||
          party_hp[i] < max_hp / 5 ||
          PAL_LavaBattleGetPlayerStatus(role, LAVA_BATTLE_STATUS_SLEEP) > 0 ||
          PAL_LavaBattleGetPlayerStatus(role, LAVA_BATTLE_STATUS_CONFUSED) > 0 ||
          PAL_LavaBattleGetPlayerStatus(role, LAVA_BATTLE_STATUS_SILENCE) > 0)
      {
         return FALSE;
      }
   }

   return TRUE;
}

int PAL_LavaFightCommitPlayerCommand(
   int command_sel,
   int *target_sel,
   int enemy_count,
   int *enemy_hp,
   int *party_hp,
   int *party_hp_max,
   int turn,
   char *message,
   int *round_result
)
{
   int acting_player;
   int any_ko;
   int damage;
   int hit_count;
   int i;
   int item_after_party_hp[3];
   int item_before_party_hp[3];
   int item_flags;
   char *item_name;
   int item_object_id;
   int item_recovered_total;
   long item_script_on_throw;
   long item_script_on_use;
   int item_target_role;
   int magic_object_id;
   char *magic_name;
   int max_hp;
   int mp_cost;
   int player_role;
   int heal_amount;
   int magic_type;
   int recovered;
   int strong_attack;

   acting_player = g_lava_battle_acting_player_index;
   if (acting_player < 0 || acting_player >= 3)
   {
      acting_player = PAL_LavaFightPickTurnActor(party_hp, turn);
   }
   player_role = g_lava_party_role[acting_player];
   g_lava_battle_round_player_trance_boost = g_lava_battle_party_trance[acting_player] > 0;
   PAL_LavaFightClearPlayerHitList(round_result);

   if (PAL_LavaBattleGetPlayerStatus(player_role, LAVA_BATTLE_STATUS_SLEEP) > 0)
   {
      sprintf(message, "沉睡未醒");
      PAL_LavaFightSetRoundEvent(TRUE, LAVA_BATTLE_EVENT_PLAYER_SLEEP, 0, 0);
      return LAVA_BATTLE_ACTION_COMMIT;
   }

   if (command_sel == LAVA_BATTLE_COMMAND_FLEE)
   {
      if (g_lava_autotest_battle && g_lava_autotest_battle_magic_seen >= 3)
      {
         sprintf(message, "战斗烟测结束");
         PAL_LavaFightSetRoundEvent(TRUE, LAVA_BATTLE_EVENT_PLAYER_FLEE_OK, 0, 0);
         return LAVA_BATTLE_ACTION_FLEE_SUCCESS;
      }
      if (PAL_LavaFightTryFlee(player_role, PAL_LavaFightFixEnemyTarget(*target_sel, enemy_count, enemy_hp), turn, enemy_count))
      {
         sprintf(message, "撤退成功");
         PAL_LavaFightSetRoundEvent(TRUE, LAVA_BATTLE_EVENT_PLAYER_FLEE_OK, 0, 0);
         return LAVA_BATTLE_ACTION_FLEE_SUCCESS;
      }

      sprintf(message, "撤退失败");
      PAL_LavaFightSetRoundEvent(TRUE, LAVA_BATTLE_EVENT_PLAYER_FLEE_FAIL, 0, 0);
      return LAVA_BATTLE_ACTION_COMMIT;
   }

   if (command_sel == LAVA_BATTLE_COMMAND_DEFEND)
   {
      if (acting_player >= 0 && acting_player < 3)
      {
         g_lava_battle_party_defending[acting_player] = 1;
      }
      sprintf(message, "摆好架势");
      PAL_LavaFightSetRoundEvent(TRUE, LAVA_BATTLE_EVENT_PLAYER_DEFEND, 0, 0);
      return LAVA_BATTLE_ACTION_COMMIT;
   }

   if (command_sel == LAVA_BATTLE_COMMAND_USE_ITEM)
   {
      item_object_id = g_lava_battle_selected_item_object_id;
      if (item_object_id <= 0 || PAL_LavaGetItemAmount(item_object_id) <= 0)
      {
         sprintf(message, "暂无可用物品");
         PAL_LavaFightSetRoundEvent(TRUE, LAVA_BATTLE_EVENT_PLAYER_ITEM_EMPTY, 0, 0);
         return LAVA_BATTLE_ACTION_COMMIT;
      }

      item_flags = PAL_LavaFightReadItemFlags(item_object_id);
      item_script_on_use = PAL_LavaReadObjectField(item_object_id, 2);
      if ((item_flags & LAVA_ITEM_FLAG_USABLE) == 0 || item_script_on_use <= 0)
      {
         sprintf(message, "无法使用物品");
         PAL_LavaFightSetRoundEvent(TRUE, LAVA_BATTLE_EVENT_PLAYER_ITEM_EMPTY, item_object_id, 0);
         return LAVA_BATTLE_ACTION_COMMIT;
      }

      item_before_party_hp[0] = party_hp[0];
      item_before_party_hp[1] = party_hp[1];
      item_before_party_hp[2] = party_hp[2];
      item_target_role = 0xFFFF;
       if ((item_flags & LAVA_ITEM_FLAG_APPLY_TO_ALL) == 0)
       {
          *target_sel = PAL_LavaFightFixLivingPartyTarget(*target_sel, party_hp, party_hp_max);
          item_target_role = g_lava_party_role[*target_sel];
       }
      if (round_result != 0)
      {
         round_result[LAVA_BATTLE_ROUND_PLAYER_TARGET] =
            (item_flags & LAVA_ITEM_FLAG_APPLY_TO_ALL) ? -1 : *target_sel;
      }

       if (item_target_role == 0xFFFF)
       {
         PAL_LavaRunTriggerScript((long)item_script_on_use, item_target_role);
      }
      else
      {
         PAL_LavaRunRoleTriggerScript((long)item_script_on_use, item_target_role);
      }
      PAL_LavaFinishScriptStep();
      PAL_LavaFightSyncPartyStateFromRoles(party_hp, party_hp_max);
      item_after_party_hp[0] = party_hp[0];
      item_after_party_hp[1] = party_hp[1];
      item_after_party_hp[2] = party_hp[2];

      if ((item_flags & LAVA_ITEM_FLAG_CONSUMING) && g_lava_script_success)
      {
         PAL_LavaAddItemToInventory(item_object_id, -1);
      }

      item_recovered_total = 0;
      for (i = 0; i < g_lava_party_count && i < 3; i++)
      {
         if (item_after_party_hp[i] > item_before_party_hp[i])
         {
            item_recovered_total += item_after_party_hp[i] - item_before_party_hp[i];
         }
      }
      if (item_recovered_total > 0)
      {
         PAL_LavaFightStorePlayerHit(round_result, 0,
            (item_flags & LAVA_ITEM_FLAG_APPLY_TO_ALL) ? 0 : *target_sel,
            item_recovered_total);
      }
      item_name = PAL_LavaReadWord(item_object_id);
      if (item_name == 0 || item_name[0] == 0)
      {
         item_name = "物品";
      }
      sprintf(message, "使用%s", item_name);
      PAL_LavaFightSetRoundEvent(TRUE, LAVA_BATTLE_EVENT_PLAYER_ITEM_USE,
         item_object_id, item_recovered_total);
      return LAVA_BATTLE_ACTION_COMMIT;
   }

   if (command_sel == LAVA_BATTLE_COMMAND_THROW_ITEM)
   {
      item_object_id = g_lava_battle_selected_item_object_id;
      if (item_object_id <= 0 || PAL_LavaGetItemAmount(item_object_id) <= 0)
      {
         sprintf(message, "暂无可投掷物品");
         PAL_LavaFightSetRoundEvent(TRUE, LAVA_BATTLE_EVENT_PLAYER_ITEM_THROW_EMPTY, 0, 0);
         return LAVA_BATTLE_ACTION_COMMIT;
      }

      item_flags = PAL_LavaFightReadItemFlags(item_object_id);
      item_script_on_throw = PAL_LavaFightReadObjectU16Field(item_object_id, 4);
      if ((item_flags & LAVA_ITEM_FLAG_THROWABLE) == 0 || item_script_on_throw <= 0)
      {
         sprintf(message, "无法投掷物品");
         PAL_LavaFightSetRoundEvent(TRUE, LAVA_BATTLE_EVENT_PLAYER_ITEM_THROW_EMPTY, item_object_id, 0);
         return LAVA_BATTLE_ACTION_COMMIT;
      }

      if ((item_flags & LAVA_ITEM_FLAG_APPLY_TO_ALL) == 0)
      {
         *target_sel = PAL_LavaFightFixEnemyTarget(*target_sel, enemy_count, enemy_hp);
      }
       else
       {
          *target_sel = -1;
       }
      if (round_result != 0)
      {
         round_result[LAVA_BATTLE_ROUND_PLAYER_TARGET] = *target_sel;
      }

       PAL_LavaRunTriggerScript(item_script_on_throw, *target_sel < 0 ? 0xFFFF : *target_sel);
      PAL_LavaFinishScriptStep();
      PAL_LavaAddItemToInventory(item_object_id, -1);

      item_name = PAL_LavaReadWord(item_object_id);
      if (item_name == 0 || item_name[0] == 0)
      {
         item_name = "物品";
      }
      sprintf(message, "投掷%s", item_name);
      PAL_LavaFightSetRoundEvent(TRUE, LAVA_BATTLE_EVENT_PLAYER_ITEM_THROW, item_object_id, 0);
      return LAVA_BATTLE_ACTION_COMMIT;
   }

   if (command_sel == LAVA_BATTLE_COMMAND_COOP_MAGIC)
   {
      any_ko = FALSE;
      if (!PAL_LavaFightCanUseCoopMagic(party_hp))
      {
         sprintf(message, "当前无法协力");
         PAL_LavaFightSetRoundEvent(TRUE, LAVA_BATTLE_EVENT_PLAYER_MAGIC_EMPTY, 0, 0);
         return LAVA_BATTLE_ACTION_COMMIT;
      }
      magic_object_id = PAL_LavaFightReadRoleCoopMagic(player_role);
      if (magic_object_id <= 0)
      {
         sprintf(message, "尚无协力仙术");
         PAL_LavaFightSetRoundEvent(TRUE, LAVA_BATTLE_EVENT_PLAYER_MAGIC_EMPTY, 0, 0);
         return LAVA_BATTLE_ACTION_COMMIT;
      }
      if (!PAL_LavaFightMagicUsableToEnemy(magic_object_id))
      {
         int coop_after_party_mp[3];
         int coop_before_party_hp[3];
         int coop_before_party_mp[3];
         int coop_before_party_status[3 * LAVA_BATTLE_STATUS_COUNT];
         int coop_helper_hit_count;
         int coop_helper_mp_hit_count;
         int coop_helper_script_success;
         int coop_helper_status_code;
         int coop_affected_roles;
         int coop_after_status;
         int coop_before_one;
         int coop_first_delta;
         int coop_first_status;
         int coop_j;
         int coop_role;
         int coop_role_changed;

         magic_name = PAL_LavaReadWord(magic_object_id);
         if (magic_name == 0 || magic_name[0] == 0)
         {
            magic_name = "协力仙术";
         }

         coop_before_party_hp[0] = party_hp[0];
         coop_before_party_hp[1] = party_hp[1];
         coop_before_party_hp[2] = party_hp[2];
         PAL_LavaFightReadPartyMPFromRoles(coop_before_party_mp);
         for (i = 0; i < 3 * LAVA_BATTLE_STATUS_COUNT; i++)
         {
            coop_before_party_status[i] = 0;
         }
         for (i = 0; i < g_lava_party_count && i < 3; i++)
         {
            coop_role = g_lava_party_role[i];
            for (coop_j = 0; coop_j < LAVA_BATTLE_STATUS_COUNT; coop_j++)
            {
               coop_before_party_status[i * LAVA_BATTLE_STATUS_COUNT + coop_j] =
                  PAL_LavaBattleGetPlayerStatus(coop_role, coop_j);
            }
         }

         magic_type = PAL_LavaFightMagicType(magic_object_id);
         if (magic_type == 5)
         {
            hit_count = 0;
            heal_amount = PAL_LavaFightMagicHealAmount(player_role, magic_object_id, turn);
            for (i = 0; i < g_lava_party_count && i < 3; i++)
            {
               max_hp = PAL_LavaRoleWordByArray(7, g_lava_party_role[i]);
               if (max_hp <= 0 || party_hp[i] <= 0)
               {
                  continue;
               }
               recovered = heal_amount;
               if (party_hp[i] + recovered > max_hp)
               {
                  recovered = max_hp - party_hp[i];
               }
               if (recovered < 0)
               {
                  recovered = 0;
               }
               party_hp[i] += heal_amount;
               if (party_hp[i] > max_hp)
               {
                  party_hp[i] = max_hp;
               }
               PAL_LavaFightWriteRoleHP(g_lava_party_role[i], party_hp[i]);
               if (recovered > 0)
               {
                  PAL_LavaFightStorePlayerHit(round_result, hit_count, i, recovered);
                  hit_count++;
               }
            }
            sprintf(message, "%s协力回复全体", magic_name);
            PAL_LavaFightSetRoundEvent(TRUE, LAVA_BATTLE_EVENT_PLAYER_COOP_HEAL_ALL, magic_object_id, heal_amount);
            return LAVA_BATTLE_ACTION_COMMIT;
         }
         else if (magic_type == 4)
         {
            *target_sel = PAL_LavaFightFixLivingPartyTarget(*target_sel, party_hp, 0);
            if (party_hp[*target_sel] <= 0)
            {
               sprintf(message, "无人可疗");
               PAL_LavaFightSetRoundEvent(TRUE, LAVA_BATTLE_EVENT_PLAYER_MAGIC_NO_TARGET, magic_object_id, 0);
               return LAVA_BATTLE_ACTION_COMMIT;
            }
            heal_amount = PAL_LavaFightMagicHealAmount(player_role, magic_object_id, turn);
            max_hp = PAL_LavaRoleWordByArray(7, g_lava_party_role[*target_sel]);
            recovered = heal_amount;
            if (party_hp[*target_sel] + recovered > max_hp)
            {
               recovered = max_hp - party_hp[*target_sel];
            }
            if (recovered < 0)
            {
               recovered = 0;
            }
            party_hp[*target_sel] += heal_amount;
            if (party_hp[*target_sel] > max_hp)
            {
               party_hp[*target_sel] = max_hp;
            }
            PAL_LavaFightWriteRoleHP(g_lava_party_role[*target_sel], party_hp[*target_sel]);
            if (recovered > 0)
            {
               PAL_LavaFightStorePlayerHit(round_result, 0, *target_sel, recovered);
            }
            sprintf(message, "%s协力回复-%d", magic_name, recovered);
            PAL_LavaFightSetRoundEvent(TRUE, LAVA_BATTLE_EVENT_PLAYER_COOP_HEAL, magic_object_id, recovered);
            return LAVA_BATTLE_ACTION_COMMIT;
         }
         else if (magic_type == 8)
         {
            coop_helper_script_success = PAL_LavaFightRunSupportMagicScripts(magic_object_id, player_role,
               acting_player, magic_type, *target_sel, party_hp, party_hp_max);
            if (!coop_helper_script_success)
            {
               sprintf(message, "%s施展失败", magic_name);
               PAL_LavaFightSetRoundEvent(TRUE, LAVA_BATTLE_EVENT_PLAYER_MAGIC_NO_TARGET, magic_object_id, 0);
               return LAVA_BATTLE_ACTION_COMMIT;
            }
            for (i = 0; i < g_lava_party_count && i < 3; i++)
            {
               if (party_hp[i] > 0)
               {
                  PAL_LavaFightSetPartyTrance(i, 3);
               }
            }
            sprintf(message, "%s协力令灵力高涨", magic_name);
            PAL_LavaFightSetRoundEvent(TRUE, LAVA_BATTLE_EVENT_PLAYER_COOP_TRANCE, magic_object_id, 3);
            return LAVA_BATTLE_ACTION_COMMIT;
         }

         coop_helper_script_success = PAL_LavaFightRunSupportMagicScripts(magic_object_id, player_role,
            acting_player, magic_type, *target_sel, party_hp, party_hp_max);
         coop_helper_hit_count = PAL_LavaFightDetectSupportMagicRecovery(
            coop_before_party_hp, party_hp, round_result);
         coop_helper_mp_hit_count = 0;
         coop_helper_status_code = 0;
         if (coop_helper_hit_count <= 0)
         {
            PAL_LavaFightReadPartyMPFromRoles(coop_after_party_mp);
            for (i = 0; i < g_lava_party_count && i < 3; i++)
            {
               recovered = coop_after_party_mp[i] - coop_before_party_mp[i];
               if (recovered <= 0)
               {
                  continue;
               }
               PAL_LavaFightStorePlayerHit(round_result, coop_helper_mp_hit_count, i, recovered);
               coop_helper_mp_hit_count++;
            }
            if (coop_helper_mp_hit_count <= 0)
            {
               coop_affected_roles = 0;
               coop_first_status = -1;
               coop_first_delta = 0;
               for (i = 0; i < g_lava_party_count && i < 3; i++)
               {
                  coop_role = g_lava_party_role[i];
                  coop_role_changed = FALSE;
                  for (coop_j = 0; coop_j < LAVA_BATTLE_STATUS_COUNT; coop_j++)
                  {
                     coop_before_one = coop_before_party_status[i * LAVA_BATTLE_STATUS_COUNT + coop_j];
                     coop_after_status = PAL_LavaBattleGetPlayerStatus(coop_role, coop_j);
                     if (coop_after_status == coop_before_one)
                     {
                        continue;
                     }
                     if (!coop_role_changed)
                     {
                        coop_affected_roles++;
                        coop_role_changed = TRUE;
                     }
                     if (coop_first_status < 0)
                     {
                        coop_first_status = coop_j;
                        coop_first_delta = coop_after_status - coop_before_one;
                     }
                     else if (coop_first_status != coop_j ||
                              (coop_after_status - coop_before_one) != coop_first_delta)
                     {
                        coop_helper_status_code = LAVA_BATTLE_HELPER_STATUS_MULTI;
                     }
                  }
                  if (coop_helper_status_code == LAVA_BATTLE_HELPER_STATUS_MULTI)
                  {
                     break;
                  }
               }
               if (coop_helper_status_code != LAVA_BATTLE_HELPER_STATUS_MULTI && coop_first_status >= 0)
               {
                  if (coop_affected_roles > 1)
                  {
                     coop_helper_status_code = LAVA_BATTLE_HELPER_STATUS_MULTI;
                  }
                  else if (coop_first_delta < 0)
                  {
                     coop_helper_status_code = -(coop_first_status + 1);
                  }
                  else
                  {
                     coop_helper_status_code = coop_first_status + 1;
                  }
               }
            }
         }
         if (coop_helper_hit_count > 1)
         {
            sprintf(message, "%s协力回复全体", magic_name);
            PAL_LavaFightSetRoundEvent(TRUE,
               LAVA_BATTLE_EVENT_PLAYER_COOP_HEAL_ALL, magic_object_id, coop_helper_hit_count);
         }
         else if (coop_helper_hit_count == 1)
         {
            sprintf(message, "%s协力回复-%d", magic_name, round_result[LAVA_BATTLE_ROUND_PLAYER_DAMAGE]);
            PAL_LavaFightSetRoundEvent(TRUE,
               LAVA_BATTLE_EVENT_PLAYER_COOP_HEAL, magic_object_id,
               round_result[LAVA_BATTLE_ROUND_PLAYER_DAMAGE]);
         }
         else if (coop_helper_mp_hit_count > 1)
         {
            sprintf(message, "%s协力回复真气全体", magic_name);
            PAL_LavaFightSetRoundEvent(TRUE,
               LAVA_BATTLE_EVENT_PLAYER_COOP_MP_ALL, magic_object_id, coop_helper_mp_hit_count);
         }
         else if (coop_helper_mp_hit_count == 1)
         {
            sprintf(message, "%s协力回复真气-%d", magic_name, round_result[LAVA_BATTLE_ROUND_PLAYER_DAMAGE]);
            PAL_LavaFightSetRoundEvent(TRUE,
               LAVA_BATTLE_EVENT_PLAYER_COOP_MP, magic_object_id,
               round_result[LAVA_BATTLE_ROUND_PLAYER_DAMAGE]);
         }
         else if (coop_helper_status_code == LAVA_BATTLE_HELPER_STATUS_MULTI)
         {
            sprintf(message, "%s协力调整全体状态", magic_name);
            PAL_LavaFightSetRoundEvent(TRUE, LAVA_BATTLE_EVENT_PLAYER_COOP_HELPER,
               magic_object_id, coop_helper_status_code);
         }
         else if (coop_helper_status_code > 0)
         {
            sprintf(message, "%s协力加持%s", magic_name,
               PAL_LavaBattleStatusName(coop_helper_status_code - 1));
            PAL_LavaFightSetRoundEvent(TRUE, LAVA_BATTLE_EVENT_PLAYER_COOP_HELPER,
               magic_object_id, coop_helper_status_code);
         }
         else if (coop_helper_status_code < 0)
         {
            sprintf(message, "%s协力解除%s", magic_name,
               PAL_LavaBattleStatusName((-coop_helper_status_code) - 1));
            PAL_LavaFightSetRoundEvent(TRUE, LAVA_BATTLE_EVENT_PLAYER_COOP_HELPER,
               magic_object_id, coop_helper_status_code);
         }
         else
         {
            sprintf(message, coop_helper_script_success ? "%s协力施展完毕" : "%s协力未起效", magic_name);
            PAL_LavaFightSetRoundEvent(TRUE, LAVA_BATTLE_EVENT_PLAYER_COOP_HELPER, magic_object_id, 0);
         }
         return LAVA_BATTLE_ACTION_COMMIT;
      }

      magic_name = PAL_LavaReadWord(magic_object_id);
      if (magic_name == 0 || magic_name[0] == 0)
      {
         magic_name = "协力仙术";
      }
      if (PAL_LavaFightMagicHitsAllEnemies(magic_object_id))
      {
         hit_count = 0;
         for (i = 0; i < enemy_count && hit_count < LAVA_BATTLE_MAX_ENEMIES; i++)
         {
            if (enemy_hp[i] <= 0)
            {
               continue;
            }
            damage = PAL_LavaFightMagicDamage(player_role, i, magic_object_id, turn) * 3 / 2;
            enemy_hp[i] -= damage;
            if (enemy_hp[i] < 0)
            {
               enemy_hp[i] = 0;
            }
            if (enemy_hp[i] <= 0)
            {
               any_ko = TRUE;
            }
            PAL_LavaFightStorePlayerHit(round_result, hit_count, i, damage);
            if (hit_count == 0)
            {
               *target_sel = i;
            }
            hit_count++;
         }
         sprintf(message, any_ko ? "%s协力破敌" : "%s协力发动", magic_name);
         PAL_LavaFightSetRoundEvent(TRUE,
            any_ko ? LAVA_BATTLE_EVENT_PLAYER_COOP_ALL_KO : LAVA_BATTLE_EVENT_PLAYER_COOP_ALL,
            magic_object_id, hit_count);
         return LAVA_BATTLE_ACTION_COMMIT;
      }

      *target_sel = PAL_LavaFightFixEnemyTarget(*target_sel, enemy_count, enemy_hp);
      damage = PAL_LavaFightMagicDamage(player_role, *target_sel, magic_object_id, turn) * 3 / 2;
      enemy_hp[*target_sel] -= damage;
      if (enemy_hp[*target_sel] < 0)
      {
         enemy_hp[*target_sel] = 0;
      }
      PAL_LavaFightStorePlayerHit(round_result, 0, *target_sel, damage);
      if (enemy_hp[*target_sel] <= 0)
      {
         sprintf(message, "%s协力命中-%d，敌人倒下", magic_name, damage);
         PAL_LavaFightSetRoundEvent(TRUE, LAVA_BATTLE_EVENT_PLAYER_COOP_HIT_KO, magic_object_id, damage);
      }
      else
      {
         sprintf(message, "%s协力命中-%d", magic_name, damage);
         PAL_LavaFightSetRoundEvent(TRUE, LAVA_BATTLE_EVENT_PLAYER_COOP_HIT, magic_object_id, damage);
      }
      return LAVA_BATTLE_ACTION_COMMIT;
   }

   if (command_sel == LAVA_BATTLE_COMMAND_MAGIC)
   {
      any_ko = FALSE;
      magic_object_id = g_lava_battle_selected_magic_object_id;
      if (g_lava_autotest_fengshen)
      {
         printf("[LAVA][FENGSHEN] commit-player actor=%d role=%d cmd=%d selected=%d state_target=%d turn=%d\n",
            acting_player, player_role, command_sel, magic_object_id, *target_sel, turn);
      }
      if (magic_object_id <= 0)
      {
         magic_object_id = PAL_LavaFightChooseFirstMagic(player_role);
      }
      if (magic_object_id <= 0)
      {
         sprintf(message, "尚未习得仙术");
         PAL_LavaFightSetRoundEvent(TRUE, LAVA_BATTLE_EVENT_PLAYER_MAGIC_EMPTY, 0, 0);
         return LAVA_BATTLE_ACTION_COMMIT;
      }

      mp_cost = PAL_LavaFightMagicCost(magic_object_id);
      if (g_lava_autotest_fengshen)
      {
         printf("[LAVA][FENGSHEN] commit-magic obj=%d mp_cost=%d role_mp=%d type=%d flags=%d all=%d\n",
            magic_object_id, mp_cost, PAL_LavaRoleWordByArray(10, player_role),
            PAL_LavaFightMagicType(magic_object_id), PAL_LavaFightMagicFlags(magic_object_id),
            PAL_LavaFightMagicHitsAllEnemies(magic_object_id));
      }
      if (PAL_LavaBattleGetPlayerStatus(player_role, LAVA_BATTLE_STATUS_SILENCE) > 0)
      {
         sprintf(message, "封印中无法施术");
         PAL_LavaFightSetRoundEvent(TRUE, LAVA_BATTLE_EVENT_PLAYER_MAGIC_SILENCE, magic_object_id, 0);
         return LAVA_BATTLE_ACTION_COMMIT;
      }
      if (PAL_LavaRoleWordByArray(10, player_role) < mp_cost)
      {
         sprintf(message, "真气不足");
         PAL_LavaFightSetRoundEvent(TRUE, LAVA_BATTLE_EVENT_PLAYER_MAGIC_NOMP, magic_object_id, mp_cost);
         return LAVA_BATTLE_ACTION_COMMIT;
      }

      magic_name = PAL_LavaReadWord(magic_object_id);
      if (magic_name == 0 || magic_name[0] == 0)
      {
         magic_name = "仙术";
      }
      if (!PAL_LavaFightMagicUsableToEnemy(magic_object_id))
      {
         int after_party_mp[3];
         int before_party_hp[3];
         int before_party_mp[3];
         int before_party_status[3 * LAVA_BATTLE_STATUS_COUNT];
         int helper_hit_count;
         int helper_mp_hit_count;
         int helper_script_success;
         int helper_status_code;
         int affected_roles;
         int after_status;
         int before_one;
         int first_delta;
         int first_status;
         int j;
         int role;
         int role_changed;

         before_party_hp[0] = party_hp[0];
         before_party_hp[1] = party_hp[1];
         before_party_hp[2] = party_hp[2];
         PAL_LavaFightReadPartyMPFromRoles(before_party_mp);
         for (i = 0; i < 3 * LAVA_BATTLE_STATUS_COUNT; i++)
         {
            before_party_status[i] = 0;
         }
         for (i = 0; i < g_lava_party_count && i < 3; i++)
         {
            role = g_lava_party_role[i];
            for (j = 0; j < LAVA_BATTLE_STATUS_COUNT; j++)
            {
               before_party_status[i * LAVA_BATTLE_STATUS_COUNT + j] =
                  PAL_LavaBattleGetPlayerStatus(role, j);
            }
         }
         magic_type = PAL_LavaFightMagicType(magic_object_id);
         if (magic_type == 5)
         {
            hit_count = 0;
            heal_amount = PAL_LavaFightMagicHealAmount(player_role, magic_object_id, turn);
            PAL_LavaFightConsumeRoleMP(player_role, mp_cost);
            for (i = 0; i < g_lava_party_count && i < 3; i++)
            {
               max_hp = PAL_LavaRoleWordByArray(7, g_lava_party_role[i]);
               if (max_hp <= 0 || party_hp[i] <= 0)
               {
                  continue;
               }
               recovered = heal_amount;
               if (party_hp[i] + recovered > max_hp)
               {
                  recovered = max_hp - party_hp[i];
               }
               if (recovered < 0)
               {
                  recovered = 0;
               }
               party_hp[i] += heal_amount;
               if (party_hp[i] > max_hp)
               {
                  party_hp[i] = max_hp;
               }
               PAL_LavaFightWriteRoleHP(g_lava_party_role[i], party_hp[i]);
               if (recovered > 0)
               {
                  PAL_LavaFightStorePlayerHit(round_result, hit_count, i, recovered);
                  hit_count++;
               }
            }
            sprintf(message, "%s回复全体", magic_name);
            PAL_LavaFightSetRoundEvent(TRUE, LAVA_BATTLE_EVENT_PLAYER_MAGIC_HEAL_ALL, magic_object_id, heal_amount);
            PAL_LavaFightConsumePartyTrance(acting_player);
            return LAVA_BATTLE_ACTION_COMMIT;
         }
         else if (magic_type == 4)
         {
            *target_sel = PAL_LavaFightFixLivingPartyTarget(*target_sel, party_hp, 0);
            if (party_hp[*target_sel] <= 0)
            {
               sprintf(message, "无人可疗");
               PAL_LavaFightSetRoundEvent(TRUE, LAVA_BATTLE_EVENT_PLAYER_MAGIC_NO_TARGET, magic_object_id, 0);
               return LAVA_BATTLE_ACTION_COMMIT;
            }
            heal_amount = PAL_LavaFightMagicHealAmount(player_role, magic_object_id, turn);
            PAL_LavaFightConsumeRoleMP(player_role, mp_cost);
            max_hp = PAL_LavaRoleWordByArray(7, g_lava_party_role[*target_sel]);
            recovered = heal_amount;
            if (party_hp[*target_sel] + recovered > max_hp)
            {
               recovered = max_hp - party_hp[*target_sel];
            }
            if (recovered < 0)
            {
               recovered = 0;
            }
            party_hp[*target_sel] += heal_amount;
            if (party_hp[*target_sel] > max_hp)
            {
               party_hp[*target_sel] = max_hp;
            }
            PAL_LavaFightWriteRoleHP(g_lava_party_role[*target_sel], party_hp[*target_sel]);
            if (recovered > 0)
            {
               PAL_LavaFightStorePlayerHit(round_result, 0, *target_sel, recovered);
            }
            sprintf(message, "%s回复-%d", magic_name, recovered);
            PAL_LavaFightSetRoundEvent(TRUE, LAVA_BATTLE_EVENT_PLAYER_MAGIC_HEAL, magic_object_id, recovered);
            PAL_LavaFightConsumePartyTrance(acting_player);
            return LAVA_BATTLE_ACTION_COMMIT;
         }
         else if (magic_type == 8)
         {
            PAL_LavaFightConsumeRoleMP(player_role, mp_cost);
            helper_script_success = PAL_LavaFightRunSupportMagicScripts(magic_object_id, player_role,
               acting_player, magic_type, *target_sel, party_hp, party_hp_max);
            if (!helper_script_success)
            {
               sprintf(message, "%s施展失败", magic_name);
               PAL_LavaFightSetRoundEvent(TRUE, LAVA_BATTLE_EVENT_PLAYER_MAGIC_NO_TARGET, magic_object_id, 0);
               return LAVA_BATTLE_ACTION_COMMIT;
            }
            PAL_LavaFightSetPartyTrance(acting_player, 3);
            sprintf(message, "%s令灵力高涨", magic_name);
            PAL_LavaFightSetRoundEvent(TRUE, LAVA_BATTLE_EVENT_PLAYER_MAGIC_TRANCE, magic_object_id, 3);
            return LAVA_BATTLE_ACTION_COMMIT;
         }

         PAL_LavaFightConsumeRoleMP(player_role, mp_cost);
         helper_script_success = PAL_LavaFightRunSupportMagicScripts(magic_object_id, player_role,
            acting_player, magic_type, *target_sel, party_hp, party_hp_max);
         helper_hit_count = PAL_LavaFightDetectSupportMagicRecovery(
            before_party_hp, party_hp, round_result);
         helper_mp_hit_count = 0;
         helper_status_code = 0;
         if (helper_hit_count <= 0)
         {
            PAL_LavaFightReadPartyMPFromRoles(after_party_mp);
            for (i = 0; i < g_lava_party_count && i < 3; i++)
            {
               recovered = after_party_mp[i] - before_party_mp[i];
               if (recovered <= 0)
               {
                  continue;
               }
               PAL_LavaFightStorePlayerHit(round_result, helper_mp_hit_count, i, recovered);
               helper_mp_hit_count++;
            }
            if (helper_mp_hit_count <= 0)
            {
               affected_roles = 0;
               first_status = -1;
               first_delta = 0;
               for (i = 0; i < g_lava_party_count && i < 3; i++)
               {
                  role = g_lava_party_role[i];
                  role_changed = FALSE;
                  for (j = 0; j < LAVA_BATTLE_STATUS_COUNT; j++)
                  {
                     before_one = before_party_status[i * LAVA_BATTLE_STATUS_COUNT + j];
                     after_status = PAL_LavaBattleGetPlayerStatus(role, j);
                     if (after_status == before_one)
                     {
                        continue;
                     }
                     if (!role_changed)
                     {
                        affected_roles++;
                        role_changed = TRUE;
                     }
                     if (first_status < 0)
                     {
                        first_status = j;
                        first_delta = after_status - before_one;
                     }
                     else if (first_status != j || (after_status - before_one) != first_delta)
                     {
                        helper_status_code = LAVA_BATTLE_HELPER_STATUS_MULTI;
                     }
                  }
                  if (helper_status_code == LAVA_BATTLE_HELPER_STATUS_MULTI)
                  {
                     break;
                  }
               }
               if (helper_status_code != LAVA_BATTLE_HELPER_STATUS_MULTI)
               {
                  if (first_status >= 0)
                  {
                     if (affected_roles > 1)
                     {
                        helper_status_code = LAVA_BATTLE_HELPER_STATUS_MULTI;
                     }
                     else if (first_delta < 0)
                     {
                        helper_status_code = -(first_status + 1);
                     }
                     else
                     {
                        helper_status_code = first_status + 1;
                     }
                  }
               }
            }
         }
         if (helper_hit_count > 1)
         {
            sprintf(message, "%s回复全体", magic_name);
            PAL_LavaFightSetRoundEvent(TRUE,
               LAVA_BATTLE_EVENT_PLAYER_MAGIC_HEAL_ALL, magic_object_id, helper_hit_count);
         }
         else if (helper_hit_count == 1)
         {
            sprintf(message, "%s回复-%d", magic_name, round_result[LAVA_BATTLE_ROUND_PLAYER_DAMAGE]);
            PAL_LavaFightSetRoundEvent(TRUE,
               LAVA_BATTLE_EVENT_PLAYER_MAGIC_HEAL, magic_object_id,
               round_result[LAVA_BATTLE_ROUND_PLAYER_DAMAGE]);
         }
         else if (helper_mp_hit_count > 1)
         {
            sprintf(message, "%s回复真气全体", magic_name);
            PAL_LavaFightSetRoundEvent(TRUE,
               LAVA_BATTLE_EVENT_PLAYER_MAGIC_MP_ALL, magic_object_id, helper_mp_hit_count);
         }
         else if (helper_mp_hit_count == 1)
         {
            sprintf(message, "%s回复真气-%d", magic_name, round_result[LAVA_BATTLE_ROUND_PLAYER_DAMAGE]);
            PAL_LavaFightSetRoundEvent(TRUE,
               LAVA_BATTLE_EVENT_PLAYER_MAGIC_MP, magic_object_id,
               round_result[LAVA_BATTLE_ROUND_PLAYER_DAMAGE]);
         }
         else if (helper_status_code == LAVA_BATTLE_HELPER_STATUS_MULTI)
         {
            sprintf(message, "%s调整全体状态", magic_name);
            PAL_LavaFightSetRoundEvent(TRUE, LAVA_BATTLE_EVENT_PLAYER_MAGIC_HELPER,
               magic_object_id, helper_status_code);
         }
         else if (helper_status_code > 0)
         {
            sprintf(message, "%s加持%s", magic_name,
               PAL_LavaBattleStatusName(helper_status_code - 1));
            PAL_LavaFightSetRoundEvent(TRUE, LAVA_BATTLE_EVENT_PLAYER_MAGIC_HELPER,
               magic_object_id, helper_status_code);
         }
         else if (helper_status_code < 0)
         {
            sprintf(message, "%s解除%s", magic_name,
               PAL_LavaBattleStatusName((-helper_status_code) - 1));
            PAL_LavaFightSetRoundEvent(TRUE, LAVA_BATTLE_EVENT_PLAYER_MAGIC_HELPER,
               magic_object_id, helper_status_code);
         }
         else
         {
            sprintf(message, helper_script_success ? "%s施展完毕" : "%s未起效", magic_name);
            PAL_LavaFightSetRoundEvent(TRUE, LAVA_BATTLE_EVENT_PLAYER_MAGIC_HELPER, magic_object_id, 0);
         }
         if (g_lava_battle_party_trance[acting_player] > 0)
         {
            PAL_LavaFightConsumePartyTrance(acting_player);
         }
         return LAVA_BATTLE_ACTION_COMMIT;
      }

      PAL_LavaFightConsumeRoleMP(player_role, mp_cost);
      if (PAL_LavaFightMagicHitsAllEnemies(magic_object_id))
      {
         hit_count = 0;
         for (i = 0; i < enemy_count && hit_count < LAVA_BATTLE_MAX_ENEMIES; i++)
         {
            if (enemy_hp[i] <= 0)
            {
               continue;
            }

            damage = PAL_LavaFightMagicDamage(player_role, i, magic_object_id, turn);
            enemy_hp[i] -= damage;
            if (g_lava_autotest_battle && enemy_hp[i] <= 0)
            {
               enemy_hp[i] = 1;
            }
            if (enemy_hp[i] < 0)
            {
               enemy_hp[i] = 0;
            }
            if (enemy_hp[i] <= 0)
            {
               any_ko = TRUE;
            }
            PAL_LavaFightStorePlayerHit(round_result, hit_count, i, damage);
            if (hit_count == 0)
            {
               *target_sel = i;
            }
            hit_count++;
         }

        sprintf(message, any_ko ? "%s席卷全场，敌阵溃散" : "%s席卷全场", magic_name);
        PAL_LavaFightSetRoundEvent(TRUE,
           any_ko ? LAVA_BATTLE_EVENT_PLAYER_MAGIC_ALL_KO : LAVA_BATTLE_EVENT_PLAYER_MAGIC_ALL,
           magic_object_id, hit_count);
         if (g_lava_battle_party_trance[acting_player] > 0)
         {
            PAL_LavaFightConsumePartyTrance(acting_player);
         }
        return LAVA_BATTLE_ACTION_COMMIT;
      }

      *target_sel = PAL_LavaFightFixEnemyTarget(*target_sel, enemy_count, enemy_hp);
      damage = PAL_LavaFightMagicDamage(player_role, *target_sel, magic_object_id, turn);
      enemy_hp[*target_sel] -= damage;
      if (g_lava_autotest_battle && enemy_hp[*target_sel] <= 0)
      {
         enemy_hp[*target_sel] = 1;
      }
      if (enemy_hp[*target_sel] < 0)
      {
         enemy_hp[*target_sel] = 0;
      }
      PAL_LavaFightStorePlayerHit(round_result, 0, *target_sel, damage);
      if (enemy_hp[*target_sel] <= 0)
      {
         sprintf(message, "%s命中-%d，敌人倒下", magic_name, damage);
         PAL_LavaFightSetRoundEvent(TRUE, LAVA_BATTLE_EVENT_PLAYER_MAGIC_HIT_KO, magic_object_id, damage);
      }
      else
      {
         sprintf(message, "%s命中-%d", magic_name, damage);
         PAL_LavaFightSetRoundEvent(TRUE, LAVA_BATTLE_EVENT_PLAYER_MAGIC_HIT, magic_object_id, damage);
      }
      if (g_lava_battle_party_trance[acting_player] > 0)
      {
         PAL_LavaFightConsumePartyTrance(acting_player);
      }
      return LAVA_BATTLE_ACTION_COMMIT;
   }

   any_ko = FALSE;
   hit_count = 0;

   if (PAL_LavaRoleWordByArray(4, player_role) != 0 &&
       PAL_LavaFightCountAlive(enemy_hp, enemy_count) > 1)
   {
      strong_attack = PAL_LavaFightPlayerStrongAttack(player_role,
         PAL_LavaFightFixEnemyTarget(*target_sel, enemy_count, enemy_hp), turn);
      for (i = 0; i < enemy_count && hit_count < LAVA_BATTLE_MAX_ENEMIES; i++)
      {
         if (enemy_hp[i] <= 0)
         {
            continue;
         }

         if (strong_attack)
         {
            damage = PAL_LavaFightPlayerStrongDamage(player_role, i, turn);
         }
         else
         {
            damage = PAL_LavaFightPlayerDamage(player_role, i, turn);
         }
         enemy_hp[i] -= damage;
         if (enemy_hp[i] < 0)
         {
            enemy_hp[i] = 0;
         }
         if (enemy_hp[i] <= 0)
         {
            any_ko = TRUE;
         }
         PAL_LavaFightStorePlayerHit(round_result, hit_count, i, damage);
         if (hit_count == 0)
         {
            *target_sel = i;
         }
         hit_count++;
      }
   }
   else
   {
      *target_sel = PAL_LavaFightFixEnemyTarget(*target_sel, enemy_count, enemy_hp);
      strong_attack = PAL_LavaFightPlayerStrongAttack(player_role, *target_sel, turn);
      if (strong_attack)
      {
         damage = PAL_LavaFightPlayerStrongDamage(player_role, *target_sel, turn);
      }
      else
      {
         damage = PAL_LavaFightPlayerDamage(player_role, *target_sel, turn);
      }
      enemy_hp[*target_sel] -= damage;
      if (enemy_hp[*target_sel] < 0)
      {
         enemy_hp[*target_sel] = 0;
      }
      if (enemy_hp[*target_sel] <= 0)
      {
         any_ko = TRUE;
      }
      PAL_LavaFightStorePlayerHit(round_result, 0, *target_sel, damage);
      hit_count = 1;
   }

   if (hit_count > 1)
   {
      sprintf(message, any_ko ? "横扫全体，敌阵大乱" : "横扫全体");
      PAL_LavaFightSetRoundEvent(TRUE,
         any_ko ? LAVA_BATTLE_EVENT_PLAYER_HIT_ALL_KO : LAVA_BATTLE_EVENT_PLAYER_HIT_ALL,
         hit_count, strong_attack);
   }
   else
   {
      damage = round_result == 0 ? damage : round_result[LAVA_BATTLE_ROUND_PLAYER_DAMAGE];
      if (enemy_hp[*target_sel] <= 0)
      {
         sprintf(message, "命中目标-%d，敌人倒下", damage);
         PAL_LavaFightSetRoundEvent(TRUE,
            strong_attack ? LAVA_BATTLE_EVENT_PLAYER_STRONG_HIT_KO : LAVA_BATTLE_EVENT_PLAYER_HIT_KO,
            *target_sel, damage);
      }
      else
      {
         sprintf(message, "命中目标-%d", damage);
         PAL_LavaFightSetRoundEvent(TRUE,
            strong_attack ? LAVA_BATTLE_EVENT_PLAYER_STRONG_HIT : LAVA_BATTLE_EVENT_PLAYER_HIT,
            *target_sel, damage);
      }
   }

   if (g_lava_battle_party_trance[acting_player] > 0)
   {
      PAL_LavaFightConsumePartyTrance(acting_player);
   }

   return LAVA_BATTLE_ACTION_COMMIT;
}

void PAL_LavaFightCommitEnemyTurn(
   int enemy_count,
   int *enemy_hp,
   int turn,
   int *party_hp,
   int *party_hp_max,
   char *message,
   int *round_result,
   int *result_target,
   int *result_damage
)
{
   int attack_times;
   int cover_happened;
   int defending;
   int enemy_attacker;
   int enemy_action;
   int enemy_magic_type;
   int all_party_magic;
   int any_ko;
   int hit;
   int hit_count;
   int i;
   int target_role;
   int total_damage;
   int enemy_damage;
   int alive_enemy;

   if (result_target != 0)
   {
      *result_target = -1;
   }
   if (result_damage != 0)
   {
      *result_damage = 0;
   }

   target_role = PAL_LavaFightChoosePartyTarget(party_hp);
   cover_happened = FALSE;
   if (party_hp[target_role] <= 0)
   {
      sprintf(message, "敌方逼近");
      PAL_LavaFightSetRoundEvent(FALSE, LAVA_BATTLE_EVENT_ENEMY_IDLE, 0, 0);
      return;
   }

   alive_enemy = PAL_LavaFightCountAlive(enemy_hp, enemy_count);
   if (alive_enemy <= 0)
   {
      sprintf(message, "敌方逼近");
      PAL_LavaFightSetRoundEvent(FALSE, LAVA_BATTLE_EVENT_ENEMY_IDLE, 0, 0);
      return;
   }

   enemy_attacker = PAL_LavaFightChooseActingEnemy(enemy_count, enemy_hp);
   g_lava_battle_round_enemy_object_id = g_lava_battle_enemy_object_id[enemy_attacker];
   g_lava_battle_round_enemy_magic_object_id = 0;
   attack_times = g_lava_battle_enemy_dual_move[enemy_attacker] ? 2 : 1;
   enemy_action = 0;
   enemy_magic_type = -1;
   all_party_magic = FALSE;
   if (g_lava_battle_enemy_magic_object_id[enemy_attacker] > 0 &&
       g_lava_battle_enemy_magic_object_id[enemy_attacker] != 0xFFFF &&
       g_lava_battle_enemy_magic_rate[enemy_attacker] > 0 &&
       RandomLong(0, 9) < g_lava_battle_enemy_magic_rate[enemy_attacker])
   {
      enemy_action = 1;
      g_lava_battle_round_enemy_magic_object_id = g_lava_battle_enemy_magic_object_id[enemy_attacker];
      enemy_magic_type = PAL_LavaFightMagicType(g_lava_battle_enemy_magic_object_id[enemy_attacker]);
      all_party_magic = PAL_LavaFightMagicApplyToAll(g_lava_battle_enemy_magic_object_id[enemy_attacker]) ||
         enemy_magic_type == 1 || enemy_magic_type == 2 || enemy_magic_type == 3 || enemy_magic_type == 5;
   }
   else if (g_lava_autotest_battle &&
      g_lava_autotest_battle_magic_seen >= 2 &&
      g_lava_autotest_battle_magic_seen < 3)
   {
      enemy_action = 1;
      g_lava_battle_round_enemy_magic_object_id = g_lava_autotest_battle_magic_object_id;
      enemy_magic_type = PAL_LavaFightMagicType(g_lava_battle_round_enemy_magic_object_id);
      all_party_magic = PAL_LavaFightMagicApplyToAll(g_lava_battle_round_enemy_magic_object_id) ||
         enemy_magic_type == 1 || enemy_magic_type == 2 || enemy_magic_type == 3 || enemy_magic_type == 5;
      printf("[LAVA][BATTLESMOKE] force enemy magic obj=%d type=%d all=%d\n",
         g_lava_battle_round_enemy_magic_object_id, enemy_magic_type, all_party_magic);
   }
   else if (g_lava_battle_enemy_equiv_item[enemy_attacker] > 0 &&
            g_lava_battle_enemy_equiv_item[enemy_attacker] != 0xFFFF &&
            g_lava_battle_enemy_equiv_rate[enemy_attacker] > 0 &&
            RandomLong(0, 9) < g_lava_battle_enemy_equiv_rate[enemy_attacker])
   {
      enemy_action = 2;
   }

   if (all_party_magic)
   {
      any_ko = FALSE;
      hit_count = 0;
      total_damage = 0;
      for (i = 0; i < g_lava_party_count && i < 3; i++)
      {
         if (party_hp[i] <= 0)
         {
            continue;
         }
         defending = g_lava_battle_party_defending[i] != 0;
         enemy_damage = PAL_LavaFightEnemyMagicDamage(
            enemy_attacker,
            g_lava_party_role[i],
            g_lava_battle_enemy_magic_object_id[enemy_attacker],
            alive_enemy,
            turn + i,
            defending);
         party_hp[i] -= enemy_damage;
         total_damage += enemy_damage;
         if (party_hp[i] <= 0)
         {
            party_hp[i] = 0;
            g_lava_battle_party_defending[i] = 0;
            any_ko = TRUE;
         }
         PAL_LavaFightWriteRoleHP(g_lava_party_role[i], party_hp[i]);
         PAL_LavaFightStoreEnemyHit(round_result, hit_count, i, enemy_damage);
         hit_count++;
      }
      if (result_target != 0)
      {
         *result_target = round_result == 0 ? target_role : round_result[LAVA_BATTLE_ROUND_ENEMY_TARGET];
      }
      if (result_damage != 0)
      {
         *result_damage = total_damage;
      }
      sprintf(message, any_ko ? "妖术袭来，有人倒下" : "妖术袭来，全体受击-%d", total_damage);
      PAL_LavaFightSetRoundEvent(FALSE,
         any_ko ? LAVA_BATTLE_EVENT_ENEMY_MAGIC_ALL_KO : LAVA_BATTLE_EVENT_ENEMY_MAGIC_ALL,
         hit_count, total_damage);
      return;
   }

   if (enemy_action != 1 && party_hp_max != 0)
   {
      int covered_target;

      covered_target = PAL_LavaFightApplyCoverTarget(target_role, party_hp, party_hp_max);
      if (covered_target != target_role)
      {
         cover_happened = TRUE;
         target_role = covered_target;
      }
   }
   if (enemy_action != 1 && !PAL_LavaFightEnemyHitPlayer(enemy_attacker, g_lava_party_role[target_role], turn))
   {
      sprintf(message, "%s闪开", PAL_LavaRoleName(g_lava_party_role[target_role]));
      if (result_target != 0)
      {
         *result_target = target_role;
      }
      PAL_LavaFightSetRoundEvent(FALSE, LAVA_BATTLE_EVENT_ENEMY_MISS, target_role, cover_happened);
      return;
   }
   total_damage = 0;
   for (hit = 0; hit < attack_times; hit++)
   {
      defending = (target_role >= 0 && target_role < 3 &&
         g_lava_battle_party_defending[target_role] != 0);
      if (enemy_action == 1)
      {
         enemy_damage = PAL_LavaFightEnemyMagicDamage(
            enemy_attacker,
            g_lava_party_role[target_role],
            g_lava_battle_enemy_magic_object_id[enemy_attacker],
            alive_enemy,
            turn + hit,
            defending);
      }
      else if (enemy_action == 2)
      {
         enemy_damage = PAL_LavaFightEnemyDamage(
            enemy_attacker,
            g_lava_party_role[target_role],
            alive_enemy,
            turn + hit,
            defending);
         enemy_damage += enemy_damage / 2 + g_lava_battle_enemy_attack_strength[enemy_attacker] / 8;
         if (enemy_damage < 1)
         {
            enemy_damage = 1;
         }
      }
      else
      {
         enemy_damage = PAL_LavaFightEnemyDamage(
            enemy_attacker,
            g_lava_party_role[target_role],
            alive_enemy,
            turn + hit,
            defending);
      }
      party_hp[target_role] -= enemy_damage;
      total_damage += enemy_damage;
      if (party_hp[target_role] <= 0)
      {
         party_hp[target_role] = 0;
         break;
      }
   }

   PAL_LavaFightWriteRoleHP(g_lava_party_role[target_role], party_hp[target_role]);

   if (result_target != 0)
   {
      *result_target = target_role;
   }
   if (result_damage != 0)
   {
      *result_damage = total_damage;
   }

   if (party_hp[target_role] <= 0)
   {
      if (target_role >= 0 && target_role < 3)
      {
         g_lava_battle_party_defending[target_role] = 0;
      }
      sprintf(message, "%s倒下", PAL_LavaRoleName(g_lava_party_role[target_role]));
      if (cover_happened)
      {
         PAL_LavaFightSetRoundEvent(FALSE, LAVA_BATTLE_EVENT_ENEMY_COVER_KO, target_role, total_damage);
      }
      else
      {
         PAL_LavaFightSetRoundEvent(FALSE,
            enemy_action == 1 ? LAVA_BATTLE_EVENT_ENEMY_MAGIC_HIT_KO :
               (enemy_action == 2 ? LAVA_BATTLE_EVENT_ENEMY_POWER_HIT_KO : LAVA_BATTLE_EVENT_ENEMY_HIT_KO),
            target_role, total_damage);
      }
   }
   else
   {
      sprintf(message, "%s受击-%d", PAL_LavaRoleName(g_lava_party_role[target_role]), total_damage);
      if (cover_happened)
      {
         PAL_LavaFightSetRoundEvent(FALSE, LAVA_BATTLE_EVENT_ENEMY_COVER_HIT, target_role, total_damage);
      }
      else
      {
         PAL_LavaFightSetRoundEvent(FALSE,
            enemy_action == 1 ? LAVA_BATTLE_EVENT_ENEMY_MAGIC_HIT :
               (enemy_action == 2 ? LAVA_BATTLE_EVENT_ENEMY_POWER_HIT : LAVA_BATTLE_EVENT_ENEMY_HIT),
            target_role, total_damage);
      }
   }
}

int PAL_LavaFightCommitRound(
   int command_sel,
   int *target_sel,
   int enemy_count,
   int *enemy_hp,
   int turn,
   int *party_hp,
   int *party_hp_max,
   char *message,
   int *round_result
)
{
   int action_result;
   int enemy_round_delay;
   int flags;
   int player_round_delay;

   if (round_result != 0)
   {
      round_result[LAVA_BATTLE_ROUND_PLAYER_TARGET] = -1;
      round_result[LAVA_BATTLE_ROUND_PLAYER_DAMAGE] = 0;
      round_result[LAVA_BATTLE_ROUND_ENEMY_TARGET] = -1;
      round_result[LAVA_BATTLE_ROUND_ENEMY_DAMAGE] = 0;
      round_result[LAVA_BATTLE_ROUND_FLAGS] = 0;
      round_result[LAVA_BATTLE_ROUND_PLAYER_DELAY] = 90;
      round_result[LAVA_BATTLE_ROUND_ENEMY_DELAY] = 90;
      PAL_LavaFightClearPlayerHitList(round_result);
      PAL_LavaFightClearEnemyHitList(round_result);
   }
   PAL_LavaFightClearRoundScript(round_result);
   PAL_LavaFightClearRoundMessages();

   action_result = PAL_LavaFightCommitPlayerCommand(
      command_sel,
      target_sel,
      enemy_count,
      enemy_hp,
      party_hp,
      party_hp_max,
      turn,
      message,
      round_result);

   flags = 0;
   if (PAL_LavaFightGetRoundEventType(TRUE) != LAVA_BATTLE_EVENT_NONE)
   {
      flags |= LAVA_BATTLE_ROUND_FLAG_PLAYER_MESSAGE;
   }
   if (round_result != 0 &&
       (round_result[LAVA_BATTLE_ROUND_PLAYER_DAMAGE] > 0 ||
        (round_result[LAVA_BATTLE_ROUND_PLAYER_HIT_COUNT] > 0 &&
         PAL_LavaFightPlayerEventNeedsDamagePhase(PAL_LavaFightGetRoundEventType(TRUE)))))
   {
      flags |= LAVA_BATTLE_ROUND_FLAG_PLAYER_DAMAGE;
      if (PAL_LavaFightGetRoundEventType(TRUE) == LAVA_BATTLE_EVENT_PLAYER_HIT_KO ||
          PAL_LavaFightGetRoundEventType(TRUE) == LAVA_BATTLE_EVENT_PLAYER_STRONG_HIT_KO ||
          PAL_LavaFightGetRoundEventType(TRUE) == LAVA_BATTLE_EVENT_PLAYER_HIT_ALL_KO ||
          PAL_LavaFightGetRoundEventType(TRUE) == LAVA_BATTLE_EVENT_PLAYER_MAGIC_HIT_KO ||
          PAL_LavaFightGetRoundEventType(TRUE) == LAVA_BATTLE_EVENT_PLAYER_MAGIC_ALL_KO ||
          PAL_LavaFightGetRoundEventType(TRUE) == LAVA_BATTLE_EVENT_PLAYER_COOP_HIT_KO ||
          PAL_LavaFightGetRoundEventType(TRUE) == LAVA_BATTLE_EVENT_PLAYER_COOP_ALL_KO)
      {
         flags |= LAVA_BATTLE_ROUND_FLAG_ENEMY_KO;
      }
   }
   if (command_sel == LAVA_BATTLE_COMMAND_FLEE &&
       action_result != LAVA_BATTLE_ACTION_FLEE_SUCCESS)
   {
      flags |= LAVA_BATTLE_ROUND_FLAG_FLEE_FAILED;
   }
   player_round_delay = (flags & (LAVA_BATTLE_ROUND_FLAG_FLEE_FAILED | LAVA_BATTLE_ROUND_FLAG_ENEMY_KO)) ? 140 : 90;

   if (action_result == LAVA_BATTLE_ACTION_FLEE_SUCCESS)
   {
      if (round_result != 0)
      {
         round_result[LAVA_BATTLE_ROUND_FLAGS] = flags;
         round_result[LAVA_BATTLE_ROUND_PLAYER_DELAY] = player_round_delay;
      }
      return LAVA_BATTLE_RESULT_FLEE;
   }

   if (!g_lava_battle_defer_enemy_until_party_done &&
       PAL_LavaFightCountAlive(enemy_hp, enemy_count) > 0)
   {
      PAL_LavaFightCommitEnemyTurn(
         enemy_count,
         enemy_hp,
         turn,
          party_hp,
          party_hp_max,
          message,
          round_result,
          round_result == 0 ? 0 : &round_result[LAVA_BATTLE_ROUND_ENEMY_TARGET],
          round_result == 0 ? 0 : &round_result[LAVA_BATTLE_ROUND_ENEMY_DAMAGE]);

      if (command_sel == LAVA_BATTLE_COMMAND_FLEE)
      {
         int enemy_target;
         int enemy_damage;

         enemy_target = round_result == 0 ? -1 : round_result[LAVA_BATTLE_ROUND_ENEMY_TARGET];
         enemy_damage = round_result == 0 ? 0 : round_result[LAVA_BATTLE_ROUND_ENEMY_DAMAGE];

         if (enemy_damage > 0)
         {
            if (party_hp[enemy_target] <= 0)
            {
               sprintf(message, "撤退失败，%s倒下", PAL_LavaRoleName(g_lava_party_role[enemy_target]));
            }
            else
            {
               sprintf(message, "撤退失败，%s受击-%d",
                  PAL_LavaRoleName(g_lava_party_role[enemy_target]), enemy_damage);
            }
            PAL_LavaFightSetRoundEvent(FALSE,
               party_hp[enemy_target] <= 0 ? LAVA_BATTLE_EVENT_ENEMY_FLEE_FAIL_KO : LAVA_BATTLE_EVENT_ENEMY_FLEE_FAIL_HIT,
               enemy_target, enemy_damage);
         }
      }
   }

   if (PAL_LavaFightGetRoundEventType(FALSE) != LAVA_BATTLE_EVENT_NONE)
   {
      flags |= LAVA_BATTLE_ROUND_FLAG_ENEMY_MESSAGE;
      if (round_result != 0 && round_result[LAVA_BATTLE_ROUND_ENEMY_DAMAGE] <= 0)
      {
         flags |= LAVA_BATTLE_ROUND_FLAG_ENEMY_IDLE;
      }
   }
   if (round_result != 0 && round_result[LAVA_BATTLE_ROUND_ENEMY_DAMAGE] > 0)
   {
      flags |= LAVA_BATTLE_ROUND_FLAG_ENEMY_DAMAGE;
      if (PAL_LavaFightGetRoundEventType(FALSE) == LAVA_BATTLE_EVENT_ENEMY_MAGIC_ALL_KO ||
          party_hp[round_result[LAVA_BATTLE_ROUND_ENEMY_TARGET]] <= 0)
      {
         flags |= LAVA_BATTLE_ROUND_FLAG_PLAYER_KO;
      }
   }
   enemy_round_delay = (flags & LAVA_BATTLE_ROUND_FLAG_PLAYER_KO) ? 140 :
      ((flags & LAVA_BATTLE_ROUND_FLAG_ENEMY_IDLE) ? 70 : 90);
   if (round_result != 0)
   {
      round_result[LAVA_BATTLE_ROUND_FLAGS] = flags;
      round_result[LAVA_BATTLE_ROUND_PLAYER_DELAY] = player_round_delay;
      round_result[LAVA_BATTLE_ROUND_ENEMY_DELAY] = enemy_round_delay;

      if (flags & LAVA_BATTLE_ROUND_FLAG_PLAYER_MESSAGE)
      {
         PAL_LavaFightAppendRoundPhase(round_result, LAVA_BATTLE_PHASE_PLAYER_MESSAGE);
      }
      if (flags & LAVA_BATTLE_ROUND_FLAG_PLAYER_DAMAGE)
      {
         PAL_LavaFightAppendRoundPhase(round_result, LAVA_BATTLE_PHASE_PLAYER_DAMAGE);
      }
      if (flags & LAVA_BATTLE_ROUND_FLAG_ENEMY_MESSAGE)
      {
         PAL_LavaFightAppendRoundPhase(round_result, LAVA_BATTLE_PHASE_ENEMY_MESSAGE);
      }
      if (flags & LAVA_BATTLE_ROUND_FLAG_ENEMY_DAMAGE)
      {
         PAL_LavaFightAppendRoundPhase(round_result, LAVA_BATTLE_PHASE_ENEMY_DAMAGE);
      }
   }

   return PAL_LavaFightCheckResult(enemy_count, enemy_hp, party_hp, message);
}

static int PAL_LavaFightCommitEnemyOnlyRound(
   int enemy_count,
   int *enemy_hp,
   int turn,
   int *party_hp,
   int *party_hp_max,
   char *message,
   int *round_result
)
{
   int enemy_round_delay;
   int flags;

   if (round_result != 0)
   {
      round_result[LAVA_BATTLE_ROUND_PLAYER_TARGET] = -1;
      round_result[LAVA_BATTLE_ROUND_PLAYER_DAMAGE] = 0;
      round_result[LAVA_BATTLE_ROUND_ENEMY_TARGET] = -1;
      round_result[LAVA_BATTLE_ROUND_ENEMY_DAMAGE] = 0;
      round_result[LAVA_BATTLE_ROUND_FLAGS] = 0;
      round_result[LAVA_BATTLE_ROUND_PLAYER_DELAY] = 90;
      round_result[LAVA_BATTLE_ROUND_ENEMY_DELAY] = 90;
      PAL_LavaFightClearPlayerHitList(round_result);
      PAL_LavaFightClearEnemyHitList(round_result);
   }
   PAL_LavaFightClearRoundScript(round_result);
   PAL_LavaFightClearRoundMessages();

   if (PAL_LavaFightCountAlive(enemy_hp, enemy_count) > 0)
   {
      PAL_LavaFightCommitEnemyTurn(
         enemy_count,
         enemy_hp,
         turn,
         party_hp,
         party_hp_max,
         message,
         round_result,
         round_result == 0 ? 0 : &round_result[LAVA_BATTLE_ROUND_ENEMY_TARGET],
         round_result == 0 ? 0 : &round_result[LAVA_BATTLE_ROUND_ENEMY_DAMAGE]);
   }

   flags = 0;
   if (PAL_LavaFightGetRoundEventType(FALSE) != LAVA_BATTLE_EVENT_NONE)
   {
      flags |= LAVA_BATTLE_ROUND_FLAG_ENEMY_MESSAGE;
      if (round_result != 0 && round_result[LAVA_BATTLE_ROUND_ENEMY_DAMAGE] <= 0)
      {
         flags |= LAVA_BATTLE_ROUND_FLAG_ENEMY_IDLE;
      }
   }
   if (round_result != 0 && round_result[LAVA_BATTLE_ROUND_ENEMY_DAMAGE] > 0)
   {
      flags |= LAVA_BATTLE_ROUND_FLAG_ENEMY_DAMAGE;
      if (PAL_LavaFightGetRoundEventType(FALSE) == LAVA_BATTLE_EVENT_ENEMY_MAGIC_ALL_KO ||
          party_hp[round_result[LAVA_BATTLE_ROUND_ENEMY_TARGET]] <= 0)
      {
         flags |= LAVA_BATTLE_ROUND_FLAG_PLAYER_KO;
      }
   }
   enemy_round_delay = (flags & LAVA_BATTLE_ROUND_FLAG_PLAYER_KO) ? 140 :
      ((flags & LAVA_BATTLE_ROUND_FLAG_ENEMY_IDLE) ? 70 : 90);
   if (round_result != 0)
   {
      round_result[LAVA_BATTLE_ROUND_FLAGS] = flags;
      round_result[LAVA_BATTLE_ROUND_ENEMY_DELAY] = enemy_round_delay;
      if (flags & LAVA_BATTLE_ROUND_FLAG_ENEMY_MESSAGE)
      {
         PAL_LavaFightAppendRoundPhase(round_result, LAVA_BATTLE_PHASE_ENEMY_MESSAGE);
      }
      if (flags & LAVA_BATTLE_ROUND_FLAG_ENEMY_DAMAGE)
      {
         PAL_LavaFightAppendRoundPhase(round_result, LAVA_BATTLE_PHASE_ENEMY_DAMAGE);
      }
   }

   return PAL_LavaFightCheckResult(enemy_count, enemy_hp, party_hp, message);
}

int PAL_LavaFightCommitRoundState(LAVA_BATTLE_STATE *state)
{
   int i;
   int acting_index;
   int enemy_phase_next;
   int forced_enemy;
   int has_more_pending;

   if (state == 0)
   {
      return LAVA_BATTLE_RESULT_LOSE;
   }

   state->battle_result = LAVA_BATTLE_RESULT_CONTINUE;
   if (g_lava_battle_enemy_phase_index >= 0)
   {
      forced_enemy = PAL_LavaFightNextEnemyPhaseIndex(g_lava_battle_enemy_phase_index,
         state->enemy_count, state->enemy_hp);
      if (forced_enemy < 0)
      {
         g_lava_battle_enemy_phase_index = -1;
         return state->battle_result;
      }
      g_lava_battle_forced_enemy_index = forced_enemy;
      state->battle_result = PAL_LavaFightCommitEnemyOnlyRound(state->enemy_count,
         state->enemy_hp, state->turn, state->party_hp, state->party_hp_max,
         state->message, state->round_result);
      g_lava_battle_forced_enemy_index = -1;
      enemy_phase_next = PAL_LavaFightNextEnemyPhaseIndex(forced_enemy + 1,
         state->enemy_count, state->enemy_hp);
      if (state->battle_result == LAVA_BATTLE_RESULT_CONTINUE && enemy_phase_next >= 0)
      {
         g_lava_battle_enemy_phase_index = enemy_phase_next;
         state->flow_action = LAVA_BATTLE_FLOW_RECHECK;
      }
      else
      {
         g_lava_battle_enemy_phase_index = -1;
      }
      state->party_defending[0] = g_lava_battle_party_defending[0];
      state->party_defending[1] = g_lava_battle_party_defending[1];
      state->party_defending[2] = g_lava_battle_party_defending[2];
      return state->battle_result;
   }

   acting_index = -1;
   for (i = 0; i < g_lava_party_count && i < 3; i++)
   {
      if (state->party_hp[i] <= 0 || !g_lava_battle_pending_ready[i])
      {
         continue;
      }

      acting_index = i;
      break;
   }

   if (acting_index < 0)
   {
      return state->battle_result;
   }

   state->acting_player_index = acting_index;
   state->round_command = g_lava_battle_pending_command[acting_index];
   state->command_sel = g_lava_battle_pending_command[acting_index];
   state->target_sel = g_lava_battle_pending_target[acting_index];
   state->magic_sel = g_lava_battle_pending_magic_sel[acting_index];
   state->magic_object_id = g_lava_battle_pending_magic_object_id[acting_index];
   state->item_sel = g_lava_battle_pending_item_sel[acting_index];
   state->item_object_id = g_lava_battle_pending_item_object_id[acting_index];
   g_lava_battle_selected_magic_object_id = state->magic_object_id;
   g_lava_battle_selected_item_object_id = state->item_object_id;
   g_lava_battle_acting_player_index = acting_index;
   has_more_pending = 0;
   for (i = acting_index + 1; i < g_lava_party_count && i < 3; i++)
   {
      if (state->party_hp[i] > 0 && g_lava_battle_pending_ready[i])
      {
         has_more_pending = 1;
         break;
      }
   }
   g_lava_battle_defer_enemy_until_party_done = has_more_pending;
   if (!has_more_pending)
   {
      g_lava_battle_forced_enemy_index = PAL_LavaFightNextEnemyPhaseIndex(0,
         state->enemy_count, state->enemy_hp);
   }
   if (g_lava_autotest_fengshen || state->magic_object_id == 0x145 || state->magic_object_id == 0x156)
   {
      printf("[LAVA][MAGICDISPATCH] commit-state actor=%d cmd=%d magic=%d target=%d enemy_hp=%d,%d,%d,%d,%d\n",
         acting_index, state->round_command, state->magic_object_id, state->target_sel,
         state->enemy_hp[0], state->enemy_hp[1], state->enemy_hp[2],
         state->enemy_hp[3], state->enemy_hp[4]);
   }

   state->battle_result = PAL_LavaFightCommitRound(state->round_command, &state->target_sel,
      state->enemy_count, state->enemy_hp, state->turn, state->party_hp, state->party_hp_max,
      state->message, state->round_result);
   g_lava_battle_defer_enemy_until_party_done = 0;
   forced_enemy = g_lava_battle_forced_enemy_index;
   g_lava_battle_forced_enemy_index = -1;
   if (g_lava_autotest_fengshen || state->magic_object_id == 0x145 || state->magic_object_id == 0x156)
   {
      printf("[LAVA][MAGICDISPATCH] commit-state-result actor=%d result=%d event=%d arg0=%d arg1=%d p_damage=%d hit_count=%d\n",
         acting_index, state->battle_result, PAL_LavaFightGetRoundEventType(TRUE),
         PAL_LavaFightGetRoundEventArg0(TRUE), PAL_LavaFightGetRoundEventArg1(TRUE),
         state->round_result[LAVA_BATTLE_ROUND_PLAYER_DAMAGE],
         state->round_result[LAVA_BATTLE_ROUND_PLAYER_HIT_COUNT]);
   }
   g_lava_battle_pending_ready[acting_index] = 0;
   if (state->battle_result == LAVA_BATTLE_RESULT_CONTINUE &&
       PAL_LavaFightCountAlive(state->enemy_hp, state->enemy_count) <= 0)
   {
      state->battle_result = PAL_LavaFightCheckResult(state->enemy_count,
         state->enemy_hp, state->party_hp, state->message);
   }

   if (state->battle_result == LAVA_BATTLE_RESULT_CONTINUE)
   {
      if (has_more_pending)
      {
         state->flow_action = LAVA_BATTLE_FLOW_RECHECK;
      }
      else
      {
         enemy_phase_next = PAL_LavaFightNextEnemyPhaseIndex(forced_enemy + 1,
            state->enemy_count, state->enemy_hp);
         if (forced_enemy >= 0 && enemy_phase_next >= 0)
         {
            g_lava_battle_enemy_phase_index = enemy_phase_next;
            has_more_pending = 1;
            state->flow_action = LAVA_BATTLE_FLOW_RECHECK;
         }
      }
   }

   if (state->battle_result != LAVA_BATTLE_RESULT_CONTINUE || !has_more_pending)
   {
      for (i = 0; i < 3; i++)
      {
         g_lava_battle_pending_ready[i] = 0;
      }
   }
     state->party_defending[0] = g_lava_battle_party_defending[0];
     state->party_defending[1] = g_lava_battle_party_defending[1];
     state->party_defending[2] = g_lava_battle_party_defending[2];
   return state->battle_result;
}
