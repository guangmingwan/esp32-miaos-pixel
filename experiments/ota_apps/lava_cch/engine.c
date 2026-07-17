/**
 * @file engine.c
 * @brief 中国象棋游戏引擎 - 走法记法转换
 * @author QWen 3.5 千问大模型
 * @date 2026
 */

#include "xiangqi.h"

/* ==================== 常量定义 ==================== */
/* 颜色常量别名（兼容旧代码） */
#define COLOR_RED CC_COLOR_RED
#define COLOR_BLACK CC_COLOR_BLACK

// 棋子价值表
int PIECE_VALUES_PY[8];
int PIECE_VALUES_FC[8];

// 动态获取棋子价值
// int get_piece_value_dynamic(PieceType type, int move_count) {
//     return PIECE_VALUES_FC[type];
// }

// 棋子中文名称
char* PIECE_NAMES[8];

// 位置加成表
int POSITION_BONUS[10][9];

// 中文数字（二维数组）
char cn_nums[9][8];

// 红方棋子名称
char red_piece_names[8][8];

// 黑方棋子名称
char black_piece_names[8][8];

// 初始化中文数组
void init_chinese_names()
{
    // 中文数字
    strcpy(cn_nums[0], "一");
    strcpy(cn_nums[1], "二");
    strcpy(cn_nums[2], "三");
    strcpy(cn_nums[3], "四");
    strcpy(cn_nums[4], "五");
    strcpy(cn_nums[5], "六");
    strcpy(cn_nums[6], "七");
    strcpy(cn_nums[7], "八");
    strcpy(cn_nums[8], "九");

    // 红方棋子名称
    strcpy(red_piece_names[0], "?");
    strcpy(red_piece_names[1], "车");
    strcpy(red_piece_names[2], "马");
    strcpy(red_piece_names[3], "相");
    strcpy(red_piece_names[4], "士");
    strcpy(red_piece_names[5], "帅");
    strcpy(red_piece_names[6], "炮");
    strcpy(red_piece_names[7], "兵");

    // 黑方棋子名称
    strcpy(black_piece_names[0], "?");
    strcpy(black_piece_names[1], "车");
    strcpy(black_piece_names[2], "马");
    strcpy(black_piece_names[3], "象");
    strcpy(black_piece_names[4], "士");
    strcpy(black_piece_names[5], "将");
    strcpy(black_piece_names[6], "炮");
    strcpy(black_piece_names[7], "卒");
}

// 移动转标准棋谱记法（如"炮二平五"、"马 8 进 7"）
// board 是 char 类型的棋盘（chess_map 格式）：1-7=红方，8-14=黑方，0=空位
char* move_to_chinese_notation(struct Move *move, char board[][11], char buffer[]) {
    int piece, piece_type, piece_color;
    char* piece_name;
    char from_col_str[8];
    char dest_str[8];
    char* move_type;
    int row, idx, steps, pos;
    int same_piece_in_col, max_row, min_row;
    char prefix[4];
    int p, p_type, p_color;
    int is_straight;
    int is_diagonal;


    piece = board[move->from_row][move->from_col];

    // 空位
    if (piece == 0) {
        buffer[0] = '.';
        buffer[1] = '.';
        buffer[2] = 0;
        return buffer;
    }

    // 获取棋子类型和颜色 (1-7=红，8-14=黑)
    // main.c 定义：1=车，2=马，3=相，4=士，5=帅，6=炮，7=兵
    if (piece <= 7) {
        piece_color = COLOR_RED;
        piece_type = piece;
    } else {
        piece_color = COLOR_BLACK;
        piece_type = piece - 7;
    }

    // 棋子名称（根据 main.c 的编号）
    if (piece_color == COLOR_RED) {
        piece_name = red_piece_names[piece_type];
    } else {
        piece_name = black_piece_names[piece_type];
    }

    // 直走子判断（车=1，炮=6，兵=7，帅=5）
     is_straight = (piece_type == 1 || piece_type == 5 || piece_type == 6 || piece_type == 7);

    // 斜走子判断（马=2，相=3，士=4）
     is_diagonal = (piece_type == 2 || piece_type == 3 || piece_type == 4);

    // 检查同一列是否有多个相同棋子（需要"前/后"区分）
    same_piece_in_col = 0;
    max_row = -1;
    min_row = 10;
    prefix[0] = 0;

    for (row = 0; row < 10; row++) {
        p = board[row][move->from_col];
        if (p == 0) continue;

        if (p <= 7) {
            p_color = COLOR_RED;
            p_type = p;
        } else {
            p_color = COLOR_BLACK;
            p_type = p - 7;
        }

        if (p_type == piece_type && p_color == piece_color) {
            same_piece_in_col++;
            if (row > max_row) max_row = row;
            if (row < min_row) min_row = row;
        }
    }

    // 如果需要"前/后"区分
    if (same_piece_in_col > 1) {
        if (move->from_row == max_row) {
            strcpy(prefix, "前");
        } else if (move->from_row == min_row) {
            strcpy(prefix, "后");
        }
    }

    // 起始列记法
    if (piece_color == COLOR_RED) {
        // 红方：从右到左，col8→一，col0→九
        idx = 8 - move->from_col;
        from_col_str[0] = cn_nums[idx][0];
        from_col_str[1] = cn_nums[idx][1];
        from_col_str[2] = 0;
    } else {
        // 黑方：从右到左，col0→1，col8→9
        from_col_str[0] = (char)('0' + move->from_col + 1);
        from_col_str[1] = 0;
    }

    // 移动类型和目标
    if (move->from_row == move->to_row) {
        // 平移动
        move_type = "平";
        if (piece_color == COLOR_RED) {
            idx = 8 - move->to_col;
            dest_str[0] = cn_nums[idx][0];
            dest_str[1] = cn_nums[idx][1];
            dest_str[2] = 0;
        } else {
            dest_str[0] = (char)('0' + move->to_col + 1);
            dest_str[1] = 0;
        }
    } else if (piece_color == COLOR_RED) {
        // 红方：row 减小是进，row 增大是退
        if (move->to_row < move->from_row) {
            move_type = "进";
        } else {
            move_type = "退";
        }

        // 直走子用车马炮兵帅，斜走子用马相士
        if (is_straight) {
            // 直走子：用步数
            steps = move->from_row - move->to_row;
            if (steps < 0) steps = -steps;
            if (steps >= 1 && steps <= 9) {
                dest_str[0] = cn_nums[steps - 1][0];
                dest_str[1] = cn_nums[steps - 1][1];
                dest_str[2] = 0;
            } else {
                dest_str[0] = '?';
                dest_str[1] = 0;
            }
        } else {
            // 斜走子：用目标列号
            idx = 8 - move->to_col;
            dest_str[0] = cn_nums[idx][0];
            dest_str[1] = cn_nums[idx][1];
            dest_str[2] = 0;
        }
    } else {
        // 黑方：row 增大是进，row 减小是退
        if (move->to_row > move->from_row) {
            move_type = "进";
        } else {
            move_type = "退";
        }

        if (is_straight) {
            // 直走子：用步数
            steps = move->to_row - move->from_row;
            if (steps < 0) steps = -steps;
            dest_str[0] = (char)('0' + steps);
            dest_str[1] = 0;
        } else {
            // 斜走子：用目标列号
            dest_str[0] = (char)('0' + move->to_col + 1);
            dest_str[1] = 0;
        }
    }

    // 组合记谱（标准格式：棋子名 + 列号 + 移动类型 + 目标）
    pos = 0;

    // 前/后（如果有）
    if (prefix[0] != 0) {
        buffer[pos++] = prefix[0];
        buffer[pos++] = prefix[1];
    }

    // 棋子名称
    idx = 0;
    while (piece_name[idx] != 0 && pos < 20) {
        buffer[pos++] = piece_name[idx];
        idx = idx + 1;
    }

    // 起始列（无前/后时）
    if (prefix[0] == 0) {
        buffer[pos++] = from_col_str[0];
        if (from_col_str[1] != 0) {
            buffer[pos++] = from_col_str[1];
        }
    }

    // 移动类型
    idx = 0;
    while (move_type[idx] != 0 && pos < 20) {
        buffer[pos++] = move_type[idx];
        idx = idx + 1;
    }

    // 目标
    idx = 0;
    while (dest_str[idx] != 0 && pos < 20) {
        buffer[pos++] = dest_str[idx];
        idx = idx + 1;
    }

    buffer[pos] = 0;
    //printf("%s",buffer);
    return buffer;
}
