/**
 * @file logger.c
 * @brief 日志输出工具模块
 * @author QWen
 * @date 2026
 *
 * 功能：提供统一的日志输出功能，输出到控制台
 */

#include "xiangqi.h"

/* 时间结构（用于日志时间戳） */
struct TIME
{
    int year;
    char month;
    char day;
    char hour;
    char minute;
    char second;
    char week;
};

/* 外部函数声明 */

/* ==================== 日志输出函数 ==================== */
/**
 * @brief 输出调试日志到控制台
 * @param message 日志消息
 */
void log_debug(const char *message)
{
    char time[8];
    GetTime(time);
    printf("[DEBUG %02d:%02d:%02d] %s\n", time[4], time[5], time[6], message);
}

/**
 * @brief 输出格式化调试日志到控制台
 * @param format 格式字符串
 * @param ... 可变参数
 */
void log_debug_fmt(const char *format, ...)
{
    char time[8];
    char buffer[256];

    GetTime(time);

    /* 使用 vsprintf 格式化输出 */
    /* 注意：LavaX 不支持 stdarg.h，使用简单替代方案 */
    /* 这里直接使用 sprintf 构造消息 */
    sprintf(buffer, format, time[4], time[5], time[6]);
    printf("[DEBUG %02d:%02d:%02d] %s\n", time[4], time[5], time[6], buffer);
}

/**
 * @brief 输出走棋日志到控制台
 * @param from_row 起始行
 * @param from_col 起始列
 * @param to_row 目标行
 * @param to_col 目标列
 * @param player 玩家 (1=红方，2=黑方)
 * @param piece 棋子编号
 */
void log_move_console(int from_row, int from_col, int to_row, int to_col, int player, int piece)
{
    char time[8];
    char name[20];
    char human_str[40];
    int is_red;
    int piece_type;
    char direction[4];
    int step_or_road;
    int road;
    int row_diff;

    GetTime();

    /* 确定棋子中文名称和颜色 */
    if (piece == RED_CHE)
    {
        sprintf(name, "Red Chariot");
        is_red = 1;
        piece_type = 1;
    }
    else if (piece == RED_MA)
    {
        sprintf(name, "Red Horse");
        is_red = 1;
        piece_type = 2;
    }
    else if (piece == RED_XIANG)
    {
        sprintf(name, "Red Elephant");
        is_red = 1;
        piece_type = 3;
    }
    else if (piece == RED_SHI)
    {
        sprintf(name, "Red Advisor");
        is_red = 1;
        piece_type = 4;
    }
    else if (piece == RED_SHUAI)
    {
        sprintf(name, "Red General");
        is_red = 1;
        piece_type = 5;
    }
    else if (piece == RED_PAO)
    {
        sprintf(name, "Red Cannon");
        is_red = 1;
        piece_type = 6;
    }
    else if (piece == RED_BING)
    {
        sprintf(name, "Red Pawn");
        is_red = 1;
        piece_type = 7;
    }
    else if (piece == BLACK_CHE)
    {
        sprintf(name, "Black Chariot");
        is_red = 0;
        piece_type = 1;
    }
    else if (piece == BLACK_MA)
    {
        sprintf(name, "Black Horse");
        is_red = 0;
        piece_type = 2;
    }
    else if (piece == BLACK_XIANG)
    {
        sprintf(name, "Black Elephant");
        is_red = 0;
        piece_type = 3;
    }
    else if (piece == BLACK_SHI)
    {
        sprintf(name, "Black Advisor");
        is_red = 0;
        piece_type = 4;
    }
    else if (piece == BLACK_JIANG)
    {
        sprintf(name, "Black General");
        is_red = 0;
        piece_type = 5;
    }
    else if (piece == BLACK_PAO)
    {
        sprintf(name, "Black Cannon");
        is_red = 0;
        piece_type = 6;
    }
    else if (piece == BLACK_ZU)
    {
        sprintf(name, "Black Pawn");
        is_red = 0;
        piece_type = 7;
    }
    else
    {
        sprintf(name, "Unknown(%d)", piece);
        /* 输出原始坐标格式 */
        printf("[%02d:%02d:%02d] %s moved from(%d,%d) to(%d,%d)\n",
               g_time_hour, g_time_minute, g_time_second, name, from_row, from_col, to_row, to_col);
        return;
    }

    /* 计算路数 (1-9, 基于列) */
    if (is_red)
    {
        road = 9 - from_col; /* 红方：col=8 -> road=1, col=0 -> road=9 */
    }
    else
    {
        road = from_col + 1; /* 黑方：col=0 -> road=1, col=8 -> road=9 */
    }

    /* 确定移动方向 */
    if (from_col != to_col && from_row == to_row)
    {
        /* 横向移动：平 */
        sprintf(direction, "Ping");
        if (is_red)
        {
            step_or_road = 9 - to_col; /* 红方 Ping 用目标 road */
        }
        else
        {
            step_or_road = to_col + 1; /* 黑方 Ping 用目标 road */
        }
    }
    else
    {
        /* 纵向移动：进或退 */
        row_diff = to_row - from_row;
        if (is_red)
        {
            /* 红方：row 减小 = 前进 (向上)，row 增大 = 后退 (向下) */
            if (row_diff < 0)
                sprintf(direction, "Jin");
            else
                sprintf(direction, "Tui");
        }
        else
        {
            /* 黑方：row 增大 = 前进 (向下)，row 减小 = 后退 (向上) */
            if (row_diff > 0)
                sprintf(direction, "Jin");
            else
                sprintf(direction, "Tui");
        }

        /* 计算步数或目标路数 */
        if (piece_type == 2 || piece_type == 3)
        {
            /* 马和象：用目标路数 */
            if (is_red)
            {
                step_or_road = 9 - to_col;
            }
            else
            {
                step_or_road = to_col + 1;
            }
        }
        else
        {
            /* 其他棋子：用垂直步数 */
            if (row_diff > 0)
            {
                step_or_road = row_diff;
            }
            else
            {
                step_or_road = -row_diff;
            }
        }
    }

    /* 构造传统记谱字符串 */
    sprintf(human_str, "%s%d%s%d", name, road, direction, step_or_road);

    /* 输出记谱和原始坐标 */
    printf("[%02d:%02d:%02d] %s (%d,%d)->(%d,%d)\n",
           g_time_hour, g_time_minute, g_time_second, human_str, from_row, from_col, to_row, to_col);
}

/**
 * @brief 输出 can_move 调试日志到控制台
 * @param row1 起始行
 * @param col1 起始列
 * @param row2 目标行
 * @param col2 目标列
 * @param piece_name 棋子名称
 * @param result 结果 (0=无效，1=有效)
 */
void log_can_move(int row1, int col1, int row2, int col2, const char *piece_name, int result)
{
    char time[8];
    GetTime(time);

    if (result)
    {
        printf("[DEBUG %02d:%02d:%02d] [can_move] (%d,%d)->(%d,%d) 棋子=%s 结果：VALID\n",
               time[4], time[5], time[6], row1, col1, row2, col2, piece_name);
    }
    else
    {
        printf("[DEBUG %02d:%02d:%02d] [can_move] (%d,%d)->(%d,%d) 棋子=%s 结果：INVALID\n",
               time[4], time[5], time[6], row1, col1, row2, col2, piece_name);
    }
}

/**
 * @brief 输出玩家走棋日志到控制台
 * @param time 时间结构
 * @param from_row 起始行
 * @param from_col 起始列
 * @param to_row 目标行
 * @param to_col 目标列
 * @param piece 棋子编号
 */
void log_player_move(char time[8], int from_row, int from_col, int to_row, int to_col, int piece)
{
    char move_notation[32];
    Move move;
    move.from_row = from_row;
    move.from_col = from_col;
    move.to_row = to_row;
    move.to_col = to_col;
    /* 注意：此处无法获取棋盘状态，只能输出坐标格式 */
    printf("[%02d:%02d:%02d] [玩家] 走棋：(%d,%d)->(%d,%d), 棋子=%d\n",
           g_time_hour, g_time_minute, g_time_second, from_row, from_col, to_row, to_col, piece);
}

/**
 * @brief 输出 AI 走棋日志到控制台
 * @param time 时间结构
 * @param from_row 起始行
 * @param from_col 起始列
 * @param to_row 目标行
 * @param to_col 目标列
 * @param piece_name 棋子名称
 */
void log_ai_move(char time[8], int from_row, int from_col, int to_row, int to_col, const char *piece_name)
{
    /* 注意：此处无法获取棋盘状态，只能输出坐标格式 */
    printf("[%02d:%02d:%02d] [AI] 走棋：(%d,%d)->(%d,%d), 棋子=%s\n",
           g_time_hour, g_time_minute, g_time_second, from_row, from_col, to_row, to_col, piece_name);
}
