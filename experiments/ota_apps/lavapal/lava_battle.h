#ifndef LAVA_BATTLE_H
#define LAVA_BATTLE_H

#define LAVA_BATTLE_ACTION_COMMIT 1
#define LAVA_BATTLE_ACTION_FLEE_SUCCESS 2

#define LAVA_BATTLE_COMMAND_ATTACK 0
#define LAVA_BATTLE_COMMAND_DEFEND 1
#define LAVA_BATTLE_COMMAND_MAGIC 2
#define LAVA_BATTLE_COMMAND_FLEE 3
#define LAVA_BATTLE_COMMAND_USE_ITEM 4
#define LAVA_BATTLE_COMMAND_COOP_MAGIC 5
#define LAVA_BATTLE_COMMAND_THROW_ITEM 6

#define LAVA_ITEM_FLAG_USABLE      0x0001
#define LAVA_ITEM_FLAG_THROWABLE   0x0004
#define LAVA_ITEM_FLAG_CONSUMING   0x0008
#define LAVA_ITEM_FLAG_APPLY_TO_ALL 0x0010

#define LAVA_BATTLE_RESULT_CONTINUE 0
#define LAVA_BATTLE_RESULT_LOSE 1
#define LAVA_BATTLE_RESULT_WIN 3
#define LAVA_BATTLE_RESULT_FLEE 0xFFFF

#define LAVA_BATTLE_FLOW_END 0
#define LAVA_BATTLE_FLOW_NEXT_ROUND 1
#define LAVA_BATTLE_FLOW_RECHECK 2

#define LAVA_BATTLE_MAX_ENEMIES 5
#define LAVA_BATTLE_MAX_PLAYER_ROLES 6

#define LAVA_BATTLE_STATUS_CONFUSED   0
#define LAVA_BATTLE_STATUS_SLOW       1
#define LAVA_BATTLE_STATUS_SLEEP      2
#define LAVA_BATTLE_STATUS_SILENCE    3
#define LAVA_BATTLE_STATUS_PUPPET     4
#define LAVA_BATTLE_STATUS_BRAVERY    5
#define LAVA_BATTLE_STATUS_PROTECT    6
#define LAVA_BATTLE_STATUS_HASTE      7
#define LAVA_BATTLE_STATUS_DUALATTACK 8
#define LAVA_BATTLE_STATUS_COUNT      9

#define LAVA_BATTLE_HELPER_STATUS_MULTI 100

#define LAVA_BATTLE_ROUND_PLAYER_TARGET 0
#define LAVA_BATTLE_ROUND_PLAYER_DAMAGE 1
#define LAVA_BATTLE_ROUND_ENEMY_TARGET 2
#define LAVA_BATTLE_ROUND_ENEMY_DAMAGE 3
#define LAVA_BATTLE_ROUND_FLAGS 4
#define LAVA_BATTLE_ROUND_PLAYER_DELAY 5
#define LAVA_BATTLE_ROUND_ENEMY_DELAY 6
#define LAVA_BATTLE_ROUND_STEP0 7
#define LAVA_BATTLE_ROUND_STEP1 8
#define LAVA_BATTLE_ROUND_STEP2 9
#define LAVA_BATTLE_ROUND_STEP3 10
#define LAVA_BATTLE_ROUND_SLOTS 11

#define LAVA_BATTLE_ROUND_FLAG_PLAYER_MESSAGE (1 << 0)
#define LAVA_BATTLE_ROUND_FLAG_PLAYER_DAMAGE  (1 << 1)
#define LAVA_BATTLE_ROUND_FLAG_ENEMY_MESSAGE  (1 << 2)
#define LAVA_BATTLE_ROUND_FLAG_ENEMY_DAMAGE   (1 << 3)
#define LAVA_BATTLE_ROUND_FLAG_FLEE_FAILED    (1 << 4)
#define LAVA_BATTLE_ROUND_FLAG_ENEMY_KO       (1 << 5)
#define LAVA_BATTLE_ROUND_FLAG_PLAYER_KO      (1 << 6)
#define LAVA_BATTLE_ROUND_FLAG_ENEMY_IDLE     (1 << 7)

#define LAVA_BATTLE_PHASE_NONE           0
#define LAVA_BATTLE_PHASE_PLAYER_MESSAGE 1
#define LAVA_BATTLE_PHASE_PLAYER_DAMAGE  2
#define LAVA_BATTLE_PHASE_ENEMY_MESSAGE  3
#define LAVA_BATTLE_PHASE_ENEMY_DAMAGE   4

#define LAVA_BATTLE_EVENT_NONE                0
#define LAVA_BATTLE_EVENT_PLAYER_FLEE_OK      1
#define LAVA_BATTLE_EVENT_PLAYER_FLEE_FAIL    2
#define LAVA_BATTLE_EVENT_PLAYER_DEFEND       3
#define LAVA_BATTLE_EVENT_PLAYER_HIT          4
#define LAVA_BATTLE_EVENT_PLAYER_HIT_KO       5
#define LAVA_BATTLE_EVENT_PLAYER_STRONG_HIT   6
#define LAVA_BATTLE_EVENT_PLAYER_STRONG_HIT_KO 7
#define LAVA_BATTLE_EVENT_ENEMY_IDLE          8
#define LAVA_BATTLE_EVENT_ENEMY_HIT           9
#define LAVA_BATTLE_EVENT_ENEMY_HIT_KO        10
#define LAVA_BATTLE_EVENT_ENEMY_FLEE_FAIL_HIT 11
#define LAVA_BATTLE_EVENT_ENEMY_FLEE_FAIL_KO  12
#define LAVA_BATTLE_EVENT_ENEMY_STRONG_HIT    13
#define LAVA_BATTLE_EVENT_ENEMY_STRONG_HIT_KO 14
#define LAVA_BATTLE_EVENT_ENEMY_COVER_HIT     15
#define LAVA_BATTLE_EVENT_ENEMY_COVER_KO      16
#define LAVA_BATTLE_EVENT_PLAYER_HIT_ALL      17
#define LAVA_BATTLE_EVENT_PLAYER_HIT_ALL_KO   18
#define LAVA_BATTLE_EVENT_PLAYER_MAGIC_HIT    19
#define LAVA_BATTLE_EVENT_PLAYER_MAGIC_HIT_KO 20
#define LAVA_BATTLE_EVENT_PLAYER_MAGIC_EMPTY  21
#define LAVA_BATTLE_EVENT_PLAYER_MAGIC_NOMP   22
#define LAVA_BATTLE_EVENT_PLAYER_MAGIC_ALL    23
#define LAVA_BATTLE_EVENT_PLAYER_MAGIC_ALL_KO 24
#define LAVA_BATTLE_EVENT_PLAYER_MAGIC_HELPER 25
#define LAVA_BATTLE_EVENT_PLAYER_MAGIC_HEAL   26
#define LAVA_BATTLE_EVENT_PLAYER_MAGIC_HEAL_ALL 27
#define LAVA_BATTLE_EVENT_PLAYER_MAGIC_TRANCE 28
#define LAVA_BATTLE_EVENT_PLAYER_MAGIC_NO_TARGET 29
#define LAVA_BATTLE_EVENT_PLAYER_MAGIC_MP     30
#define LAVA_BATTLE_EVENT_PLAYER_MAGIC_MP_ALL 31
#define LAVA_BATTLE_EVENT_PLAYER_MAGIC_SILENCE 32
#define LAVA_BATTLE_EVENT_PLAYER_SLEEP        33
#define LAVA_BATTLE_EVENT_PLAYER_ITEM_EMPTY   34
#define LAVA_BATTLE_EVENT_PLAYER_ITEM_USE     35
#define LAVA_BATTLE_EVENT_PLAYER_COOP_HIT     36
#define LAVA_BATTLE_EVENT_PLAYER_COOP_HIT_KO  37
#define LAVA_BATTLE_EVENT_PLAYER_COOP_ALL     38
#define LAVA_BATTLE_EVENT_PLAYER_COOP_ALL_KO  39
#define LAVA_BATTLE_EVENT_PLAYER_COOP_HELPER  40
#define LAVA_BATTLE_EVENT_PLAYER_COOP_HEAL    41
#define LAVA_BATTLE_EVENT_PLAYER_COOP_HEAL_ALL 42
#define LAVA_BATTLE_EVENT_PLAYER_COOP_MP      43
#define LAVA_BATTLE_EVENT_PLAYER_COOP_MP_ALL  44
#define LAVA_BATTLE_EVENT_PLAYER_COOP_TRANCE  45
#define LAVA_BATTLE_EVENT_ENEMY_MISS          46
#define LAVA_BATTLE_EVENT_ENEMY_MAGIC_HIT     47
#define LAVA_BATTLE_EVENT_ENEMY_MAGIC_HIT_KO  48
#define LAVA_BATTLE_EVENT_ENEMY_POWER_HIT     49
#define LAVA_BATTLE_EVENT_ENEMY_POWER_HIT_KO  50
#define LAVA_BATTLE_EVENT_ENEMY_MAGIC_ALL     51
#define LAVA_BATTLE_EVENT_ENEMY_MAGIC_ALL_KO  52
#define LAVA_BATTLE_EVENT_PLAYER_ITEM_THROW   53
#define LAVA_BATTLE_EVENT_PLAYER_ITEM_THROW_EMPTY 54

#define LAVA_BATTLE_ROUND_PLAYER_HIT_COUNT    11
#define LAVA_BATTLE_ROUND_PLAYER_TARGET1      12
#define LAVA_BATTLE_ROUND_PLAYER_DAMAGE1      13
#define LAVA_BATTLE_ROUND_PLAYER_TARGET2      14
#define LAVA_BATTLE_ROUND_PLAYER_DAMAGE2      15
#define LAVA_BATTLE_ROUND_PLAYER_TARGET3      16
#define LAVA_BATTLE_ROUND_PLAYER_DAMAGE3      17
#define LAVA_BATTLE_ROUND_PLAYER_TARGET4      18
#define LAVA_BATTLE_ROUND_PLAYER_DAMAGE4      19
#define LAVA_BATTLE_ROUND_ENEMY_HIT_COUNT     20
#define LAVA_BATTLE_ROUND_ENEMY_TARGET1       21
#define LAVA_BATTLE_ROUND_ENEMY_DAMAGE1       22
#define LAVA_BATTLE_ROUND_ENEMY_TARGET2       23
#define LAVA_BATTLE_ROUND_ENEMY_DAMAGE2       24
#define LAVA_BATTLE_ROUND_ENEMY_TARGET3       25
#define LAVA_BATTLE_ROUND_ENEMY_DAMAGE3       26
#undef LAVA_BATTLE_ROUND_SLOTS
#define LAVA_BATTLE_ROUND_SLOTS 27

struct tagLAVA_BATTLE_STATE
{
   int battle_result;
   int flow_action;
   int enemy_team;
   int command_sel;
   int magic_sel;
   int magic_object_id;
   int item_sel;
   int item_object_id;
   int round_command;
   int target_sel;
   int acting_player_index;
   int enemy_count;
   int exp_gained;
   int cash_gained;
   int rewards_applied;
   int level_up_role_count;
   int level_up_role[3];
   int level_up_before[30];
   int level_up_after[30];
   int enemy_object_id[LAVA_BATTLE_MAX_ENEMIES];
   int enemy_hp[LAVA_BATTLE_MAX_ENEMIES];
   int enemy_hp_max[LAVA_BATTLE_MAX_ENEMIES];
   int party_hp[3];
   int party_hp_max[3];
   int party_defending[3];
   int party_pose[3];
   int turn;
   char message[64];
   int round_result[LAVA_BATTLE_ROUND_SLOTS];
};
#define LAVA_BATTLE_STATE struct tagLAVA_BATTLE_STATE

void PAL_LavaBattlePrepareVisuals(LAVA_BATTLE_STATE *state);
void PAL_LavaBattleFreeVisuals(void);
addr PAL_LavaBattleGetPlayerAssetFile(void);
void PAL_LavaBattleDrawSceneFrame(LAVA_BATTLE_STATE *state, int target_sel);
void PAL_LavaBattleDrawEnemyTargetOverlay(LAVA_BATTLE_STATE *state, int target_sel);
void PAL_LavaBattleClearKeepEffect(void);
void PAL_LavaBattleSetKeepEffect(addr frame_rle, int x, int y);
void PAL_LavaBattleDrawKeepEffect(void);
void PAL_LavaBattleClearCurrentEffect(void);
void PAL_LavaBattleSetCurrentEffect(addr frame_rle, int x, int y);
void PAL_LavaBattleAddCurrentEffect(addr frame_rle, int x, int y);
void PAL_LavaBattleDrawCurrentEffect(void);
void PAL_LavaBattleClearSummonHold(void);
void PAL_LavaBattleSetSummonHold(addr frame_rle, int cx, int cy);
void PAL_LavaBattleApplyBackgroundShift(int shift);
void PAL_LavaBattleGetEnemyPos(int enemy_index, int *x, int *y);
void PAL_LavaBattleClearPlayerStatuses(void);
void PAL_LavaBattleClearPlayerTransformSprites(void);
int PAL_LavaBattleGetPlayerStatus(int player_role, int status_id);
int PAL_LavaBattleGetPlayerTransformSprite(int player_role);
int PAL_LavaBattleSetPlayerStatus(int player_role, int status_id, int num_round);
void PAL_LavaBattleSetPlayerTransformSprite(int player_role, int sprite_num);
void PAL_LavaBattleRemovePlayerStatus(int player_role, int status_id);
char *PAL_LavaBattleStatusName(int status_id);
char *PAL_LavaBattleStatusShortName(int status_id);
int PAL_LavaFightGetRoundEventType(int player_side);
int PAL_LavaFightGetRoundEventArg0(int player_side);
int PAL_LavaFightGetRoundEventArg1(int player_side);
int PAL_LavaFightGetRoundPlayerTranceBoost(void);
int PAL_LavaFightGetRoundEnemyObjectID(void);
int PAL_LavaFightGetRoundEnemyMagicObjectID(void);
int PAL_LavaFightResolveMagicIndex(int magic_object_id);
int PAL_LavaFightReadMagicField(int magic_index, int field_index);
int PAL_LavaFightMagicBaseDamage(int magic_object_id);
int PAL_LavaFightMagicCost(int magic_object_id);
int PAL_LavaFightMagicByIndex(int player_role, int magic_sel);
int PAL_LavaFightMagicCount(int player_role);

#endif
