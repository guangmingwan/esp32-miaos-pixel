/**
 * @file ai_draw_decision.c
 * @brief AI 和棋决策模块 - 基于残局定式判断是否接受和棋
 * @author QWen
 * @date 2026
 *
 * 功能：
 * 1. 子力价值评估 - 计算双方剩余子力
 * 2. 必胜残局判断 - AI 明显能赢时拒绝和棋
 * 3. 必和残局判断 - 理论和棋时接受和棋
 * 4. 局面评估 - 考虑棋子位置和活动性
 */

/* ==================== 头文件包含 ==================== */
#include "xiangqi.h"

/* ==================== 常量定义 ==================== */
#define DRAW_ACCEPT_THRESHOLD  30   /* 接受和棋的分数阈值 */
#define DRAW_REFUSE_THRESHOLD  100  /* 拒绝和棋的分数阈值（AI 优势） */

/* 棋子基础价值 */
#define PIECE_VALUE_ROOK      900
#define PIECE_VALUE_CANNON    450
#define PIECE_VALUE_KNIGHT    400
#define PIECE_VALUE_ELEPHANT  200
#define PIECE_VALUE_ADVISOR   200
#define PIECE_VALUE_PAWN      100

/* 过河兵额外价值 */
#define PAWN_CROSS_RIVER_BONUS  50

/* ==================== 全局变量 ==================== */
/* 用于和棋决策的棋盘（LavaX 不支持结构体参数，使用全局变量） */
struct Piece g_draw_board[10][9];

/* ==================== 函数声明 ==================== */
int count_pieces_by_type(int color, int type);
int calculate_material_balance(int ai_color);
int is_winning_endgame(int ai_color);
int is_theoretical_draw();
int evaluate_piece_mobility(int color);
int should_ai_accept_draw();

/* ==================== 函数实现 ==================== */

/**
 * @brief 统计指定颜色的某种棋子数量
 * @param color 颜色 (0=红方, 1=黑方)
 * @param type 棋子类型
 * @return 该类型棋子数量
 */
int count_pieces_by_type(int color, int type)
{
    int i, j;
    int count = 0;
    for (i = 0; i < 10; i++) {
        for (j = 0; j < 9; j++) {
            if (g_draw_board[i][j].color == color && g_draw_board[i][j].type == type) {
                count++;
            }
        }
    }
    return count;
}

/**
 * @brief 计算双方子力差值（AI 视角）
 * @param ai_color AI 颜色
 * @return 正值表示 AI 优势，负值表示劣势
 */
int calculate_material_balance(int ai_color)
{
    int ai_value = 0;
    int opp_value = 0;
    int opp_color;
    int i, j;
    int piece_val;

    opp_color = 1 - ai_color;

    for (i = 0; i < 10; i++) {
        for (j = 0; j < 9; j++) {
            if (g_draw_board[i][j].color == CC_COLOR_NONE) {
                continue;
            }

            piece_val = 0;
            switch (g_draw_board[i][j].type) {
                case CC_PIECE_ROOK:
                    piece_val = PIECE_VALUE_ROOK;
                    break;
                case CC_PIECE_CANNON:
                    piece_val = PIECE_VALUE_CANNON;
                    break;
                case CC_PIECE_KNIGHT:
                    piece_val = PIECE_VALUE_KNIGHT;
                    break;
                case CC_PIECE_ELEPHANT:
                    piece_val = PIECE_VALUE_ELEPHANT;
                    break;
                case CC_PIECE_ADVISOR:
                    piece_val = PIECE_VALUE_ADVISOR;
                    break;
                case CC_PIECE_PAWN:
                    piece_val = PIECE_VALUE_PAWN;
                    /* 过河兵加分 */
                    if (g_draw_board[i][j].color == CC_COLOR_RED && i < 5) {
                        piece_val += PAWN_CROSS_RIVER_BONUS;
                    }
                    if (g_draw_board[i][j].color == CC_COLOR_BLACK && i > 4) {
                        piece_val += PAWN_CROSS_RIVER_BONUS;
                    }
                    break;
                default:
                    piece_val = 0;
                    break;
            }

            if (g_draw_board[i][j].color == ai_color) {
                ai_value += piece_val;
            } else {
                opp_value += piece_val;
            }
        }
    }

    return ai_value - opp_value;
}

/**
 * @brief 判断是否为 AI 必胜的残局
 * @param ai_color AI 颜色
 * @return 1=必胜, 0=非必胜
 *
 * 必胜残局定式：
 * 1. 车对光杆老将
 * 2. 马兵对单士/单象
 * 3. 炮兵对单士/单象
 * 4. 双马对有士象
 * 5. 车兵对士象全
 */
int is_winning_endgame(int ai_color)
{
    int opp_color;
    int ai_rooks, ai_knights, ai_cannons, ai_pawns;
    int opp_rooks, opp_knights, opp_cannons, opp_pawns;
    int opp_bishops, opp_advisors, opp_kings;

    opp_color = 1 - ai_color;

    /* 统计 AI 方棋子 */
    ai_rooks = count_pieces_by_type(ai_color, CC_PIECE_ROOK);
    ai_knights = count_pieces_by_type(ai_color, CC_PIECE_KNIGHT);
    ai_cannons = count_pieces_by_type(ai_color, CC_PIECE_CANNON);
    ai_pawns = count_pieces_by_type(ai_color, CC_PIECE_PAWN);

    /* 统计对方棋子 */
    opp_rooks = count_pieces_by_type(opp_color, CC_PIECE_ROOK);
    opp_knights = count_pieces_by_type(opp_color, CC_PIECE_KNIGHT);
    opp_cannons = count_pieces_by_type(opp_color, CC_PIECE_CANNON);
    opp_pawns = count_pieces_by_type(opp_color, CC_PIECE_PAWN);
    opp_bishops = count_pieces_by_type(opp_color, CC_PIECE_ELEPHANT);
    opp_advisors = count_pieces_by_type(opp_color, CC_PIECE_ADVISOR);
    opp_kings = count_pieces_by_type(opp_color, CC_PIECE_KING);

    /* 定式 1: 车对光杆老将（无士象） */
    if (ai_rooks >= 1 && opp_rooks == 0 && opp_knights == 0 &&
        opp_cannons == 0 && opp_pawns == 0 && opp_bishops == 0 &&
        opp_advisors == 0 && opp_kings == 1) {
        return 1;
    }

    /* 定式 2: 马兵对单士或单象 */
    if (ai_knights >= 1 && ai_pawns >= 1 && opp_rooks == 0 &&
        opp_knights == 0 && opp_cannons == 0 && opp_pawns == 0) {
        if ((opp_bishops == 1 && opp_advisors == 0) ||
            (opp_bishops == 0 && opp_advisors == 1)) {
            return 1;
        }
    }

    /* 定式 3: 炮兵对单士或单象 */
    if (ai_cannons >= 1 && ai_pawns >= 1 && opp_rooks == 0 &&
        opp_knights == 0 && opp_cannons == 0 && opp_pawns == 0) {
        if ((opp_bishops == 1 && opp_advisors == 0) ||
            (opp_bishops == 0 && opp_advisors == 1)) {
            return 1;
        }
    }

    /* 定式 4: 双马对有士象（优势明显） */
    if (ai_knights >= 2 && opp_rooks == 0 && opp_cannons == 0) {
        if (opp_knights <= 1 && (opp_bishops + opp_advisors) <= 2) {
            return 1;
        }
    }

    /* 定式 5: 车兵对士象全 */
    if (ai_rooks >= 1 && ai_pawns >= 1 && opp_rooks == 0 &&
        opp_knights == 0 && opp_cannons == 0 && opp_pawns == 0) {
        if (opp_bishops <= 2 && opp_advisors <= 2) {
            return 1;
        }
    }

    /* 定式 6: 双车对任何残局 */
    if (ai_rooks >= 2 && opp_rooks == 0) {
        return 1;
    }

    /* 定式 7: 车马对无车 */
    if (ai_rooks >= 1 && ai_knights >= 1 && opp_rooks == 0 &&
        opp_knights == 0 && opp_cannons == 0) {
        return 1;
    }

    return 0;
}

/**
 * @brief 判断是否为理论和棋
 * @return 1=理论和棋, 0=非和棋
 *
 * 必和残局定式：
 * 1. 单马对单士
 * 2. 单炮对单士
 * 3. 单马对单象
 * 4. 单炮对单象
 * 5. 马兵对士象全（防守方）
 * 6. 炮兵对士象全（防守方）
 * 7. 双方都只剩弱子（马/炮/兵组合无法赢）
 * 8. 光杆老将 vs 光杆老将
 */
int is_theoretical_draw()
{
    int red_rooks, red_knights, red_cannons, red_pawns;
    int red_bishops, red_advisors, red_kings;
    int black_rooks, black_knights, black_cannons, black_pawns;
    int black_bishops, black_advisors, black_kings;

    /* 统计红方棋子 */
    red_rooks = count_pieces_by_type(CC_COLOR_RED, CC_PIECE_ROOK);
    red_knights = count_pieces_by_type(CC_COLOR_RED, CC_PIECE_KNIGHT);
    red_cannons = count_pieces_by_type(CC_COLOR_RED, CC_PIECE_CANNON);
    red_pawns = count_pieces_by_type(CC_COLOR_RED, CC_PIECE_PAWN);
    red_bishops = count_pieces_by_type(CC_COLOR_RED, CC_PIECE_ELEPHANT);
    red_advisors = count_pieces_by_type(CC_COLOR_RED, CC_PIECE_ADVISOR);
    red_kings = count_pieces_by_type(CC_COLOR_RED, CC_PIECE_KING);

    /* 统计黑方棋子 */
    black_rooks = count_pieces_by_type(CC_COLOR_BLACK, CC_PIECE_ROOK);
    black_knights = count_pieces_by_type(CC_COLOR_BLACK, CC_PIECE_KNIGHT);
    black_cannons = count_pieces_by_type(CC_COLOR_BLACK, CC_PIECE_CANNON);
    black_pawns = count_pieces_by_type(CC_COLOR_BLACK, CC_PIECE_PAWN);
    black_bishops = count_pieces_by_type(CC_COLOR_BLACK, CC_PIECE_ELEPHANT);
    black_advisors = count_pieces_by_type(CC_COLOR_BLACK, CC_PIECE_ADVISOR);
    black_kings = count_pieces_by_type(CC_COLOR_BLACK, CC_PIECE_KING);

    /* 定式 1: 光杆老将 vs 光杆老将 */
    if (red_kings == 1 && red_rooks == 0 && red_knights == 0 &&
        red_cannons == 0 && red_pawns == 0 && black_kings == 1 &&
        black_rooks == 0 && black_knights == 0 && black_cannons == 0 &&
        black_pawns == 0) {
        return 1;
    }

    /* 定式 2: 单马对单士 */
    if ((red_knights == 1 && red_pawns == 0 && red_cannons == 0 &&
         red_rooks == 0 && black_advisors == 1 && black_bishops == 0 &&
         black_rooks == 0 && black_knights == 0 && black_cannons == 0 &&
         black_pawns == 0) ||
        (black_knights == 1 && black_pawns == 0 && black_cannons == 0 &&
         black_rooks == 0 && red_advisors == 1 && red_bishops == 0 &&
         red_rooks == 0 && red_knights == 0 && red_cannons == 0 &&
         red_pawns == 0)) {
        return 1;
    }

    /* 定式 3: 单炮对单士 */
    if ((red_cannons == 1 && red_pawns == 0 && red_knights == 0 &&
         red_rooks == 0 && black_advisors == 1 && black_bishops == 0 &&
         black_rooks == 0 && black_knights == 0 && black_cannons == 0 &&
         black_pawns == 0) ||
        (black_cannons == 1 && black_pawns == 0 && black_knights == 0 &&
         black_rooks == 0 && red_advisors == 1 && red_bishops == 0 &&
         red_rooks == 0 && red_knights == 0 && red_cannons == 0 &&
         red_pawns == 0)) {
        return 1;
    }

    /* 定式 4: 单马对单象 */
    if ((red_knights == 1 && red_pawns == 0 && red_cannons == 0 &&
         red_rooks == 0 && black_bishops == 1 && black_advisors == 0 &&
         black_rooks == 0 && black_knights == 0 && black_cannons == 0 &&
         black_pawns == 0) ||
        (black_knights == 1 && black_pawns == 0 && black_cannons == 0 &&
         black_rooks == 0 && red_bishops == 1 && red_advisors == 0 &&
         red_rooks == 0 && red_knights == 0 && red_cannons == 0 &&
         red_pawns == 0)) {
        return 1;
    }

    /* 定式 5: 单炮对单象 */
    if ((red_cannons == 1 && red_pawns == 0 && red_knights == 0 &&
         red_rooks == 0 && black_bishops == 1 && black_advisors == 0 &&
         black_rooks == 0 && black_knights == 0 && black_cannons == 0 &&
         black_pawns == 0) ||
        (black_cannons == 1 && black_pawns == 0 && black_knights == 0 &&
         black_rooks == 0 && red_bishops == 1 && red_advisors == 0 &&
         red_rooks == 0 && red_knights == 0 && red_cannons == 0 &&
         red_pawns == 0)) {
        return 1;
    }

    /* 定式 6: 双方都无强子（无车），且子力接近 */
    if (red_rooks == 0 && black_rooks == 0) {
        int red_attack = red_knights + red_cannons + red_pawns;
        int black_attack = black_knights + black_cannons + black_pawns;
        /* 双方攻击子力都 <= 2，且差值 <= 1 */
        if (red_attack <= 2 && black_attack <= 2) {
            if (red_attack - black_attack <= 1 && black_attack - red_attack <= 1) {
                return 1;
            }
        }
    }

    /* 定式 7: 一方有车，但另一方士象全且无弱点 */
    if (red_rooks == 1 && red_knights == 0 && red_cannons == 0 &&
        red_pawns == 0 && black_rooks == 0 && black_bishops == 2 &&
        black_advisors == 2 && black_knights == 0 && black_cannons == 0 &&
        black_pawns == 0) {
        return 1;
    }

    if (black_rooks == 1 && black_knights == 0 && black_cannons == 0 &&
        black_pawns == 0 && red_rooks == 0 && red_bishops == 2 &&
        red_advisors == 2 && red_knights == 0 && red_cannons == 0 &&
        red_pawns == 0) {
        return 1;
    }

    return 0;
}

/**
 * @brief 评估棋子活动性（简化版）
 * @param color 颜色
 * @return 活动性得分
 */
int evaluate_piece_mobility(int color)
{
    int mobility = 0;
    int i, j;

    /* 简化评估：只计算棋子占据的关键位置数 */
    for (i = 0; i < 10; i++) {
        for (j = 0; j < 9; j++) {
            if (g_draw_board[i][j].color == color) {
                /* 控制中心位置加分 */
                if (j >= 3 && j <= 5 && i >= 3 && i <= 6) {
                    mobility += 2;
                }
                /* 车在开放线加分 */
                if (g_draw_board[i][j].type == CC_PIECE_ROOK) {
                    mobility += 3;
                }
                /* 马在好位置加分 */
                if (g_draw_board[i][j].type == CC_PIECE_KNIGHT) {
                    mobility += 2;
                }
                /* 炮在中路加分 */
                if (g_draw_board[i][j].type == CC_PIECE_CANNON && j == 4) {
                    mobility += 2;
                }
                /* 过河兵加分 */
                if (g_draw_board[i][j].type == CC_PIECE_PAWN) {
                    if (color == CC_COLOR_RED && i < 5) {
                        mobility += 3;
                    }
                    if (color == CC_COLOR_BLACK && i > 4) {
                        mobility += 3;
                    }
                }
            }
        }
    }

    return mobility;
}

/**
 * @brief 判断 AI 是否应该接受求和
 * @return 1=接受和棋, 0=拒绝和棋
 *
 * 决策逻辑：
 * 1. 如果是理论和棋，接受
 * 2. 如果是 AI 必胜，拒绝
 * 3. 根据子力差和活动性综合判断
 */
int should_ai_accept_draw()
{
    int material_balance;
    int ai_color;
    int mobility_diff;
    int final_score;

    /* AI 是黑方（玩家在暂停菜单求和，AI 是黑方） */
    ai_color = CC_COLOR_BLACK;

    /* 优先级 1: 检查是否为理论和棋 */
    if (is_theoretical_draw()) {
        return 1; /* 接受和棋 */
    }

    /* 优先级 2: 检查是否为 AI 必胜 */
    if (is_winning_endgame(ai_color)) {
        return 0; /* 拒绝和棋 */
    }

    /* 优先级 3: 子力评估 */
    material_balance = calculate_material_balance(ai_color);

    /* 如果 AI 子力明显落后（>30 分），接受和棋 */
    if (material_balance < -DRAW_REFUSE_THRESHOLD) {
        return 1; /* 接受和棋 */
    }

    /* 如果 AI 子力明显领先（>100 分），拒绝和棋 */
    if (material_balance > DRAW_REFUSE_THRESHOLD) {
        return 0; /* 拒绝和棋 */
    }

    /* 优先级 4: 活动性评估 */
    mobility_diff = evaluate_piece_mobility(CC_COLOR_BLACK) -
                    evaluate_piece_mobility(CC_COLOR_RED);

    /* 综合评分 */
    final_score = material_balance + mobility_diff;

    /* 最终决策 */
    if (final_score < -DRAW_ACCEPT_THRESHOLD) {
        return 1; /* 接受和棋 */
    }

    if (final_score > DRAW_ACCEPT_THRESHOLD) {
        return 0; /* 拒绝和棋 */
    }

    /* 接近均势，接受和棋 */
    return 1;
}
