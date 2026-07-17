// ==================== ai_xqwlight_adapter.lava ====================
// xqwlight AI - 桥接层 (兼容旧 ai_cchess_interface API)

// ==================== 棋子映射表 ====================
// lavax piece ID -> xqwlight piece ID
// lavax: 1=RED_CHE, 2=RED_MA, 3=RED_XIANG, 4=RED_SHI, 5=RED_SHUAI,
//        6=RED_PAO, 7=RED_BING, 8=BLACK_CHE, 9=BLACK_MA, 10=BLACK_XIANG,
//        11=BLACK_SHI, 12=BLACK_JIANG, 13=BLACK_PAO, 14=BLACK_ZU
// xqwlight: 8=RED_KING, 9=RED_ADVISOR, 10=RED_BISHOP, 11=RED_KNIGHT,
//           12=RED_ROOK, 13=RED_CANNON, 14=RED_PAWN,
//           16=BLACK_KING, 17=BLACK_ADVISOR, 18=BLACK_BISHOP, 19=BLACK_KNIGHT,
//           20=BLACK_ROOK, 21=BLACK_CANNON, 22=BLACK_PAWN

int g_xq_piece_map[15] = {0, 12, 11, 10, 9, 8, 13, 14, 20, 19, 18, 17, 16, 21, 22};

// 搜索结果
int g_xq_result_from_col;
int g_xq_result_from_row;
int g_xq_result_to_col;
int g_xq_result_to_row;
long g_xq_fallback_mvs[MAX_MOVE_NUM];

// ==================== 初始化 ====================

void ai_cchess_init()
{
    positionInit();
    bookInit();
    clearHashTable();
    clearHistoryAndKillers();
}

// ==================== 设置 AI 思考时间 ====================

void ai_cchess_set_timeout(int timeout_seconds)
{
    char time_buf[8];

    g_ai_timeout_seconds = timeout_seconds;

    // 记录开始时间
    GetTime(time_buf);
    g_ai_start_second = time_buf[6];
}

// ==================== AI 思考并走棋 ====================

void ai_cchess_think_and_move(int is_red)
{
    long mv;
    int sq_src, sq_dst;
    int max_depth;
    int row, col, sq, piece;
    int xq_pc;
    int piece_count;
    int fallback_count;
    int fallback_i;

    /* 在函数内部初始化映射表（避免 LavaX 全局数组初始化问题） */
    int xq_map[15];
    xq_map[0] = 0;
    xq_map[1] = 12;  /* 红车 -> xqwlight 红车 */
    xq_map[2] = 11;  /* 红马 -> xqwlight 红马 */
    xq_map[3] = 10;  /* 红相 -> xqwlight 红相 */
    xq_map[4] = 9;   /* 红士 -> xqwlight 红士 */
    xq_map[5] = 8;   /* 红帅 -> xqwlight 红帅 */
    xq_map[6] = 13;  /* 红炮 -> xqwlight 红炮 */
    xq_map[7] = 14;  /* 红兵 -> xqwlight 红兵 */
    xq_map[8] = 20;  /* 黑车 -> xqwlight 黑车 */
    xq_map[9] = 19;  /* 黑马 -> xqwlight 黑马 */
    xq_map[10] = 18; /* 黑象 -> xqwlight 黑象 */
    xq_map[11] = 17; /* 黑士 -> xqwlight 黑士 */
    xq_map[12] = 16; /* 黑将 -> xqwlight 黑将 */
    xq_map[13] = 21; /* 黑炮 -> xqwlight 黑炮 */
    xq_map[14] = 22; /* 黑卒 -> xqwlight 黑卒 */

    // 1. 将 chess_map 转换到 g_pos_squares
    positionClear();
    piece_count = 0;
    for (row = 0; row <= 9; row++) {
        for (col = 0; col <= 8; col++) {
            piece = chess_map[row][col];
            if (piece == 0) continue;
            sq = (col + 3) + ((row + 3) << 4);
            xq_pc = xq_map[piece];
            positionAddPiece(sq, xq_pc, 0);
            piece_count++;
        }
    }
    // 2. 设置走棋方
    if (is_red == 0) {
        positionChangeSide();
    }
    positionSetIrrev();

    // 3. 根据栈空间限制设置最大搜索深度
    // 时间控制由 g_ai_timeout_seconds 控制
    #ifdef LAVA_NATIVE_COMPILED
    max_depth = 100; // 原生编译环境，栈空间较大，可以搜索更深
    #else
    max_depth = 6;
    #endif

    // 4. 搜索
    mv = searchMain(max_depth);

    // 4.1 最终合法性兜底：搜索/开局库都不允许返回未解将的非法着法
    if (mv != 0) {
        if (positionMakeMove(mv) != 0) {
            positionUndoMakeMove();
        } else {
            DPRINTF("[AI] 搜索结果非法，尝试回退到第一个合法走法\n");
            mv = 0;
        }
    }

    if (mv == 0) {
        fallback_count = positionGenerateAllMoves(g_xq_fallback_mvs);
        for (fallback_i = 0; fallback_i < fallback_count; fallback_i++) {
            if (positionMakeMove(g_xq_fallback_mvs[fallback_i]) != 0) {
                positionUndoMakeMove();
                mv = g_xq_fallback_mvs[fallback_i];
                DPRINTF("[AI] 使用回退合法走法=%ld\n", mv);
                break;
            }
        }
    }

    // 验证
    if (mv != 0) {
        int check_sq = mv & 255;
        int check_pc = g_pos_squares[check_sq];
    }

    // 5. 转换结果
    g_xq_result_from_col = -1;
    g_xq_result_from_row = -1;
    g_xq_result_to_col = -1;
    g_xq_result_to_row = -1;

    if (mv != 0) {
        sq_src = mv & 255;
        sq_dst = mv >> 8;
        g_xq_result_from_col = (sq_src & 15) - 3;
        g_xq_result_from_row = (sq_src >> 4) - 3;
        g_xq_result_to_col = (sq_dst & 15) - 3;
        g_xq_result_to_row = (sq_dst >> 4) - 3;
    }
}

// ==================== 获取 AI 走法结果 ====================

int ai_cchess_get_from_col()
{
    return g_xq_result_from_col;
}

int ai_cchess_get_from_row()
{
    return g_xq_result_from_row;
}

int ai_cchess_get_to_col()
{
    return g_xq_result_to_col;
}

int ai_cchess_get_to_row()
{
    return g_xq_result_to_row;
}
