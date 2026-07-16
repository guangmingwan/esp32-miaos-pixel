#include "lava_battle.h"

int g_lava_battle_player_status[LAVA_BATTLE_MAX_PLAYER_ROLES][LAVA_BATTLE_STATUS_COUNT];
int g_lava_battle_player_transform_sprite[LAVA_BATTLE_MAX_PLAYER_ROLES];

void PAL_LavaBattleClearPlayerStatuses(void)
{
   int i;
   int j;

   for (i = 0; i < LAVA_BATTLE_MAX_PLAYER_ROLES; i++)
   {
      for (j = 0; j < LAVA_BATTLE_STATUS_COUNT; j++)
      {
         g_lava_battle_player_status[i][j] = 0;
      }
   }
}

void PAL_LavaBattleClearPlayerTransformSprites(void)
{
   int i;

   for (i = 0; i < LAVA_BATTLE_MAX_PLAYER_ROLES; i++)
   {
      g_lava_battle_player_transform_sprite[i] = 0;
   }
}

int PAL_LavaBattleGetPlayerTransformSprite(int player_role)
{
   if (player_role < 0 || player_role >= LAVA_BATTLE_MAX_PLAYER_ROLES)
   {
      return 0;
   }

   return g_lava_battle_player_transform_sprite[player_role];
}

void PAL_LavaBattleSetPlayerTransformSprite(int player_role, int sprite_num)
{
   if (player_role < 0 || player_role >= LAVA_BATTLE_MAX_PLAYER_ROLES)
   {
      return;
   }

   g_lava_battle_player_transform_sprite[player_role] = sprite_num;
}

int PAL_LavaBattleGetPlayerStatus(int player_role, int status_id)
{
   if (player_role < 0 || player_role >= LAVA_BATTLE_MAX_PLAYER_ROLES ||
       status_id < 0 || status_id >= LAVA_BATTLE_STATUS_COUNT)
   {
      return 0;
   }

   return g_lava_battle_player_status[player_role][status_id];
}

int PAL_LavaBattleSetPlayerStatus(int player_role, int status_id, int num_round)
{
   int current_hp;

   if (player_role < 0 || player_role >= LAVA_BATTLE_MAX_PLAYER_ROLES ||
       status_id < 0 || status_id >= LAVA_BATTLE_STATUS_COUNT ||
       num_round <= 0)
   {
      return FALSE;
   }

   if (status_id == LAVA_BATTLE_STATUS_SLOW &&
       g_lava_battle_player_status[player_role][LAVA_BATTLE_STATUS_HASTE] > 0)
   {
      PAL_LavaBattleRemovePlayerStatus(player_role, LAVA_BATTLE_STATUS_HASTE);
      return TRUE;
   }

   if (status_id == LAVA_BATTLE_STATUS_HASTE &&
       g_lava_battle_player_status[player_role][LAVA_BATTLE_STATUS_SLOW] > 0)
   {
      PAL_LavaBattleRemovePlayerStatus(player_role, LAVA_BATTLE_STATUS_SLOW);
      return TRUE;
   }

   if (status_id == LAVA_BATTLE_STATUS_CONFUSED ||
       status_id == LAVA_BATTLE_STATUS_SLOW ||
       status_id == LAVA_BATTLE_STATUS_SLEEP ||
       status_id == LAVA_BATTLE_STATUS_SILENCE)
   {
      if (g_lava_battle_player_status[player_role][status_id] == 0)
      {
         g_lava_battle_player_status[player_role][status_id] = num_round;
      }
      return TRUE;
   }

   current_hp = PAL_LavaRoleWordByArray(9, player_role);
   if (status_id == LAVA_BATTLE_STATUS_PUPPET)
   {
      if (current_hp != 0)
      {
         return FALSE;
      }
      if (g_lava_battle_player_status[player_role][status_id] < num_round)
      {
         g_lava_battle_player_status[player_role][status_id] = num_round;
      }
      return TRUE;
   }

   if (status_id == LAVA_BATTLE_STATUS_BRAVERY ||
       status_id == LAVA_BATTLE_STATUS_PROTECT ||
       status_id == LAVA_BATTLE_STATUS_DUALATTACK ||
       status_id == LAVA_BATTLE_STATUS_HASTE)
   {
      if (current_hp != 0 &&
          g_lava_battle_player_status[player_role][status_id] < num_round)
      {
         g_lava_battle_player_status[player_role][status_id] = num_round;
      }
      return TRUE;
   }

   return FALSE;
}

void PAL_LavaBattleRemovePlayerStatus(int player_role, int status_id)
{
   if (player_role < 0 || player_role >= LAVA_BATTLE_MAX_PLAYER_ROLES ||
       status_id < 0 || status_id >= LAVA_BATTLE_STATUS_COUNT)
   {
      return;
   }

   if (g_lava_battle_player_status[player_role][status_id] <= 999)
   {
      g_lava_battle_player_status[player_role][status_id] = 0;
   }
}

char *PAL_LavaBattleStatusName(int status_id)
{
   if (status_id == LAVA_BATTLE_STATUS_CONFUSED) return "混乱";
   if (status_id == LAVA_BATTLE_STATUS_SLOW) return "迟缓";
   if (status_id == LAVA_BATTLE_STATUS_SLEEP) return "沉睡";
   if (status_id == LAVA_BATTLE_STATUS_SILENCE) return "封印";
   if (status_id == LAVA_BATTLE_STATUS_PUPPET) return "傀儡";
   if (status_id == LAVA_BATTLE_STATUS_BRAVERY) return "勇武";
   if (status_id == LAVA_BATTLE_STATUS_PROTECT) return "护体";
   if (status_id == LAVA_BATTLE_STATUS_HASTE) return "神行";
   if (status_id == LAVA_BATTLE_STATUS_DUALATTACK) return "连击";
   return "状态";
}

char *PAL_LavaBattleStatusShortName(int status_id)
{
   if (status_id == LAVA_BATTLE_STATUS_CONFUSED) return "CF";
   if (status_id == LAVA_BATTLE_STATUS_SLOW) return "SL";
   if (status_id == LAVA_BATTLE_STATUS_SLEEP) return "SP";
   if (status_id == LAVA_BATTLE_STATUS_SILENCE) return "SI";
   if (status_id == LAVA_BATTLE_STATUS_PUPPET) return "PP";
   if (status_id == LAVA_BATTLE_STATUS_BRAVERY) return "BR";
   if (status_id == LAVA_BATTLE_STATUS_PROTECT) return "PR";
   if (status_id == LAVA_BATTLE_STATUS_HASTE) return "HS";
   if (status_id == LAVA_BATTLE_STATUS_DUALATTACK) return "DA";
   return "";
}

static int PAL_LavaScriptAdjustRoleHP(int player_role, int delta_hp)
{
   int hp;
   int max_hp;
   int next_hp;

   if (player_role < 0)
   {
      return FALSE;
   }

   hp = PAL_LavaRoleWordByArray(9, player_role);
   max_hp = PAL_LavaRoleWordByArray(7, player_role);
   printf("[LAVA][ADJUST_HP] role=%d hp=%d max_hp=%d delta=%d\n",
      player_role, hp, max_hp, delta_hp);
   if (hp < 0)
   {
      hp = 0;
   }
   if (max_hp < 0)
   {
      max_hp = 0;
   }

   next_hp = hp + delta_hp;
   if (next_hp < 0)
   {
      next_hp = 0;
   }
   if (max_hp > 0 && next_hp > max_hp)
   {
      next_hp = max_hp;
   }
   if (next_hp == hp)
   {
      printf("[LAVA][ADJUST_HP] no change hp=%d next=%d\n", hp, next_hp);
      return FALSE;
   }

   PAL_LavaWriteU16((addr)g_lava_data_buf, 9 * 6 * 2 + player_role * 2, next_hp);
   printf("[LAVA][ADJUST_HP] wrote hp=%d at offset=%d\n",
      next_hp, 9 * 6 * 2 + player_role * 2);
   return TRUE;
}

static int PAL_LavaScriptAdjustRoleMP(int player_role, int delta_mp)
{
   int max_mp;
   int mp;
   int next_mp;

   if (player_role < 0)
   {
      return FALSE;
   }

   mp = PAL_LavaRoleWordByArray(10, player_role);
   max_mp = PAL_LavaRoleWordByArray(8, player_role);
   printf("[LAVA][ADJUST_MP] role=%d mp=%d max_mp=%d delta=%d\n",
      player_role, mp, max_mp, delta_mp);
   if (mp < 0)
   {
      mp = 0;
   }
   if (max_mp < 0)
   {
      max_mp = 0;
   }

   next_mp = mp + delta_mp;
   if (next_mp < 0)
   {
      next_mp = 0;
   }
   if (max_mp > 0 && next_mp > max_mp)
   {
      next_mp = max_mp;
   }
   if (next_mp == mp)
   {
      printf("[LAVA][ADJUST_MP] no change mp=%d next=%d\n", mp, next_mp);
      return FALSE;
   }

   PAL_LavaWriteU16((addr)g_lava_data_buf, 10 * 6 * 2 + player_role * 2, next_mp);
   printf("[LAVA][ADJUST_MP] wrote mp=%d at offset=%d\n",
      next_mp, 10 * 6 * 2 + player_role * 2);
   return TRUE;
}

static int PAL_LavaResolveFacingScriptObject(long script_index)
{
   char entry[8];
   long idx;
   int steps;

   idx = script_index;
   steps = 0;
   while (idx > 0 && steps < 32)
   {
      long op;
      long a;
      long b;
      long c;
      addr evt;
      int facing_rel_x;
      int facing_rel_y;

      if (!PAL_LavaReadScriptEntry(idx, (addr)entry))
      {
         return 0;
      }

      op = PAL_LavaReadU16((addr)entry, 0);
      if (op != 0x0081)
      {
         return 0;
      }

      a = PAL_LavaReadU16((addr)entry, 2);
      b = PAL_LavaReadU16((addr)entry, 4);
      c = PAL_LavaReadU16((addr)entry, 6);
      evt = PAL_LavaSceneEventData((int)a);
      if (a >= g_lava_scene_event_first && a <= g_lava_scene_event_last &&
          evt != 0 && PAL_LavaReadS16(evt, 12) > 0)
      {
         facing_rel_x = PAL_LavaReadU16(evt, 2);
         facing_rel_y = PAL_LavaReadU16(evt, 4);
         facing_rel_x += (g_lava_party_direction == kDirWest || g_lava_party_direction == kDirSouth) ? 16 : -16;
         facing_rel_y += (g_lava_party_direction == kDirWest || g_lava_party_direction == kDirNorth) ? 8 : -8;
         facing_rel_x -= g_lava_party_x;
         facing_rel_y -= g_lava_party_y;
         if (abs(facing_rel_x) + abs(facing_rel_y * 2) < (int)b * 32 + 16)
         {
            return (int)a;
         }
      }

      idx = c;
      steps++;
   }

   return 0;
}

static long PAL_LavaRunTriggerScript(long script_index, int object_id)
{
   char entry[8];
   long idx;
   int steps;
   int dialog_started;
   long next_entry;
   long persist_entry;
   addr evt;
   int target_x;
   int target_y;
   int resolved_last_object;

   if (script_index <= 0)
   {
      return script_index;
   }

   resolved_last_object = 0;
   if (object_id == -1 || object_id == 0xFFFF)
   {
      object_id = g_lava_last_event_object;
      resolved_last_object = 1;
   }
   else if (object_id > 0)
   {
      g_lava_last_event_object = object_id;
   }

   if (resolved_last_object && object_id > 0)
   {
      int facing_object_id;

      facing_object_id = PAL_LavaResolveFacingScriptObject(script_index);
      if (facing_object_id > 0)
      {
         object_id = facing_object_id;
         g_lava_last_event_object = object_id;
      }
   }

   idx = script_index;
   next_entry = script_index;
   persist_entry = script_index;
   steps = 0;
   dialog_started = 0;
   g_lava_dialog_event_count = 0;
   g_lava_script_success = 1;

   while (steps < 256)
   {
      long op;
      long a;
      long b;
      long c;
      int idle;

      if (!PAL_LavaReadScriptEntry(idx, (addr)entry))
      {
         return next_entry;
      }

      op = PAL_LavaReadU16((addr)entry, 0);
      a = PAL_LavaReadU16((addr)entry, 2);
      b = PAL_LavaReadU16((addr)entry, 4);
      c = PAL_LavaReadU16((addr)entry, 6);

      if (g_lava_autotest_search || g_lava_autotest_load)
      {
         printf("[LAVA][TRIGGERSTEP] object=%d idx=%d op=%d a=%d b=%d c=%d\n",
            object_id, idx, (int)op, (int)a, (int)b, (int)c);
      }

      if (op == 0x0000)
      {
         break;
      }
      else if (op >= 0x000B && op <= 0x000E)
      {
          if (g_lava_role_script == 0)
          {
             evt = PAL_LavaSceneEventData(object_id);
             if (evt != 0)
             {
                PAL_LavaSceneWalkEventOneStep(evt, (int)(op - 0x000B), 2);
             }
          }
          idx++;
          next_entry = idx;
       }
      else if (op == 0x0001)
      {
         next_entry = idx + 1;
         persist_entry = next_entry;
         break;
      }
      else if (op == 0x0002)
      {
         evt = PAL_LavaSceneEventData(object_id);
         if (evt != 0)
         {
            idle = PAL_LavaReadU16(evt, 24);
            if (b == 0 || ++idle < b)
            {
               PAL_LavaWriteU16(evt, 24, idle);
               next_entry = (int)a;
               persist_entry = next_entry;
               break;
            }
            else
            {
               PAL_LavaWriteU16(evt, 24, 0);
               idx++;
               next_entry = idx;
            }
         }
         else
         {
            next_entry = (int)a;
            persist_entry = next_entry;
            break;
         }
      }
      else if (op == 0x0003)
      {
         evt = PAL_LavaSceneEventData(object_id);
         if (evt != 0)
         {
            idle = PAL_LavaReadU16(evt, 24);
            if (b == 0 || ++idle < b)
            {
               PAL_LavaWriteU16(evt, 24, idle);
               idx = a;
               next_entry = (int)a;
               continue;
            }
            else
            {
               PAL_LavaWriteU16(evt, 24, 0);
               idx++;
               next_entry = idx;
            }
         }
         else
         {
            idx = a;
            next_entry = (int)a;
         }
      }
      else if (op == 0x0005)
      {
         if (!g_lava_fast_script_probe && dialog_started && g_lava_dialog_event_count < LAVA_DIALOG_EVENT_MAX)
         {
            PAL_LavaEnsureDialogReplaySnapshot();
            g_lava_dialog_event_type[g_lava_dialog_event_count] = LAVA_DIALOG_EVENT_RESTORE;
            g_lava_dialog_event_a[g_lava_dialog_event_count] = 0;
            g_lava_dialog_event_b[g_lava_dialog_event_count] = 0;
            g_lava_dialog_event_c[g_lava_dialog_event_count] = 0;
            g_lava_dialog_event_count++;
         }
         idx++;
         next_entry = idx;
      }
      else if (op == 0x0007)
      {
         int battle_result;

         if (g_lava_dialog_event_count > 0)
         {
            g_lava_dialog_before_battle = 1;
            PAL_LavaRunPendingDialog();
         }

         if (g_lava_autotest_search || g_lava_autotest_load)
         {
            printf("[LAVA][BATTLEOP] object=%d idx=%d team=%d win_jump=%d flee_jump=%d boss=%d\n",
               object_id, (int)idx, (int)a, (int)b, (int)c, c == 0 ? 1 : 0);
         }
         battle_result = PAL_StartBattle((WORD)a, c == 0 ? 1 : 0);
         if (g_lava_autotest_search || g_lava_autotest_load)
         {
            printf("[LAVA][BATTLEOP] result=%d next=%d\n", battle_result, (int)idx + 1);
         }
         if (battle_result == 1 && b != 0)
         {
            idx = b;
            next_entry = idx;
         }
         else if (battle_result == 0xFFFF && c != 0)
         {
            idx = c;
            next_entry = idx;
         }
         else
         {
            idx++;
            next_entry = idx;
         }
       }
      else if (op == 0x0008)
      {
         idx++;
         next_entry = idx;
         persist_entry = next_entry;
      }
      else if (op == 0x0009)
      {
         int wait_frames;
         int wait_i;

         wait_frames = (a != 0) ? (int)a : 1;
         for (wait_i = 0; wait_i < wait_frames; wait_i++)
         {
            PAL_LavaAdvanceScriptFrame();
         }
         idx++;
         next_entry = idx;
      }
      else if (op == 0x0004)
      {
         PAL_LavaRunTriggerScript((long)a, b == 0 ? object_id : (int)b);
         idx++;
         next_entry = idx;
         }
      else if (op == 0x000F)
      {
          if (g_lava_role_script == 0)
          {
             evt = PAL_LavaResolveEventTarget(object_id, object_id);
             if (evt != 0)
             {
                if (a != 0xFFFF)
                {
                   PAL_LavaWriteU16(evt, 20, (int)a);
                }
                if (b != 0xFFFF)
                {
                   PAL_LavaWriteU16(evt, 22, (int)b);
                }
               if (!g_lava_suppress_trigger_visual_events)
               {
                  PAL_LavaQueueDialogEvent(LAVA_DIALOG_EVENT_EVENT_POSE,
                     object_id, PAL_LavaReadU16(evt, 20), PAL_LavaReadU16(evt, 22));
               }
             }
          }
          idx++;
         next_entry = idx;
       }
      else if (op == 0x0011)
      {
          if (g_lava_role_script)
          {
             idx++;
             next_entry = idx;
          }
          else
          {
             evt = PAL_LavaResolveEventTarget(object_id, object_id);
             if (evt != 0)
             {
                if (((object_id & 1) ^ (g_lava_logic_frame_num & 1)) == 0)
                {
                   PAL_LavaAdvanceScriptFrame();
                   next_entry = idx;
                }
                else if (PAL_LavaSceneWalkEventObjectSpeed(evt, a, b, c, 2))
                {
                   idx++;
                   next_entry = idx;
                }
                else
                {
                   PAL_LavaAdvanceScriptFrame();
                   next_entry = idx;
                }
             }
             else
             {
                idx++;
                next_entry = idx;
             }
          }
       }
      else if (op == 0x0010)
      {
          if (g_lava_role_script)
          {
             idx++;
             next_entry = idx;
          }
          else
          {
             evt = PAL_LavaResolveEventTarget(object_id, object_id);
             if (evt != 0)
             {
                if (PAL_LavaSceneWalkEventObject(evt, a, b, c))
                {
                   idx++;
                   next_entry = idx;
                }
                else
                {
                   PAL_LavaAdvanceScriptFrame();
                   next_entry = idx;
                }
             }
             else
             {
                idx++;
                next_entry = idx;
             }
          }
       }
      else if (op == 0x0015)
      {
         g_lava_party_direction = a;
         g_lava_party_frame = b;
         idx++;
         next_entry = idx;
      }
      else if (op == 0x0014)
      {
          if (g_lava_role_script == 0)
          {
             evt = PAL_LavaSceneEventData(object_id);
             if (evt != 0)
             {
                PAL_LavaWriteU16(evt, 22, (int)a);
                PAL_LavaWriteU16(evt, 20, 0);
                if (!g_lava_suppress_trigger_visual_events)
                {
                   PAL_LavaQueueDialogEvent(LAVA_DIALOG_EVENT_EVENT_GESTURE,
                      object_id, (int)a, 0);
                }
             }
          }
          idx++;
         next_entry = idx;
      }
       else if (op == 0x0013)
       {
           if (g_lava_role_script == 0)
           {
             evt = PAL_LavaResolveEventTarget(object_id, (int)a);
             if (evt != 0)
             {
                PAL_LavaWriteU16(evt, 2, (int)b);
                PAL_LavaWriteU16(evt, 4, (int)c);
                if (!g_lava_suppress_trigger_visual_events)
                {
                   PAL_LavaQueueDialogEvent(LAVA_DIALOG_EVENT_EVENT_POS,
                      (a == 0 || a == 0xFFFF) ? object_id : (int)a, (int)b, (int)c);
                }
             }
          }
          idx++;
         next_entry = idx;
      }
      else if (op == 0x0016)
      {
          if (g_lava_role_script == 0)
          {
             evt = PAL_LavaResolveEventTarget(object_id, (int)a);
             if (evt != 0)
             {
                PAL_LavaWriteU16(evt, 20, (int)b);
                PAL_LavaWriteU16(evt, 22, (int)c);
                if (!g_lava_suppress_trigger_visual_events)
                {
                   PAL_LavaQueueDialogEvent(LAVA_DIALOG_EVENT_EVENT_POSE,
                      (a == 0 || a == 0xFFFF) ? object_id : (int)a, (int)b, (int)c);
                }
             }
          }
           idx++;
          next_entry = idx;
       }
      else if (op == 0x0030)
      {
         int target_role;

         target_role = c == 0 ? object_id : (int)c - 1;
         PAL_LavaSetRoleTempStatPercent((int)a, target_role, (int)b);
         idx++;
         next_entry = idx;
      }
      else if (op == 0x001B)
      {
         printf("[LAVA][OP001B] a=%d b=%d object_id=%d delta=%d\n",
            a, b, object_id, PAL_LavaReadS16((addr)entry, 4));
         if (a != 0)
         {
            int any_changed;
            int i;
            int role;

            any_changed = FALSE;
            g_lava_script_success = FALSE;
            for (i = 0; i < g_lava_party_count && i < 3; i++)
            {
               role = g_lava_party_role[i];
               if (PAL_LavaScriptAdjustRoleHP(role, PAL_LavaReadS16((addr)entry, 4)))
               {
                  any_changed = TRUE;
               }
            }
            if (any_changed)
            {
               g_lava_script_success = TRUE;
            }
         }
         else if (!PAL_LavaScriptAdjustRoleHP(object_id, PAL_LavaReadS16((addr)entry, 4)))
         {
            g_lava_script_success = FALSE;
         }
         idx++;
         next_entry = idx;
       }
      else if (op == 0x001C)
      {
         if (a != 0)
         {
            int any_changed_mp;
            int i_mp;
            int role_mp;

            any_changed_mp = FALSE;
            g_lava_script_success = FALSE;
            for (i_mp = 0; i_mp < g_lava_party_count && i_mp < 3; i_mp++)
            {
               role_mp = g_lava_party_role[i_mp];
               if (PAL_LavaScriptAdjustRoleMP(role_mp, PAL_LavaReadS16((addr)entry, 4)))
               {
                  any_changed_mp = TRUE;
               }
            }
            if (any_changed_mp)
            {
               g_lava_script_success = TRUE;
            }
         }
         else if (!PAL_LavaScriptAdjustRoleMP(object_id, PAL_LavaReadS16((addr)entry, 4)))
         {
            g_lava_script_success = FALSE;
         }
         idx++;
         next_entry = idx;
       }
      else if (op == 0x001D)
      {
         if (a != 0)
         {
            int any_changed_hpmp;
            int delta_hpmp;
            int i_hpmp;
            int role_hpmp;

            any_changed_hpmp = FALSE;
            delta_hpmp = PAL_LavaReadS16((addr)entry, 4);
            g_lava_script_success = FALSE;
            for (i_hpmp = 0; i_hpmp < g_lava_party_count && i_hpmp < 3; i_hpmp++)
            {
               role_hpmp = g_lava_party_role[i_hpmp];
               if (PAL_LavaScriptAdjustRoleHP(role_hpmp, delta_hpmp) ||
                   PAL_LavaScriptAdjustRoleMP(role_hpmp, delta_hpmp))
               {
                  any_changed_hpmp = TRUE;
               }
            }
            if (any_changed_hpmp)
            {
               g_lava_script_success = TRUE;
            }
         }
         else
         {
            int delta_one_hpmp;

            delta_one_hpmp = PAL_LavaReadS16((addr)entry, 4);
            if (!PAL_LavaScriptAdjustRoleHP(object_id, delta_one_hpmp) &&
                !PAL_LavaScriptAdjustRoleMP(object_id, delta_one_hpmp))
            {
               g_lava_script_success = FALSE;
            }
         }
         idx++;
         next_entry = idx;
       }
      else if (op == 0x002D)
      {
         if (!PAL_LavaBattleSetPlayerStatus(object_id, (int)a, (int)b))
         {
            g_lava_script_success = FALSE;
         }
         idx++;
         next_entry = idx;
      }
      else if (op == 0x002F)
      {
         PAL_LavaBattleRemovePlayerStatus(object_id, (int)a);
         idx++;
         next_entry = idx;
      }
      else if (op == 0x0031)
      {
         PAL_LavaBattleSetPlayerTransformSprite(object_id, (int)a);
         idx++;
         next_entry = idx;
      }
      else if (op == 0x003C)
      {
         if (g_lava_dialog_event_count < LAVA_DIALOG_EVENT_MAX)
         {
            PAL_LavaEnsureDialogReplaySnapshot();
            g_lava_dialog_event_type[g_lava_dialog_event_count] = LAVA_DIALOG_EVENT_START;
            g_lava_dialog_event_a[g_lava_dialog_event_count] = LAVA_DIALOG_UPPER;
            g_lava_dialog_event_b[g_lava_dialog_event_count] = a;
            g_lava_dialog_event_c[g_lava_dialog_event_count] = (b == 0) ? -1 : b;
            g_lava_dialog_event_count++;
         }
         dialog_started = 1;
         idx++;
         next_entry = idx;
      }
      else if (op == 0x003D)
      {
         if (g_lava_dialog_event_count < LAVA_DIALOG_EVENT_MAX)
         {
            PAL_LavaEnsureDialogReplaySnapshot();
            g_lava_dialog_event_type[g_lava_dialog_event_count] = LAVA_DIALOG_EVENT_START;
            g_lava_dialog_event_a[g_lava_dialog_event_count] = LAVA_DIALOG_LOWER;
            g_lava_dialog_event_b[g_lava_dialog_event_count] = a;
            g_lava_dialog_event_c[g_lava_dialog_event_count] = (b == 0) ? -1 : b;
            g_lava_dialog_event_count++;
         }
         dialog_started = 1;
         idx++;
         next_entry = idx;
      }
      else if (op == 0x003E)
      {
         if (g_lava_dialog_event_count < LAVA_DIALOG_EVENT_MAX)
         {
            PAL_LavaEnsureDialogReplaySnapshot();
            g_lava_dialog_event_type[g_lava_dialog_event_count] = LAVA_DIALOG_EVENT_START;
            g_lava_dialog_event_a[g_lava_dialog_event_count] = LAVA_DIALOG_CENTER;
            g_lava_dialog_event_b[g_lava_dialog_event_count] = 0;
            g_lava_dialog_event_c[g_lava_dialog_event_count] = 0x4F;
            g_lava_dialog_event_count++;
         }
         dialog_started = 1;
         idx++;
         next_entry = idx;
      }
      else if (op == 0x003B)
      {
         if (g_lava_dialog_event_count < LAVA_DIALOG_EVENT_MAX)
         {
            PAL_LavaEnsureDialogReplaySnapshot();
            g_lava_dialog_event_type[g_lava_dialog_event_count] = LAVA_DIALOG_EVENT_START;
            g_lava_dialog_event_a[g_lava_dialog_event_count] = LAVA_DIALOG_CENTER;
            g_lava_dialog_event_b[g_lava_dialog_event_count] = 0;
            g_lava_dialog_event_c[g_lava_dialog_event_count] = 0x4F;
            g_lava_dialog_event_count++;
         }
         dialog_started = 1;
         idx++;
         next_entry = idx;
      }
      else if (op == 0x0040)
      {
          if (g_lava_role_script == 0)
          {
             evt = PAL_LavaResolveEventTarget(object_id, (int)a);
             if (evt != 0)
             {
                PAL_LavaWriteU16(evt, 14, (int)b);
             }
          }
          idx++;
         next_entry = idx;
      }
      else if (op == 0x0043)
      {
         idx++;
         next_entry = idx;
      }
      else if (op == 0x0044)
      {
         evt = PAL_LavaResolveEventTarget(object_id, object_id);
         PAL_LavaRidePartyOnEventObject(object_id, evt, (int)a, (int)b, (int)c, 4);
         idx++;
         next_entry = idx;
      }
      else if (op == 0x0046)
      {
         g_lava_party_x = a * 32 + c * 16;
         g_lava_party_y = b * 16 + c * 8;
         idx++;
         next_entry = idx;
      }
      else if (op == 0x0047)
      {
         idx++;
         next_entry = idx;
      }
      else if (op == 0x0049)
      {
          evt = PAL_LavaResolveEventTarget(object_id, (int)a);
          if (evt != 0)
          {
            PAL_LavaWriteS16(evt, 12, (int)b);
            if (!g_lava_suppress_trigger_visual_events)
            {
               PAL_LavaQueueDialogEvent(LAVA_DIALOG_EVENT_EVENT_STATE,
                  (a == 0 || a == 0xFFFF) ? object_id : (int)a, (int)b, 0);
            }
            printf("[LAVA][TRIGGER] set state target=%d value=%d current=%d\n",
               (a == 0 || a == 0xFFFF) ? object_id : (int)a,
               (int)b,
               PAL_LavaReadS16(evt, 12));
         }
          idx++;
          next_entry = idx;
       }
      else if (op == 0x0038)
      {
         WORD teleport_script;

         teleport_script = PAL_LavaSceneTeleportScriptFor(g_lava_scene_num);
         if (teleport_script != 0)
         {
            PAL_LavaRunTriggerScript((long)teleport_script, object_id);
            if (g_lava_scene_num != 0)
            {
               idx++;
               next_entry = idx;
               persist_entry = next_entry;
            }
            else
            {
               idx = (int)a;
               next_entry = idx;
            }
         }
         else
         {
            idx = (int)a;
            next_entry = idx;
         }
      }
      else if (op == 0x004A)
      {
         g_lava_num_battle_field = a;
         idx++;
         next_entry = idx;
      }
      else if (op == 0x0045)
      {
         idx++;
         next_entry = idx;
      }
      else if (op == 0x0059)
      {
          if (a > 0 && a != g_lava_scene_num)
          {
             g_lava_defer_scene_preview_draw = 1;
             PAL_LavaSetScene((int)a);
             g_lava_defer_scene_preview_draw = 0;
          }
          idx++;
          next_entry = idx;
          /*
           * Door/scene-switch shell scripts usually continue into a local
           * fade-out tail, but the owning object's trigger should remain at
           * the original entry so the doorway is reusable on re-entry.
           */
          }
      else if (op == 0x0077)
      {
         idx++;
         next_entry = idx;
       }
      else if (op == 0x006D)
      {
         PAL_LavaSetSceneScriptHooks((int)a, (int)b, (int)c);
         if (g_lava_autotest_search)
         {
            printf("[LAVA][TRIGGER] scenehook scene=%d enter=%d teleport=%d\n",
               (int)a, (int)b, (int)c);
         }
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
         next_entry = idx;
       }
      else if (op == 0x0065)
      {
         int party_index;

         if (a >= 0 && a < 6)
         {
            PAL_LavaSetRoleSpriteNum((int)a, (int)b);
            for (party_index = 0; party_index < g_lava_party_count && party_index < 3; party_index++)
            {
               if (g_lava_party_role[party_index] == (int)a)
               {
                  g_lava_player_sprite_num[party_index] = b;
               }
            }
            if (c != 0)
            {
               PAL_LavaRefreshPartySprites();
            }
         }
         idx++;
         next_entry = idx;
      }
      else if (op == 0x006E)
      {
         int step_x;
         int step_y;

         step_x = PAL_LavaReadS16((addr)entry, 2);
         step_y = PAL_LavaReadS16((addr)entry, 4);
         g_lava_view_x += step_x;
         g_lava_view_y += step_y;
         if (g_lava_view_x < 0) g_lava_view_x = 0;
         if (g_lava_view_y < 0) g_lava_view_y = 0;
         g_lava_party_layer = c * 8;
         PAL_LavaSyncPartyToViewport();
         PAL_LavaApplyPartyStepDelta(step_x, step_y);
         if (g_lava_autotest_search || g_lava_autotest_walk || g_lava_autotest_exits || g_lava_autotest_scene5 || g_lava_autotest_scene6 || g_lava_autotest_scene13 || g_lava_autotest_scene9 || g_lava_autotest_hooks)
         {
            printf("[LAVA][STEP] op=006E view=(%d,%d) party=(%d,%d) delta=(%d,%d) dir=%d frame=%d layer=%d\n",
               g_lava_view_x, g_lava_view_y,
               g_lava_party_x, g_lava_party_y,
               step_x, step_y,
               g_lava_party_direction, g_lava_party_frame, g_lava_party_layer);
         }
         idx++;
         next_entry = idx;
      }
      else if (op == 0x006F)
      {
         addr current_evt;

         current_evt = PAL_LavaResolveEventTarget(object_id, (int)a);
         evt = PAL_LavaSceneEventData(object_id);
         if (current_evt != 0 && evt != 0 &&
             PAL_LavaReadS16(current_evt, 12) == (int)b)
         {
            PAL_LavaWriteS16(evt, 12, (int)b);
         }
         idx++;
         next_entry = idx;
      }
      else if (op == 0x0070)
      {
         target_x = a * 32 + c * 16;
         target_y = b * 16 + c * 8;
         PAL_LavaWalkPartyToTarget(target_x, target_y);
         idx++;
         next_entry = idx;
        }
      else if (op == 0x0075)
      {
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
         next_entry = idx;
      }
      else if (op == 0x007A)
      {
         target_x = a * 32 + c * 16;
         target_y = b * 16 + c * 8;
         PAL_LavaWalkPartyToTargetSpeed(target_x, target_y, 8, 4, "007A");
         idx++;
         next_entry = idx;
      }
      else if (op == 0x001F)
      {
         int delta;
         int total;

         delta = PAL_LavaReadS16((addr)entry, 4);
         if (delta == 0)
         {
            delta = 1;
         }
         total = PAL_LavaAddItemToInventory((int)a, delta);
         printf("[LAVA][TRIGGER] add item=%d delta=%d total=%d\n", (int)a, delta, total);
         idx++;
         next_entry = idx;
       }
      else if (op == 0x0020)
      {
         int remove_count;
         int remove_total;

         remove_count = (int)b;
         if (remove_count == 0)
         {
            remove_count = 1;
         }
         if (remove_count <= PAL_LavaGetItemAmount((int)a) || c == 0)
         {
            remove_total = PAL_LavaAddItemToInventory((int)a, -remove_count);
            printf("[LAVA][TRIGGER] remove item=%d count=%d total=%d\n",
               (int)a, remove_count, remove_total);
            idx++;
         }
         else
         {
            idx = c;
            g_lava_script_success = FALSE;
         }
         next_entry = idx;
      }
      else if (op == 0x001E)
      {
         idle = PAL_LavaReadS16((addr)entry, 2);
         if (idle < 0)
         {
            c = 0 - idle;
            if (g_lava_cash < c)
            {
               idx = (int)b;
            }
            else
            {
               g_lava_cash += idle;
               idx++;
            }
         }
         else
         {
            g_lava_cash += idle;
            idx++;
           }
         next_entry = idx;
        }
      else if (op == 0x0024)
      {
         int target_id;

         target_id = (a == 0 || a == 0xFFFF) ? object_id : (int)a;
         evt = PAL_LavaResolveEventTarget(object_id, target_id);
         if (evt != 0)
         {
            PAL_LavaWriteU16(evt, 10, (int)b);
            g_lava_autoscript_pc[target_id - 1] = 0;
            g_lava_autoscript_idle[target_id - 1] = 0;
          }
          idx++;
          next_entry = idx;
       }
      else if (op == 0x0025)
      {
          evt = PAL_LavaResolveEventTarget(object_id, (a == 0 || a == 0xFFFF) ? object_id : (int)a);
          if (evt != 0)
          {
             PAL_LavaWriteU16(evt, 8, (int)b);
          }
          idx++;
          next_entry = idx;
       }
      else if (op == 0x0050)
      {
         VIDEO_UpdateScreen(0);
         PAL_FadeOut(a ? (int)a : 1);
         g_lava_gpGlobals.fNeedToFadeIn = TRUE;
         idx++;
         next_entry = idx;
      }
      else if (op == 0x0051)
      {
         VIDEO_UpdateScreen(0);
         PAL_SetPalette(g_lava_gpGlobals.wNumPalette, g_lava_gpGlobals.fNightPalette);
         PAL_FadeIn(a ? (int)a : 1);
         g_lava_gpGlobals.fNeedToFadeIn = FALSE;
         idx++;
         next_entry = idx;
      }
      else if (op == 0x0053)
      {
         g_lava_gpGlobals.fNightPalette = FALSE;
         idx++;
         next_entry = idx;
      }
      else if (op == 0x0081)
      {
         addr facing_current_evt;
         addr facing_target_evt;
         int facing_rel_x;
         int facing_rel_y;

         facing_current_evt = PAL_LavaSceneEventData(object_id);
         facing_target_evt = PAL_LavaSceneEventData((int)a);
         if (a < g_lava_scene_event_first || a > g_lava_scene_event_last ||
             facing_current_evt == 0 || facing_target_evt == 0)
         {
            idx = c;
            g_lava_script_success = FALSE;
         }
         else
         {
            facing_rel_x = PAL_LavaReadU16(facing_current_evt, 2);
            facing_rel_y = PAL_LavaReadU16(facing_current_evt, 4);
            facing_rel_x += (g_lava_party_direction == kDirWest || g_lava_party_direction == kDirSouth) ? 16 : -16;
            facing_rel_y += (g_lava_party_direction == kDirWest || g_lava_party_direction == kDirNorth) ? 8 : -8;
            facing_rel_x -= g_lava_party_x;
            facing_rel_y -= g_lava_party_y;
            if (abs(facing_rel_x) + abs(facing_rel_y * 2) < (int)b * 32 + 16 && PAL_LavaReadS16(facing_target_evt, 12) > 0)
            {
               if (b > 0)
               {
                  PAL_LavaWriteU16(facing_current_evt, 14, 2 + (int)b);
               }
               idx++;
            }
            else
            {
               idx = c;
               g_lava_script_success = FALSE;
            }
         }
         next_entry = idx;
      }
       else if (op == 0x0054)
       {
          g_lava_gpGlobals.fNightPalette = TRUE;
          idx++;
          next_entry = idx;
       }
       else if (op == 0x0055)
       {
          int lava_magic_role;

          lava_magic_role = (int)b;
          if (lava_magic_role == 0)
          {
             lava_magic_role = object_id;
          }
          else
          {
             lava_magic_role--;
          }
          if (lava_magic_role >= 0 && lava_magic_role < 6)
          {
             PAL_LavaAddRoleMagicToDataBuf(lava_magic_role, (int)a);
          }
          idx++;
          next_entry = idx;
       }
      else if (op == 0x0052)
      {
         evt = PAL_LavaSceneEventData(object_id);
         if (evt != 0)
         {
            PAL_LavaWriteS16(evt, 12, -PAL_LavaReadS16(evt, 12));
            PAL_LavaWriteU16(evt, 0, a ? (int)a : 800);
         }
         idx++;
         next_entry = idx;
       }
       else if (op == 0x0058)
       {
          if (PAL_LavaGetItemAmount((int)a) < (int)b)
          {
             idx = (int)c;
          }
          else
          {
             idx++;
          }
          next_entry = idx;
       }
      else if (op == 0x006C)
      {
         evt = PAL_LavaResolveEventTarget(object_id, (int)a);
         if (evt != 0)
         {
            PAL_LavaWriteU16(evt, 2, PAL_LavaReadU16(evt, 2) + PAL_LavaReadS16((addr)entry, 4));
            PAL_LavaWriteU16(evt, 4, PAL_LavaReadU16(evt, 4) + PAL_LavaReadS16((addr)entry, 6));
            PAL_LavaAdvanceEventFrame(evt);
         }
         idx++;
         next_entry = idx;
      }
      else if (op == 0x007D)
      {
         evt = PAL_LavaResolveEventTarget(object_id, (int)a);
         if (evt != 0)
         {
            PAL_LavaWriteU16(evt, 2, PAL_LavaReadU16(evt, 2) + PAL_LavaReadS16((addr)entry, 4));
            PAL_LavaWriteU16(evt, 4, PAL_LavaReadU16(evt, 4) + PAL_LavaReadS16((addr)entry, 6));
            PAL_LavaQueueDialogEvent(LAVA_DIALOG_EVENT_EVENT_POS,
               (a <= 0 || a == 0xFFFF) ? object_id : (int)a,
               PAL_LavaReadU16(evt, 2), PAL_LavaReadU16(evt, 4));
         }
         idx++;
         next_entry = idx;
      }
      else if (op == 0x0085)
      {
         if (!g_lava_fast_script_probe)
         {
            int wait_ms;

            wait_ms = (int)a * 10;
            if (wait_ms > FRAME_TIME)
            {
               wait_ms = FRAME_TIME;
            }
            if (wait_ms > 0)
            {
               UTIL_Delay(wait_ms);
            }
          }
           idx++;
           next_entry = idx;
            }
      else if (op == 0x0088)
      {
         if (g_lava_cash > 5000)
         {
            g_lava_cash -= 5000;
         }
         else
         {
            g_lava_cash = 0;
         }
         idx++;
         next_entry = idx;
      }
      else if (op == 0x008F)
      {
         g_lava_cash /= 2;
         idx++;
         next_entry = idx;
      }
      else if (op == 0x00A1)
      {
         g_lava_party_frame = 0;
         idx++;
         next_entry = idx;
       }
      else if (op == 0x009A)
      {
         int object_index;

         for (object_index = (int)a; object_index <= (int)b; object_index++)
         {
            evt = PAL_LavaSceneEventData(object_index);
            if (evt != 0)
            {
               PAL_LavaWriteS16(evt, 12, (int)c);
            }
         }
          idx++;
          next_entry = idx;
       }
      else if (op == 0x0098)
      {
         g_lava_follower_count = 0;
         if (a > 0 && a <= 6)
         {
            g_lava_follower_role[g_lava_follower_count++] = a - 1;
         }
         if (b > 0 && b <= 6 && g_lava_follower_count < 2)
         {
            g_lava_follower_role[g_lava_follower_count++] = b - 1;
         }
         PAL_LavaRefreshPartySprites();
         PAL_LavaResetPartyTrail();
         idx++;
         next_entry = idx;
      }
      else if (op == 0x008E)
      {
          if (g_lava_dialog_event_count < LAVA_DIALOG_EVENT_MAX)
          {
             PAL_LavaEnsureDialogReplaySnapshot();
             g_lava_dialog_event_type[g_lava_dialog_event_count] = LAVA_DIALOG_EVENT_RESTORE;
             g_lava_dialog_event_a[g_lava_dialog_event_count] = 0;
             g_lava_dialog_event_b[g_lava_dialog_event_count] = 0;
             g_lava_dialog_event_c[g_lava_dialog_event_count] = 0;
             g_lava_dialog_event_count++;
          }
          dialog_started = 1;
          idx++;
          next_entry = idx;
       }
          else if (op == 0xFFFF)
         {
         if (g_lava_autotest_search)
         {
            printf("[LAVA][TRIGGERMSG] object=%d idx=%d msg=%d src=%s\n",
               object_id, idx, (int)a,
               PAL_LavaMsgUsesFallback((int)a) ? "fallback" : "mmsg");
         }
         if (g_lava_dialog_event_count < LAVA_DIALOG_EVENT_MAX && PAL_LavaScene1MsgText(a) != 0)
         {
            PAL_LavaEnsureDialogReplaySnapshot();
            g_lava_dialog_event_type[g_lava_dialog_event_count] = LAVA_DIALOG_EVENT_TEXT;
            g_lava_dialog_event_a[g_lava_dialog_event_count] = a;
            g_lava_dialog_event_b[g_lava_dialog_event_count] = 0;
            g_lava_dialog_event_c[g_lava_dialog_event_count] = 0;
            g_lava_dialog_event_count++;
         }
         else if (PAL_LavaScene1MsgText(a) == 0)
         {
            printf("[LAVA][DIALOG] missing msg=%d\n", (int)a);
         }
         idx++;
         next_entry = idx;
      }
      else if (dialog_started)
      {
         printf("[LAVA][TRIGGER] stop at unknown op=%d a=%d b=%d c=%d (dialog mode)\n",
            (int)op, (int)a, (int)b, (int)c);
         break;
      }
      else
      {
         printf("[LAVA][TRIGGER] skip unknown op=%d a=%d b=%d c=%d\n",
            (int)op, (int)a, (int)b, (int)c);
         idx++;
         next_entry = idx;
      }

      steps++;
   }

      printf("[LAVA][TRIGGER] object=%d next=%d dialog=%d\n",
         object_id, persist_entry, g_lava_dialog_event_count);
      evt = PAL_LavaSceneEventData(object_id);
      if (evt != 0)
      {
         printf("[LAVA][TRIGGER] state object=%d trigger=%d mode=%d state=%d pos=(%d,%d)\n",
            object_id,
            PAL_LavaReadU16(evt, 8),
            PAL_LavaReadU16(evt, 14),
            PAL_LavaReadS16(evt, 12),
            PAL_LavaReadU16(evt, 2),
            PAL_LavaReadU16(evt, 4));
      }
   return persist_entry;
}
