// ==================== ai_xqwlight_search.lava ====================
// xqwlight AI - 搜索引擎 (全局状态版)
// 优化版 32767置换表+ 16384历史表+ 48层杀手走法(~663KB)

// ==================== 搜索引擎全局状态 ====================

/* 将 xqwlight 内部坐标转换为主游戏棋盘坐标 */
/* xqwlight: sq = (col+3) + ((row+3)<<4), 主游戏: row=0-9, col=0-8 */
int xq_sq_to_row(int sq)
{
    return (sq >> 4) - 3;
}

int xq_sq_to_col(int sq)
{
    return (sq & 15) - 3;
}

/* 棋子编码转中文名 (支持 xqwlight 编码和主游戏编码) */
/* xqwlight: 红方 8-14, 黑方 16-22 */
/* 主游戏: 红方 1-7, 黑方 8-14 (但适配器会转换为 xqwlight 编码) */
char* piece_code_to_name(int pc)
{
    /* 空位 */
    if (pc == 0) return "空位";

    /* xqwlight 黑方编码 (16-22) - 优先判断 */
    if (pc == 16) return "黑将";
    if (pc == 17) return "黑士";
    if (pc == 18) return "黑象";
    if (pc == 19) return "黑马";
    if (pc == 20) return "黑车";
    if (pc == 21) return "黑炮";
    if (pc == 22) return "黑卒";

    /* xqwlight 红方编码 (8-14) */
    if (pc == 8) return "红帅";
    if (pc == 9) return "红士";
    if (pc == 10) return "红相";
    if (pc == 11) return "红马";
    if (pc == 12) return "红车";
    if (pc == 13) return "红炮";
    if (pc == 14) return "红兵";

    /* 主游戏编码 (1-7 红方) - 理论上不应该出现 */
    if (pc == 1) return "红车";
    if (pc == 2) return "红马";
    if (pc == 3) return "红相";
    if (pc == 4) return "红士";
    if (pc == 5) return "红帅";
    if (pc == 6) return "红炮";
    if (pc == 7) return "红兵";

    return "未知";
}

int g_hashMask;
long g_mvResult;
long g_allNodes;
long g_hashHits;
int g_stopFlag;

// 时间控制全局变量
int g_ai_timeout_seconds;     // AI 思考超时时间（秒）
int g_ai_start_second;        // 开始思考时的秒数
char g_ai_time_buf[8];        // 时间缓冲区
int g_ai_remaining_seconds;   // 剩余秒数

// 置换表 - 最大优化版: 32767条目 (约176KB)
// 1MB RAM 限制下最大化置换表以提升剪枝效率
#define HASH_ENTRIES 32767
char g_hashDepth[32767];
char g_hashFlag[32767];
int g_hashVl[32767];
long g_hashMv[32767];
long g_hashLockLow[32767];
long g_hashLockHigh[32767];

// 历史启发表 - 扩展到 16384 (64KB)
int g_historyTable[16384];

// 杀手走法 - 扩展到 48 层
#define KILLER_DEPTH 48
long g_mvKiller[KILLER_DEPTH][2];

// 排序器全局状态 (替代 SortItem 结构)
int g_sort_index_stack[LIMIT_DEPTH];
int g_sort_moves_stack[LIMIT_DEPTH];
int g_sort_phase_stack[LIMIT_DEPTH];
int g_sort_singleReply_stack[LIMIT_DEPTH];
long g_sort_mvHash_stack[LIMIT_DEPTH];
long g_sort_mvKiller1_stack[LIMIT_DEPTH];
long g_sort_mvKiller2_stack[LIMIT_DEPTH];
long g_sort_mvs_stack[LIMIT_DEPTH][MAX_GEN_MOVES];
int g_sort_vls_stack[LIMIT_DEPTH][MAX_GEN_MOVES];

// 静态搜索全局数组 (避免栈溢出)
long g_qmvs[MAX_GEN_MOVES];
int g_qvls[MAX_GEN_MOVES];

// 静态搜索全局变量 (减少栈使用)
int g_q_vl, g_q_vlBest, g_q_genMoves, g_q_i;
long g_q_mv;

// 静态搜索递归保存栈 (按 g_pos_distance 分层)
int g_q_vlBest_stack[LIMIT_DEPTH];

// 完整搜索全局变量 (减少栈使用)
int g_f_vl, g_f_vlBest, g_f_hashFlag, g_f_newDepth, g_f_numMvs, g_f_i;
long g_f_mvBest, g_f_mvHash, g_f_mv;

// 完整搜索递归保存栈 (按 g_pos_distance 分层)
int g_f_vlBest_stack[LIMIT_DEPTH];
int g_f_hashFlag_stack[LIMIT_DEPTH];
long g_f_mvBest_stack[LIMIT_DEPTH];

// 根节点搜索全局变量 (减少栈使用)
int g_r_vlBest, g_r_vl, g_r_newDepth, g_r_numMvs, g_r_i;
long g_r_mv;


// 走法备份 (递归安全，searchFull 不会修改此数组)

// searchFull 专用走法数组 (与searchRoot/searchUnique 隔离, 通过 g_full_stack 实现递归安全)

// searchFull 递归保存区 (按 g_pos_distance 分层, 每层 64 个 long, 最大 12 层)

// ==================== 前向声明 ====================

int checkTimeout();
int searchQuiesc(int vlAlpha, int vlBeta);
int searchFull(int vlAlpha, int vlBeta, int depth, int noNull);
int searchNoNull(int vlAlpha, int vlBeta, int depth);

// ==================== 置换表函数 ====================

int getHashSlot()
{
    long key;
    int slot;

    key = g_pos_zobristKey;
    if (key < 0) {
        key = key & 0x7fffffff;
    }
    slot = key % HASH_ENTRIES;
    return slot;
}

int probeHash(int vlAlpha, int vlBeta, int depth)
{
    int slot;
    int mateFlag;
    int vl;

    slot = getHashSlot();

    // 检查锁
    if (g_hashLockLow[slot] != g_pos_zobristLock) {
        return -MATE_VALUE;
    }
    if (g_hashLockHigh[slot] != 0) {
        return -MATE_VALUE;
    }

    g_f_mvHash = g_hashMv[slot];
    mateFlag = 0;

    vl = g_hashVl[slot];
    if (vl > WIN_VALUE) {
        if (vl <= BAN_VALUE) {
            return -MATE_VALUE;
        }
        vl = vl - g_pos_distance;
        mateFlag = 1;
    } else {
        if (vl < -WIN_VALUE) {
            if (vl >= -BAN_VALUE) {
                return -MATE_VALUE;
            }
            vl = vl + g_pos_distance;
            mateFlag = 1;
        } else {
            if (vl == positionDrawValue()) {
                return -MATE_VALUE;
            }
        }
    }

    if (g_hashDepth[slot] >= depth) {
        if (g_hashFlag[slot] == HASH_BETA) {
            if (vl >= vlBeta) {
                g_hashHits++;
                return vl;
            }
            return -MATE_VALUE;
        }
        if (g_hashFlag[slot] == HASH_ALPHA) {
            if (vl <= vlAlpha) {
                g_hashHits++;
                return vl;
            }
            return -MATE_VALUE;
        }
        g_hashHits++;
        return vl;
    }
    if (mateFlag != 0) {
        g_hashHits++;
        return vl;
    }

    return -MATE_VALUE;
}

void recordHash(int flag, int vl, int depth, long mv)
{
    int slot;

    slot = getHashSlot();
    if (g_hashDepth[slot] > depth) return;

    g_hashFlag[slot] = flag;
    g_hashDepth[slot] = depth;

    if (vl > WIN_VALUE) {
        if (mv == 0) {
            if (vl <= BAN_VALUE) return;
        }
        g_hashVl[slot] = vl + g_pos_distance;
    } else {
        if (vl < -WIN_VALUE) {
            if (mv == 0) {
                if (vl >= -BAN_VALUE) return;
            }
            g_hashVl[slot] = vl - g_pos_distance;
        } else {
            if (vl == positionDrawValue()) {
                if (mv == 0) return;
            }
            g_hashVl[slot] = vl;
        }
    }

    g_hashMv[slot] = mv;
    g_hashLockLow[slot] = g_pos_zobristLock;
    g_hashLockHigh[slot] = 0;
}

void clearHashTable()
{
    int i;
    for (i = 0; i < 32767; i++) {
        g_hashDepth[i] = 0;
        g_hashFlag[i] = 0;
        g_hashVl[i] = 0;
        g_hashMv[i] = 0;
        g_hashLockLow[i] = 0;
        g_hashLockHigh[i] = 0;
    }
}

void clearHistoryAndKillers()
{
    int i;
    for (i = 0; i < 16384; i++) {
        g_historyTable[i] = 0;
    }
    for (i = 0; i < KILLER_DEPTH; i++) {
        g_mvKiller[i][0] = 0;
        g_mvKiller[i][1] = 0;
    }
}

// ==================== 排序函数 ====================

void sortInit(long mvHash)
{
    int level;
    int i, numAll;
    long mv;
    int hIdx;

    level = g_pos_distance;
    if (level >= LIMIT_DEPTH) {
        level = LIMIT_DEPTH - 1;
    }

    if (positionInCheck() == 0) {
        g_sort_phase_stack[level] = 0;
        g_sort_mvHash_stack[level] = mvHash;
        if (g_pos_distance < KILLER_DEPTH) {
            g_sort_mvKiller1_stack[level] = g_mvKiller[g_pos_distance][0];
            g_sort_mvKiller2_stack[level] = g_mvKiller[g_pos_distance][1];
        } else {
            g_sort_mvKiller1_stack[level] = 0;
            g_sort_mvKiller2_stack[level] = 0;
        }
        g_sort_singleReply_stack[level] = 0;
    } else {
        g_sort_phase_stack[level] = 4;
        g_sort_mvHash_stack[level] = 0;
        g_sort_mvKiller1_stack[level] = 0;
        g_sort_mvKiller2_stack[level] = 0;
        g_sort_moves_stack[level] = 0;

        numAll = positionGenerateAllMoves(g_sort_mvs_stack[level]);
        for (i = 0; i < numAll; i++) {
            mv = g_sort_mvs_stack[level][i];
            if (positionMakeMove(mv) == 0) {
                continue;
            }
            positionUndoMakeMove();
            g_sort_mvs_stack[level][g_sort_moves_stack[level]] = mv;
            if (mv == mvHash) {
                g_sort_vls_stack[level][g_sort_moves_stack[level]] = 30000;
            } else {
                hIdx = positionHistoryIndex(mv);
                g_sort_vls_stack[level][g_sort_moves_stack[level]] = g_historyTable[hIdx & 16383];
            }
            g_sort_moves_stack[level]++;
        }
        shellSort(g_sort_mvs_stack[level], g_sort_vls_stack[level], 0, g_sort_moves_stack[level]);
        g_sort_index_stack[level] = 0;
        if (g_sort_moves_stack[level] == 1) {
            g_sort_singleReply_stack[level] = 1;
        } else {
            g_sort_singleReply_stack[level] = 0;
        }
    }
}

long sortNext()
{
    int level;
    long mv;

    level = g_pos_distance;
    if (level >= LIMIT_DEPTH) {
        level = LIMIT_DEPTH - 1;
    }

    if (g_sort_phase_stack[level] == 0) {
        g_sort_phase_stack[level] = 1;
        if (g_sort_mvHash_stack[level] > 0) {
            return g_sort_mvHash_stack[level];
        }
    }
    if (g_sort_phase_stack[level] == 1) {
        g_sort_phase_stack[level] = 2;
        if (g_sort_mvKiller1_stack[level] != g_sort_mvHash_stack[level]) {
            if (g_sort_mvKiller1_stack[level] > 0) {
                if (positionLegalMove(g_sort_mvKiller1_stack[level]) != 0) {
                    return g_sort_mvKiller1_stack[level];
                }
            }
        }
    }
    if (g_sort_phase_stack[level] == 2) {
        g_sort_phase_stack[level] = 3;
        if (g_sort_mvKiller2_stack[level] != g_sort_mvHash_stack[level]) {
            if (g_sort_mvKiller2_stack[level] > 0) {
                if (positionLegalMove(g_sort_mvKiller2_stack[level]) != 0) {
                    return g_sort_mvKiller2_stack[level];
                }
            }
        }
    }
    if (g_sort_phase_stack[level] == 3) {
        g_sort_phase_stack[level] = 4;
        g_sort_moves_stack[level] = positionGenerateAllMoves(g_sort_mvs_stack[level]);
        g_sort_index_stack[level] = 0;
        while (g_sort_index_stack[level] < g_sort_moves_stack[level]) {
            {
                int hIdx = positionHistoryIndex(g_sort_mvs_stack[level][g_sort_index_stack[level]]);
                g_sort_vls_stack[level][g_sort_index_stack[level]] = g_historyTable[hIdx & 16383];
            }
            g_sort_index_stack[level]++;
        }
        shellSort(g_sort_mvs_stack[level], g_sort_vls_stack[level], 0, g_sort_moves_stack[level]);
        g_sort_index_stack[level] = 0;
    }

    while (g_sort_index_stack[level] < g_sort_moves_stack[level]) {
        mv = g_sort_mvs_stack[level][g_sort_index_stack[level]];
        g_sort_index_stack[level]++;
        if (mv != g_sort_mvHash_stack[level]) {
            if (mv != g_sort_mvKiller1_stack[level]) {
                if (mv != g_sort_mvKiller2_stack[level]) {
                    return mv;
                }
            }
        }
    }

    return 0;
}

// ==================== 设置最佳走法 ====================

void setBestMove(long mv, int depth)
{
    int hIdx;
    hIdx = positionHistoryIndex(mv) & 16383;
    g_historyTable[hIdx] = g_historyTable[hIdx] + depth * depth;

    if (g_pos_distance < KILLER_DEPTH) {
        if (g_mvKiller[g_pos_distance][0] != mv) {
            g_mvKiller[g_pos_distance][1] = g_mvKiller[g_pos_distance][0];
            g_mvKiller[g_pos_distance][0] = mv;
        }
    }
}

// ==================== 静态搜索 ====================

int searchQuiesc(int vlAlpha, int vlBeta)
{
    g_allNodes++;

    if (g_stopFlag != 0) return 0;

    // 每512 个节点检查一次超时
    if ((g_allNodes & 511) == 0) {
        if (checkTimeout() != 0) return 0;
    }

    g_q_vl = positionMateValue();
    if (g_q_vl >= vlBeta) return g_q_vl;

    // 重复检测
    {
        int repStatus;
        repStatus = positionRepStatus();
        if (repStatus > 0) {
            return positionRepValue(repStatus);
        }
    }

    if (g_pos_distance >= LIMIT_DEPTH) {
        return positionEvaluate();
    }

    g_q_vlBest = -MATE_VALUE;

    if (positionInCheck() != 0) {
        // 被将军时生成所有走法
        g_q_genMoves = positionGenerateAllMoves(g_qmvs);
        for (g_q_i = 0; g_q_i < g_q_genMoves; g_q_i++) {
            int hIdx = positionHistoryIndex(g_qmvs[g_q_i]) & 16383;
            g_qvls[g_q_i] = g_historyTable[hIdx];
        }
        shellSort(g_qmvs, g_qvls, 0, g_q_genMoves);
    } else {
        // 不被将军时，先评估当前局面
        g_q_vl = positionEvaluate();
        if (g_q_vl >= vlBeta) return g_q_vl;
        g_q_vlBest = g_q_vl;
        if (g_q_vl > vlAlpha) vlAlpha = g_q_vl;

        // 只生成吃子走法
        g_q_genMoves = positionGenerateMoves(g_qmvs, g_qvls);
        shellSort(g_qmvs, g_qvls, 0, g_q_genMoves);

        // 过滤低价值吃子走法
            for (g_q_i = 0; g_q_i < g_q_genMoves; g_q_i++) {
            if (g_qvls[g_q_i] < 10) {
                g_q_genMoves = g_q_i;
                break;
            }
            if (g_qvls[g_q_i] < 20) {
                if (HOME_HALF(DST(g_qmvs[g_q_i]), g_pos_sdPlayer) != 0) {
                    g_q_genMoves = g_q_i;
                    break;
                }
            }
        }
    }

    // 搜索吃子走法
    for (g_q_i = 0; g_q_i < g_q_genMoves; g_q_i++) {
        g_q_mv = g_qmvs[g_q_i];
        if (positionMakeMove(g_q_mv) == 0) continue;

        // 保存当前层vlBest，防止递归覆盖
        if (g_pos_distance < LIMIT_DEPTH) {
            g_q_vlBest_stack[g_pos_distance] = g_q_vlBest;
        }
        g_q_vl = -searchQuiesc(-vlBeta, -vlAlpha);
        // 恢复当前层vlBest
        if (g_pos_distance < LIMIT_DEPTH) {
            g_q_vlBest = g_q_vlBest_stack[g_pos_distance];
        }

        positionUndoMakeMove();

        if (g_q_vl > g_q_vlBest) {
            if (g_q_vl >= vlBeta) return g_q_vl;
            g_q_vlBest = g_q_vl;
            if (g_q_vl > vlAlpha) vlAlpha = g_q_vl;
        }
    }

    if (g_q_vlBest == -MATE_VALUE) {
        return positionMateValue();
    }
    return g_q_vlBest;
}

// ==================== 完整搜索 ====================

int searchNoNull(int vlAlpha, int vlBeta, int depth)
{
    return searchFull(vlAlpha, vlBeta, depth, 1);
}

int searchFull(int vlAlpha, int vlBeta, int depth, int noNull)
{
    int sortSingleReply;
    int src_sq, dst_sq, pc_src, pc_dst;
    int my_side;
    // 记录当前是谁在走棋，用于日志显示和分数转换
    my_side = g_pos_sdPlayer;
    // 使用全局变量减少栈使用
    if (depth <= 0) {
        return searchQuiesc(vlAlpha, vlBeta);
    }

    g_allNodes++;

    if (g_stopFlag != 0) return 0;

    // 每512 个节点检查一次超时
    if ((g_allNodes & 511) == 0) {
        if (checkTimeout() != 0) return 0;
    }

    g_f_vl = positionMateValue();
    if (g_f_vl >= vlBeta) return g_f_vl;

    // 重复检测
    {
        int repStatus;
        repStatus = positionRepStatus();
        if (repStatus > 0) {
            return positionRepValue(repStatus);
        }
    }

    g_f_mvHash = 0;
    g_f_vl = probeHash(vlAlpha, vlBeta, depth);
    if (g_f_vl > -MATE_VALUE) return g_f_vl;

    if (g_pos_distance >= LIMIT_DEPTH) {
        return positionEvaluate();
    }

    if (noNull == 0) {
        if (positionInCheck() == 0) {
            if (positionNullOkay() != 0) {
                positionNullMove();
                g_f_vl = -searchNoNull(-vlBeta, 1 - vlBeta, depth - NULL_DEPTH - 1);
                positionUndoNullMove();
                if (g_f_vl >= vlBeta) {
                    if (positionNullSafe() != 0 || searchNoNull(vlAlpha, vlBeta, depth - NULL_DEPTH) >= vlBeta) {
                        return g_f_vl;
                    }
                }
            }
        }
    }

    g_f_hashFlag = HASH_ALPHA;
    g_f_vlBest = -MATE_VALUE;
    g_f_mvBest = 0;

    sortInit(g_f_mvHash);
    sortSingleReply = 0;
    if (g_pos_distance < LIMIT_DEPTH) {
        sortSingleReply = g_sort_singleReply_stack[g_pos_distance];
    }

    while (1) {
        g_f_mv = sortNext();
        if (g_f_mv <= 0) {
            break;
        }

        if (positionMakeMove(g_f_mv) == 0) {
            continue;
        }

        if (positionInCheck() != 0 || sortSingleReply != 0) {
            g_f_newDepth = depth;
        } else {
            g_f_newDepth = depth - 1;
        }

        /* 1. 保存当前层的最佳值，防止被递归调用覆盖 */
        if (g_pos_distance < LIMIT_DEPTH) {
            g_f_vlBest_stack[g_pos_distance] = g_f_vlBest;
            g_f_hashFlag_stack[g_pos_distance] = g_f_hashFlag;
            g_f_mvBest_stack[g_pos_distance] = g_f_mvBest;
        }

        // PVS 搜索
        if (g_f_newDepth <= 0) {
            g_f_vl = -searchQuiesc(-vlBeta, -vlAlpha);
        } else if (g_f_vlBest == -MATE_VALUE) {
            g_f_vl = -searchFull(-vlBeta, -vlAlpha, g_f_newDepth, 0);
        } else {
            g_f_vl = -searchFull(-vlAlpha - 1, -vlAlpha, g_f_newDepth, 0);
            if (g_f_vl > vlAlpha) {
                if (g_f_vl < vlBeta) {
                    g_f_vl = -searchFull(-vlBeta, -vlAlpha, g_f_newDepth, 0);
                }
            }
        }

        /* 2. 恢复当前层的最佳值 */
        if (g_pos_distance < LIMIT_DEPTH) {
            g_f_vlBest = g_f_vlBest_stack[g_pos_distance];
            g_f_hashFlag = g_f_hashFlag_stack[g_pos_distance];
            g_f_mvBest = g_f_mvBest_stack[g_pos_distance];
        }

        positionUndoMakeMove();

        if (g_stopFlag != 0) break;

        /* 3. 比较并更新最佳值 */
        /* 3. 比较并更新最佳值 (注意：引擎内部始终是最大化，但日志显示需区分红黑) */
        if (g_f_vl > g_f_vlBest) {
            /* 打印新最佳：统一转换为黑方视角 */
            if (g_can_move_debug_enabled) {
                int disp_vl, disp_old;
                int side_is_red = (my_side == 0);

                /* 获取旧的最佳值（未更新前） */
                int old_best_val = g_f_vlBest;

                /* 转换为黑方视角：红方走棋时取反（显示负数），黑方走棋不变 */
                disp_vl = side_is_red ? -g_f_vl : g_f_vl;
                disp_old = side_is_red ? -old_best_val : old_best_val;

                src_sq = (int)(g_f_mv & 255);
                dst_sq = (int)((g_f_mv >> 8) & 255);
                pc_src = g_pos_squares[src_sq];
                pc_dst = g_pos_squares[dst_sq];

            }
            g_f_vlBest = g_f_vl;
            if (g_f_vl >= vlBeta) {
                g_f_hashFlag = HASH_BETA;
                g_f_mvBest = g_f_mv;
                break;
            }
            if (g_f_vl > vlAlpha) {
                vlAlpha = g_f_vl;
                g_f_hashFlag = HASH_PV;
                g_f_mvBest = g_f_mv;
            }
        }
    }

    if (g_f_vlBest == -MATE_VALUE) {
        return positionMateValue();
    }

    recordHash(g_f_hashFlag, g_f_vlBest, depth, g_f_mvBest);
    if (g_f_mvBest > 0) {
        setBestMove(g_f_mvBest, depth);
    }

    return g_f_vlBest;
}

// ==================== 根节点搜索 ====================

int searchRoot(int depth)
{
    int r_src, r_dst, r_pcSrc, r_pcDst, r_legal, r_total;
    int r_reject_count;

    if (g_stopFlag != 0) {
        return 0;
    }

    g_r_vlBest = -MATE_VALUE;
    r_legal = 0;
    r_total = 0;
    r_reject_count = 0;
    g_checked_debug = 0;

    // 生成所有走法并备份
    sortInit(g_mvResult);
    while (1) {
        g_r_mv = sortNext();
        if (g_r_mv <= 0) {
            break;
        }
        r_total++;
        r_src = (int)(g_r_mv & 255);
        r_dst = (int)((g_r_mv >> 8) & 255);
        r_pcSrc = g_pos_squares[r_src];
        r_pcDst = g_pos_squares[r_dst];
        if (positionMakeMove(g_r_mv) == 0) {
            if (depth <= 1 && r_reject_count < 5) {
                DPRINTF("[根节点] 拒绝 (%d,%d)->(%d,%d) %s->%s\n",
                       xq_sq_to_row(r_src), xq_sq_to_col(r_src),
                       xq_sq_to_row(r_dst), xq_sq_to_col(r_dst),
                       piece_code_to_name(r_pcSrc), piece_code_to_name(r_pcDst));
            }
            r_reject_count++;
            continue;
        }
        r_legal++;
        if (positionInCheck() != 0) {
            g_r_newDepth = depth;
        } else {
            g_r_newDepth = depth - 1;
        }

        // PVS 搜索
        if (g_r_vlBest == -MATE_VALUE) {
            g_r_vl = -searchNoNull(-MATE_VALUE, MATE_VALUE, g_r_newDepth);
        } else {
            g_r_vl = -searchFull(-g_r_vlBest - 1, -g_r_vlBest, g_r_newDepth, 0);
            if (g_r_vl > g_r_vlBest) {
                g_r_vl = -searchNoNull(-MATE_VALUE, -g_r_vlBest, g_r_newDepth);
            }
        }
        positionUndoMakeMove();

        if (g_stopFlag != 0) break;

        if (depth <= 1) {
            DPRINTF("[根节点] OK (%d,%d)->(%d,%d) %s->%s 评分=%d 最佳=%d\n",
                   xq_sq_to_row(r_src), xq_sq_to_col(r_src),
                   xq_sq_to_row(r_dst), xq_sq_to_col(r_dst),
                   piece_code_to_name(r_pcSrc), piece_code_to_name(r_pcDst), g_r_vl, g_r_vlBest);
        }
        if (g_r_vl > g_r_vlBest) {
            g_r_vlBest = g_r_vl;
            g_mvResult = g_r_mv;
        }
    }

    DPRINTF("[根节点] depth=%d legal=%d total=%d best value=%d best=(%d,%d)->(%d,%d)\n",
           depth, r_legal, r_total, g_r_vlBest,
           xq_sq_to_row((int)(g_mvResult & 255)), xq_sq_to_col((int)(g_mvResult & 255)),
           xq_sq_to_row((int)((g_mvResult >> 8) & 255)), xq_sq_to_col((int)((g_mvResult >> 8) & 255)));

    setBestMove(g_mvResult, depth);
    return g_r_vlBest;
}

// ==================== 唯一性检测 ====================

int searchUnique(int vlBeta, int depth)
{
    int vl;
    long mv;
    int newDepth;

    int numMvs, i, bestScore;
    long g_sort_mvs[MAX_GEN_MOVES];
    long g_mvs_backup[MAX_GEN_MOVES];
    int g_u_vls[MAX_GEN_MOVES];

    if (g_stopFlag != 0) return 0;

    // 生成所有走法并备份
    numMvs = positionGenerateAllMoves(g_sort_mvs);
    for (i = 0; i < numMvs; i++) {
        bestScore = g_historyTable[positionHistoryIndex(g_sort_mvs[i]) & 16383];
        if (g_sort_mvs[i] == g_mvResult) {
            bestScore = 30000;
        }
        g_u_vls[i] = bestScore;
        g_mvs_backup[i] = g_sort_mvs[i];
    }
    shellSort(g_mvs_backup, g_u_vls, 0, numMvs);

    // 跳过第一个走法（g_mvResult）
    for (i = 0; i < numMvs; i++) {
        mv = g_mvs_backup[i];
        if (mv == g_mvResult) {
            continue;
        }

        if (positionMakeMove(mv) == 0) {
            continue;
        }

        if (positionInCheck() != 0) {
            newDepth = depth;
        } else {
            newDepth = depth - 1;
        }

        vl = -searchFull(-vlBeta, 1 - vlBeta, newDepth, 0);

        positionUndoMakeMove();

        if (g_stopFlag != 0) return 0;
        if (vl >= vlBeta) return 0;
    }

    return 1;
}

// ==================== 主搜索入口 ====================

// 检查是否超时并刷新倒计时显示（用于深层搜索）
int checkTimeout()
{
    int current_second;
    int elapsed;
    char countdown[36];

    GetTime(g_ai_time_buf);
    current_second = g_ai_time_buf[6];

    // 计算已用时间（处理秒数回绕）
    if (current_second >= g_ai_start_second) {
        elapsed = current_second - g_ai_start_second;
    } else {
        elapsed = (60 - g_ai_start_second) + current_second;
    }

    // 计算剩余时间
    g_ai_remaining_seconds = g_ai_timeout_seconds - elapsed;
    if (g_ai_remaining_seconds < 0) {
        g_ai_remaining_seconds = 0;
    }

    // AI status belongs in the landscape side panel, away from the board.
    Block(STATUS_X + 5, 67, STATUS_RIGHT - 5, 80, 1);

    // 绘制倒计时
    sprintf(countdown, "AI思考:%02d", g_ai_remaining_seconds);
    TextOut(STATUS_X + 9, 68, countdown, 9);

    // 绘制难度等级标签（紧邻倒计时右侧，同一layer）
    // if (g_current_difficulty == 1) {
    //     strcpy(lv_text, "Lv1");
    // } else if (g_current_difficulty == 2) {
    //     strcpy(lv_text, "Lv2");
    // } else {
    //     strcpy(lv_text, "Lv3");
    // }
    // TextOut(100, 58, lv_text, 3);

    Refresh();

    if (elapsed >= g_ai_timeout_seconds) {
        g_stopFlag = 1;
        return 1;
    }
    return 0;
}

// 检查是否超时并显示倒计时（用于顶层循环，与checkTimeout功能相同）
int checkTimeoutAndDisplay()
{
    return checkTimeout();
}

long searchMain(int maxDepth)
{
    int i, vl;
    int depth;
    int depth_reached;
    long bookMove;
    int sq;

    // 打印当前局面（仅调试时）
    if (g_can_move_debug_enabled)
    {
        int red_count = 0;
        int black_count = 0;
        char* side_str = g_pos_sdPlayer == 0 ? "红方" : "黑方";
        DPRINTF("[AI-%s] 搜索开始: 最大深度=%d\n", side_str, maxDepth);
        for (sq = 0; sq < 256; sq++)
        {
            if (g_pos_squares[sq] != 0)
            {
                int game_row = xq_sq_to_row(sq);
                int game_col = xq_sq_to_col(sq);
                /* 只打印有效棋盘区域内的棋子 */
                if (game_row >= 0 && game_row <= 9 && game_col >= 0 && game_col <= 8)
                {
                    int pc = (int)g_pos_squares[sq];
                    DPRINTF("  (%d,%d) %s\n", game_row, game_col, piece_code_to_name(pc));
                    if (pc >= 8 && pc <= 14) red_count++;
                    if (pc >= 16 && pc <= 22) black_count++;
                }
            }
        }
        DPRINTF("[AI-%s] 棋子统计: 红方=%d, 黑方=%d\n", side_str, red_count, black_count);
    }

    // 先尝试开局库
    bookMove = positionBookMoveSimple();
    if (bookMove > 0)
    {
        DPRINTF("[开局库] 找到匹配走法\n");
        g_mvResult = bookMove;
        return bookMove;
    }

    // 清理置换表、杀手走法、历史表
    clearHashTable();
    clearHistoryAndKillers();

    g_mvResult = 0;
    g_allNodes = 0;
    g_hashHits = 0;
    g_pos_distance = 0;
    g_stopFlag = 0;
    g_hashMask = HASH_ENTRIES;

    depth_reached = 0;

    // 迭代深化
    for (depth = 1; depth <= maxDepth; depth++) {
        // 检查超时
        if (checkTimeoutAndDisplay() != 0) break;

        vl = searchRoot(depth);
        depth_reached = depth;

        DPRINTF("[AI-%s] depth=%d vl=%d best=(%d,%d)->(%d,%d)\n",
               g_pos_sdPlayer == 0 ? "红方" : "黑方",
               depth, vl,
               xq_sq_to_row((int)(g_mvResult & 255)), xq_sq_to_col((int)(g_mvResult & 255)),
               xq_sq_to_row((int)((g_mvResult >> 8) & 255)), xq_sq_to_col((int)((g_mvResult >> 8) & 255)));

        if (g_stopFlag != 0) break;
        if (vl > WIN_VALUE) break;
        if (vl < -WIN_VALUE) break;
        if (searchUnique(1 - WIN_VALUE, depth) != 0) break;
    }

    DPRINTF("[AI-%s] searchMain: depth_reached=%d, nodes=%ld, hash_hits=%ld, timeout=%s\n",
           g_pos_sdPlayer == 0 ? "红方" : "黑方",
           depth_reached, g_allNodes, g_hashHits, g_stopFlag!=0? "true" : "false");

    return g_mvResult;
}
