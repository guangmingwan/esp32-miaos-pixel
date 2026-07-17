// ==================== test_unit.c ====================
// 中国象棋 AI 单元测试
// 用法: lvm2 -v cchess.lav test_func_name
//       这会调用 runtest("test_func_name") 分派到对应测试函数
// 测试结果输出到 /LavaData/test_result.log

#include "test_log.c"

void clear_gui_board_for_test()
{
    int row;
    int col;

    for (row = 0; row <= 11; row++) {
        for (col = 0; col <= 10; col++) {
            chess_map[row][col] = 0;
        }
    }
}

// ==================== xqwlight 核心测试 ====================

// 测试1: 棋子编码和SIDE_TAG
void test_piece_encoding()
{
    test_log_section("棋子编码测试");

    test_assert_int_eq("红帅编码", 8, 8 + PIECE_KING);
    test_assert_int_eq("红车编码", 12, 8 + PIECE_ROOK);
    test_assert_int_eq("红炮编码", 13, 8 + PIECE_CANNON);
    test_assert_int_eq("黑将编码", 16, 16 + PIECE_KING);
    test_assert_int_eq("黑车编码", 20, 16 + PIECE_ROOK);
    test_assert_int_eq("黑炮编码", 21, 16 + PIECE_CANNON);

    test_assert_int_eq("SIDE_TAG(0)红方", 8, SIDE_TAG(0));
    test_assert_int_eq("SIDE_TAG(1)黑方", 16, SIDE_TAG(1));
    test_assert_int_eq("OPP_SIDE_TAG(0)", 16, OPP_SIDE_TAG(0));
    test_assert_int_eq("OPP_SIDE_TAG(1)", 8, OPP_SIDE_TAG(1));
}

// 测试2: checked() - 飞将检测
void test_checked_flying_general()
{
    test_log_section("飞将检测测试");

    // 初始化
    positionInit();
    positionClear();

    // 场景1: 将帅同列中间无子 -> 应检测到飞将
    // 红帅在 (12,7)=sq199, 黑将在 (3,7)=sq55
    positionAddPiece(199, 8 + PIECE_KING, 0);   // 红帅
    positionAddPiece(55, 16 + PIECE_KING, 0);    // 黑将
    g_pos_sdPlayer = 1; // 黑方走棋

    // 黑方checked()应返回1(飞将)
    test_assert_int_eq("将帅同列无子-飞将", 1, checked());

    // 场景2: 中间放一个棋子 -> 不应飞将
    positionAddPiece(103, 16 + PIECE_PAWN, 0);   // 黑卒在(6,7)=sq103
    g_pos_sdPlayer = 1;
    test_assert_int_eq("将帅同列有子-非飞将", 0, checked());

    // 场景3: 移开中间棋子 -> 又飞将
    positionAddPiece(103, 16 + PIECE_PAWN, 1);   // 删除黑卒
    g_pos_sdPlayer = 1;
    test_assert_int_eq("移开遮挡-又飞将", 1, checked());
}

// 测试3: checked() - 车将军
void test_checked_rook()
{
    test_log_section("车将军检测测试");

    positionInit();
    positionClear();

    // 红帅在 (12,7)=sq199, 黑将在 (3,7)=sq55
    positionAddPiece(199, 8 + PIECE_KING, 0);
    positionAddPiece(55, 16 + PIECE_KING, 0);
    positionAddPiece(103, 16 + PIECE_PAWN, 0);  // 黑卒挡飞将

    // 黑将在(3,7), 红车在(3,3)=sq51 同行将军
    positionAddPiece(51, 8 + PIECE_ROOK, 0);
    g_pos_sdPlayer = 1;
    test_assert_int_eq("红车同行将军", 1, checked());

    // 中间放黑卒挡住红车
    positionAddPiece(53, 16 + PIECE_PAWN, 0);
    g_pos_sdPlayer = 1;
    test_assert_int_eq("红车被挡-不将军", 0, checked());
}

// 测试4: checked() - 炮将军
void test_checked_cannon()
{
    test_log_section("炮将军检测测试");

    positionInit();
    positionClear();

    // 黑将在 (3,7)=sq55, 红帅在 (12,7)=sq199 (中间有遮挡不飞将)
    positionAddPiece(55, 16 + PIECE_KING, 0);
    positionAddPiece(199, 8 + PIECE_KING, 0);
    positionAddPiece(103, 16 + PIECE_PAWN, 0);  // 挡飞将

    // 红炮在 (3,11)=sq59, 黑象在 (3,9)=sq57 作炮架
    positionAddPiece(59, 8 + PIECE_CANNON, 0);
    positionAddPiece(57, 16 + PIECE_BISHOP, 0);
    g_pos_sdPlayer = 1;
    test_assert_int_eq("红炮隔象将军", 1, checked());

    // 移走炮架
    positionAddPiece(57, 16 + PIECE_BISHOP, 1);
    g_pos_sdPlayer = 1;
    test_assert_int_eq("无炮架-不将军", 0, checked());
}

// 测试5: positionMakeMove - 合法/非法走法
void test_make_move()
{
    int result;
    long mv;

    test_log_section("走法执行测试");

    positionInit();
    positionClear();

    // 标准开局简化: 黑将(3,7)=55, 红帅(12,7)=199
    // 黑车(3,3)=51, 黑士(3,8)=56
    // 红炮(3,10)=58, 黑象(3,9)=57
    positionAddPiece(55, 16 + PIECE_KING, 0);    // 黑将
    positionAddPiece(199, 8 + PIECE_KING, 0);    // 红帅
    positionAddPiece(51, 16 + PIECE_ROOK, 0);    // 黑车a
    positionAddPiece(56, 16 + PIECE_ADVISOR, 0); // 黑士
    positionAddPiece(58, 8 + PIECE_CANNON, 0);   // 红炮
    positionAddPiece(57, 16 + PIECE_BISHOP, 0);  // 黑象
    positionAddPiece(103, 16 + PIECE_PAWN, 0);   // 黑卒挡飞将

    g_pos_sdPlayer = 1; // 黑方走棋
    positionSetIrrev();

    // 黑车前进 (51 -> 67) 合法
    mv = MOVE(51, 67);
    result = positionMakeMove(mv);
    test_assert_int_eq("黑车前进-合法", 1, result);
    if (result != 0) positionUndoMakeMove();

    // 黑士移开导致红炮将军 -> 非法
    // 黑士(56)移到(71)，红炮通过黑象炮架将军
    mv = MOVE(56, 71);
    result = positionMakeMove(mv);
    test_assert_int_eq("黑士移开-红炮将军-非法", 0, result);
}

// 测试5b: GUI层飞将检测
void test_gui_flying_general_check()
{
    test_log_section("GUI层飞将检测测试");

    clear_gui_board_for_test();
    chess_map[9][4] = RED_SHUAI;
    chess_map[0][4] = BLACK_JIANG;

    test_assert_int_eq("GUI飞将-红方被将军", 1, is_red_in_check());
    test_assert_int_eq("GUI飞将-黑方被将军", 1, is_black_in_check());
}

// 测试5c: GUI层完整合法性检查 - 被将军时必须解将
void test_gui_must_resolve_check()
{
    test_log_section("GUI层解将合法性测试");

    clear_gui_board_for_test();

    /* 红车在同一路将军黑将 */
    chess_map[9][4] = RED_SHUAI;
    chess_map[3][4] = RED_CHE;
    chess_map[0][4] = BLACK_JIANG;
    chess_map[0][0] = BLACK_CHE;

    test_assert_int_eq("黑方当前被将军", 1, is_black_in_check());
    test_assert_int_eq("黑车横移-基本走法成立", 1, can_move(0, 0, 1, 0));
    test_assert_int_eq("黑车横移-不解将必须拒绝", 0, is_move_legal_with_self_check(0, 0, 1, 0));
    test_assert_int_eq("黑将平移-可以解将", 1, is_move_legal_with_self_check(4, 0, 3, 0));
}

// 测试6: 走法生成 - 黑车吃红炮
void test_rook_capture()
{
    int numMoves;
    long mvs[128];
    int i;
    int found;
    int result;
    long mv_test;

    test_log_section("黑车吃子走法生成测试");

    positionInit();
    positionClear();

    // 场景: 黑车旁边有红炮
    // 黑将(3,7)=55, 红帅(12,7)=199
    // 黑车(3,11)=59, 红炮(3,10)=58 (紧邻)
    positionAddPiece(55, 16 + PIECE_KING, 0);
    positionAddPiece(199, 8 + PIECE_KING, 0);
    positionAddPiece(59, 16 + PIECE_ROOK, 0);    // 黑车
    positionAddPiece(58, 8 + PIECE_CANNON, 0);   // 红炮
    positionAddPiece(103, 16 + PIECE_PAWN, 0);   // 黑卒挡飞将

    g_pos_sdPlayer = 1;
    positionSetIrrev();

    // 生成所有走法
    numMoves = positionGenerateAllMoves(mvs);
    test_assert_true("黑方有走法", numMoves > 0 ? 1 : 0, "");

    // 查找黑车吃红炮的走法
    found = 0;
    for (i = 0; i < numMoves; i++) {
        if (SRC(mvs[i]) == 59 && DST(mvs[i]) == 58) {
            found = 1;
        }
    }
    test_assert_true("黑车吃红炮走法已生成", found, "");

    // 验证走法合法性
    if (found != 0) {
        mv_test = MOVE(59, 58);
        result = positionMakeMove(mv_test);
        test_assert_int_eq("黑车吃红炮-合法", 1, result);
        if (result != 0) {
            positionUndoMakeMove();
        }
    }
}

// 测试7: positionHistoryIndex 哈希公式
void test_history_index()
{
    long mv;
    int idx;
    long src_full, dst_full, result;

    test_log_section("历史索引哈希测试");

    // 测试: SRC(mv)<<8 | DST(mv)) & 4095
    // mv = MOVE(sqSrc, sqDst) = sqSrc + (sqDst << 8)
    mv = MOVE(51, 58);  // 黑车从sq51到sq58
    idx = positionHistoryIndex(mv);
    // 预期: ((51 << 8) | 58) & 4095 = (13056 | 58) & 4095 = 13114 & 4095 = 730
    src_full = 51;
    dst_full = 58;
    result = ((src_full << 8) | dst_full) & 4095;
    test_assert_int_eq("历史索引计算", (int)result, idx);

    // 不同走法应产生不同索引
    mv = MOVE(55, 71);
    idx = positionHistoryIndex(mv);
    test_assert_true("不同走法不同索引", idx != (int)result ? 1 : 0, "");
}

// 测试8: 红炮打黑马后黑车吃炮的完整场景
void test_rook_recapture()
{
    int numMoves;
    long mvs[128];
    int i;
    int found;
    int result;
    long mv_test;

    test_log_section("黑车吃回红炮完整测试");

    positionInit();
    positionClear();

    // 模拟红炮打黑马后的局面 (日志中的实际局面)
    // 黑方: 将(3,7)=55, 士(3,6)=54, 士(3,8)=56, 象(3,5)=53, 象(3,9)=57
    //        车(3,3)=51, 车(3,11)=59, 炮(5,4)=84, 炮(5,10)=90
    //        卒(6,3)=99, 卒(6,5)=101, 卒(6,7)=103, 卒(6,9)=105, 卒(6,11)=107
    // 红方: 帅(12,7)=199, 仕(12,6)=198, 仕(12,8)=200, 相(12,5)=197, 相(12,9)=201
    //        马(12,4)=196, 马(12,10)=202, 车(12,3)=195, 车(12,11)=203
    //        炮(10,4)=164, 兵(9,3)=147, 兵(9,5)=149, 兵(9,7)=151, 兵(9,9)=153, 兵(9,11)=155
    // 红炮(3,10)=58 (刚吃了黑马)

    positionAddPiece(55, 16 + PIECE_KING, 0);     // 黑将
    positionAddPiece(54, 16 + PIECE_ADVISOR, 0);   // 黑士
    positionAddPiece(56, 16 + PIECE_ADVISOR, 0);   // 黑士
    positionAddPiece(53, 16 + PIECE_BISHOP, 0);    // 黑象
    positionAddPiece(57, 16 + PIECE_BISHOP, 0);    // 黑象
    positionAddPiece(51, 16 + PIECE_ROOK, 0);      // 黑车a
    positionAddPiece(59, 16 + PIECE_ROOK, 0);      // 黑车b
    positionAddPiece(84, 16 + PIECE_CANNON, 0);    // 黑炮
    positionAddPiece(90, 16 + PIECE_CANNON, 0);    // 黑炮
    positionAddPiece(99, 16 + PIECE_PAWN, 0);      // 黑卒
    positionAddPiece(101, 16 + PIECE_PAWN, 0);     // 黑卒
    positionAddPiece(103, 16 + PIECE_PAWN, 0);     // 黑卒
    positionAddPiece(105, 16 + PIECE_PAWN, 0);     // 黑卒
    positionAddPiece(107, 16 + PIECE_PAWN, 0);     // 黑卒

    positionAddPiece(199, 8 + PIECE_KING, 0);      // 红帅
    positionAddPiece(198, 8 + PIECE_ADVISOR, 0);    // 红仕
    positionAddPiece(200, 8 + PIECE_ADVISOR, 0);    // 红仕
    positionAddPiece(197, 8 + PIECE_BISHOP, 0);     // 红相
    positionAddPiece(201, 8 + PIECE_BISHOP, 0);     // 红相
    positionAddPiece(196, 8 + PIECE_KNIGHT, 0);     // 红马
    positionAddPiece(202, 8 + PIECE_KNIGHT, 0);     // 红马
    positionAddPiece(195, 8 + PIECE_ROOK, 0);       // 红车
    positionAddPiece(203, 8 + PIECE_ROOK, 0);       // 红车
    positionAddPiece(164, 8 + PIECE_CANNON, 0);     // 红炮
    positionAddPiece(58, 8 + PIECE_CANNON, 0);      // 红炮(打马后)
    positionAddPiece(147, 8 + PIECE_PAWN, 0);       // 红兵
    positionAddPiece(149, 8 + PIECE_PAWN, 0);       // 红兵
    positionAddPiece(151, 8 + PIECE_PAWN, 0);       // 红兵
    positionAddPiece(153, 8 + PIECE_PAWN, 0);       // 红兵
    positionAddPiece(155, 8 + PIECE_PAWN, 0);       // 红兵

    g_pos_sdPlayer = 1; // 黑方走棋
    positionSetIrrev();

    // 1. 检查黑车(59)吃红炮(58)的走法是否生成
    numMoves = positionGenerateAllMoves(mvs);
    test_assert_true("黑方有走法", numMoves > 0 ? 1 : 0, "");

    found = 0;
    for (i = 0; i < numMoves; i++) {
        if (SRC(mvs[i]) == 59 && DST(mvs[i]) == 58) {
            found = 1;
        }
    }
    test_assert_true("黑车b吃红炮走法已生成", found, "");

    // 2. 检查黑车吃红炮是否合法 (positionMakeMove)
    if (found != 0) {
        mv_test = MOVE(59, 58);
        result = positionMakeMove(mv_test);
        test_assert_int_eq("黑车b吃红炮-positionMakeMove", 1, result);
        if (result != 0) {
            positionUndoMakeMove();
        }
    }

    // 3. 黑车a(51)到红炮(58)路径上有黑象(53)挡住
    //    positionLegalMove 应拒绝此走法
    mv_test = MOVE(51, 58);
    result = positionLegalMove(mv_test);
    test_assert_int_eq("黑车a吃红炮-路径被挡", 0, result);

    // 4. 检查checked()在此局面不误判
    g_pos_sdPlayer = 1;
    test_assert_int_eq("黑方不被将军", 0, checked());
}

// 测试9: 搜索层 - 红炮二进七打马后，黑方应吃回红炮
void test_search_cannon_capture()
{
    long mv;
    int sq_src, sq_dst;
    int capture_move;

    test_log_section("搜索层-炮二进七后黑方应吃炮");

    positionInit();
    positionClear();

    // 标准开局"炮二进七"后的局面
    // 红方二路炮从(7,7)进七到(0,7)打黑马
    // xqwlight坐标: sq = (col+3) + ((row+3)<<4)
    //
    // 黑方(上方):
    //   将(0,4)=sq67, 士(0,3)=sq66, 士(0,5)=sq68
    //   象(0,2)=sq65, 象(0,6)=sq69
    //   车(0,0)=sq51, 车(0,8)=sq59
    //   马(0,1)=sq52(已被打掉), 马(0,7)=sq58还在
    //   炮(2,1)=sq83, 炮(2,7)=sq89
    //   卒(3,0)=sq99, 卒(3,2)=sq101, 卒(3,4)=sq103, 卒(3,6)=sq105, 卒(3,8)=sq107
    // 红方(下方):
    //   帅(9,4)=sq147, 仕(9,3)=sq146, 仕(9,5)=sq148
    //   相(9,2)=sq145, 相(9,6)=sq149
    //   马(9,1)=sq144, 马(9,7)=sq150
    //   车(9,0)=sq143, 车(9,8)=sq151
    //   炮(7,1)=sq115, 炮(0,7)=sq58(打马后，吃了黑马)
    //   兵(6,0)=sq131, 兵(6,2)=sq133, 兵(6,4)=sq135, 兵(6,6)=sq137, 兵(6,8)=sq139

    // 黑方棋子
    positionAddPiece(67, 16 + PIECE_KING, 0);     // 黑将 (0,4)
    positionAddPiece(66, 16 + PIECE_ADVISOR, 0);   // 黑士 (0,3)
    positionAddPiece(68, 16 + PIECE_ADVISOR, 0);   // 黑士 (0,5)
    positionAddPiece(65, 16 + PIECE_BISHOP, 0);    // 黑象 (0,2)
    positionAddPiece(69, 16 + PIECE_BISHOP, 0);    // 黑象 (0,6)
    positionAddPiece(51, 16 + PIECE_ROOK, 0);      // 黑车 (0,0)
    positionAddPiece(59, 16 + PIECE_ROOK, 0);      // 黑车 (0,8)
    positionAddPiece(58, 16 + PIECE_KNIGHT, 0);    // 黑马 (0,7) 还在
    positionAddPiece(83, 16 + PIECE_CANNON, 0);    // 黑炮 (2,1)
    positionAddPiece(89, 16 + PIECE_CANNON, 0);    // 黑炮 (2,7)
    positionAddPiece(99, 16 + PIECE_PAWN, 0);      // 黑卒 (3,0)
    positionAddPiece(101, 16 + PIECE_PAWN, 0);     // 黑卒 (3,2)
    positionAddPiece(103, 16 + PIECE_PAWN, 0);     // 黑卒 (3,4)
    positionAddPiece(105, 16 + PIECE_PAWN, 0);     // 黑卒 (3,6)
    positionAddPiece(107, 16 + PIECE_PAWN, 0);     // 黑卒 (3,8)

    // 红方棋子
    positionAddPiece(147, 8 + PIECE_KING, 0);      // 红帅 (9,4)
    positionAddPiece(146, 8 + PIECE_ADVISOR, 0);   // 红仕 (9,3)
    positionAddPiece(148, 8 + PIECE_ADVISOR, 0);   // 红仕 (9,5)
    positionAddPiece(145, 8 + PIECE_BISHOP, 0);    // 红相 (9,2)
    positionAddPiece(149, 8 + PIECE_BISHOP, 0);    // 红相 (9,6)
    positionAddPiece(144, 8 + PIECE_KNIGHT, 0);    // 红马 (9,1)
    positionAddPiece(150, 8 + PIECE_KNIGHT, 0);    // 红马 (9,7)
    positionAddPiece(143, 8 + PIECE_ROOK, 0);      // 红车 (9,0)
    positionAddPiece(151, 8 + PIECE_ROOK, 0);      // 红车 (9,8)
    positionAddPiece(115, 8 + PIECE_CANNON, 0);    // 红炮 (7,1)
    positionAddPiece(170, 8 + PIECE_CANNON, 0);    // 红炮(7,7)→还没打马

    // 先模拟红炮打黑马: 红炮从sq170移到sq58，吃掉黑马sq58
    // 黑马sq58被吃掉
    positionAddPiece(58, 16 + PIECE_KNIGHT, 1);   // 删除黑马
    positionAddPiece(170, 8 + PIECE_CANNON, 1);   // 删除红炮原位
    positionAddPiece(58, 8 + PIECE_CANNON, 0);    // 红炮到sq58

    positionAddPiece(131, 8 + PIECE_PAWN, 0);      // 红兵 (6,0)
    positionAddPiece(133, 8 + PIECE_PAWN, 0);      // 红兵 (6,2)
    positionAddPiece(135, 8 + PIECE_PAWN, 0);      // 红兵 (6,4)
    positionAddPiece(137, 8 + PIECE_PAWN, 0);      // 红兵 (6,6)
    positionAddPiece(139, 8 + PIECE_PAWN, 0);      // 红兵 (6,8)

    g_pos_sdPlayer = 1; // 黑方走棋
    positionSetIrrev();

    // 设置超时（用GetTime获取当前秒数，避免立刻超时）
    g_ai_timeout_seconds = 30;
    {
        char time_buf[8];
        GetTime(time_buf);
        g_ai_start_second = time_buf[6];
    }

    // 调用搜索 (depth=4)
    mv = searchMain(4);

    DPRINTF("  AI result: mv=0x%lx\n", mv);

    if (mv != 0) {
        sq_src = SRC(mv);
        sq_dst = DST(mv);
        DPRINTF("  src=%d(row=%d,col=%d) dst=%d(row=%d,col=%d)\n",
               sq_src, sq_src >> 4, sq_src & 15,
               sq_dst, sq_dst >> 4, sq_dst & 15);
        DPRINTF("  src_pc=%d dst_pc=%d\n",
               (int)g_pos_squares[sq_src], (int)g_pos_squares[sq_dst]);

        // 黑车(59)应吃红炮(58)，或者黑马(58)已不存在
        // 红炮在sq58，黑车sq59紧邻，应能吃
        capture_move = 0;
        if (sq_dst == 58) {
            capture_move = 1;
        }
        test_assert_true("AI应选择吃红炮(sq58)", capture_move, "");
    } else {
        test_assert_true("AI应返回有效走法", 0, "searchMain返回0");
    }
}

// 测试10: 通过chess_map模拟真实流程 - 炮二进七后黑方应吃炮
void test_real_cannon_capture()
{
    int ai_from_col, ai_from_row, ai_to_col, ai_to_row;
    int capture_move;

    test_log_section("真实流程-炮二进七后黑方应吃炮");

    // 初始化标准开局
    chess_init();

    // 红方走"炮二进七": 红炮(7,7) -> (0,7) 打黑马
    // chess_map[row][col], RED_PAO=6, BLACK_MA=9
    chess_map[0][7] = RED_PAO;   // 红炮到(0,7)吃掉黑马
    chess_map[7][7] = 0;         // 红炮原位清空

    // 打印局面
    DPRINTF("  红炮已到(0,7), 黑车在(0,8)\n");
    DPRINTF("  chess_map[0][7]=%d(RED_PAO=6)\n", chess_map[0][7]);
    DPRINTF("  chess_map[0][8]=%d(BLACK_CHE=8)\n", chess_map[0][8]);
    DPRINTF("  chess_map[0][1]=%d(BLACK_MA=9)\n", chess_map[0][1]);

    // 调用AI (通过adapter，与真实游戏流程一致)
    ai_cchess_init();
    ai_cchess_set_timeout(30);
    ai_cchess_think_and_move(0);  // is_red=0 表示黑方走棋

    ai_from_col = ai_cchess_get_from_col();
    ai_from_row = ai_cchess_get_from_row();
    ai_to_col = ai_cchess_get_to_col();
    ai_to_row = ai_cchess_get_to_row();

    DPRINTF("  AI走法: 行%d列%d -> 行%d列%d\n", ai_from_row, ai_from_col, ai_to_row, ai_to_col);
    DPRINTF("  棋子=%d\n", chess_map[ai_from_row][ai_from_col]);

    // 黑车在行0列8，红炮在行0列7，黑车应吃红炮
    capture_move = 0;
    if (ai_to_col == 7 && ai_to_row == 0) {
        // 目标是红炮所在位置
        capture_move = 1;
    }
    test_assert_true("AI应选择吃红炮(行0,列7)", capture_move, "");

    if (capture_move == 0) {
        DPRINTF("  [诊断] AI没吃炮! 选了行%d列%d->行%d列%d\n",
               ai_from_row, ai_from_col, ai_to_row, ai_to_col);
        DPRINTF("  [诊断] 棋子=%d (8=黑车,9=黑马,13=黑炮)\n",
               chess_map[ai_from_row][ai_from_col]);
    }
}

// ==================== 游戏结束检测测试 ====================

// 测试11: 黑车吃掉红帅 - check_game_over 应返回 2（黑方胜）
void test_game_over_black_captures_red_king()
{
    int result;
    int row, col;

    test_log_section("游戏结束-黑方吃红帅");

    /* 初始化标准开局 */
    chess_init();

    /* 清空棋盘 */
    for (row = 0; row <= 9; row++)
    {
        for (col = 0; col <= 8; col++)
        {
            chess_map[row][col] = 0;
        }
    }

    /* 设置简单局面：红帅和黑车紧邻 */
    /* 红方：帅在 (9,4) */
    /* 黑方：将在 (0,4), 车在 (9,3) 可吃帅 */
    chess_map[9][4] = RED_SHUAI;    /* 红帅 */
    chess_map[0][4] = BLACK_JIANG;  /* 黑将 */
    chess_map[9][3] = BLACK_CHE;    /* 黑车 */

    /* 先测试吃帅前游戏未结束 */
    result = check_game_over();
    test_assert_int_eq("吃帅前-游戏继续", 0, result);

    /* 模拟黑车吃掉红帅 */
    chess_map[9][4] = BLACK_CHE;    /* 黑车移到帅的位置 */
    chess_map[9][3] = 0;            /* 原位清空 */

    result = check_game_over();
    test_assert_int_eq("黑车吃红帅-黑方胜", 2, result);
}

// 测试12: 黑方将死红方 - 红帅无法逃脱将军
void test_game_over_black_checkmates_red()
{
    int result;
    int row, col;

    test_log_section("游戏结束-黑方将死红方");

    /* 初始化标准开局 */
    chess_init();

    /* 清空棋盘 */
    for (row = 0; row <= 9; row++)
    {
        for (col = 0; col <= 8; col++)
        {
            chess_map[row][col] = 0;
        }
    }

    /* 设置将死局面：红帅被困在九宫内 */
    /* 红方：帅在 (7,4) -- 红宫内 */
    /* 黑方：将在 (0,3), 车在 (7,0), 车在 (8,0) */
    /* 黑车(7,0)在同行给帅将军，黑车(8,0)控制下一行 */
    /* 红帅无法逃到任何安全位置 -> 将死 */
    chess_map[7][4] = RED_SHUAI;    /* 红帅 */
    chess_map[0][3] = BLACK_JIANG;  /* 黑将 */
    chess_map[7][0] = BLACK_CHE;    /* 黑车a */
    chess_map[8][0] = BLACK_CHE;    /* 黑车b */

    /* 验证红方被将军 */
    test_assert_int_eq("红方被将军", 1, is_red_in_check());

    /* 验证将死 */
    test_assert_int_eq("红方被将死", 1, is_checkmate(0));

    /* check_game_over 应返回 2（黑方胜） */
    result = check_game_over();
    test_assert_int_eq("黑方将死红方-黑方胜", 2, result);
}

// 测试13: 红方将死黑方 - 黑将无法逃脱将军
void test_game_over_red_checkmates_black()
{
    int result;
    int row, col;

    test_log_section("游戏结束-红方将死黑方");

    /* 初始化标准开局 */
    chess_init();

    /* 清空棋盘 */
    for (row = 0; row <= 9; row++)
    {
        for (col = 0; col <= 8; col++)
        {
            chess_map[row][col] = 0;
        }
    }

    /* 设置将死局面：黑将被困在九宫内 */
    /* 黑方：将在 (2,4) -- 黑宫内 */
    /* 红方：帅在 (9,3), 车在 (2,0), 车在 (1,0) */
    /* 红车(2,0)在同行给将将军，红车(1,0)控制上一行 */
    /* 黑将无法逃到任何安全位置 -> 将死 */
    chess_map[2][4] = BLACK_JIANG;  /* 黑将 */
    chess_map[9][3] = RED_SHUAI;    /* 红帅 */
    chess_map[2][0] = RED_CHE;      /* 红车a */
    chess_map[1][0] = RED_CHE;      /* 红车b */

    /* 验证黑方被将军 */
    test_assert_int_eq("黑方被将军", 1, is_black_in_check());

    /* 验证将死 */
    test_assert_int_eq("黑方被将死", 1, is_checkmate(1));

    /* check_game_over 应返回 1（红方胜） */
    result = check_game_over();
    test_assert_int_eq("红方将死黑方-红方胜", 1, result);
}

// 测试14: 困毙 - 红方未被将军但无合法走法，红方判负
void test_game_over_red_stalemate()
{
    int result;
    int row, col;

    test_log_section("游戏结束-红方困毙");

    /* 初始化标准开局 */
    chess_init();

    /* 清空棋盘 */
    for (row = 0; row <= 9; row++)
    {
        for (col = 0; col <= 8; col++)
        {
            chess_map[row][col] = 0;
        }
    }

    /* 设置困毙局面：红帅和红仕被完全封锁 */
    /* 红方：帅在 (9,4), 仕在 (9,3), 仕在 (9,5) */
    /* 黑方：将在 (0,3), 车在 (8,3), 车在 (8,5) */
    /*       炮在 (7,4)（炮架）, 卒在 (7,3), 卒在 (7,5) */
    /* 红帅不能走 (9,3)/(9,5) 自己仕占着 */
    /* 红帅不能走 (8,3)/(8,5) 被黑车占着（走过去被吃但不是将军） */
    /* 红帅不能走 (8,4) 被黑炮通过卒攻击 */
    /* 仕不能走到任何不被将军的位置 */
    /* -> 红方无合法走法 -> 困毙 */
    chess_map[9][4] = RED_SHUAI;    /* 红帅 */
    chess_map[9][3] = RED_SHI;      /* 红仕 */
    chess_map[9][5] = RED_SHI;      /* 红仕 */
    chess_map[0][3] = BLACK_JIANG;  /* 黑将 */
    chess_map[8][3] = BLACK_CHE;    /* 黑车a */
    chess_map[8][5] = BLACK_CHE;    /* 黑车b */
    chess_map[7][4] = BLACK_PAO;    /* 黑炮 */
    chess_map[7][3] = BLACK_ZU;     /* 黑卒 */
    chess_map[7][5] = BLACK_ZU;     /* 黑卒 */

    /* 验证红方未被将军 */
    test_assert_int_eq("红方未被将军", 0, is_red_in_check());

    /* 验证红方无合法走法 */
    test_assert_int_eq("红方无合法走法", 0, has_legal_moves(1));

    /* check_game_over 应返回 2（红方困毙，黑方胜） */
    result = check_game_over();
    test_assert_int_eq("红方困毙-黑方胜", 2, result);
}

// ==================== runtest 分派函数 ====================

void runtest(char name[])
{
    int found;
    found = 0;

    if (str_equals(name, "test_piece_encoding")) {
        test_log_init();
        test_piece_encoding();
        test_log_summary();
        found = 1;
    }
    if (str_equals(name, "test_checked_flying_general")) {
        test_log_init();
        test_checked_flying_general();
        test_log_summary();
        found = 1;
    }
    if (str_equals(name, "test_checked_rook")) {
        test_log_init();
        test_checked_rook();
        test_log_summary();
        found = 1;
    }
    if (str_equals(name, "test_checked_cannon")) {
        test_log_init();
        test_checked_cannon();
        test_log_summary();
        found = 1;
    }
    if (str_equals(name, "test_make_move")) {
        test_log_init();
        test_make_move();
        test_log_summary();
        found = 1;
    }
    if (str_equals(name, "test_gui_flying_general_check")) {
        test_log_init();
        test_gui_flying_general_check();
        test_log_summary();
        found = 1;
    }
    if (str_equals(name, "test_gui_must_resolve_check")) {
        test_log_init();
        test_gui_must_resolve_check();
        test_log_summary();
        found = 1;
    }
    if (str_equals(name, "test_rook_capture")) {
        test_log_init();
        test_rook_capture();
        test_log_summary();
        found = 1;
    }
    if (str_equals(name, "test_history_index")) {
        test_log_init();
        test_history_index();
        test_log_summary();
        found = 1;
    }
    if (str_equals(name, "test_rook_recapture")) {
        test_log_init();
        test_rook_recapture();
        test_log_summary();
        found = 1;
    }
    if (str_equals(name, "test_search_cannon_capture")) {
        test_log_init();
        test_search_cannon_capture();
        test_log_summary();
        found = 1;
    }
    if (str_equals(name, "test_real_cannon_capture")) {
        test_log_init();
        test_real_cannon_capture();
        test_log_summary();
        found = 1;
    }
    if (str_equals(name, "test_game_over_black_captures_red_king")) {
        test_log_init();
        test_game_over_black_captures_red_king();
        test_log_summary();
        found = 1;
    }
    if (str_equals(name, "test_game_over_black_checkmates_red")) {
        test_log_init();
        test_game_over_black_checkmates_red();
        test_log_summary();
        found = 1;
    }
    if (str_equals(name, "test_game_over_red_checkmates_black")) {
        test_log_init();
        test_game_over_red_checkmates_black();
        test_log_summary();
        found = 1;
    }
    if (str_equals(name, "test_game_over_red_stalemate")) {
        test_log_init();
        test_game_over_red_stalemate();
        test_log_summary();
        found = 1;
    }
    if (str_equals(name, "test_all")) {
        test_log_init();
        test_piece_encoding();
        test_checked_flying_general();
        test_checked_rook();
        test_checked_cannon();
        test_make_move();
        test_gui_flying_general_check();
        test_gui_must_resolve_check();
        test_rook_capture();
        test_history_index();
        test_rook_recapture();
        test_search_cannon_capture();
        test_game_over_black_captures_red_king();
        test_game_over_black_checkmates_red();
        test_game_over_red_checkmates_black();
        test_game_over_red_stalemate();
        test_log_summary();
        found = 1;
    }

    if (found == 0) {
        DPRINTF("ERROR: unknown test '%s'\n", name);
        DPRINTF("Available tests:\n");
        DPRINTF("  test_piece_encoding\n");
        DPRINTF("  test_checked_flying_general\n");
        DPRINTF("  test_checked_rook\n");
        DPRINTF("  test_checked_cannon\n");
        DPRINTF("  test_make_move\n");
        DPRINTF("  test_gui_flying_general_check\n");
        DPRINTF("  test_gui_must_resolve_check\n");
        DPRINTF("  test_rook_capture\n");
        DPRINTF("  test_history_index\n");
        DPRINTF("  test_rook_recapture\n");
        DPRINTF("  test_search_cannon_capture\n");
        DPRINTF("  test_real_cannon_capture\n");
        DPRINTF("  test_game_over_black_captures_red_king\n");
        DPRINTF("  test_game_over_black_checkmates_red\n");
        DPRINTF("  test_game_over_red_checkmates_black\n");
        DPRINTF("  test_game_over_red_stalemate\n");
        DPRINTF("  test_all\n");
    }
}
