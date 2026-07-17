// ==================== ai_xqwlight_position.lava ====================
// xqwlight AI - 局面逻辑 (全局状态版)

// ==================== 全局面面状态 ====================

int g_pos_sdPlayer;      // 0=红方, 1=黑方
char g_pos_squares[256]; // 棋盘
long g_pos_zobristKey;   // Zobrist 哈希键
long g_pos_zobristLock;  // Zobrist 哈希锁
int g_pos_vlWhite;       // 红方子力价值
int g_pos_vlBlack;       // 黑方子力价值
int g_pos_moveNum;       // 走法计数
int g_pos_distance;      // 距根节点的距离

long g_pos_mvList[MAX_MOVE_NUM];  // 走法历史
int g_pos_pcList[MAX_MOVE_NUM];  // 被吃子历史
long g_pos_keyList[MAX_MOVE_NUM]; // Zobrist 键历史
int g_pos_chkList[MAX_MOVE_NUM]; // 将军历史

// ==================== 内部函数 ====================

// 检查当前走棋方是否被将军
int g_checked_debug;  // 调试开关, 1=打印诊断

int checked()
{
    int sqSrc, sqDst, i, j;
    int pcSelfSide, pcOppSide;
    int pc, delta, sq;
    int temp1, temp2;

    pcSelfSide = SIDE_TAG(g_pos_sdPlayer);
    pcOppSide = OPP_SIDE_TAG(g_pos_sdPlayer);

    // 找到己方将/帅位置
    sqSrc = 0;
    while (sqSrc < 256) {
        if (g_pos_squares[sqSrc] == pcSelfSide + PIECE_KING) {
            break;
        }
        sqSrc++;
    }
    if (sqSrc >= 256) {
        if (g_checked_debug != 0) {
            DPRINTF("[CHK] sd=%d KING NOT FOUND!\n", g_pos_sdPlayer);
        }
        return 0; // FALSE
    }

    if (g_checked_debug != 0) {
        DPRINTF("[CHK] sd=%d king_sq=%d selfTag=%d oppTag=%d\n",
               g_pos_sdPlayer, sqSrc, pcSelfSide, pcOppSide);
    }

    // 检查将/帅对面 (飞将)
    for (i = 0; i < 4; i++) {
        delta = g_kingDelta[i];
        sqDst = sqSrc + delta;
        if (IN_FORT(sqDst) != 0) {
            if (g_pos_squares[sqDst] == pcOppSide + PIECE_KING) {
                if (g_checked_debug != 0) {
                    DPRINTF("[CHK] ADJACENT KING! dir=%d sqDst=%d\n", i, sqDst);
                }
                return 1; // TRUE
            }
        }
    }

    // 检查飞将 (将帅对面，中间无子)
    for (i = 0; i < 4; i++) {
        delta = g_kingDelta[i];
        sqDst = sqSrc + delta;
        while (IN_BOARD(sqDst) != 0) {
            pc = g_pos_squares[sqDst];
            if (pc != 0) {
                if (pc == pcOppSide + PIECE_KING) {
                    if (g_checked_debug != 0) {
                        DPRINTF("[CHK] FLYING GENERAL! dir=%d sqDst=%d\n", i, sqDst);
                    }
                    return 1;
                }
                break;
            }
            sqDst += delta;
        }
    }

    // 检查直线上的车和炮 (通过将/帅的4个方向)
    for (i = 0; i < 4; i++) {
        delta = g_kingDelta[i];
        sqDst = sqSrc + delta;
        while (IN_BOARD(sqDst) != 0) {
            pc = g_pos_squares[sqDst];
            if (pc != 0) {
                if (pc == pcOppSide + PIECE_ROOK) {
                    if (g_checked_debug != 0) {
                        DPRINTF("[CHK] ROOK CHECK! dir=%d sqDst=%d\n", i, sqDst);
                    }
                    return 1; // TRUE
                }
                // 检查炮: 需要恰好一个炮架
                sqDst += delta;
                while (IN_BOARD(sqDst) != 0) {
                    pc = g_pos_squares[sqDst];
                    if (pc != 0) {
                        if (pc == pcOppSide + PIECE_CANNON) {
                            if (g_checked_debug != 0) {
                                DPRINTF("[CHK] CANNON CHECK! dir=%d sqDst=%d\n", i, sqDst);
                            }
                            return 1; // TRUE
                        }
                        break;
                    }
                    sqDst += delta;
                }
                break;
            }
            sqDst += delta;
        }
    }

    // 检查马将军
    for (i = 0; i < 4; i++) {
        temp1 = g_knightCheckDelta[i][0];
        sqDst = sqSrc + temp1;
        if (sqDst >= 0 && sqDst < 256) {
            if (IN_BOARD(sqDst) != 0) {
                if (g_pos_squares[sqDst] == pcOppSide + PIECE_KNIGHT) {
                    temp2 = g_knightDelta[i][0];
                    sq = sqSrc + temp2;
                    if (sq >= 0 && sq < 256) {
                        if (g_pos_squares[sq] == 0) {
                            return 1; // TRUE
                        }
                    }
                }
            }
        }
        temp1 = g_knightCheckDelta[i][1];
        sqDst = sqSrc + temp1;
        if (sqDst >= 0 && sqDst < 256) {
            if (IN_BOARD(sqDst) != 0) {
                if (g_pos_squares[sqDst] == pcOppSide + PIECE_KNIGHT) {
                    temp2 = g_knightDelta[i][1];
                    sq = sqSrc + temp2;
                    if (sq >= 0 && sq < 256) {
                        if (g_pos_squares[sq] == 0) {
                            return 1; // TRUE
                        }
                    }
                }
            }
        }
    }

    // 检查兵/卒将军
    sqDst = SQUARE_FORWARD(sqSrc, g_pos_sdPlayer);
    if (IN_BOARD(sqDst) != 0) {
        if (g_pos_squares[sqDst] == pcOppSide + PIECE_PAWN) {
            return 1; // TRUE
        }
    }
    // 检查左右方向
    sqDst = sqSrc - 1;
    if (IN_BOARD(sqDst) != 0) {
        if (g_pos_squares[sqDst] == pcOppSide + PIECE_PAWN) {
            return 1; // TRUE
        }
    }
    sqDst = sqSrc + 1;
    if (IN_BOARD(sqDst) != 0) {
        if (g_pos_squares[sqDst] == pcOppSide + PIECE_PAWN) {
            return 1; // TRUE
        }
    }

    return 0; // FALSE
}

// ==================== 局面函数 ====================

void positionInit()
{
    char key[1];
    int i, j;

    key[0] = 0;
    rc4Init(key, 1);
    g_zobristKeyPlayer = rc4NextLong();
    rc4NextLong(); // 跳过 ZobristLock0
    g_zobristLockPlayer = rc4NextLong();

    for (i = 0; i < 14; i++) {
        for (j = 0; j < 256; j++) {
            g_zobristKeyTable[i][j] = rc4NextLong();
            rc4NextLong(); // 跳过 ZobristLock0
            g_zobristLockTable[i][j] = rc4NextLong();
        }
    }
    initLookupTables();
}

void positionClear()
{
    int i;

    g_pos_sdPlayer = 0;
    for (i = 0; i < 256; i++) {
        g_pos_squares[i] = 0;
    }
    g_pos_zobristKey = 0;
    g_pos_zobristLock = 0;
    g_pos_vlWhite = 0;
    g_pos_vlBlack = 0;
    g_pos_moveNum = 0;
    g_pos_distance = 0;
}

void positionSetIrrev()
{
    g_pos_mvList[0] = 0;
    g_pos_pcList[0] = 0;
    g_pos_chkList[0] = checked();
    g_pos_moveNum = 1;
    g_pos_distance = 0;
}

// 添加或删除棋子
void positionAddPiece(int sq, int pc, int del)
{
    int pcAdjust;
    int flippedSq;

    /* pc==0 表示空位，无需处理 */
    if (pc == 0) {
        return;
    }

    if (del != 0) {
        g_pos_squares[sq] = 0;
    } else {
        g_pos_squares[sq] = pc;
    }

    pcAdjust = 0;
    if (pc < 16) {
        pcAdjust = pc - 8;
        if (del != 0) {
            g_pos_vlWhite = g_pos_vlWhite - g_pieceValue[pcAdjust][sq];
        } else {
            g_pos_vlWhite = g_pos_vlWhite + g_pieceValue[pcAdjust][sq];
        }
    } else {
        pcAdjust = pc - 16;
        flippedSq = SQUARE_FLIP(sq);
        if (del != 0) {
            g_pos_vlBlack = g_pos_vlBlack - g_pieceValue[pcAdjust][flippedSq];
        } else {
            g_pos_vlBlack = g_pos_vlBlack + g_pieceValue[pcAdjust][flippedSq];
        }
        pcAdjust = pcAdjust + 7;
    }
    g_pos_zobristKey = g_pos_zobristKey ^ g_zobristKeyTable[pcAdjust][sq];
    g_pos_zobristLock = g_pos_zobristLock ^ g_zobristLockTable[pcAdjust][sq];
}

void positionChangeSide()
{
    g_pos_sdPlayer = 1 - g_pos_sdPlayer;
    g_pos_zobristKey = g_pos_zobristKey ^ g_zobristKeyPlayer;
    g_pos_zobristLock = g_pos_zobristLock ^ g_zobristLockPlayer;
}

// 生成所有走法 (vls != 0 时只生成吃子走法)
int positionGenerateMoves(long mvs[], int vls[])
{
    int moves;
    int sqSrc, sqDst, i, j;
    int pcSrc, pcDst;
    int pcSelfSide, pcOppSide;
    int delta;
    int hasVls;

    moves = 0;
    pcSelfSide = SIDE_TAG(g_pos_sdPlayer);
    pcOppSide = OPP_SIDE_TAG(g_pos_sdPlayer);
    hasVls = 0;
    if (vls != 0) {
        hasVls = 1;
    }

    for (sqSrc = 0; sqSrc < 256; sqSrc++) {
        pcSrc = g_pos_squares[sqSrc];
        if ((pcSrc & pcSelfSide) == 0) continue;

        switch (pcSrc - pcSelfSide) {
        case PIECE_KING:
            for (i = 0; i < 4; i++) {
                sqDst = sqSrc + g_kingDelta[i];
                if (IN_FORT(sqDst) == 0) continue;
                pcDst = g_pos_squares[sqDst];
                if (hasVls == 0) {
                    if ((pcDst & pcSelfSide) == 0) {
                        mvs[moves] = MOVE(sqSrc, sqDst);
                        moves++;
                    }
                } else {
                    if ((pcDst & pcOppSide) != 0) {
                        mvs[moves] = MOVE(sqSrc, sqDst);
                        vls[moves] = MVV_LVA(pcDst, 5);
                        moves++;
                    }
                }
            }
            break;

        case PIECE_ADVISOR:
            for (i = 0; i < 4; i++) {
                sqDst = sqSrc + g_advisorDelta[i];
                if (IN_FORT(sqDst) == 0) continue;
                pcDst = g_pos_squares[sqDst];
                if (hasVls == 0) {
                    if ((pcDst & pcSelfSide) == 0) {
                        mvs[moves] = MOVE(sqSrc, sqDst);
                        moves++;
                    }
                } else {
                    if ((pcDst & pcOppSide) != 0) {
                        mvs[moves] = MOVE(sqSrc, sqDst);
                        vls[moves] = MVV_LVA(pcDst, 1);
                        moves++;
                    }
                }
            }
            break;

        case PIECE_BISHOP:
            for (i = 0; i < 4; i++) {
                sqDst = sqSrc + g_advisorDelta[i];
                if (IN_BOARD(sqDst) == 0) continue;
                if (HOME_HALF(sqDst, g_pos_sdPlayer) == 0) continue;
                if (g_pos_squares[sqDst] != 0) continue;
                sqDst += g_advisorDelta[i];
                pcDst = g_pos_squares[sqDst];
                if (hasVls == 0) {
                    if ((pcDst & pcSelfSide) == 0) {
                        mvs[moves] = MOVE(sqSrc, sqDst);
                        moves++;
                    }
                } else {
                    if ((pcDst & pcOppSide) != 0) {
                        mvs[moves] = MOVE(sqSrc, sqDst);
                        vls[moves] = MVV_LVA(pcDst, 1);
                        moves++;
                    }
                }
            }
            break;

        case PIECE_KNIGHT:
            for (i = 0; i < 4; i++) {
                sqDst = sqSrc + g_kingDelta[i];
                if (IN_BOARD(sqDst) == 0) continue;
                if (g_pos_squares[sqDst] != 0) continue;
                for (j = 0; j < 2; j++) {
                    sqDst = sqSrc + g_knightDelta[i][j];
                    if (IN_BOARD(sqDst) == 0) continue;
                    pcDst = g_pos_squares[sqDst];
                    if (hasVls == 0) {
                        if ((pcDst & pcSelfSide) == 0) {
                            mvs[moves] = MOVE(sqSrc, sqDst);
                            moves++;
                        }
                    } else {
                        if ((pcDst & pcOppSide) != 0) {
                            mvs[moves] = MOVE(sqSrc, sqDst);
                            vls[moves] = MVV_LVA(pcDst, 1);
                            moves++;
                        }
                    }
                }
            }
            break;

        case PIECE_ROOK:
            for (i = 0; i < 4; i++) {
                delta = g_kingDelta[i];
                sqDst = sqSrc + delta;
                while (IN_BOARD(sqDst) != 0) {
                    pcDst = g_pos_squares[sqDst];
                    if (pcDst == 0) {
                        if (hasVls == 0) {
                            mvs[moves] = MOVE(sqSrc, sqDst);
                            moves++;
                        }
                    } else {
                        if ((pcDst & pcOppSide) != 0) {
                            mvs[moves] = MOVE(sqSrc, sqDst);
                            if (hasVls != 0) {
                                vls[moves] = MVV_LVA(pcDst, 4);
                            }
                            moves++;
                        }
                        break;
                    }
                    sqDst += delta;
                }
            }
            break;

        case PIECE_CANNON:
            for (i = 0; i < 4; i++) {
                delta = g_kingDelta[i];
                sqDst = sqSrc + delta;
                while (IN_BOARD(sqDst) != 0) {
                    pcDst = g_pos_squares[sqDst];
                    if (pcDst == 0) {
                        if (hasVls == 0) {
                            mvs[moves] = MOVE(sqSrc, sqDst);
                            moves++;
                        }
                    } else {
                        break;
                    }
                    sqDst += delta;
                }
                sqDst += delta;
                while (IN_BOARD(sqDst) != 0) {
                    pcDst = g_pos_squares[sqDst];
                    if (pcDst != 0) {
                        if ((pcDst & pcOppSide) != 0) {
                            mvs[moves] = MOVE(sqSrc, sqDst);
                            if (hasVls != 0) {
                                vls[moves] = MVV_LVA(pcDst, 4);
                            }
                            moves++;
                        }
                        break;
                    }
                    sqDst += delta;
                }
            }
            break;

        case PIECE_PAWN:
            sqDst = SQUARE_FORWARD(sqSrc, g_pos_sdPlayer);
            if (IN_BOARD(sqDst) != 0) {
                pcDst = g_pos_squares[sqDst];
                if (hasVls == 0) {
                    if ((pcDst & pcSelfSide) == 0) {
                        mvs[moves] = MOVE(sqSrc, sqDst);
                        moves++;
                    }
                } else {
                    if ((pcDst & pcOppSide) != 0) {
                        mvs[moves] = MOVE(sqSrc, sqDst);
                        vls[moves] = MVV_LVA(pcDst, 2);
                        moves++;
                    }
                }
            }
            if (AWAY_HALF(sqSrc, g_pos_sdPlayer) != 0) {
                sqDst = sqSrc - 1;
                if (IN_BOARD(sqDst) != 0) {
                    pcDst = g_pos_squares[sqDst];
                    if (hasVls == 0) {
                        if ((pcDst & pcSelfSide) == 0) {
                            mvs[moves] = MOVE(sqSrc, sqDst);
                            moves++;
                        }
                    } else {
                        if ((pcDst & pcOppSide) != 0) {
                            mvs[moves] = MOVE(sqSrc, sqDst);
                            vls[moves] = MVV_LVA(pcDst, 2);
                            moves++;
                        }
                    }
                }
                sqDst = sqSrc + 1;
                if (IN_BOARD(sqDst) != 0) {
                    pcDst = g_pos_squares[sqDst];
                    if (hasVls == 0) {
                        if ((pcDst & pcSelfSide) == 0) {
                            mvs[moves] = MOVE(sqSrc, sqDst);
                            moves++;
                        }
                    } else {
                        if ((pcDst & pcOppSide) != 0) {
                            mvs[moves] = MOVE(sqSrc, sqDst);
                            vls[moves] = MVV_LVA(pcDst, 2);
                            moves++;
                        }
                    }
                }
            }
            break;
        }
    }

    return moves;
}

// 生成所有走法 (不含价值)
int positionGenerateAllMoves(long mvs[])
{
    return positionGenerateMoves(mvs, 0);
}

// 走法合法性验证
int positionLegalMove(long mv)
{
    int sqSrc, sqDst;
    int pcSrc, pcDst;
    int pcSelfSide;
    int i, delta, sq;
    int count;
    int pinIdx;

    sqSrc = SRC(mv);
    if (sqSrc < 0 || sqSrc >= 256) return 0;
    pcSrc = g_pos_squares[sqSrc];
    pcSelfSide = SIDE_TAG(g_pos_sdPlayer);
    if ((pcSrc & pcSelfSide) == 0) return 0;

    sqDst = DST(mv);
    if (sqDst < 0 || sqDst >= 256) return 0;
    pcDst = g_pos_squares[sqDst];
    if ((pcDst & pcSelfSide) != 0) return 0;
    switch (pcSrc - pcSelfSide) {
    case PIECE_KING:
        if (IN_FORT(sqDst) == 0) return 0;
        pinIdx = sqDst - sqSrc + 256;
        if (pinIdx < 0 || pinIdx >= 512) return 0;
        if (g_legalSpan[pinIdx] != 1) return 0;
        return 1;

    case PIECE_ADVISOR:
        if (IN_FORT(sqDst) == 0) return 0;
        pinIdx = sqDst - sqSrc + 256;
        if (pinIdx < 0 || pinIdx >= 512) return 0;
        if (g_legalSpan[pinIdx] != 2) return 0;
        return 1;

    case PIECE_BISHOP:
        if (IN_BOARD(sqDst) == 0) return 0;
        if (HOME_HALF(sqDst, g_pos_sdPlayer) == 0) return 0;
        pinIdx = sqDst - sqSrc + 256;
        if (pinIdx < 0 || pinIdx >= 512) return 0;
        if (g_legalSpan[pinIdx] != 3) return 0;
        sq = (sqSrc + sqDst) >> 1;
        if (sq < 0 || sq >= 256) return 0;
        if (g_pos_squares[sq] != 0) return 0;
        return 1;

    case PIECE_KNIGHT:
        if (IN_BOARD(sqDst) == 0) return 0;
        pinIdx = sqDst - sqSrc + 256;
        if (pinIdx < 0 || pinIdx >= 512) return 0;
        if (g_legalSpan[pinIdx] == 0) return 0;
        sq = sqSrc + g_knightPin[pinIdx];
        if (sq < 0 || sq >= 256) return 0;
        if (g_pos_squares[sq] != 0) return 0;
        return 1;

    case PIECE_ROOK:
        if (IN_BOARD(sqDst) == 0) return 0;
        if (SAME_RANK(sqSrc, sqDst) != 0) {
            if (sqDst > sqSrc) {
                delta = 1;
            } else {
                delta = -1;
            }
            sq = sqSrc + delta;
            while (sq != sqDst) {
                if (g_pos_squares[sq] != 0) return 0;
                sq += delta;
            }
            return 1;
        }
        if (SAME_FILE(sqSrc, sqDst) != 0) {
            if (sqDst > sqSrc) {
                delta = 16;
            } else {
                delta = -16;
            }
            sq = sqSrc + delta;
            while (sq != sqDst) {
                if (g_pos_squares[sq] != 0) return 0;
                sq += delta;
            }
            return 1;
        }
        return 0;

    case PIECE_CANNON:
        if (IN_BOARD(sqDst) == 0) return 0;
        count = 0;
        if (SAME_RANK(sqSrc, sqDst) != 0) {
            if (sqDst > sqSrc) {
                delta = 1;
            } else {
                delta = -1;
            }
            sq = sqSrc + delta;
            while (sq != sqDst) {
                if (g_pos_squares[sq] != 0) count++;
                sq += delta;
            }
            if (pcDst == 0) {
                if (count == 0) return 1;
            } else {
                if (count == 1) return 1;
            }
            return 0;
        }
        if (SAME_FILE(sqSrc, sqDst) != 0) {
            if (sqDst > sqSrc) {
                delta = 16;
            } else {
                delta = -16;
            }
            sq = sqSrc + delta;
            while (sq != sqDst) {
                if (g_pos_squares[sq] != 0) count++;
                sq += delta;
            }
            if (pcDst == 0) {
                if (count == 0) return 1;
            } else {
                if (count == 1) return 1;
            }
            return 0;
        }
        return 0;

    case PIECE_PAWN:
        if (AWAY_HALF(sqSrc, g_pos_sdPlayer) != 0) {
            if (sqDst == SQUARE_FORWARD(sqSrc, g_pos_sdPlayer)) return 1;
            if (sqDst == sqSrc - 1) return 1;
            if (sqDst == sqSrc + 1) return 1;
            return 0;
        } else {
            if (sqDst == SQUARE_FORWARD(sqSrc, g_pos_sdPlayer)) return 1;
            return 0;
        }
    }

    return 0;
}

// 内部: 执行走子
void movePiece()
{
    int sqSrc, sqDst;
    int pc;
    sqSrc = SRC(g_pos_mvList[g_pos_moveNum]);
    sqDst = DST(g_pos_mvList[g_pos_moveNum]);
    g_pos_pcList[g_pos_moveNum] = g_pos_squares[sqDst];
    if (g_pos_pcList[g_pos_moveNum] > 0) {
        positionAddPiece(sqDst, g_pos_pcList[g_pos_moveNum], 1);
    }
    pc = g_pos_squares[sqSrc];
    positionAddPiece(sqSrc, pc, 1);
    positionAddPiece(sqDst, pc, 0);
}

// 内部: 撤销走子
void undoMovePiece()
{
    int sqSrc, sqDst;
    int pc;

    sqSrc = SRC(g_pos_mvList[g_pos_moveNum]);
    sqDst = DST(g_pos_mvList[g_pos_moveNum]);
    pc = g_pos_squares[sqDst];

    positionAddPiece(sqDst, pc, 1);
    positionAddPiece(sqSrc, pc, 0);
    if (g_pos_pcList[g_pos_moveNum] > 0) {
        positionAddPiece(sqDst, g_pos_pcList[g_pos_moveNum], 0);
    }
}

// 执行走法
int positionMakeMove(long mv)
{
    g_pos_keyList[g_pos_moveNum] = g_pos_zobristKey;
    g_pos_mvList[g_pos_moveNum] = mv;
    movePiece();

    if (checked() != 0) {
        undoMovePiece();
        return 0;
    }

    positionChangeSide();
    g_pos_chkList[g_pos_moveNum] = checked();
    g_pos_moveNum++;
    g_pos_distance++;
    return 1;
}

// 撤销走法
void positionUndoMakeMove()
{
    g_pos_moveNum--;
    g_pos_distance--;
    positionChangeSide();
    undoMovePiece();
}

// 空着
void positionNullMove()
{
    g_pos_keyList[g_pos_moveNum] = g_pos_zobristKey;
    positionChangeSide();
    g_pos_mvList[g_pos_moveNum] = 0;
    g_pos_pcList[g_pos_moveNum] = 0;
    g_pos_chkList[g_pos_moveNum] = 0;
    g_pos_moveNum++;
    g_pos_distance++;
}

// 撤销空着
void positionUndoNullMove()
{
    g_pos_moveNum--;
    g_pos_distance--;
    positionChangeSide();
}

// 是否被将军
int positionInCheck()
{
    return g_pos_chkList[g_pos_moveNum - 1];
}

// 是否吃子
int positionCaptured()
{
    if (g_pos_pcList[g_pos_moveNum - 1] > 0) {
        return 1;
    }
    return 0;
}

// 将杀分数
int positionMateValue()
{
    return -MATE_VALUE + g_pos_distance;
}

// 和棋分数
int positionDrawValue()
{
    int d;
    d = g_pos_distance & 1;
    if (d == 0) {
        return -DRAW_VALUE;
    }
    return DRAW_VALUE;
}

// 重复局面检测
int positionRepStatus()
{
    int i;
    long zk;

    zk = g_pos_zobristKey;

    for (i = g_pos_moveNum - 2; i >= 0; i -= 2) {
        if (g_pos_keyList[i] == zk) {
            if (i >= g_pos_moveNum - 4) {
                return 1;
            }
            if (g_pos_chkList[i] != 0) {
                return 2;
            }
            if (g_pos_chkList[g_pos_moveNum - 1] != 0) {
                return 2;
            }
            return 4;
        }
    }
    return 0;
}

// 重复局面分数
int positionRepValue(int vlRep)
{
    int vl;
    if (vlRep == 1) {
        vl = -BAN_VALUE;
    } else {
        if (vlRep == 2) {
            vl = WIN_VALUE;
        } else {
            vl = -DRAW_VALUE;
        }
    }
    return vl;
}

// 评估函数
int positionEvaluate()
{
    int diff;
    diff = g_pos_vlWhite - g_pos_vlBlack;
    if (g_pos_sdPlayer == 0) {
        return diff;
    }
    return -diff;
}

// 空着安全阈值
int positionNullOkay()
{
    int vl;
    if (g_pos_sdPlayer == 0) {
        vl = g_pos_vlWhite;
    } else {
        vl = g_pos_vlBlack;
    }
    if (vl > NULL_OKAY_MARGIN) {
        return 1;
    }
    return 0;
}

// 空着绝对安全阈值
int positionNullSafe()
{
    int vl;
    if (g_pos_sdPlayer == 0) {
        vl = g_pos_vlWhite;
    } else {
        vl = g_pos_vlBlack;
    }
    if (vl > NULL_SAFE_MARGIN) {
        return 1;
    }
    return 0;
}

// 历史启发表索引
int positionHistoryIndex(long mv)
{
    long src_full;
    long dst_full;
    long result;
    src_full = SRC(mv);
    dst_full = DST(mv);
    result = ((src_full << 8) | dst_full) & 4095;
    return (int)result;
}
