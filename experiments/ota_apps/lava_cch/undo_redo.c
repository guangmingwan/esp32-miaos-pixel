/**
 * @file undo_redo.c
 * @brief 悔棋和恢复功能模块
 * @author QWen
 * @date 2026
 *
 * 功能：
 * 1. 记录每步走棋历史（包括吃子信息）
 * 2. 悔棋（undo）- 撤销走棋
 * 3. 恢复（redo）- 重新执行被撤销的走棋
 */

/* ==================== 头文件包含 ==================== */
#include "xiangqi.h"

/* ==================== 常量定义 ==================== */
#define MAX_UNDO_REDO_HISTORY 200  /* 最大悔棋/恢复历史记录数 */

/* ==================== 全局变量 ==================== */
/* 悔棋历史记录 */
struct MoveRecord g_undo_history[MAX_UNDO_REDO_HISTORY];
int g_undo_history_count = 0;

/* 恢复历史记录 */
struct MoveRecord g_redo_history[MAX_UNDO_REDO_HISTORY];
int g_redo_history_count = 0;

/* 当前轮到哪方（用于悔棋时判断） */
int g_undo_current_turn = 1;  /* 1=红方，2=黑方 */

/* 保存每步棋后的最后走棋位置标记 */
int g_undo_last_red_from_col[MAX_UNDO_REDO_HISTORY];
int g_undo_last_red_from_row[MAX_UNDO_REDO_HISTORY];
int g_undo_last_red_to_col[MAX_UNDO_REDO_HISTORY];
int g_undo_last_red_to_row[MAX_UNDO_REDO_HISTORY];
int g_undo_last_black_from_col[MAX_UNDO_REDO_HISTORY];
int g_undo_last_black_from_row[MAX_UNDO_REDO_HISTORY];
int g_undo_last_black_to_col[MAX_UNDO_REDO_HISTORY];
int g_undo_last_black_to_row[MAX_UNDO_REDO_HISTORY];

/* 恢复历史中的最后走棋位置标记 */
int g_redo_last_red_from_col[MAX_UNDO_REDO_HISTORY];
int g_redo_last_red_from_row[MAX_UNDO_REDO_HISTORY];
int g_redo_last_red_to_col[MAX_UNDO_REDO_HISTORY];
int g_redo_last_red_to_row[MAX_UNDO_REDO_HISTORY];
int g_redo_last_black_from_col[MAX_UNDO_REDO_HISTORY];
int g_redo_last_black_from_row[MAX_UNDO_REDO_HISTORY];
int g_redo_last_black_to_col[MAX_UNDO_REDO_HISTORY];
int g_redo_last_black_to_row[MAX_UNDO_REDO_HISTORY];

/* 用于记录前的位置标记（由 main.c 设置） */
int g_record_red_from_col, g_record_red_from_row, g_record_red_to_col, g_record_red_to_row;
int g_record_black_from_col, g_record_black_from_row, g_record_black_to_col, g_record_black_to_row;

/* 外部全局变量引用（来自 main.c） - 由于 undo_redo.c 被 include 进 main.c，这些变量已经可用 */
/* extern int last_red_from_col, last_red_from_row, last_red_to_col, last_red_to_row; */
/* extern int last_black_from_col, last_black_from_row, last_black_to_col, last_black_to_row; */
/* extern int chess_map[12][11]; */

/* ==================== 函数声明 ==================== */
void undo_redo_init();
void undo_redo_record_move(int from_row, int from_col, int to_row, int to_col, int captured_piece);
int undo_redo_undo();
int undo_redo_redo();
void undo_redo_clear_redo();

/* ==================== 函数实现 ==================== */

/**
 * @brief 初始化悔棋/恢复模块
 */
void undo_redo_init()
{
    int i;
    g_undo_history_count = 0;
    g_redo_history_count = 0;
    g_undo_current_turn = 1;

    /* 清空历史记录 */
    for (i = 0; i < MAX_UNDO_REDO_HISTORY; i++) {
        g_undo_history[i].move.from_row = -1;
        g_undo_history[i].move.from_col = -1;
        g_undo_history[i].move.to_row = -1;
        g_undo_history[i].move.to_col = -1;
        g_undo_history[i].captured_piece.type = CC_PIECE_EMPTY;
        g_undo_history[i].captured_piece.color = CC_COLOR_NONE;

        g_redo_history[i].move.from_row = -1;
        g_redo_history[i].move.from_col = -1;
        g_redo_history[i].move.to_row = -1;
        g_redo_history[i].move.to_col = -1;
        g_redo_history[i].captured_piece.type = CC_PIECE_EMPTY;
        g_redo_history[i].captured_piece.color = CC_COLOR_NONE;

        /* 初始化位置标记数组 */
        g_undo_last_red_from_col[i] = -1;
        g_undo_last_red_from_row[i] = -1;
        g_undo_last_red_to_col[i] = -1;
        g_undo_last_red_to_row[i] = -1;
        g_undo_last_black_from_col[i] = -1;
        g_undo_last_black_from_row[i] = -1;
        g_undo_last_black_to_col[i] = -1;
        g_undo_last_black_to_row[i] = -1;

        g_redo_last_red_from_col[i] = -1;
        g_redo_last_red_from_row[i] = -1;
        g_redo_last_red_to_col[i] = -1;
        g_redo_last_red_to_row[i] = -1;
        g_redo_last_black_from_col[i] = -1;
        g_redo_last_black_from_row[i] = -1;
        g_redo_last_black_to_col[i] = -1;
        g_redo_last_black_to_row[i] = -1;
    }
}

/**
 * @brief 记录一步走棋历史
 * @param from_row 起始行
 * @param from_col 起始列
 * @param to_row 目标行
 * @param to_col 目标列
 * @param captured_piece 被吃掉的棋子（0表示未吃子）
 * @note 位置标记通过全局变量 g_record_red_from_col 等传递
 */
void undo_redo_record_move(int from_row, int from_col, int to_row, int to_col, int captured_piece)
{
    int piece_type;
    int piece_color;
    int idx;
    int red_from_col, red_from_row, red_to_col, red_to_row;
    int black_from_col, black_from_row, black_to_col, black_to_row;

    /* 从全局变量获取位置标记 */
    red_from_col = g_record_red_from_col;
    red_from_row = g_record_red_from_row;
    red_to_col = g_record_red_to_col;
    red_to_row = g_record_red_to_row;
    black_from_col = g_record_black_from_col;
    black_from_row = g_record_black_from_row;
    black_to_col = g_record_black_to_col;
    black_to_row = g_record_black_to_row;

    if (g_undo_history_count >= MAX_UNDO_REDO_HISTORY) {
        return;  /* 历史记录已满 */
    }

    idx = g_undo_history_count;

    /* 记录走棋信息 */
    g_undo_history[idx].move.from_row = from_row;
    g_undo_history[idx].move.from_col = from_col;
    g_undo_history[idx].move.to_row = to_row;
    g_undo_history[idx].move.to_col = to_col;

    /* 解析被吃掉的棋子 */
    if (captured_piece == 0) {
        g_undo_history[idx].captured_piece.type = CC_PIECE_EMPTY;
        g_undo_history[idx].captured_piece.color = CC_COLOR_NONE;
    } else if (captured_piece >= 1 && captured_piece <= 7) {
        /* 红方棋子 */
        g_undo_history[idx].captured_piece.color = CC_COLOR_RED;
        piece_type = 0;
        if (captured_piece == 1) piece_type = CC_PIECE_ROOK;
        else if (captured_piece == 2) piece_type = CC_PIECE_KNIGHT;
        else if (captured_piece == 3) piece_type = CC_PIECE_ELEPHANT;
        else if (captured_piece == 4) piece_type = CC_PIECE_ADVISOR;
        else if (captured_piece == 5) piece_type = CC_PIECE_KING;
        else if (captured_piece == 6) piece_type = CC_PIECE_CANNON;
        else if (captured_piece == 7) piece_type = CC_PIECE_PAWN;
        g_undo_history[idx].captured_piece.type = piece_type;
    } else if (captured_piece >= 8 && captured_piece <= 14) {
        /* 黑方棋子 */
        g_undo_history[idx].captured_piece.color = CC_COLOR_BLACK;
        piece_type = 0;
        if (captured_piece == 8) piece_type = CC_PIECE_ROOK;
        else if (captured_piece == 9) piece_type = CC_PIECE_KNIGHT;
        else if (captured_piece == 10) piece_type = CC_PIECE_ELEPHANT;
        else if (captured_piece == 11) piece_type = CC_PIECE_ADVISOR;
        else if (captured_piece == 12) piece_type = CC_PIECE_KING;
        else if (captured_piece == 13) piece_type = CC_PIECE_CANNON;
        else if (captured_piece == 14) piece_type = CC_PIECE_PAWN;
        g_undo_history[idx].captured_piece.type = piece_type;
    } else {
        g_undo_history[idx].captured_piece.type = CC_PIECE_EMPTY;
        g_undo_history[idx].captured_piece.color = CC_COLOR_NONE;
    }

    /* 保存位置标记 */
    g_undo_last_red_from_col[idx] = red_from_col;
    g_undo_last_red_from_row[idx] = red_from_row;
    g_undo_last_red_to_col[idx] = red_to_col;
    g_undo_last_red_to_row[idx] = red_to_row;
    g_undo_last_black_from_col[idx] = black_from_col;
    g_undo_last_black_from_row[idx] = black_from_row;
    g_undo_last_black_to_col[idx] = black_to_col;
    g_undo_last_black_to_row[idx] = black_to_row;

    g_undo_history_count++;

    /* 每次走新棋后，清空恢复历史 */
    g_redo_history_count = 0;
}

/**
 * @brief 将 Piece 结构转换为 chess_map 棋子编号
 * @param piece_type 棋子类型
 * @param piece_color 棋子颜色
 * @return 棋子编号（1-14），0表示空
 */
int piece_to_chess_map_id(int piece_type, int piece_color)
{
    int result = 0;

    if (piece_type == CC_PIECE_EMPTY) {
        return 0;
    }

    if (piece_color == CC_COLOR_RED) {
        if (piece_type == CC_PIECE_ROOK) result = 1;
        else if (piece_type == CC_PIECE_KNIGHT) result = 2;
        else if (piece_type == CC_PIECE_ELEPHANT) result = 3;
        else if (piece_type == CC_PIECE_ADVISOR) result = 4;
        else if (piece_type == CC_PIECE_KING) result = 5;
        else if (piece_type == CC_PIECE_CANNON) result = 6;
        else if (piece_type == CC_PIECE_PAWN) result = 7;
    } else if (piece_color == CC_COLOR_BLACK) {
        if (piece_type == CC_PIECE_ROOK) result = 8;
        else if (piece_type == CC_PIECE_KNIGHT) result = 9;
        else if (piece_type == CC_PIECE_ELEPHANT) result = 10;
        else if (piece_type == CC_PIECE_ADVISOR) result = 11;
        else if (piece_type == CC_PIECE_KING) result = 12;
        else if (piece_type == CC_PIECE_CANNON) result = 13;
        else if (piece_type == CC_PIECE_PAWN) result = 14;
    }

    return result;
}

/**
 * @brief 执行悔棋操作
 * @return 1=成功，0=失败（无历史记录）
 */
int undo_redo_undo()
{
    int move_idx;
    int from_row, from_col, to_row, to_col;
    int moved_piece;
    struct Piece captured;

    if (g_undo_history_count <= 0) {
        return 0;  /* 无历史记录 */
    }

    /* 获取最后一步历史记录 */
    move_idx = g_undo_history_count - 1;
    from_row = g_undo_history[move_idx].move.from_row;
    from_col = g_undo_history[move_idx].move.from_col;
    to_row = g_undo_history[move_idx].move.to_row;
    to_col = g_undo_history[move_idx].move.to_col;

    /* 获取被吃掉的棋子（逐字段复制） */
    captured.type = g_undo_history[move_idx].captured_piece.type;
    captured.color = g_undo_history[move_idx].captured_piece.color;

    /* 获取移动的棋子（当前在目标位置） */
    moved_piece = chess_map[to_row][to_col];

    /* 撤销走棋：将棋子移回原位置 */
    chess_map[from_row][from_col] = moved_piece;

    /* 恢复被吃掉的棋子 */
    if (captured.type != CC_PIECE_EMPTY) {
        chess_map[to_row][to_col] = piece_to_chess_map_id(captured.type, captured.color);
    } else {
        chess_map[to_row][to_col] = 0;
    }

    /* 将这步棋移到恢复历史（逐字段复制） */
    if (g_redo_history_count < MAX_UNDO_REDO_HISTORY) {
        int redo_idx = g_redo_history_count;
        g_redo_history[redo_idx].move.from_row = g_undo_history[move_idx].move.from_row;
        g_redo_history[redo_idx].move.from_col = g_undo_history[move_idx].move.from_col;
        g_redo_history[redo_idx].move.to_row = g_undo_history[move_idx].move.to_row;
        g_redo_history[redo_idx].move.to_col = g_undo_history[move_idx].move.to_col;
        g_redo_history[redo_idx].captured_piece.type = g_undo_history[move_idx].captured_piece.type;
        g_redo_history[redo_idx].captured_piece.color = g_undo_history[move_idx].captured_piece.color;

        /* 保存当前位置标记到恢复历史 */
        g_redo_last_red_from_col[redo_idx] = last_red_from_col;
        g_redo_last_red_from_row[redo_idx] = last_red_from_row;
        g_redo_last_red_to_col[redo_idx] = last_red_to_col;
        g_redo_last_red_to_row[redo_idx] = last_red_to_row;
        g_redo_last_black_from_col[redo_idx] = last_black_from_col;
        g_redo_last_black_from_row[redo_idx] = last_black_from_row;
        g_redo_last_black_to_col[redo_idx] = last_black_to_col;
        g_redo_last_black_to_row[redo_idx] = last_black_to_row;

        g_redo_history_count++;
    }

    /* 减少悔棋历史计数 */
    g_undo_history_count--;

    /* 恢复位置标记到悔棋前的状态 */
    if (g_undo_history_count > 0) {
        int prev_idx = g_undo_history_count - 1;
        last_red_from_col = g_undo_last_red_from_col[prev_idx];
        last_red_from_row = g_undo_last_red_from_row[prev_idx];
        last_red_to_col = g_undo_last_red_to_col[prev_idx];
        last_red_to_row = g_undo_last_red_to_row[prev_idx];
        last_black_from_col = g_undo_last_black_from_col[prev_idx];
        last_black_from_row = g_undo_last_black_from_row[prev_idx];
        last_black_to_col = g_undo_last_black_to_col[prev_idx];
        last_black_to_row = g_undo_last_black_to_row[prev_idx];
    } else {
        /* 没有历史记录了，重置为初始状态 */
        last_red_from_col = -1;
        last_red_from_row = -1;
        last_red_to_col = -1;
        last_red_to_row = -1;
        last_black_from_col = -1;
        last_black_from_row = -1;
        last_black_to_col = -1;
        last_black_to_row = -1;
    }

    /* 切换回上一方的回合 */
    if (g_undo_current_turn == 1) {
        g_undo_current_turn = 2;
    } else {
        g_undo_current_turn = 1;
    }

    return 1;
}

/**
 * @brief 执行恢复操作
 * @return 1=成功，0=失败（无恢复历史）
 */
int undo_redo_redo()
{
    int move_idx;
    int from_row, from_col, to_row, to_col;
    int moved_piece;
    struct Piece captured;
    int captured_id;

    if (g_redo_history_count <= 0) {
        return 0;  /* 无恢复历史 */
    }

    /* 获取最后一步恢复历史 */
    move_idx = g_redo_history_count - 1;
    from_row = g_redo_history[move_idx].move.from_row;
    from_col = g_redo_history[move_idx].move.from_col;
    to_row = g_redo_history[move_idx].move.to_row;
    to_col = g_redo_history[move_idx].move.to_col;

    /* 获取被吃掉的棋子（逐字段复制） */
    captured.type = g_redo_history[move_idx].captured_piece.type;
    captured.color = g_redo_history[move_idx].captured_piece.color;

    /* 获取移动的棋子（当前在原位置） */
    moved_piece = chess_map[from_row][from_col];

    /* 记录被吃掉的棋子（如果有） */
    captured_id = chess_map[to_row][to_col];

    /* 重新执行走棋 */
    chess_map[to_row][to_col] = moved_piece;
    chess_map[from_row][from_col] = 0;

    /* 将这步棋移回悔棋历史（逐字段复制） */
    if (g_undo_history_count < MAX_UNDO_REDO_HISTORY) {
        int undo_idx = g_undo_history_count;
        g_undo_history[undo_idx].move.from_row = g_redo_history[move_idx].move.from_row;
        g_undo_history[undo_idx].move.from_col = g_redo_history[move_idx].move.from_col;
        g_undo_history[undo_idx].move.to_row = g_redo_history[move_idx].move.to_row;
        g_undo_history[undo_idx].move.to_col = g_redo_history[move_idx].move.to_col;
        g_undo_history[undo_idx].captured_piece.type = g_redo_history[move_idx].captured_piece.type;
        g_undo_history[undo_idx].captured_piece.color = g_redo_history[move_idx].captured_piece.color;

        /* 恢复位置标记到悔棋历史 */
        g_undo_last_red_from_col[undo_idx] = g_redo_last_red_from_col[move_idx];
        g_undo_last_red_from_row[undo_idx] = g_redo_last_red_from_row[move_idx];
        g_undo_last_red_to_col[undo_idx] = g_redo_last_red_to_col[move_idx];
        g_undo_last_red_to_row[undo_idx] = g_redo_last_red_to_row[move_idx];
        g_undo_last_black_from_col[undo_idx] = g_redo_last_black_from_col[move_idx];
        g_undo_last_black_from_row[undo_idx] = g_redo_last_black_from_row[move_idx];
        g_undo_last_black_to_col[undo_idx] = g_redo_last_black_to_col[move_idx];
        g_undo_last_black_to_row[undo_idx] = g_redo_last_black_to_row[move_idx];

        g_undo_history_count++;
    }

    /* 减少恢复历史计数 */
    g_redo_history_count--;

    /* 恢复位置标记 */
    if (g_redo_history_count > 0) {
        int prev_redo_idx = g_redo_history_count - 1;
        last_red_from_col = g_redo_last_red_from_col[prev_redo_idx];
        last_red_from_row = g_redo_last_red_from_row[prev_redo_idx];
        last_red_to_col = g_redo_last_red_to_col[prev_redo_idx];
        last_red_to_row = g_redo_last_red_to_row[prev_redo_idx];
        last_black_from_col = g_redo_last_black_from_col[prev_redo_idx];
        last_black_from_row = g_redo_last_black_from_row[prev_redo_idx];
        last_black_to_col = g_redo_last_black_to_col[prev_redo_idx];
        last_black_to_row = g_redo_last_black_to_row[prev_redo_idx];
    } else {
        /* 没有恢复历史了，恢复到悔棋前的状态 */
        if (g_undo_history_count > 0) {
            int last_undo_idx = g_undo_history_count - 1;
            last_red_from_col = g_undo_last_red_from_col[last_undo_idx];
            last_red_from_row = g_undo_last_red_from_row[last_undo_idx];
            last_red_to_col = g_undo_last_red_to_col[last_undo_idx];
            last_red_to_row = g_undo_last_red_to_row[last_undo_idx];
            last_black_from_col = g_undo_last_black_from_col[last_undo_idx];
            last_black_from_row = g_undo_last_black_from_row[last_undo_idx];
            last_black_to_col = g_undo_last_black_to_col[last_undo_idx];
            last_black_to_row = g_undo_last_black_to_row[last_undo_idx];
        } else {
            /* 完全没有历史，重置 */
            last_red_from_col = -1;
            last_red_from_row = -1;
            last_red_to_col = -1;
            last_red_to_row = -1;
            last_black_from_col = -1;
            last_black_from_row = -1;
            last_black_to_col = -1;
            last_black_to_row = -1;
        }
    }

    /* 切换到下一方的回合 */
    if (g_undo_current_turn == 1) {
        g_undo_current_turn = 2;
    } else {
        g_undo_current_turn = 1;
    }

    return 1;
}

/**
 * @brief 清空恢复历史（当走新棋时调用）
 */
void undo_redo_clear_redo()
{
    g_redo_history_count = 0;
}
