// ===================== Project Background =====================
// Project Name: Chinese Chess AI Battle System (Standalone Version);
// 1. Objective: For chess beginners to practice, supporting human-machine battles
// 2. Tech Stack: LavaX language, Alpha-Beta pruning algorithm, no third-party framework dependencies
// 3. LAVAX language specifications are in LavaX Programming Manual.md in the same directory
// 4. Compiler restrictions: Local variable definitions must be at the beginning of functions
// 5. Chess AI design is in Chinese Chess.md in the same directory
// 6. Hardware target: esp32s3, screen resolution 320x240, 4-bit grayscale
// 7. Core constraints: AI search depth 8 layers, response time 1 second, compatible with Windows
// 8. Current stage: Chess game rule verification completed, AI algorithm efficiency to be optimized
// 9. Source code encoding format: Chinese GBK
// 10. Input method: dpad buttons, button values - Left:0x17, Right:0x16, Up:0x14, Down:0x15,
//     A/B/X/Y: 0x0D/0x1B/0x1F/0x01 respectively
// ==================================================
// Time structure (for logging);
#ifdef __LAVA__
#width 320
#height 240
#color 4
#bigram
#encoding utf8
#endif

#include "xiangqi.h"

// 追踪 game_over 变化的宏
#define SET_GAME_OVER(val) printf("[TRACE] game_over set to %d at %s:%d\n", val, __FILE__, __LINE__); game_over = (val);

// 如果需要完全替换 game_over = 1 的写法，可以这样用：
// game_over = 1;  // 改为：
// SET_GAME_OVER(1);

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

// 棋子类型常量（必须在使用前定义）
#define CC_PIECE_EMPTY 0
#define CC_PIECE_KING 1
#define CC_PIECE_ROOK 2
#define CC_PIECE_CANNON 3
#define CC_PIECE_KNIGHT 4
#define CC_PIECE_ELEPHANT 5
#define CC_PIECE_ADVISOR 6
#define CC_PIECE_PAWN 7

// 搜索深度限制
#define CC_MAX_DEPTH 12

#define WIDTH LAVA_CCH_SCREEN_WIDTH
#define HEIGHT LAVA_CCH_SCREEN_HEIGHT

#define GE_SZ 23
#define QIPAN_X 18
#define QIPAN_Y 16
#define PIECE_SOURCE_SIZE 13
#define PIECE_SIZE 23
#define PIECE_SOURCE_STRIDE 7
#define PIECE_BITMAP_STRIDE ((PIECE_SIZE + 1) / 2)
#define PIECE_BITMAP_BYTES (PIECE_BITMAP_STRIDE * PIECE_SIZE)
#define STATUS_X 224
#define STATUS_RIGHT 312

#define RED_CHE 1
// Chariot (Rook);
#define RED_MA 2
// Horse (Knight);
#define RED_XIANG 3
// Elephant (Bishop);
#define RED_SHI 4
// Advisor (Guard);
#define RED_SHUAI 5
// General (King);
#define RED_PAO 6
// Cannon
#define RED_BING 7
// Pawn
#define BLACK_CHE 8
// Chariot (Rook);
#define BLACK_MA 9
// Horse (Knight);
#define BLACK_XIANG 10
// Elephant (Bishop);
#define BLACK_SHI 11
// Advisor (Guard);
#define BLACK_JIANG 12
// General (King);
#define BLACK_PAO 13
// Cannon
#define BLACK_ZU 14
// Pawn

// ==================== 实用宏定义 ====================
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b));
#define ABS(x) ((x) >= 0 ? (x) : -(x));
#define SIGN(x) ((x) > 0 ? 1 : ((x) < 0 ? -1 : 0));
#define IS_RED(piece) ((piece) >= 1 && (piece) <= 7);
#define IS_BLACK(piece) ((piece) >= 8 && (piece) <= 14);

// ==================== 棋子价值查找表 ====================
// 索引: 0-14, 对应空位和各棋子
// 来自 cchess-fc 的 PIECE_VALUES_FC: 车=96, 炮=48, 马=48, 象=32, 士=32, 兵=16, 将=255
// 棋子编号: 1=红车, 2=红马, 3=红象, 4=红士, 5=红帅, 6=红炮, 7=红兵
//          8=黑车, 9=黑马, 10=黑象, 11=黑士, 12=黑将, 13=黑炮, 14=黑卒
int cc_piece_value_table[15] = {
    0,   // 0: 空
    96,  // 1: RED_CHE (车);
    48,  // 2: RED_MA (马);
    32,  // 3: RED_XIANG (象);
    32,  // 4: RED_SHI (士);
    255, // 5: RED_SHUAI (帅);
    48,  // 6: RED_PAO (炮);
    16,  // 7: RED_BING (兵);
    96,  // 8: BLACK_CHE (车);
    48,  // 9: BLACK_MA (马);
    32,  // 10: BLACK_XIANG (象);
    32,  // 11: BLACK_SHI (士);
    255, // 12: BLACK_JIANG (将);
    48,  // 13: BLACK_PAO (炮);
    16   // 14: BLACK_ZU (卒);
};

// ==================== 位置加权表 ====================
// 来自 cchess-fc check.c 的完整位置加权表
// 全局位置加分表 [row][col]
int cc_position_bonus[10][9];

/* can_move 调试日志开关（0=关闭, 1=开启） */
int g_can_move_debug_enabled = 0;

// 车的位置价值表（红方视角，黑方需要对称翻转）
// 原版 ROOK_POSITION 数据，扁平初始化
int cc_rook_position[10][9] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    7, 6, 8, 10, 12, 10, 8, 6, 7,
    6, 8, 10, 12, 14, 12, 10, 8, 6,
    6, 8, 10, 14, 16, 14, 10, 8, 6,
    6, 8, 10, 12, 14, 12, 10, 8, 6,
    6, 8, 10, 12, 14, 12, 10, 8, 6,
    6, 8, 10, 12, 14, 12, 10, 8, 6,
    4, 6, 8, 10, 12, 10, 8, 6, 4,
    2, 4, 6, 8, 10, 8, 6, 4, 2,
    0, 0, 0, 0, 0, 0, 0, 0, 0};

// 马的位置价值表（红方视角）
// 原版 KNIGHT_POSITION 数据
int cc_knight_position[10][9] = {
    0, -5, 0, 0, 0, 0, 0, -5, 0,
    0, 0, 4, 0, 0, 0, 4, 0, 0,
    2, 10, 18, 8, 8, 8, 18, 10, 2,
    2, 8, 20, 12, 12, 12, 20, 8, 2,
    4, 10, 22, 16, 16, 16, 22, 10, 4,
    4, 10, 22, 16, 16, 16, 22, 10, 4,
    2, 8, 20, 12, 12, 12, 20, 8, 2,
    2, 10, 18, 8, 8, 8, 18, 10, 2,
    0, 0, 4, 0, 0, 0, 4, 0, 0,
    0, -5, 0, 0, 0, 0, 0, -5, 0};

// 炮的位置价值表（红方视角）
// 原版 CANNON_POSITION 数据
int cc_cannon_position[10][9] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    6, 8, 12, 16, 16, 16, 12, 8, 6,
    6, 6, 10, 14, 14, 14, 10, 6, 6,
    4, 6, 10, 14, 14, 14, 10, 6, 4,
    4, 6, 10, 14, 14, 14, 10, 6, 4,
    5, 7, 11, 15, 15, 15, 11, 7, 5,
    2, 4, 8, 12, 12, 12, 8, 4, 2,
    0, 0, 6, 8, 8, 8, 6, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0};

// 兵/卒的位置价值表（红方视角）
// 原版 PAWN_POSITION 数据
int cc_pawn_position[10][9] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    5, 5, 8, 10, 10, 10, 8, 5, 5,
    5, 5, 8, 10, 10, 10, 8, 5, 5,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0};

// 将/帅的位置价值表（红方视角）
// 原版 KING_POSITION 数据
int cc_king_position[10][9] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, -18, -16, -16, 0, 0, 0,
    0, 0, 0, -16, -12, -16, 0, 0, 0,
    0, 0, 0, 12, 16, 12, 0, 0, 0};

// 象/相的位置价值表（红方视角）
// 原版 ELEPHANT_POSITION 数据
int cc_elephant_position[10][9] = {
    0, 0, 8, 0, 0, 0, 8, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    6, 0, 0, 0, 12, 0, 0, 0, 6,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, -5, 0, 0, 0, -5, 0, 0,
    0, 0, -5, 0, 0, 0, -5, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    6, 0, 0, 0, 12, 0, 0, 0, 6,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 8, 0, 0, 0, 8, 0, 0};

// 士/仕的位置价值表（红方视角）
// 原版 ADVISOR_POSITION 数据
int cc_advisor_position[10][9] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 50, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 50, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0};
void cc_init_all_position_tables()
{
    // 位置加权表已在定义时初始化，此函数保留为空以兼容旧代码
}

// 旧版兼容：初始化全局位置加权表
void cc_init_position_bonus()
{
    // 调用新版初始化（实际为空）
    cc_init_all_position_tables();
}

// 获取棋子位置价值（考虑对称性）
// is_red: 1=红方, 0=黑方
// row, col: 当前位置 (0-based);
int cc_get_piece_position_value(int piece_type, int row, int col, int is_red)
{
    int r;

    // 大多数位置表是红方视角，黑方需要翻转
    // 但 PAWN_POSITION 是黑卒视角，红兵需要翻转
    if (piece_type == CC_PIECE_PAWN)
    {
        // 兵的位置表是黑卒视角（黑卒过河在row 5-6）
        // 红兵需要翻转：红兵在row 4时，翻转后查row 5
        r = (is_red != 0) ? (9 - row) : row;
    }
    else
    {
        // 其他位置表是红方视角
        // 黑方需要翻转
        r = (is_red != 0) ? row : (9 - row);
    }

    if (piece_type == CC_PIECE_KING)
    {
        return cc_king_position[r][col];
    }
    else if (piece_type == CC_PIECE_ROOK)
    {
        return cc_rook_position[r][col];
    }
    else if (piece_type == CC_PIECE_KNIGHT)
    {
        return cc_knight_position[r][col];
    }
    else if (piece_type == CC_PIECE_CANNON)
    {
        return cc_cannon_position[r][col];
    }
    else if (piece_type == CC_PIECE_PAWN)
    {
        return cc_pawn_position[r][col];
    }
    else if (piece_type == CC_PIECE_ELEPHANT)
    {
        return cc_elephant_position[r][col];
    }
    else if (piece_type == CC_PIECE_ADVISOR)
    {
        return cc_advisor_position[r][col];
    }
    return 0;
}

char qizi_bmp[][98] = {
    // Width=13, Height=196
    0x0, 0xa, 0xaa, 0xaa, 0xaa, 0x0, 0x0, 0x0, 0xaa, 0xf, 0x0, 0xa, 0xa0, 0x0, 0xa, 0xaf, 0xff, 0xff, 0xff, 0xaa, 0x0, 0xa, 0x0, 0xf0, 0x0, 0x0, 0xa, 0x0, 0xa0, 0x0, 0xf0, 0xf0,
    0x0, 0x0, 0xa0, 0xa0, 0xf, 0x0, 0xf0, 0x0, 0x0, 0xa0, 0xa0, 0xff, 0xff, 0xff, 0xff, 0xf0, 0xa0, 0xa0, 0x0, 0x0, 0xf0, 0x0, 0x0, 0xa0, 0xa0, 0x0, 0x0, 0xf0, 0x0, 0x0, 0xa0, 0xa,
    0xff, 0xff, 0xff, 0xff, 0xfa, 0x0, 0xa, 0xa0, 0x0, 0xf0, 0x0, 0xaa, 0x0, 0x0, 0xaa, 0x0, 0xf0, 0xa, 0xa0, 0x0, 0x0, 0xa, 0xaa, 0xaa, 0xaa, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0xa, 0xaa, 0xaa, 0xaa, 0x0, 0x0, 0x0, 0xaa, 0xff, 0xff, 0xfa, 0xa0, 0x0, 0xa, 0xa0, 0x0, 0x0, 0xf, 0xaa, 0x0, 0xa, 0x0, 0xf0, 0x0, 0xf, 0xa, 0x0, 0xa0, 0x0,
    0xf0, 0x0, 0xf0, 0x0, 0xa0, 0xa0, 0xf, 0x0, 0x0, 0xf0, 0x0, 0xa0, 0xa0, 0xf, 0xff, 0xff, 0xff, 0xf0, 0xa0, 0xa0, 0x0, 0x0, 0x0, 0x0, 0xf0, 0xa0, 0xaf, 0xff, 0xff, 0xff, 0xf0, 0xf0,
    0xa0, 0xa, 0x0, 0x0, 0x0, 0x0, 0xfa, 0x0, 0xa, 0xa0, 0x0, 0x0, 0x0, 0xaa, 0x0, 0x0, 0xaa, 0x0, 0xf, 0xfa, 0xa0, 0x0, 0x0, 0xa, 0xaa, 0xaa, 0xaa, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0xa, 0xaa, 0xaa, 0xaa, 0x0, 0x0, 0x0, 0xaa, 0x0, 0x0, 0xa, 0xa0, 0x0, 0xa, 0xaf, 0x0, 0xf, 0xff, 0xaa, 0x0, 0xa, 0xf, 0x0, 0xf, 0x0, 0xa, 0x0,
    0xaf, 0xff, 0xff, 0xf, 0x0, 0xf, 0xa0, 0xa0, 0xf, 0x0, 0xf, 0xff, 0xff, 0xa0, 0xa0, 0xff, 0xf0, 0xf, 0x0, 0xf, 0xa0, 0xa0, 0xff, 0xf, 0xf, 0xff, 0xff, 0xa0, 0xaf, 0xf, 0x0, 0xf,
    0x0, 0xf, 0xa0, 0xa, 0xf, 0x0, 0xf, 0x0, 0xa, 0x0, 0xa, 0xaf, 0x0, 0xf, 0xff, 0xaa, 0x0, 0x0, 0xaa, 0x0, 0xf, 0xa, 0xaf, 0x0, 0x0, 0xa, 0xaa, 0xaa, 0xaa, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xa, 0xaa, 0xaa, 0xaa, 0x0, 0x0, 0x0, 0xaa, 0xf0, 0x0, 0xfa, 0xa0, 0x0, 0xa, 0xa0, 0xf0, 0x0, 0xf0, 0xaa, 0x0, 0xa, 0xf, 0x0, 0x0, 0xf0,
    0xa, 0x0, 0xa0, 0xf, 0x0, 0x0, 0xf0, 0x0, 0xa0, 0xa0, 0xff, 0xf, 0xff, 0xff, 0xff, 0xa0, 0xaf, 0xf, 0x0, 0x0, 0xf0, 0x0, 0xa0, 0xa0, 0xf, 0x0, 0x0, 0xf0, 0x0, 0xa0, 0xa0, 0xf,
    0x0, 0x0, 0xf0, 0x0, 0xa0, 0xa, 0xf, 0x0, 0x0, 0xf0, 0xa, 0x0, 0xa, 0xaf, 0x0, 0x0, 0xf0, 0xaa, 0x0, 0x0, 0xaa, 0xf, 0xff, 0xfa, 0xaf, 0x0, 0x0, 0xa, 0xaa, 0xaa, 0xaa, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xa, 0xaa, 0xaa, 0xaa, 0x0, 0x0, 0x0, 0xaa, 0xf0, 0x0, 0xfa, 0xa0, 0x0, 0xa, 0xa0, 0xf0, 0x0, 0xf0, 0xaa, 0x0, 0xa, 0xf0, 0xf0,
    0xff, 0xff, 0xfa, 0x0, 0xa0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf, 0xa0, 0xa0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf, 0xa0, 0xa0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf, 0xa0, 0xa0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf, 0xa0,
    0xa0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf, 0xa0, 0xa, 0xf, 0x0, 0xf0, 0xf0, 0xfa, 0x0, 0xa, 0xa0, 0x0, 0x0, 0xf0, 0xaa, 0x0, 0xf, 0xaa, 0x0, 0x0, 0xfa, 0xa0, 0x0, 0x0, 0xa, 0xaa, 0xaa,
    0xaa, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xa, 0xaa, 0xaa, 0xaa, 0x0, 0x0, 0x0, 0xaa, 0x0, 0xf0, 0xa, 0xa0, 0x0, 0xa, 0xaf, 0x0, 0xff, 0xff, 0xaa, 0x0, 0xa,
    0xf, 0xf0, 0xf0, 0x0, 0xfa, 0x0, 0xaf, 0xf, 0xf, 0xff, 0xf0, 0xf0, 0xa0, 0xaf, 0xf, 0x0, 0xf0, 0xf0, 0xf0, 0xa0, 0xaf, 0xf, 0x0, 0xf0, 0xf0, 0xf0, 0xa0, 0xa0, 0xf, 0x0, 0xff, 0xf0,
    0xf0, 0xa0, 0xa0, 0xf, 0xf0, 0xf0, 0xf, 0xf0, 0xa0, 0xa, 0xf0, 0xf, 0xf0, 0x0, 0xa, 0x0, 0xa, 0xa0, 0x0, 0xf0, 0x0, 0xaa, 0x0, 0xf, 0xaa, 0x0, 0xff, 0xfa, 0xaf, 0x0, 0x0, 0xa,
    0xaa, 0xaa, 0xaa, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xa, 0xaa, 0xaa, 0xaa, 0x0, 0x0, 0x0, 0xaa, 0x0, 0xf, 0xfa, 0xa0, 0x0, 0xa, 0xaf, 0xff, 0xf0, 0x0, 0xaa,
    0x0, 0xa, 0xf, 0x0, 0x0, 0x0, 0xa, 0x0, 0xa0, 0xf, 0xff, 0xff, 0xff, 0xf0, 0xa0, 0xa0, 0xf, 0x0, 0x0, 0xf0, 0x0, 0xa0, 0xa0, 0xf, 0x0, 0x0, 0xf0, 0x0, 0xa0, 0xa0, 0xf, 0x0,
    0x0, 0xf0, 0x0, 0xa0, 0xaf, 0xff, 0xff, 0xff, 0xff, 0xff, 0xa0, 0xa, 0x0, 0xf0, 0x0, 0xf0, 0xa, 0x0, 0xa, 0xaf, 0x0, 0x0, 0xf, 0xaa, 0x0, 0xf, 0xaa, 0x0, 0x0, 0xa, 0xa0, 0x0,
    0x0, 0xa, 0xaa, 0xaa, 0xaa, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xf, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0xff, 0xf0, 0xff, 0xff, 0xf0, 0x0, 0xf, 0x0, 0x0, 0x0,
    0x0, 0xf, 0x0, 0xf, 0xff, 0xf, 0xff, 0xff, 0xff, 0x0, 0xff, 0xff, 0xf, 0xf, 0xff, 0xff, 0xf0, 0xff, 0xf0, 0xff, 0xf, 0xff, 0xff, 0xf0, 0xff, 0x0, 0x0, 0x0, 0x0, 0xf, 0xf0, 0xff,
    0xff, 0xff, 0xf, 0xff, 0xff, 0xf0, 0xff, 0xff, 0xff, 0xf, 0xff, 0xff, 0xf0, 0xf, 0x0, 0x0, 0x0, 0x0, 0xf, 0x0, 0xf, 0xff, 0xff, 0xf, 0xff, 0xff, 0x0, 0x0, 0xff, 0xff, 0xf, 0xff,
    0xf0, 0x0, 0x0, 0xf, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xf, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0xf0, 0x0, 0x0, 0x0, 0xf0, 0x0, 0xf, 0xff,
    0xff, 0xff, 0xf0, 0xff, 0x0, 0xf, 0xff, 0xf, 0xff, 0xf0, 0xff, 0x0, 0xff, 0xff, 0xf, 0xff, 0xf, 0xff, 0xf0, 0xff, 0xf0, 0xff, 0xff, 0xf, 0xff, 0xf0, 0xff, 0xf0, 0x0, 0x0, 0x0, 0xf,
    0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf, 0xf0, 0xf0, 0x0, 0x0, 0x0, 0xf, 0xf, 0xf0, 0xf, 0xff, 0xff, 0xff, 0xff, 0xf, 0x0, 0xf, 0xff, 0xff, 0xff, 0xff, 0xf, 0x0, 0x0, 0xff, 0xff,
    0xf0, 0x0, 0xf0, 0x0, 0x0, 0xf, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xf, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0xff, 0x0, 0x0, 0xf, 0xf0, 0x0,
    0xf, 0xf0, 0xff, 0xf0, 0xff, 0xff, 0x0, 0xf, 0x0, 0x0, 0x0, 0x0, 0xf, 0x0, 0xff, 0xf0, 0xff, 0xf, 0xff, 0xf, 0xf0, 0xff, 0xf0, 0x0, 0x0, 0x0, 0xf, 0xf0, 0xff, 0xff, 0xf, 0xf,
    0xff, 0xf, 0xf0, 0xff, 0x0, 0xf0, 0xf0, 0xf0, 0xff, 0xf0, 0xff, 0xff, 0xf, 0x0, 0xf, 0xff, 0xf0, 0xf, 0x0, 0xf0, 0xf0, 0xf0, 0xff, 0x0, 0xf, 0xff, 0xf, 0xf0, 0xff, 0xf, 0x0, 0x0,
    0xf0, 0xf0, 0x0, 0xff, 0xf0, 0x0, 0x0, 0xf, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xf, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0xff, 0xff, 0xf, 0xff,
    0xf0, 0x0, 0xf, 0xff, 0xff, 0xf, 0xff, 0xff, 0x0, 0xf, 0xff, 0xff, 0xf, 0xff, 0xff, 0x0, 0xff, 0xff, 0xff, 0xf, 0xff, 0xff, 0xf0, 0xf0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xf0, 0xff, 0xff,
    0xff, 0xf, 0xff, 0xff, 0xf0, 0xff, 0xff, 0xff, 0xf, 0xff, 0xff, 0xf0, 0xff, 0xff, 0xff, 0xf, 0xff, 0xff, 0xf0, 0xf, 0xff, 0xff, 0xf, 0xff, 0xff, 0x0, 0xf, 0xff, 0xff, 0xf, 0xff, 0xff,
    0x0, 0x0, 0xf0, 0x0, 0x0, 0x0, 0xf0, 0x0, 0x0, 0xf, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xf, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0xf0, 0xff,
    0xf0, 0xff, 0xf0, 0x0, 0xf, 0xf0, 0xff, 0xf0, 0x0, 0xf, 0x0, 0xf, 0xf0, 0xff, 0xf, 0xff, 0xf, 0x0, 0xff, 0x0, 0xf0, 0xf0, 0xf0, 0xff, 0xf0, 0xff, 0xf0, 0xff, 0xff, 0xf, 0xff, 0xf0,
    0xff, 0xf0, 0xff, 0xf0, 0xff, 0xf, 0xf0, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0xf0, 0xf0, 0xf0, 0xff, 0xf, 0xff, 0xf, 0xf0, 0xf, 0xf0, 0xff, 0xf0, 0xff, 0xf, 0x0, 0xf, 0xf0, 0xff, 0xf0,
    0xff, 0xf, 0x0, 0x0, 0xf0, 0xff, 0xff, 0x0, 0xf0, 0x0, 0x0, 0xf, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xf, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0,
    0xf0, 0xff, 0xf, 0xff, 0xf0, 0x0, 0xf, 0xf0, 0xff, 0x0, 0x0, 0xf, 0x0, 0xf, 0xf0, 0xf, 0xf, 0xff, 0xf, 0x0, 0xf0, 0xf0, 0xf0, 0x0, 0xf, 0xf, 0xf0, 0xf0, 0xf0, 0xff, 0xf, 0xf,
    0xf, 0xf0, 0xf0, 0xf0, 0xff, 0xf, 0xf, 0xf, 0xf0, 0xff, 0xf0, 0xff, 0x0, 0xf, 0xf, 0xf0, 0xff, 0xf0, 0xf, 0xf, 0xf0, 0xf, 0xf0, 0xf, 0xf, 0xf0, 0xf, 0xff, 0xff, 0x0, 0xf, 0xf,
    0xff, 0xf, 0xff, 0xff, 0x0, 0x0, 0xff, 0xff, 0x0, 0x0, 0xf0, 0x0, 0x0, 0xf, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xf, 0xff, 0xff, 0xff, 0x0,
    0x0, 0x0, 0xff, 0xff, 0xf, 0xff, 0xf0, 0x0, 0xf, 0x0, 0x0, 0x0, 0x0, 0xf, 0x0, 0xf, 0xff, 0xf, 0xff, 0xf, 0xff, 0x0, 0xff, 0xff, 0xf, 0xff, 0xf, 0xff, 0xf0, 0xff, 0xf0, 0xf0,
    0xf0, 0xf0, 0xff, 0xf0, 0xff, 0xf, 0xff, 0xf, 0xff, 0xf, 0xf0, 0xff, 0xff, 0xff, 0xf, 0xff, 0xff, 0xf0, 0xf0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xf0, 0xf, 0xff, 0xff, 0xf, 0xff, 0xff, 0x0,
    0xf, 0xff, 0xff, 0xf, 0xff, 0xff, 0x0, 0x0, 0xff, 0xff, 0xf, 0xff, 0xf0, 0x0, 0x0, 0xf, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    // Width=13, Height=196
    0x0, 0xf, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0xff, 0x0, 0x0, 0xf, 0xf0, 0x0, 0xf, 0x0, 0x0, 0xf0, 0x0, 0xf, 0x0, 0xf, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0, 0xf0, 0xf, 0x0, 0xf0,
    0xf, 0x0, 0xf0, 0xf0, 0xf, 0xff, 0xff, 0xff, 0x0, 0xf0, 0xf0, 0xf, 0x0, 0xf0, 0xf, 0x0, 0xf0, 0xf0, 0xf, 0xff, 0xff, 0xff, 0x0, 0xf0, 0xf0, 0x0, 0x0, 0xf0, 0x0, 0x0, 0xf0, 0xf,
    0xff, 0xff, 0xff, 0xff, 0xff, 0x0, 0xf, 0x0, 0x0, 0xf0, 0x0, 0xf, 0x0, 0x0, 0xff, 0x0, 0x0, 0xf, 0xf0, 0x0, 0x0, 0xf, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0xf, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0xff, 0x0, 0x0, 0xf, 0xf0, 0x0, 0xf, 0x0, 0xff, 0xff, 0xff, 0xff, 0x0, 0xf, 0x0, 0xf0, 0xf, 0x0, 0xf, 0x0, 0xf0, 0x0,
    0xff, 0xff, 0xff, 0x0, 0xf0, 0xf0, 0x0, 0xf0, 0xf, 0x0, 0x0, 0xf0, 0xf0, 0x0, 0xff, 0xff, 0xff, 0x0, 0xf0, 0xf0, 0x0, 0xf0, 0xf, 0x0, 0x0, 0xf0, 0xf0, 0x0, 0xff, 0xff, 0xff, 0xf0,
    0xf0, 0xf, 0xf, 0xf, 0xf, 0x0, 0xff, 0x0, 0xf, 0xf0, 0xf0, 0xf0, 0xff, 0xf, 0x0, 0x0, 0xff, 0x0, 0x0, 0xf, 0xf0, 0x0, 0x0, 0xf, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0xf, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0xff, 0x0, 0x0, 0xf, 0xf0, 0x0, 0xf, 0x0, 0xf0, 0x0, 0x0, 0xf, 0x0, 0xf, 0x0, 0xf0, 0xf, 0xff, 0xff, 0x0,
    0xf0, 0xff, 0xff, 0xff, 0x0, 0xf0, 0xf0, 0xf0, 0x0, 0xf0, 0xf, 0xff, 0xf0, 0xf0, 0xf0, 0xf, 0xff, 0xf, 0x0, 0xf0, 0xf0, 0xf0, 0xf, 0xf0, 0xf, 0xff, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf,
    0x0, 0xf0, 0xf0, 0xf, 0x0, 0xf0, 0xf, 0xff, 0xff, 0x0, 0xf, 0x0, 0xf0, 0xf, 0x0, 0xff, 0x0, 0x0, 0xff, 0x0, 0x0, 0xf, 0xf0, 0x0, 0x0, 0xf, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xf, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0xff, 0x0, 0x0, 0xf, 0xf0, 0x0, 0xf, 0x0, 0xf, 0x0, 0xf0, 0xf, 0x0, 0xf, 0x0, 0xf0, 0x0, 0xf0,
    0xf, 0x0, 0xf0, 0x0, 0xf0, 0x0, 0xf0, 0x0, 0xf0, 0xf0, 0xf, 0xff, 0xff, 0xff, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0x0, 0xf0, 0x0, 0xf0, 0xf0, 0x0, 0xf0, 0x0, 0xf0, 0x0, 0xf0, 0xf0, 0x0,
    0xf0, 0x0, 0xf0, 0x0, 0xf0, 0xf, 0x0, 0xf0, 0xff, 0xff, 0xff, 0x0, 0xf, 0x0, 0xf0, 0x0, 0x0, 0xf, 0x0, 0x0, 0xff, 0x0, 0x0, 0xf, 0xf0, 0x0, 0x0, 0xf, 0xff, 0xff, 0xff, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xf, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0xff, 0x0, 0x0, 0xf, 0xf0, 0x0, 0xf, 0xf, 0x0, 0x0, 0xf0, 0xf, 0x0, 0xf, 0xff, 0xf0,
    0x0, 0xf0, 0xf, 0x0, 0xf0, 0xf0, 0xf0, 0xff, 0xff, 0xf0, 0xf0, 0xf0, 0xff, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0x0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xff, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0,
    0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf, 0xff, 0xf0, 0xf0, 0xff, 0xff, 0x0, 0xf, 0xf0, 0x0, 0x0, 0xf0, 0xf, 0x0, 0x0, 0xff, 0x0, 0x0, 0xf, 0xf0, 0x0, 0x0, 0xf, 0xff, 0xff,
    0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xf, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0xff, 0x0, 0x0, 0xf, 0xf0, 0x0, 0xf, 0xf, 0x0, 0xf0, 0x0, 0xf, 0x0, 0xf,
    0xf, 0x0, 0xff, 0xff, 0xff, 0x0, 0xf0, 0xff, 0xf, 0x0, 0x0, 0xf0, 0xf0, 0xf0, 0xff, 0xf0, 0xff, 0xf0, 0xf0, 0xf0, 0xf0, 0xf, 0x0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf0, 0xf, 0x0, 0xff, 0xf,
    0xf0, 0xf0, 0xf0, 0xf, 0x0, 0xf0, 0x0, 0x0, 0xf0, 0xf, 0xf0, 0xf0, 0xf0, 0x0, 0xff, 0x0, 0xf, 0xf0, 0xf0, 0xff, 0xff, 0xff, 0x0, 0x0, 0xff, 0x0, 0x0, 0xf, 0xf0, 0x0, 0x0, 0xf,
    0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xf, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0xff, 0x0, 0x0, 0xf, 0xf0, 0x0, 0xf, 0x0, 0x0, 0x0, 0xff, 0xf,
    0x0, 0xf, 0x0, 0xff, 0xff, 0x0, 0xf, 0x0, 0xf0, 0x0, 0xf0, 0x0, 0x0, 0x0, 0xf0, 0xf0, 0x0, 0xff, 0xff, 0xff, 0x0, 0xf0, 0xf0, 0x0, 0xf0, 0x0, 0xf0, 0x0, 0xf0, 0xf0, 0x0, 0xf0,
    0x0, 0xf0, 0x0, 0xf0, 0xf0, 0xff, 0xff, 0xff, 0xff, 0xf0, 0xf0, 0xf, 0x0, 0xf0, 0x0, 0xf0, 0xf, 0x0, 0xf, 0xf, 0x0, 0x0, 0xf, 0xf, 0x0, 0x0, 0xff, 0x0, 0x0, 0xf, 0xf0, 0x0,
    0x0, 0xf, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xf, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x0, 0xf, 0xff, 0xff, 0xf,
    0xff, 0xff, 0x0, 0xf, 0x0, 0x0, 0x0, 0x0, 0xf, 0x0, 0xff, 0xf0, 0xff, 0xf, 0xf0, 0xff, 0xf0, 0xff, 0xf0, 0x0, 0x0, 0x0, 0xff, 0xf0, 0xff, 0xf0, 0xff, 0xf, 0xf0, 0xff, 0xf0, 0xff,
    0xf0, 0x0, 0x0, 0x0, 0xff, 0xf0, 0xff, 0xff, 0xff, 0xf, 0xff, 0xff, 0xf0, 0xf, 0x0, 0x0, 0x0, 0x0, 0xf, 0x0, 0xf, 0xff, 0xff, 0xf, 0xff, 0xff, 0x0, 0x0, 0xff, 0xff, 0xff, 0xff,
    0xf0, 0x0, 0x0, 0xf, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xf, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x0, 0xf, 0xff,
    0x0, 0x0, 0x0, 0xf, 0x0, 0xf, 0xff, 0xf, 0xf0, 0xff, 0xff, 0x0, 0xff, 0xff, 0x0, 0x0, 0x0, 0xff, 0xf0, 0xff, 0xff, 0xf, 0xf0, 0xff, 0xff, 0xf0, 0xff, 0xff, 0x0, 0x0, 0x0, 0xff,
    0xf0, 0xff, 0xff, 0xf, 0xf0, 0xff, 0xff, 0xf0, 0xff, 0xff, 0x0, 0x0, 0x0, 0xf, 0xf0, 0xf, 0xf0, 0xf0, 0xf0, 0xff, 0xf, 0x0, 0xf, 0xf, 0xf, 0xf, 0x0, 0xff, 0x0, 0x0, 0xff, 0xff,
    0xff, 0xff, 0xf0, 0x0, 0x0, 0xf, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xf, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x0,
    0xf, 0xff, 0x0, 0x0, 0xff, 0xff, 0x0, 0xf, 0xf0, 0xff, 0xf0, 0xff, 0xff, 0x0, 0xff, 0x0, 0x0, 0x0, 0x0, 0xff, 0xf0, 0xff, 0xf0, 0xff, 0xf, 0xf0, 0xff, 0xf0, 0xff, 0xf0, 0x0, 0x0,
    0x0, 0xff, 0xf0, 0xff, 0xff, 0x0, 0xf, 0xf0, 0xff, 0xf0, 0xff, 0x0, 0xff, 0x0, 0xf, 0xff, 0xf0, 0xf, 0xff, 0x0, 0xf0, 0xf0, 0xf, 0x0, 0xf, 0x0, 0xff, 0x0, 0xff, 0xff, 0x0, 0x0,
    0xff, 0xff, 0xff, 0xff, 0xf0, 0x0, 0x0, 0xf, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xf, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0xff, 0xff, 0xff, 0xff,
    0xf0, 0x0, 0xf, 0xff, 0xff, 0xf, 0xff, 0xff, 0x0, 0xf, 0xff, 0xff, 0xf, 0xff, 0xff, 0x0, 0xff, 0xff, 0xff, 0xf, 0xff, 0xff, 0xf0, 0xff, 0x0, 0x0, 0x0, 0x0, 0xf, 0xf0, 0xff, 0xff,
    0xff, 0xf, 0xff, 0xff, 0xf0, 0xff, 0xff, 0xff, 0xf, 0xff, 0xff, 0xf0, 0xff, 0xff, 0xff, 0xf, 0xff, 0xff, 0xf0, 0xf, 0xff, 0xff, 0xf, 0xff, 0xff, 0x0, 0xf, 0xf0, 0x0, 0x0, 0x0, 0xff,
    0x0, 0x0, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x0, 0x0, 0xf, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xf, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0xff, 0xff,
    0xff, 0xff, 0xf0, 0x0, 0xf, 0xff, 0xf0, 0xf0, 0xff, 0xff, 0x0, 0xf, 0xf0, 0xf0, 0xf0, 0x0, 0xf, 0x0, 0xff, 0xf0, 0xf0, 0xf, 0xf, 0xf, 0xf0, 0xff, 0xf0, 0x0, 0xf0, 0xf0, 0xff, 0xf0,
    0xff, 0xff, 0xf0, 0x0, 0xf, 0xff, 0xf0, 0xff, 0x0, 0x0, 0xff, 0xf0, 0xff, 0xf0, 0xff, 0xf0, 0xf0, 0x0, 0x0, 0xf, 0xf0, 0xf, 0xf0, 0xf0, 0xf0, 0xf0, 0xff, 0x0, 0xf, 0xf, 0xf0, 0xff,
    0x0, 0xff, 0x0, 0x0, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x0, 0x0, 0xf, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xf, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0,
    0xff, 0xff, 0xff, 0xff, 0xf0, 0x0, 0xf, 0xff, 0xff, 0xf0, 0xff, 0xff, 0x0, 0xf, 0x0, 0x0, 0xf0, 0x0, 0xf, 0x0, 0xff, 0xff, 0xf, 0xf, 0xff, 0xf, 0xf0, 0xff, 0xf0, 0xff, 0xf0, 0x0,
    0xf, 0xf0, 0xff, 0x0, 0x0, 0xf0, 0xff, 0xf, 0xf0, 0xff, 0xf0, 0xf0, 0xf0, 0x0, 0xf, 0xf0, 0xff, 0xf0, 0xf0, 0xf0, 0xff, 0xff, 0xf0, 0xf, 0xf0, 0x0, 0xf0, 0xff, 0xf, 0x0, 0xf, 0xf0,
    0xff, 0xff, 0x0, 0xf, 0x0, 0x0, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x0, 0x0, 0xf, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xf, 0xff, 0xff, 0xff, 0x0,
    0x0, 0x0, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x0, 0xf, 0xff, 0xff, 0xf, 0xff, 0xff, 0x0, 0xf, 0x0, 0x0, 0x0, 0x0, 0xf, 0x0, 0xff, 0xff, 0xf, 0xff, 0xf, 0xff, 0xf0, 0xff, 0xff, 0xf,
    0xff, 0xf, 0xff, 0xf0, 0xff, 0xf0, 0xf0, 0xf0, 0xf0, 0xff, 0xf0, 0xff, 0xf, 0xff, 0xf, 0xff, 0xf, 0xf0, 0xff, 0x0, 0x0, 0x0, 0x0, 0xf, 0xf0, 0xf, 0xff, 0xff, 0xf, 0xff, 0xff, 0x0,
    0xf, 0xff, 0xff, 0xf, 0xff, 0xff, 0x0, 0x0, 0xff, 0xff, 0xff, 0xff, 0xf0, 0x0, 0x0, 0xf, 0xff, 0xff, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

#ifdef LAVA_NATIVE_COMPILED
char qizi_bmp_scaled[14][PIECE_BITMAP_BYTES];
int qizi_bmp_scaled_ready = 0;

void init_scaled_piece_bitmaps()
{
    int piece;
    int x;
    int y;

    if (qizi_bmp_scaled_ready != 0)
    {
        return;
    }

    memset(qizi_bmp_scaled, 0, sizeof(qizi_bmp_scaled));
    for (piece = 0; piece < 14; piece++)
    {
        for (y = 0; y < PIECE_SIZE; y++)
        {
            int src_y = y * PIECE_SOURCE_SIZE / PIECE_SIZE;
            for (x = 0; x < PIECE_SIZE; x++)
            {
                int src_x = x * PIECE_SOURCE_SIZE / PIECE_SIZE;
                int src_byte = qizi_bmp[piece][src_y * PIECE_SOURCE_STRIDE + src_x / 2] & 0xFF;
                int pixel = (src_x & 1) != 0 ? src_byte & 0x0F : src_byte >> 4;
                int dst_index = y * PIECE_BITMAP_STRIDE + x / 2;

                if ((x & 1) != 0)
                {
                    qizi_bmp_scaled[piece][dst_index] |= pixel;
                }
                else
                {
                    qizi_bmp_scaled[piece][dst_index] = pixel << 4;
                }
            }
        }
    }
    qizi_bmp_scaled_ready = 1;
}
#endif

char chess_map[12][11] = {
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
};

// ==================== AI Related Functions ====================

// Move arrays (each move occupies 5 array elements);
int move_from_col[320];
int move_from_row[320];
int move_to_col[320];
int move_to_row[320];
int move_score[320];

/* xqwlight-only build */

/* 开局库函数声明 */

// Selected piece position
int sel_col = -1, sel_row = -1; // Selected piece coordinates (-1 means not selected);
int cur_x = 8, cur_y = 9;       // Current cursor position (初始在红方右路车位置);

// 红方光标位置（用于红方回合）
int red_cur_col = 8, red_cur_row = 9;

// 黑方光标位置（用于黑方回合）
int black_cur_col = 4, black_cur_row = 4;

// 当前回合：1=红方（玩家），2=黑方（AI）
int current_turn = 1;

int ai_timeout_seconds = 45;
int ai_start_tick;
int ai_last_display_tick;
int ai_elapsed_seconds;
int ai_should_stop = 0;

/* AI 拒绝和棋的随机提示语 */
char g_refuse_msgs[][32] = {
    "AI拒绝和棋！",
    "想跑？没门！",
    "优势在我，不和！",
    "才不跟你就此罢手！",
    "求和？等我赢！"
};

// xqwlight AI 模块全局变量在此文件中定义 (通过 ai_xqwlight_*.c)

// 红方最后走棋位置（用于显示反色框）
int last_red_from_col = -1, last_red_from_row = -1;
int last_red_to_col = -1, last_red_to_row = -1;

// 黑方最后走棋位置（用于显示反色框）
int last_black_from_col = -1, last_black_from_row = -1;
int last_black_to_col = -1, last_black_to_row = -1;

/* 包含开局库模块 */

/* 包含 engine 模块（记谱函数） */
#include "engine.c"

/* 包含 GUI 模块（难度选择窗口） */
#include "gui.c"
#include "dialog.c"
#include "dialog_pause.c"

/* 包含 xqwlight AI 模块 */
#include "ai_xqwlight_data.c"
#include "ai_xqwlight_util.c"
#include "ai_xqwlight_position.c"
#include "ai_xqwlight_book.c"
#include "ai_xqwlight_search.c"
#include "ai_xqwlight_adapter.c"

/* 包含 AI 和棋决策模块 */
#include "ai_draw_decision.c"

/* 包含悔棋/恢复模块 */
#include "undo_redo.c"

// Log move to file (append mode, human-readable format);
// 参数格式：(from_row, from_col, to_row, to_col);
void log_move(int from_row, int from_col, int to_row, int to_col, int player, int piece)
{
    char name[20];
    char human_str[40];
    int is_red;
    int piece_type;
    char direction[10];
    int step_or_road;
    int road;
    int row_diff;
    char time[8];

    // cchess-fc AI 变量
    int ai_from_row, ai_from_col, ai_to_row, ai_to_col;

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
        /* 输出原始坐标格式到控制台 */
        DPRINTF("%s moved from(%d,%d) to(%d,%d)\n", name, from_row, from_col, to_row, to_col);
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

    /* 输出记谱和原始坐标到控制台 */
    DPRINTF("%s (%d,%d)->(%d,%d)\n", human_str, from_row, from_col, to_row, to_col);
}
/* ==================== 棋子相关函数 ==================== */
/**
 * @brief 获取棋子颜色
 * @param piece 棋子编号
 * @return 1=红方，2=黑方，0=空位
 */
int get_piece_color(int piece)
{
    if (piece == 0)
        return 0;
    if (piece >= 1 && piece <= 7)
        return 1; /* 红方 */
    return 2;     /* 黑方 */
}

/* ==================== 位置验证函数 ==================== */
/**
 * @brief 检查位置是否在棋盘内
 * @param col 列坐标 (0-8);
 * @param row 行坐标 (0-9);
 * @return 1=有效，0=无效
 */
int is_valid_pos(int col, int row)
{
    if (col < 0 || col > 8)
        return 0;
    if (row < 0 || row > 9)
        return 0;
    return 1;
}

// Check if there are pieces blocking the path (for Chariot and Cannon)
int has_piece_between(int col1, int row1, int col2, int row2)
{
    int dcol, drow;
    int col, row;
    int steps, i;
    int dir;

    dcol = col2 - col1;
    drow = row2 - row1;

    // Must be on the same row or column
    if (dcol != 0 && drow != 0)
        return -1;

    if (dcol != 0)
    {
        // Horizontal movement
        steps = dcol;
        if (steps < 0)
            steps = -steps;
        if (dcol > 0)
            dir = 1;
        else
            dir = -1;
        col = col1 + dir;
        for (i = 0; i < steps - 1; i++)
        {
            if (chess_map[row1][col] != 0)
                return 1; // Piece blocking
            col = col + dir;
        }
    }
    else
    {
        // Vertical movement
        steps = drow;
        if (steps < 0)
            steps = -steps;
        if (drow > 0)
            dir = 1;
        else
            dir = -1;
        row = row1 + dir;
        for (i = 0; i < steps - 1; i++)
        {
            if (chess_map[row][col1] != 0)
                return 1; // Piece blocking
            row = row + dir;
        }
    }
    return 0; // No blocking
}

// Count number of pieces on path (for Cannon only);
int count_pieces_between(int col1, int row1, int col2, int row2)
{
    int dcol, drow;
    int col, row;
    int steps, i;
    int dir;
    int count;

    dcol = col2 - col1;
    drow = row2 - row1;

    // Must be on the same row or column
    if (dcol != 0 && drow != 0)
        return 0;

    count = 0;

    if (dcol != 0)
    {
        // Horizontal movement
        steps = dcol;
        if (steps < 0)
            steps = -steps;
        if (dcol > 0)
            dir = 1;
        else
            dir = -1;
        col = col1 + dir;
        for (i = 0; i < steps - 1; i++)
        {
            if (chess_map[row1][col] != 0)
                count = count + 1;
            col = col + dir;
        }
    }
    else
    {
        // Vertical movement
        steps = drow;
        if (steps < 0)
            steps = -steps;
        if (drow > 0)
            dir = 1;
        else
            dir = -1;
        row = row1 + dir;
        for (i = 0; i < steps - 1; i++)
        {
            if (chess_map[row][col1] != 0)
                count = count + 1;
            row = row + dir;
        }
    }

    return count;
}

// Check if horse's leg is blocked
int is_horse_blocked(int col1, int row1, int col2, int row2)
{
    int dcol, drow;

    dcol = col2 - col1;
    drow = row2 - row1;

    if (dcol == 2)
        return chess_map[row1][col1 + 1] != 0; // Move right
    if (dcol == -2)
        return chess_map[row1][col1 - 1] != 0; // Move left
    if (drow == 2)
        return chess_map[row1 + 1][col1] != 0; // Move down
    if (drow == -2)
        return chess_map[row1 - 1][col1] != 0; // Move up
    return 0;
}

// Check if elephant's eye is blocked
int is_elephant_blocked(int col1, int row1, int col2, int row2)
{
    int mcol, mrow;

    mcol = (col1 + col2) / 2;
    mrow = (row1 + row2) / 2;
    return chess_map[mrow][mcol] != 0;
}

// Check if position is within palace (9-grid area);
int is_in_palace(int col, int row, int is_red)
{
    if (col < 3 || col > 5)
        return 0;
    if (is_red)
    {
        if (row < 7 || row > 9)
            return 0;
    }
    else
    {
        if (row < 0 || row > 2)
            return 0;
    }
    return 1;
}

// Check if position is in own territory (for Elephant);
int is_in_own_territory(int row, int is_red)
{
    if (is_red)
        return row >= 5; // Red is at bottom, row>=5 is own territory
    return row <= 4;     // Black is at top, row<=4 is own territory
}

int are_kings_facing();
int is_red_in_check();
int is_black_in_check();
int is_move_legal_with_self_check(int col1, int row1, int col2, int row2);

// Determine if piece can move to target position
int can_move(int col1, int row1, int col2, int row2)
{
    int piece;
    int color;
    int target_color;
    int dcol, drow;
    int abs_dcol, abs_drow;
    int is_red;
    char piece_name[20];

    /* 获取棋子 */
    piece = chess_map[row1][col1];

    /* 获取棋子名称 */
    if (piece == 1)
        sprintf(piece_name, "RED_ROOK");
    else if (piece == 2)
        sprintf(piece_name, "RED_KNIGHT");
    else if (piece == 3)
        sprintf(piece_name, "RED_ELEPHANT");
    else if (piece == 4)
        sprintf(piece_name, "RED_ADVISOR");
    else if (piece == 5)
        sprintf(piece_name, "RED_KING");
    else if (piece == 6)
        sprintf(piece_name, "RED_CANNON");
    else if (piece == 7)
        sprintf(piece_name, "RED_PAWN");
    else if (piece == 8)
        sprintf(piece_name, "BLK_ROOK");
    else if (piece == 9)
        sprintf(piece_name, "BLK_KNIGHT");
    else if (piece == 10)
        sprintf(piece_name, "BLK_ELEPHANT");
    else if (piece == 11)
        sprintf(piece_name, "BLK_ADVISOR");
    else if (piece == 12)
        sprintf(piece_name, "BLK_KING");
    else if (piece == 13)
        sprintf(piece_name, "BLK_CANNON");
    else if (piece == 14)
        sprintf(piece_name, "BLK_PAWN");
    else
        sprintf(piece_name, "EMPTY");

    // 调试
    if(can_move_debug == 1)  DPRINTF("[can_move] Checking: (%d,%d)->(%d,%d), piece=%s\n", row1, col1, row2, col2, piece_name);

    // Check if target position is valid
    if (!is_valid_pos(col2, row2))
    {
        if(can_move_debug == 1)  DPRINTF("[can_move] Result: INVALID (target position out of bounds)");
        return 0;
    }

    // Get the chess piece
    if (piece == 0)
    {
        if(can_move_debug == 1)  DPRINTF("[can_move] Result: INVALID (source position is empty)");
        return 0;
    }

    // Get color information
    color = get_piece_color(piece);
    target_color = get_piece_color(chess_map[row2][col2]);

    // Cannot capture own pieces
    if (target_color == color)
    {
        if(can_move_debug == 1)  DPRINTF("[can_move] Result: INVALID (cannot capture own piece)");
        return 0;
    }

    dcol = col2 - col1;
    drow = row2 - row1;
    abs_dcol = dcol;
    abs_drow = drow;
    if (abs_dcol < 0)
        abs_dcol = -abs_dcol;
    if (abs_drow < 0)
        abs_drow = -abs_drow;

    // Determine if it's red side
    if (color == 1)
        is_red = 1;
    else
        is_red = 0;

    /* 根据棋子类型确定移动规则（使用 if-else 代替 switch） */
    /* 车 */
    if (piece == RED_CHE || piece == BLACK_CHE)
    {
        if(can_move_debug == 1)  DPRINTF("[can_move] Rook check: (%d,%d)->(%d,%d), dcol=%d, drow=%d\n", row1, col1, row2, col2, dcol, drow);
        if (dcol != 0 && drow != 0)
        {
            if(can_move_debug == 1)  DPRINTF("[can_move] Result: INVALID (rook must move in straight line)\n");
            return 0;
        }
        if (has_piece_between(col1, row1, col2, row2) == 0)
        {
            if(can_move_debug == 1)  DPRINTF("[can_move] Result: VALID (rook move)");
            return 1;
        }
        if(can_move_debug == 1)  DPRINTF("[can_move] Result: INVALID (rook path blocked)");
        return 0;
    }

    // Horse (Knight);
    if (piece == RED_MA || piece == BLACK_MA)
    {
        if (abs_dcol == 1 && abs_drow == 2)
        {
            if (!is_horse_blocked(col1, row1, col2, row2))
                return 1;
            return 0;
        }
        if (abs_dcol == 2 && abs_drow == 1)
        {
            if (!is_horse_blocked(col1, row1, col2, row2))
                return 1;
            return 0;
        }
        return 0;
    }

    // Elephant (Bishop);
    if (piece == RED_XIANG || piece == BLACK_XIANG)
    {
        if (abs_dcol != 2 || abs_drow != 2)
            return 0;
        if (is_elephant_blocked(col1, row1, col2, row2))
            return 0;
        if (!is_in_own_territory(row2, is_red))
            return 0;
        return 1;
    }

    // Advisor (Guard);
    if (piece == RED_SHI || piece == BLACK_SHI)
    {
        if (abs_dcol != 1 || abs_drow != 1)
            return 0;
        if (!is_in_palace(col2, row2, is_red))
            return 0;
        return 1;
    }

    // General (King);
    if (piece == RED_SHUAI || piece == BLACK_JIANG)
    {
        if (abs_dcol + abs_drow != 1)
            return 0;
        if (!is_in_palace(col2, row2, is_red))
            return 0;
        return 1;
    }

    // Cannon
    if (piece == RED_PAO || piece == BLACK_PAO)
    {
        if (dcol != 0 && drow != 0)
            return 0;
        if (chess_map[row2][col2] == 0)
        {
            // Moving: No pieces allowed in path
            if (has_piece_between(col1, row1, col2, row2) == 0)
                return 1;
            return 0;
        }
        else
        {
            // Capturing: Exactly one piece (cannon mount) must be in path
            if (count_pieces_between(col1, row1, col2, row2) == 1)
                return 1;
            return 0;
        }
    }

    /* 红兵 */
    if (piece == RED_BING)
    {
        if(can_move_debug == 1)  DPRINTF("[can_move] Red Pawn check: (%d,%d)->(%d,%d), dcol=%d, drow=%d, row1=%d\n", row1, col1, row2, col2, dcol, drow, row1);
        /* 只能前进、左移或右移，不能后退 */
        if (drow > 0)
        {
            if(can_move_debug == 1)  DPRINTF("[can_move] Result: INVALID (cannot move down)\n");
            return 0;
        }
        if (drow < 0 && abs_dcol > 0)
        {
            if(can_move_debug == 1)  DPRINTF("[can_move] Result: INVALID (cannot move diagonally)\n");
            return 0;
        }
        // Can only move forward before crossing river (row1 > 4), can move horizontally after crossing (row1 <= 4);
        if (row1 <= 4)
        {
            // After crossing river
            if (abs_dcol + abs_drow == 1)
            {
                if(can_move_debug == 1)  DPRINTF("[can_move] Result: VALID (pawn move after river)\n");
                return 1;
            }
            if(can_move_debug == 1)  DPRINTF("[can_move] Result: INVALID (invalid pawn move after river)\n");
            return 0;
        }
        else
        {
            // Before crossing river, can only move forward
            if (drow == -1 && dcol == 0)
            {
                if(can_move_debug == 1)  DPRINTF("[can_move] Result: VALID (pawn forward move)\n");
                return 1;
            }
            if(can_move_debug == 1)  DPRINTF("[can_move] Result: INVALID (pawn can only move forward)\n");
            return 0;
        }
    }

    // Black Pawn
    if (piece == BLACK_ZU)
    {
        // Can only move forward, left, or right, cannot move backward
        if (drow < 0)
            return 0; // Cannot move up
        // Can only move forward before crossing river (row1 < 5), can move horizontally after (row1 >= 5);
        if (row1 < 5)
        {
            // Before crossing river, can only move forward
            if (drow == 1 && dcol == 0)
                return 1;
            return 0;
        }
        else
        {
            // After crossing river
            if (abs_dcol + abs_drow == 1)
                return 1;
            return 0;
        }
    }

    return 0;
}

// Check whether the two kings face each other directly on the same file
int are_kings_facing()
{
    int red_row;
    int red_col;
    int black_row;
    int black_col;
    int found_red;
    int found_black;
    int row;

    found_red = 0;
    found_black = 0;
    red_row = -1;
    red_col = -1;
    black_row = -1;
    black_col = -1;

    for (row = 0; row <= 9; row++)
    {
        int col;
        for (col = 0; col <= 8; col++)
        {
            if (chess_map[row][col] == RED_SHUAI)
            {
                red_row = row;
                red_col = col;
                found_red = 1;
            }
            else if (chess_map[row][col] == BLACK_JIANG)
            {
                black_row = row;
                black_col = col;
                found_black = 1;
            }
        }
    }

    if (found_red == 0 || found_black == 0)
        return 0;
    if (red_col != black_col)
        return 0;

    for (row = black_row + 1; row < red_row; row++)
    {
        if (chess_map[row][red_col] != 0)
            return 0;
    }

    return 1;
}

// Full legality check: basic move rules + own king safety after the move
int is_move_legal_with_self_check(int col1, int row1, int col2, int row2)
{
    int piece;
    int color;
    int captured_piece;
    int legal;

    if (!is_valid_pos(col1, row1) || !is_valid_pos(col2, row2))
        return 0;
    if (can_move(col1, row1, col2, row2) == 0)
        return 0;

    piece = chess_map[row1][col1];
    if (piece == 0)
        return 0;

    color = get_piece_color(piece);
    captured_piece = chess_map[row2][col2];

    chess_map[row2][col2] = piece;
    chess_map[row1][col1] = 0;

    legal = 1;
    if (color == 1)
    {
        if (is_red_in_check() != 0)
            legal = 0;
    }
    else if (color == 2)
    {
        if (is_black_in_check() != 0)
            legal = 0;
    }
    else
    {
        legal = 0;
    }

    chess_map[row1][col1] = piece;
    chess_map[row2][col2] = captured_piece;
    return legal;
}

// Get base value of chess piece
int get_piece_base_value(int piece)
{
    if (piece == RED_SHUAI || piece == BLACK_JIANG)
        return 255;
    if (piece == RED_CHE || piece == BLACK_CHE)
        return 96;
    if (piece == RED_MA || piece == BLACK_MA)
        return 48;
    if (piece == RED_PAO || piece == BLACK_PAO)
        return 48;
    if (piece == RED_XIANG || piece == BLACK_XIANG)
        return 32;
    if (piece == RED_SHI || piece == BLACK_SHI)
        return 32;
    if (piece == RED_BING || piece == BLACK_ZU)
        return 16;
    return 0;
}

// Check if piece is under attack by enemy
int is_under_attack_by_enemy(int col, int row, int is_red)
{
    int enemy_color;
    int from_col, from_row;
    int piece;

    if (is_red)
        enemy_color = 2; // Red side checks for black attacks
    else
        enemy_color = 1; // Black side checks for red attacks

    // Iterate through all enemy pieces
    for (from_row = 0; from_row <= 9; from_row++)
    {
        for (from_col = 0; from_col <= 8; from_col++)
        {
            piece = chess_map[from_row][from_col];
            if (piece == 0)
                continue;

            if (get_piece_color(piece) == enemy_color)
            {
                if (can_move(from_col, from_row, col, row))
                {
                    return 1; // Under attack
                }
            }
        }
    }
    return 0;
}

// Check if piece has protection (friendly pieces nearby);
int has_protection(int col, int row, int is_red)
{
    int my_color;
    int dcol, drow;
    int ncol, nrow;
    int piece;

    if (is_red)
        my_color = 1;
    else
        my_color = 2;

    // Check 3x3 area around for friendly pieces
    for (drow = -1; drow <= 1; drow++)
    {
        for (dcol = -1; dcol <= 1; dcol++)
        {
            if (dcol == 0 && drow == 0)
                continue;

            ncol = col + dcol;
            nrow = row + drow;

            if (ncol >= 0 && ncol <= 8 && nrow >= 0 && nrow <= 9)
            {
                piece = chess_map[nrow][ncol];
                if (piece != 0 && get_piece_color(piece) == my_color)
                {
                    return 1;
                }
            }
        }
    }
    return 0;
}

// Calculate total value of protecting pieces
int get_protection_value(int col, int row, int is_red)
{
    int my_color;
    int px, py;
    int piece;
    int total_value;
    int protector_count;
    int i;

    if (is_red)
        my_color = 1;
    else
        my_color = 2;

    total_value = 0;
    protector_count = 0;

    // Check all friendly pieces to see if they can move to (col,row) for protection
    for (py = 0; py <= 9; py++)
    {
        for (px = 0; px <= 8; px++)
        {
            piece = chess_map[py][px];
            if (piece == 0)
                continue;
            if (get_piece_color(piece) != my_color)
                continue;

            // Check if this piece can move to (col,row) for protection
            if (can_move(px, py, col, row))
            {
                total_value = total_value + get_piece_base_value(piece);
                protector_count = protector_count + 1;
                if (protector_count >= 3)
                    break; // Count maximum 3 protectors
            }
        }
        if (protector_count >= 3)
            break;
    }

    return total_value;
}

// Count number of attacking pieces
int count_attackers(int col, int row, int is_red)
{
    int enemy_color;
    int ex, ey;
    int piece;
    int count;

    if (is_red)
        enemy_color = 2;
    else
        enemy_color = 1;

    count = 0;

    // Check all enemy pieces to see if they can attack (col,row);
    for (ey = 0; ey <= 9; ey++)
    {
        for (ex = 0; ex <= 8; ex++)
        {
            piece = chess_map[ey][ex];
            if (piece == 0)
                continue;
            if (get_piece_color(piece) != enemy_color)
                continue;

            // Check if this piece can attack (col,row);
            if (can_move(ex, ey, col, row))
            {
                count = count + 1;
                if (count >= 3)
                    return 3; // Return maximum 3
            }
        }
    }

    return count;
}

// Calculate final piece value (considering risk);
// can_counter_exemption: whether has counter-capture exemption (can counter after capture);
int get_piece_final_value_ex(int piece, int col, int row, int is_red, int can_counter_exemption)
{
    int base_value;
    int final_value;
    int attack_count;
    int protector_value;

    base_value = get_piece_base_value(piece);
    if (base_value == 0)
        return 0;

    final_value = base_value;

    // Risk deduction: deduct based on piece value when under attack
    if (is_under_attack_by_enemy(col, row, is_red))
    {
        // Reduce risk deduction if has counter-capture exemption
        if (can_counter_exemption)
        {
            // Exempt most risk deduction, only keep basic deduction
            final_value = final_value - (base_value / 20); // Half deduction
        }
        else
        {
            // Basic deduction: 10% of piece value when under attack
            final_value = final_value - (base_value / 10);

            // Check if has protection
            protector_value = get_protection_value(col, row, is_red);

            // Large deduction if no protection or insufficient protection
            if (protector_value < base_value / 2)
            {
                // Insufficient protection, deduct 50% value (about to be captured);
                final_value = final_value - (base_value / 2);
            }

            // Additional deduction when attacked by multiple pieces
            attack_count = count_attackers(col, row, is_red);
            if (attack_count >= 2)
            {
                final_value = final_value - (base_value / 4);
            }

            // Severe penalty for high-value pieces without protection
            if (protector_value == 0)
            {
                if (piece == RED_CHE || piece == BLACK_CHE)
                {
                    // Chariot without protection under attack, almost captured
                    final_value = final_value - 400;
                }
                else if (piece == RED_MA || piece == BLACK_MA || piece == RED_PAO || piece == BLACK_PAO)
                {
                    // Horse or Cannon without protection under attack
                    final_value = final_value - 200;
                }
                else if (piece == RED_BING || piece == BLACK_ZU)
                {
                    // Pawn without protection under attack (low value before crossing river, high value after);
                    if (is_red && row <= 4)
                    {
                        // Red pawn has crossed river, higher value
                        final_value = final_value - 30;
                    }
                    else if (!is_red && row >= 5)
                    {
                        // Black pawn has crossed river, higher value
                        final_value = final_value - 30;
                    }
                    else
                    {
                        // Not crossed river, lower value but still needs protection
                        final_value = final_value - 15;
                    }
                }
            }
        }

        // Severe penalty when General is under attack (not completely canceled even with exemption);
        if (piece == RED_SHUAI || piece == BLACK_JIANG)
        {
            if (can_counter_exemption)
            {
                final_value = final_value - 10000; // Still deduct some after exemption
            }
            else
            {
                final_value = final_value - 50000;
            }
        }
    }

    // Ensure value is not less than 0
    if (final_value < 0)
        final_value = 0;

    return final_value;
}

// Keep compatibility with original function, call new function (no exemption);
int get_piece_final_value(int piece, int col, int row, int is_red)
{
    return get_piece_final_value_ex(piece, col, row, is_red, 0);
}

// Detect if Red General is in check (attacked by Black);
int is_red_in_check()
{
    int col, row;

    if (are_kings_facing() != 0)
        return 1;

    // Find position of Red General
    for (row = 0; row <= 9; row++)
    {
        for (col = 0; col <= 8; col++)
        {
            if (chess_map[row][col] == RED_SHUAI)
            {
                // Check if attacked by Black
                return is_under_attack_by_enemy(col, row, 1);
            }
        }
    }
    return 0;
}

// Detect if Black General is in check (attacked by Red);
int is_black_in_check()
{
    int col, row;

    if (are_kings_facing() != 0)
        return 1;

    // Find position of Black General
    for (row = 0; row <= 9; row++)
    {
        for (col = 0; col <= 8; col++)
        {
            if (chess_map[row][col] == BLACK_JIANG)
            {
                // Check if attacked by Red
                return is_under_attack_by_enemy(col, row, 0);
            }
        }
    }
    return 0;
}

// Detect if opponent is checkmated (in check and cannot escape);
int is_checkmate(int is_red_attacker)
{
    int in_check_flag;
    int has_escape;
    int tx, ty;
    int sx, sy;
    int piece;
    int col, row;
    int saved_piece;

    // Determine which side is in check
    if (is_red_attacker)
    {
        // Red attacking, check if Black is in check
        in_check_flag = is_black_in_check();
    }
    else
    {
        // Black attacking, check if Red is in check
        in_check_flag = is_red_in_check();
    }

    // Not checkmate if not in check
    if (in_check_flag == 0)
        return 0;

    // Check if checked side has legal moves to escape check
    has_escape = 0;

    // Iterate through all pieces of checked side
    for (row = 0; row <= 9 && has_escape == 0; row++)
    {
        for (col = 0; col <= 8 && has_escape == 0; col++)
        {
            piece = chess_map[row][col];
            if (piece == 0)
                continue;

            // Check if it's a piece of the checked side
            if (is_red_attacker)
            {
                // Red attacking, find Black pieces
                if (get_piece_color(piece) != 2)
                    continue;
            }
            else
            {
                // Black attacking, find Red pieces
                if (get_piece_color(piece) != 1)
                    continue;
            }

            // Try all possible moves
            for (ty = 0; ty <= 9 && has_escape == 0; ty++)
            {
                for (tx = 0; tx <= 8 && has_escape == 0; tx++)
                {
                    if (can_move(col, row, tx, ty))
                    {
                        // Simulate the move
                        saved_piece = chess_map[ty][tx];
                        chess_map[ty][tx] = piece;
                        chess_map[row][col] = 0;

                        // Check if check is escaped
                        if (is_red_attacker)
                        {
                            if (is_black_in_check() == 0)
                                has_escape = 1;
                        }
                        else
                        {
                            if (is_red_in_check() == 0)
                                has_escape = 1;
                        }

                        // Restore chessboard
                        chess_map[row][col] = piece;
                        chess_map[ty][tx] = saved_piece;
                    }
                }
            }
        }
    }

    // Checkmate if in check and no escape
    if (has_escape == 0)
        return 1;
    return 0;
}

/**
 * @brief 检查指定方是否有合法走法（走后不让自己被将军）
 * @param is_red 1=检查红方, 0=检查黑方
 * @return 1=有合法走法, 0=无合法走法（困毙）
 */
int has_legal_moves(int is_red)
{
    int has_move;
    int tx, ty;
    int piece;
    int col, row;
    int saved_piece;

    has_move = 0;

    /* 遍历指定方的所有棋子 */
    for (row = 0; row <= 9 && has_move == 0; row++)
    {
        for (col = 0; col <= 8 && has_move == 0; col++)
        {
            piece = chess_map[row][col];
            if (piece == 0)
                continue;

            /* 检查是否属于指定方 */
            if (is_red)
            {
                if (get_piece_color(piece) != 1)
                    continue;
            }
            else
            {
                if (get_piece_color(piece) != 2)
                    continue;
            }

            /* 尝试所有可能的目标位置 */
            for (ty = 0; ty <= 9 && has_move == 0; ty++)
            {
                for (tx = 0; tx <= 8 && has_move == 0; tx++)
                {
                    if (can_move(col, row, tx, ty))
                    {
                        /* 模拟走法 */
                        saved_piece = chess_map[ty][tx];
                        chess_map[ty][tx] = piece;
                        chess_map[row][col] = 0;

                        /* 走后自己不能被将军 */
                        if (is_red)
                        {
                            if (is_red_in_check() == 0)
                                has_move = 1;
                        }
                        else
                        {
                            if (is_black_in_check() == 0)
                                has_move = 1;
                        }

                        /* 恢复棋盘 */
                        chess_map[row][col] = piece;
                        chess_map[ty][tx] = saved_piece;
                    }
                }
            }
        }
    }

    return has_move;
}

// Detect if move is suicidal (piece is left unprotected and under attack after move);
int is_suicide_move(int from_col, int from_row, int to_col, int to_row, int is_red)
{
    int piece;
    int captured;
    int is_suicide;
    int protector_value;
    int base_value;
    int captured_value;

    piece = chess_map[from_row][from_col];
    captured = chess_map[to_row][to_col];
    base_value = get_piece_base_value(piece);

    // Simulate the move
    chess_map[to_row][to_col] = piece;
    chess_map[from_row][from_col] = 0;

    is_suicide = 0;

    // Check if piece is under attack and unprotected after move
    if (is_under_attack_by_enemy(to_col, to_row, is_red))
    {
        protector_value = get_protection_value(to_col, to_row, is_red);

        // Considered suicide if unprotected/insufficiently protected and captured piece value is much lower
        if (protector_value < base_value / 2)
        {
            captured_value = get_piece_base_value(captured);

            // Suicide if captured piece value is much lower than own value
            if (captured_value < base_value / 2)
            {
                is_suicide = 1;
            }
        }
    }

    // Restore chessboard
    chess_map[from_row][from_col] = piece;
    chess_map[to_row][to_col] = captured;

    return is_suicide;
}

// Detect if can counter-attack after capture (exempt from risk deduction);
int can_counter_attack(int from_col, int from_row, int to_col, int to_row, int is_red)
{
    int piece;
    int captured;
    int can_counter;
    int enemy_color;
    int ex, ey;
    int enemy_piece;
    int counter_value;

    piece = chess_map[from_row][from_col];
    captured = chess_map[to_row][to_col];

    // No exemption needed if no piece is captured
    if (captured == 0)
        return 0;

    // Simulate the move
    chess_map[to_row][to_col] = piece;
    chess_map[from_row][from_col] = 0;

    can_counter = 0;

    // Determine enemy color
    if (is_red)
        enemy_color = 2;
    else
        enemy_color = 1;

    // Check if can counter-attack enemy pieces after capture
    for (ey = 0; ey <= 9 && can_counter == 0; ey++)
    {
        for (ex = 0; ex <= 8 && can_counter == 0; ex++)
        {
            enemy_piece = chess_map[ey][ex];
            if (enemy_piece == 0)
                continue;
            if (get_piece_color(enemy_piece) != enemy_color)
                continue;

            // Check if can attack enemy piece from (to_col, to_row);
            if (can_move(ex, ey, to_col, to_row))
            {
                // Can counter-attack with profit (value of countered piece >= half of captured piece value);
                counter_value = get_piece_base_value(enemy_piece);
                if (counter_value >= get_piece_base_value(captured) / 2)
                {
                    can_counter = 1;
                }
            }
        }
    }

    // Restore chessboard
    chess_map[from_row][from_col] = piece;
    chess_map[to_row][to_col] = captured;

    return can_counter;
}

// Calculate piece position bonus
int get_position_bonus(int piece, int col, int row)
{
    int bonus;
    int is_red;
    int count;
    int dcol, drow;
    int has_block;
    int has_clear_line;

    bonus = 0;
    if (piece >= 1 && piece <= 7)
        is_red = 1;
    else
        is_red = 0;

    // Position bonus for Pawns - enhanced values
    if (piece == RED_BING)
    {
        if (row <= 5)
            bonus = 20; // +20 after crossing river
        if (row <= 2)
            bonus = 40; // +40 deep in enemy territory
        if (row <= 1)
            bonus = 60; // +60 threatening general
    }
    if (piece == BLACK_ZU)
    {
        if (row >= 4)
            bonus = 20; // +20 after crossing river
        if (row >= 7)
            bonus = 40; // +40 deep in enemy territory
        if (row >= 8)
            bonus = 60; // +60 threatening general
    }

    // Position bonus for Chariots (Rooks) - enhanced values and logic
    if (piece == RED_CHE || piece == BLACK_CHE)
    {
        // Palace control bonus
        if (col >= 3 && col <= 5)
        {
            if (row >= 0 && row <= 2)
                bonus = bonus + 30; // +30 near opponent's palace
            if (row >= 7 && row <= 9)
                bonus = bonus + 30; // +30 near own palace
        }

        // Open lines bonus - improved algorithm
        count = 0;
        // Horizontal check
        if (col > 0)
        {
            dcol = -1;
            while (col + dcol >= 0)
            {
                if (chess_map[row][col + dcol] != 0)
                {
                    count++;
                    break;
                }
                dcol = dcol - 1;
            }
        }
        if (col < 8)
        {
            dcol = 1;
            while (col + dcol <= 8)
            {
                if (chess_map[row][col + dcol] != 0)
                {
                    count++;
                    break;
                }
                dcol = dcol + 1;
            }
        }
        // Vertical check
        if (row > 0)
        {
            drow = -1;
            while (row + drow >= 0)
            {
                if (chess_map[row + drow][col] != 0)
                {
                    count++;
                    break;
                }
                drow = drow - 1;
            }
        }
        if (row < 9)
        {
            drow = 1;
            while (row + drow <= 9)
            {
                if (chess_map[row + drow][col] != 0)
                {
                    count++;
                    break;
                }
                drow = drow + 1;
            }
        }

        if (count <= 1)
            bonus = bonus + 25; // +25 for open lines (increased from +8);
    }

    // Position bonus for Horses (Knights) and Cannons - improved zone evaluation
    if (piece == RED_MA || piece == BLACK_MA || piece == RED_PAO || piece == BLACK_PAO)
    {
        // Defense zone bonus based on player color
        if (is_red)
        {
            if (row >= 6 && row <= 9)
                bonus = bonus + 15; // +15 in red defense zone
        }
        else
        {
            if (row >= 0 && row <= 3)
                bonus = bonus + 15; // +15 in black defense zone
        }
    }

    // Detect unblocked Horse (check four directions);
    if (piece == RED_MA || piece == BLACK_MA)
    {
        has_block = 0;

        // Horse moves in "day" shape, check four blocking positions
        // Up direction (col, row-1);
        if (row > 0 && chess_map[row - 1][col] != 0)
            has_block = 1;
        // Down direction (col, row+1);
        if (row < 9 && chess_map[row + 1][col] != 0)
            has_block = 1;
        // Left direction (col-1, row);
        if (col > 0 && chess_map[row][col - 1] != 0)
            has_block = 1;
        // Right direction (col+1, row);
        if (col < 8 && chess_map[row][col + 1] != 0)
            has_block = 1;

        // +12 for no blocking (increased from +4);
        if (has_block == 0)
            bonus = bonus + 12;
    }

    // Detect unobstructed Cannon (at least one clear line in four directions);
    if (piece == RED_PAO || piece == BLACK_PAO)
    {
        has_clear_line = 0;

        // Check horizontal left direction
        if (col > 0)
        {
            dcol = col - 1;
            while (dcol >= 0)
            {
                if (chess_map[row][dcol] != 0)
                    break;
                dcol = dcol - 1;
            }
            if (dcol < 0)
                has_clear_line = 1; // Unobstructed
        }

        // Check horizontal right direction
        if (has_clear_line == 0 && col < 8)
        {
            dcol = col + 1;
            while (dcol <= 8)
            {
                if (chess_map[row][dcol] != 0)
                    break;
                dcol = dcol + 1;
            }
            if (dcol > 8)
                has_clear_line = 1; // Unobstructed
        }

        // Check vertical up direction
        if (has_clear_line == 0 && row > 0)
        {
            drow = row - 1;
            while (drow >= 0)
            {
                if (chess_map[drow][col] != 0)
                    break;
                drow = drow - 1;
            }
            if (drow < 0)
                has_clear_line = 1; // Unobstructed
        }

        // Check vertical down direction
        if (has_clear_line == 0 && row < 9)
        {
            drow = row + 1;
            while (drow <= 9)
            {
                if (chess_map[drow][col] != 0)
                    break;
                drow = drow + 1;
            }
            if (drow > 9)
                has_clear_line = 1; // Unobstructed
        }

        // +12 for at least one clear line (increased from +4);
        if (has_clear_line == 1)
            bonus = bonus + 12;
    }

    // Position bonus for Generals (Kings) - improved positioning
    if (piece == RED_SHUAI || piece == BLACK_JIANG)
    {
        if (col == 4)
        {
            if (is_red && row == 8)
                bonus = 40; // +40 for center position
            if (!is_red && row == 1)
                bonus = 40; // +40 for center position
        }
        else if (col == 3 || col == 5)
        {
            if (is_red && row == 8)
                bonus = 20; // +20 for side position
            if (!is_red && row == 1)
                bonus = 20; // +20 for side position
        }
    }

    // Out-of-bounds penalty for Elephants (Bishops) and Advisors (Guards) - increased penalty
    if (piece == RED_XIANG)
    {
        // Red Elephant cannot cross river, must be at row>=5
        if (row < 5)
            bonus = -25;
    }
    if (piece == BLACK_XIANG)
    {
        // Black Elephant cannot cross river, must be at row<=4
        if (row > 4)
            bonus = -25;
    }
    if (piece == RED_SHI)
    {
        // Red Advisor must be in palace (col=3-5, row=7-9);
        if (col < 3 || col > 5 || row < 7 || row > 9)
            bonus = -25;
    }
    if (piece == BLACK_SHI)
    {
        // Black Advisor must be in palace (col=3-5, row=0-2);
        if (col < 3 || col > 5 || row < 0 || row > 2)
            bonus = -25;
    }

    return bonus;
}

// Evaluate board position (from one side's perspective);
int evaluate_board(int is_red)
{
    int col, row;
    int piece;
    int color;
    int total_score;
    int piece_value;
    int bonus;
    int is_piece_red;
    int red_checkmate;
    int black_checkmate;
    int center_control_bonus;
    int SHUAI_safety_bonus;
    int SHUAI_col, SHUAI_row;

    total_score = 0;

    // Check checkmate status
    red_checkmate = is_checkmate(0);   // Whether Black checkmates Red
    black_checkmate = is_checkmate(1); // Whether Red checkmates Black

    // Return maximum evaluation if checkmating opponent
    if (is_red && black_checkmate)
        return 32767;
    if (!is_red && red_checkmate)
        return 32767;

    // Return minimum evaluation if checkmated by opponent
    if (is_red && red_checkmate)
        return -32767;
    if (!is_red && black_checkmate)
        return -32767;

    // Additional bonus for checking (not checkmate);
    if (is_red && is_black_in_check())
        total_score = total_score + 500;
    if (!is_red && is_red_in_check())
        total_score = total_score + 500;

    // Find General position and evaluate safety
    SHUAI_safety_bonus = 0;
    if (is_red)
    {
        // Red perspective: find Black General
        for (row = 0; row <= 2; row++)
        {
            for (col = 3; col <= 5; col++)
            {
                if (chess_map[row][col] == BLACK_JIANG)
                {
                    SHUAI_col = col;
                    SHUAI_row = row;
                    // Black General inside palace
                    if (row == 0)
                        SHUAI_safety_bonus = SHUAI_safety_bonus + 50; // Bottom line is safest
                    else if (row == 1)
                        SHUAI_safety_bonus = SHUAI_safety_bonus + 30;
                    else
                        SHUAI_safety_bonus = SHUAI_safety_bonus + 20;
                    // Check center control
                    if (col == 4)
                        SHUAI_safety_bonus = SHUAI_safety_bonus + 100; // Bonus for center position
                    break;
                }
            }
        }
    }
    else
    {
        // Black perspective: find Red General
        for (row = 7; row <= 9; row++)
        {
            for (col = 3; col <= 5; col++)
            {
                if (chess_map[row][col] == RED_SHUAI)
                {
                    SHUAI_col = col;
                    SHUAI_row = row;
                    // Red General inside palace
                    if (row == 9)
                        SHUAI_safety_bonus = SHUAI_safety_bonus + 50;
                    else if (row == 8)
                        SHUAI_safety_bonus = SHUAI_safety_bonus + 30;
                    else
                        SHUAI_safety_bonus = SHUAI_safety_bonus + 20;
                    // Check center control
                    if (col == 4)
                        SHUAI_safety_bonus = SHUAI_safety_bonus + 100;
                    break;
                }
            }
        }
    }

    // Evaluate center control (5th file is critical);
    center_control_bonus = 0;
    for (row = 0; row <= 9; row++)
    {
        piece = chess_map[row][3]; // Check center file
        if (piece != 0)
        {
            color = get_piece_color(piece);
            if (color == 1)
            {
                // Red controls center
                if (is_red)
                    center_control_bonus = center_control_bonus + 50;
                else
                    center_control_bonus = center_control_bonus - 50;
            }
            else
            {
                // Black controls center
                if (is_red)
                    center_control_bonus = center_control_bonus - 50;
                else
                    center_control_bonus = center_control_bonus + 50;
            }
        }
    }

    // Adjust General safety and center control weights based on perspective
    if (is_red)
    {
        // Red perspective: Black General in more danger is better, Red General safer is better
        total_score = total_score - SHUAI_safety_bonus * 2; // Penalty: more unsafe Black General = higher Red score
        total_score = total_score + center_control_bonus;
    }
    else
    {
        // Black perspective: Red General in more danger is better, Black General safer is better
        total_score = total_score + SHUAI_safety_bonus * 2; // Bonus: safer Black General = higher Black score
        total_score = total_score - center_control_bonus;
    }

    for (row = 0; row <= 9; row++)
    {
        for (col = 0; col <= 8; col++)
        {
            piece = chess_map[row][col];
            if (piece == 0)
                continue;
            if (piece < 1 || piece > 14)
                continue; // Invalid piece code

            color = get_piece_color(piece);
            if (color == 1)
                is_piece_red = 1;
            else
                is_piece_red = 0;

            bonus = get_position_bonus(piece, col, row);
            piece_value = get_piece_final_value(piece, col, row, is_piece_red);
            piece_value = piece_value + bonus;

            if (color == 1)
            {
                // Red pieces
                if (is_red)
                    total_score = total_score + piece_value;
                else
                    total_score = total_score - piece_value;
            }
            else
            {
                // Black pieces
                if (is_red)
                    total_score = total_score - piece_value;
                else
                    total_score = total_score + piece_value;
            }
        }
    }

    return total_score;
}

// Generate all legal moves for one side
int generate_moves_main(int max_moves, int is_red)
{
    int col, row;
    int tcol, trow;
    int piece;
    int color;
    int move_count;
    int captured_piece;
    int is_capture;
    int is_check;
    int saved_target;
    int from_col_saved, from_row_saved;

    move_count = 0;

    for (row = 0; row <= 9 && move_count < max_moves; row++)
    {
        for (col = 0; col <= 8 && move_count < max_moves; col++)
        {
            piece = chess_map[row][col];
            if (piece == 0)
                continue;
            if (piece < 1 || piece > 14)
                continue; // Invalid piece code

            color = get_piece_color(piece);
            if ((is_red && color == 1) || (!is_red && color == 2))
            {
                // Save original coordinates for chessboard restoration
                from_col_saved = col;
                from_row_saved = row;

                // Iterate through all possible target positions
                for (trow = 0; trow <= 9 && move_count < max_moves; trow++)
                {
                    for (tcol = 0; tcol <= 8 && move_count < max_moves; tcol++)
                    {
                        if (can_move(col, row, tcol, trow))
                        {
                            move_from_col[move_count] = col;
                            move_from_row[move_count] = row;
                            move_to_col[move_count] = tcol;
                            move_to_row[move_count] = trow;

                            captured_piece = chess_map[trow][tcol];
                            is_capture = 0;
                            if (captured_piece != 0)
                                is_capture = 1;

                            // Detect check (simulate move first);
                            is_check = 0;
                            saved_target = chess_map[trow][tcol];
                            chess_map[trow][tcol] = piece;
                            chess_map[from_row_saved][from_col_saved] = 0;
                            if (is_red && is_black_in_check())
                                is_check = 1;
                            if (!is_red && is_red_in_check())
                                is_check = 1;
                            chess_map[from_row_saved][from_col_saved] = piece;
                            chess_map[trow][tcol] = saved_target;

                            // Calculate move score (capture first, check second);
                            move_score[move_count] = get_piece_base_value(captured_piece);
                            if (is_check)
                                move_score[move_count] = move_score[move_count] + 32; // Check bonus
                            move_count++;
                        }
                    }
                }
            }
        }
    }

    return move_count;
}

int minimax(int depth, int is_maximizing, int is_red, int alpha, int beta)
{
    int move_count;
    int i;
    int best_score;
    int score;
    int piece;
    int captured;
    int from_col, from_row, to_col, to_row;
    int is_check;
    int is_capture;
    int actual_depth;
    int is_suicide;
    int can_counter;
    int suicide_penalty;
    int piece_val;
    int saved_alpha, saved_beta;
    int is_exempt;
    int current_tick;
    int elapsed_ticks;
    int elapsed_seconds;
    int display_interval;
    char time_str[10];
    char debug_str[20];
    //struct TIME time;
    char time[8];

    /* cchess-fc AI 变量 */
    int ai_from_col, ai_from_row, ai_to_col, ai_to_row;

    saved_alpha = alpha;
    saved_beta = beta;

    // Check timeout and update display (time[6] as tick counter);
    GetTime(time);
    current_tick = time[6];
    elapsed_ticks = current_tick - ai_start_tick;
    // Handle loop overflow (0-255);
    if (elapsed_ticks < 0)
        elapsed_ticks = elapsed_ticks + 256;
    // Calculate elapsed seconds (256 ticks = 1 second);
    elapsed_seconds = elapsed_ticks / 256;

    // Update display every 5 seconds (5*256=1280 ticks);
    display_interval = elapsed_ticks - ai_last_display_tick;
    if (display_interval < 0)
        display_interval = display_interval + 256;
    if (display_interval >= 1280 || ai_elapsed_seconds != elapsed_seconds)
    {
        ai_last_display_tick = current_tick;
        ai_elapsed_seconds = elapsed_seconds;
    }

    // Timeout if exceeding 30 seconds (30*256=7680 ticks);
    if (elapsed_ticks >= 7680)
    {
        ai_should_stop = 1;
        strcpy(debug_str, "*TO2");
        GetTime(time);
        DPRINTF("[DEBUG %02d:%02d:%02d] %s\n", time[4], time[5], time[6], debug_str);
        return 0; // Timeout, return neutral value
    }

    actual_depth = depth;
    if (depth > 0 && depth < 2)
    {
        // Check if current position is in check
        if (is_red)
            is_check = is_red_in_check();
        else
            is_check = is_black_in_check();

        // Extend search depth if in check or capture
        if (is_check)
            actual_depth = 2;
    }

    if (actual_depth == 0)
    {
        return evaluate_board(is_red);
    }

    move_count = generate_moves_main(64, is_red);
    if (move_count == 0)
    {
        strcpy(debug_str, "*MC0");
        GetTime(time);
        DPRINTF("[DEBUG %02d:%02d:%02d] %s\n", time[4], time[5], time[6], debug_str);

        if (is_maximizing)
            return -10000;
        return 10000;
    }

    if (is_maximizing)
    {
        best_score = -32767;
        for (i = 0; i < move_count; i++)
        {
            from_col = move_from_col[i];
            from_row = move_from_row[i];
            to_col = move_to_col[i];
            to_row = move_to_row[i];

            // Simulate the move
            piece = chess_map[from_row][from_col];
            captured = chess_map[to_row][to_col];
            chess_map[to_row][to_col] = piece;
            chess_map[from_row][from_col] = 0;

            // Detect capture
            is_capture = 0;
            if (captured != 0)
                is_capture = 1;

            // Detect check
            is_check = 0;
            if (is_red && is_black_in_check())
                is_check = 1;
            if (!is_red && is_red_in_check())
                is_check = 1;

            // Calculate move evaluation

            score = minimax(actual_depth - 1, 0, is_red, alpha, beta);

            // Restore chessboard
            chess_map[from_row][from_col] = piece;
            chess_map[to_row][to_col] = captured;

            if (score > best_score)
                best_score = score;

            // if (is_exempt == 0) {
            //     if (score <= 160) return best_score;
            //     if (score >= 65000) return best_score;
            // }

            if (best_score >= saved_beta)
                return best_score;
            if (best_score > saved_alpha)
                saved_alpha = best_score;
        }
        return best_score;
    }
    else
    {
        best_score = 32767;
        for (i = 0; i < move_count; i++)
        {
            from_col = move_from_col[i];
            from_row = move_from_row[i];
            to_col = move_to_col[i];
            to_row = move_to_row[i];

            // Simulate the move
            piece = chess_map[from_row][from_col];
            captured = chess_map[to_row][to_col];
            chess_map[to_row][to_col] = piece;
            chess_map[from_row][from_col] = 0;

            // Detect capture
            is_capture = 0;
            if (captured != 0)
                is_capture = 1;

            // Detect check
            is_check = 0;
            if (is_red && is_black_in_check())
                is_check = 1;
            if (!is_red && is_red_in_check())
                is_check = 1;

            // Calculate move evaluation
            if (is_capture || is_check)
            {
                strcpy(debug_str, "C&C");
                GetTime(time);
                DPRINTF("[DEBUG %02d:%02d:%02d] %s\n", time[4], time[5], time[6], debug_str);

                score = minimax(actual_depth, 1, is_red, alpha, beta);
            }
            else
            {
                score = minimax(actual_depth - 1, 1, is_red, alpha, beta);
            }

            // Restore chessboard
            chess_map[from_row][from_col] = piece;
            chess_map[to_row][to_col] = captured;

            if (score < best_score)
                best_score = score;

            // if (is_exempt == 0) {
            //     if (score >= 65000) return best_score;
            //     if (score <= 160) return best_score;
            // }

            if (best_score <= saved_alpha)
                return best_score;
            if (best_score < saved_beta)
                saved_beta = best_score;
        }
        return best_score;
    }
}
// Check if game is over (whether General is captured);
// Return value: 0 = game continues, 1 = Red wins (Black General captured), 2 = Black wins (Red General captured);
int check_game_over()
{
    int col, row;
    int red_SHUAI_found, BLACK_JIANG_found;

    red_SHUAI_found = 0;
    BLACK_JIANG_found = 0;

    for (row = 0; row <= 9; row++)
    {
        for (col = 0; col <= 8; col++)
        {
            if (chess_map[row][col] == RED_SHUAI)
                red_SHUAI_found = 1;
            if (chess_map[row][col] == BLACK_JIANG)
                BLACK_JIANG_found = 1;
        }
    }

    if (red_SHUAI_found == 0)
        return 2; // Red General captured, Black wins
    if (BLACK_JIANG_found == 0)
        return 1; // Black General captured, Red wins

    /* 将死检测：对方将/帅在棋盘上但无法逃脱将军 */
    if (is_checkmate(1))
        return 1; /* 红方将死黑方，红方胜 */
    if (is_checkmate(0))
        return 2; /* 黑方将死红方，黑方胜 */

    /* 困毙检测：未被将军但无合法走法，该方判负 */
    if (has_legal_moves(1) == 0)
        return 2; /* 红方困毙，黑方胜 */
    if (has_legal_moves(0) == 0)
        return 1; /* 黑方困毙，红方胜 */

    return 0;     // Game continues
}

// AI makes a move
void ai_move_main(int is_red)
{
    int move_count;
    int i, j;
    int best_score;
    int best_index;
    int score;
    int piece;
    int captured;
    int from_col, from_row, to_col, to_row;
    char debug_str[20];
    char time_str[10];
    int piece_color;
    int captured_color;
    int alpha, beta;
    int temp;
    int temp_x, temp_y, temp_score;
    int greedy_index;
    int found_move;
    int attack_bonus;
    int move_idx;
    int hx1, hy1, hx2, hy2;
    int bx, by, btx, bty; /* 开局库走法坐标 */
    int is_suicide;
    int can_counter;
    int suicide_penalty;
    int is_exempt; /* FC 剪枝豁免标志 */
    int SHUAI_piece;
    int sx, sy;
    int dirs[4][1];
    int d;
    int nx, ny;
    int best_defense_score;
    int best_defense_idx;
    char time[8];

    /* cchess-fc AI 变量 */
    int ai_from_col, ai_from_row, ai_to_col, ai_to_row;


    /* 使用开局库走前 18 步（72 个历史记录，每步 4 个元素） */
    /* xqwlight only: opening book disabled */
    if (0)
    {
        DPRINTF("[AI] trying opening book\n");
        /* 尝试匹配开局库并获取走法 */
        if (0)
        {
            /* 检查走法是否合法 */
            if (can_move(bx, by, btx, bty))
            {
                piece = chess_map[by][bx];
                captured = chess_map[bty][btx];

                /* 执行走法 */
                chess_map[bty][btx] = piece;
                chess_map[by][bx] = 0;

                /* 记录到历史 */
                log_move(bx, by, btx, bty, 0, piece);

                strcpy(debug_str, "OPN");
                GetTime(time);
                DPRINTF("[DEBUG %02d:%02d:%02d] %s\n", time[4], time[5], time[6], debug_str);
                return;
            }
        }
    }

    strcpy(debug_str, "GEN");
    GetTime(time);
    DPRINTF("[DEBUG %02d:%02d:%02d] %s\n", time[4], time[5], time[6], debug_str);

    move_count = generate_moves_main(64, is_red);

    debug_str[0] = 'G';
    debug_str[1] = 'E';
    debug_str[2] = 'N';
    debug_str[3] = ':';
    debug_str[4] = '0' + (move_count / 10);
    debug_str[5] = '0' + (move_count % 10);
    debug_str[6] = 0;
    {
        GetTime(time);
        DPRINTF("[DEBUG %02d:%02d:%02d] %s\n", time[4], time[5], time[6], debug_str);
    }

    for (i = 0; i < move_count; i++)
    {
        from_col = move_from_col[i];
        from_row = move_from_row[i];
        to_col = move_to_col[i];
        to_row = move_to_row[i];

        captured = chess_map[to_row][to_col];
        if (captured != 0)
        {
            // Calculate capture bonus value
            attack_bonus = get_piece_base_value(captured);
            piece = chess_map[from_row][from_col];

            // Check if source position has a valid piece
            if (piece == 0)
            {
                continue; // Invalid move, skip
            }

            // Execute immediately if capturing high-value pieces (Chariot, Horse, Cannon);
            if (attack_bonus >= 48)
            {
                // Check suicide exemption (FC style: allow exchanges with counter-attack potential);
                chess_map[to_row][to_col] = piece;
                chess_map[from_row][from_col] = 0;

                is_suicide = is_suicide_move(from_col, from_row, to_col, to_row, is_red);
                can_counter = can_counter_attack(from_col, from_row, to_col, to_row, is_red);

                /* 如果非自杀或有反击则执行走法 */
                if (!is_suicide || can_counter)
                {
                    /* 记录到历史 */
                    log_move(from_col, from_row, to_col, to_row, 0, piece); /* 黑方走法 */

                    strcpy(debug_str, "GRD");
                    GetTime(time);
                    DPRINTF("[DEBUG %02d:%02d:%02d] %s\n", time[4], time[5], time[6], debug_str);
                    return;
                }

                // Restore board if suicide with no counter-attack, continue search
                chess_map[from_row][from_col] = piece;
                chess_map[to_row][to_col] = captured;
            }
        }
    }

    // Start AI timing
    GetTime(time);
    ai_start_tick = time[6];
    ai_last_display_tick = ai_start_tick;
    ai_elapsed_seconds = 0;
    ai_should_stop = 0;

    // Sort moves by score descending (FC style: prioritize captures and checks);
    for (i = 0; i < move_count - 1; i++)
    {
        for (j = i + 1; j < move_count; j++)
        {
            if (move_score[i] < move_score[j])
            {
                // Swap moves
                temp = move_from_col[i];
                move_from_col[i] = move_from_col[j];
                move_from_col[j] = temp;

                temp = move_from_row[i];
                move_from_row[i] = move_from_row[j];
                move_from_row[j] = temp;

                temp = move_to_col[i];
                move_to_col[i] = move_to_col[j];
                move_to_col[j] = temp;

                temp = move_to_row[i];
                move_to_row[i] = move_to_row[j];
                move_to_row[j] = temp;

                temp = move_score[i];
                move_score[i] = move_score[j];
                move_score[j] = temp;
            }
        }
    }

    // Check if in check, prioritize escape moves
    if (is_red && is_black_in_check())
    {
        // Red is in check
        best_defense_score = -1000;
        best_defense_idx = -1;
        for (i = 0; i < move_count; i++)
        {
            from_col = move_from_col[i];
            from_row = move_from_row[i];
            to_col = move_to_col[i];
            to_row = move_to_row[i];

            piece = chess_map[from_row][from_col];
            captured = chess_map[to_row][to_col];

            chess_map[to_row][to_col] = piece;
            chess_map[from_row][from_col] = 0;

            if (is_black_in_check() == 0)
            {
                score = evaluate_board(is_red);
                if (score > best_defense_score)
                {
                    best_defense_score = score;
                    best_defense_idx = i;
                }
            }

            chess_map[from_row][from_col] = piece;
            chess_map[to_row][to_col] = captured;
        }

        if (best_defense_idx != -1)
        {
            from_col = move_from_col[best_defense_idx];
            from_row = move_from_row[best_defense_idx];
            to_col = move_to_col[best_defense_idx];
            to_row = move_to_row[best_defense_idx];

            piece = chess_map[from_row][from_col];
            captured = chess_map[to_row][to_col];

            chess_map[to_row][to_col] = piece;
            chess_map[from_row][from_col] = 0;

            /* 记录到历史 */
            log_move(from_col, from_row, to_col, to_row, 0, piece);

            strcpy(debug_str, "DEF");
            GetTime(time);
            DPRINTF("[DEBUG %02d:%02d:%02d] %s\n", time[4], time[5], time[6], debug_str);
            return;
        }
    }
    else if (!is_red && is_red_in_check())
    {
        // Black is in check
        best_defense_score = -1000;
        best_defense_idx = -1;
        for (i = 0; i < move_count; i++)
        {
            from_col = move_from_col[i];
            from_row = move_from_row[i];
            to_col = move_to_col[i];
            to_row = move_to_row[i];

            piece = chess_map[from_row][from_col];
            captured = chess_map[to_row][to_col];

            chess_map[to_row][to_col] = piece;
            chess_map[from_row][from_col] = 0;

            if (is_red_in_check() == 0)
            {
                score = evaluate_board(is_red);
                if (score > best_defense_score)
                {
                    best_defense_score = score;
                    best_defense_idx = i;
                }
            }

            chess_map[from_row][from_col] = piece;
            chess_map[to_row][to_col] = captured;
        }

        if (best_defense_idx != -1)
        {
            from_col = move_from_col[best_defense_idx];
            from_row = move_from_row[best_defense_idx];
            to_col = move_to_col[best_defense_idx];
            to_row = move_to_row[best_defense_idx];

            piece = chess_map[from_row][from_col];
            captured = chess_map[to_row][to_col];

            chess_map[to_row][to_col] = piece;
            chess_map[from_row][from_col] = 0;

            /* 记录到历史 */
            log_move(from_col, from_row, to_col, to_row, 0, piece);

            strcpy(debug_str, "DEF");
            GetTime(time);
            DPRINTF("[DEBUG %02d:%02d:%02d] %s\n", time[4], time[5], time[6], debug_str);
            return;
        }
    }

    // Display generated move count
    debug_str[0] = 'M';
    debug_str[1] = ':';
    debug_str[2] = '0' + (move_count / 10);
    debug_str[3] = '0' + (move_count % 10);
    debug_str[4] = 0;
    GetTime(time);
    DPRINTF("[DEBUG %02d:%02d:%02d] %s\n", time[4], time[5], time[6], debug_str);

    if (move_count == 0)
    {
        // Record no moves available status
        strcpy(debug_str, "MOVE0");
        GetTime(time);
        DPRINTF("[DEBUG %02d:%02d:%02d] %s\n", time[4], time[5], time[6], debug_str);

        // Fallback: move General as last resort
        SHUAI_piece = BLACK_JIANG;
        // Find Black General position
        for (sy = 0; sy <= 9; sy++)
        {
            for (sx = 0; sx <= 8; sx++)
            {
                if (chess_map[sy][sx] == SHUAI_piece)
                {
                    // Initialize four direction vectors (up, down, right, left);
                    dirs[0][0] = 0;
                    dirs[0][0] = 1;
                    dirs[1][0] = 0;
                    dirs[1][0] = -1;
                    dirs[2][0] = 1;
                    dirs[2][0] = 0;
                    dirs[3][0] = -1;
                    dirs[3][0] = 0;
                    for (d = 0; d < 4; d++)
                    {
                        nx = sx + dirs[d][0];
                        ny = sy + dirs[d][0];
                        if (can_move(sx, sy, nx, ny))
                        {
                            /* 执行备选走法 */
                            chess_map[ny][nx] = SHUAI_piece;
                            chess_map[sy][sx] = 0;
                            /* 记录到历史 */
                            log_move(sx, sy, nx, ny, 0, SHUAI_piece);
                            /* 显示紧急走法信息 */
                            debug_str[0] = 'E';
                            debug_str[1] = 'M';
                            debug_str[2] = 'R';
                            debug_str[3] = 0;
                            GetTime(time);
                            DPRINTF("[DEBUG %02d:%02d:%02d] %s\n", time[4], time[5], time[6], debug_str);
                            return;
                        }
                    }
                    // No valid General moves found
                    strcpy(debug_str, "EMF");
                    GetTime(time);
                    DPRINTF("[DEBUG %02d:%02d:%02d] %s\n", time[4], time[5], time[6], debug_str);
                    return;
                }
            }
        }
        debug_str[0] = 'E';
        debug_str[1] = 'M';
        debug_str[2] = 'N';
        debug_str[3] = 'F';
        debug_str[4] = 0;
        GetTime(time);
        DPRINTF("[DEBUG %02d:%02d:%02d] %s\n", time[4], time[5], time[6], debug_str);
        return;
    }

    alpha = 64;
    beta = 65400;

    best_score = -32767;
    best_index = 0;

    ai_start_tick = 0;
    ai_should_stop = 0;

    if (ai_should_stop != 0)
    {
        debug_str[0] = 'E';
        debug_str[1] = 'M';
        debug_str[2] = 'R';
        debug_str[3] = 0;
        GetTime(time);
        DPRINTF("[DEBUG %02d:%02d:%02d] %s\n", time[4], time[5], time[6], debug_str);
        return;
    }

    for (i = 0; i < move_count && ai_should_stop == 0; i++)
    {
        ai_elapsed_seconds = i * 2;
        if (ai_elapsed_seconds >= ai_timeout_seconds)
        {
            ai_should_stop = 1;
            debug_str[0] = 'T';
            debug_str[1] = 'I';
            debug_str[2] = 'M';
            debug_str[3] = 'E';
            debug_str[4] = 'O';
            debug_str[5] = 'U';
            debug_str[6] = 'T';
            debug_str[7] = ':';
            debug_str[8] = '0' + (ai_elapsed_seconds / 10);
            debug_str[9] = '0' + (ai_elapsed_seconds % 10);
            debug_str[10] = 'S';
            debug_str[11] = 0;
            GetTime(time);
            DPRINTF("[DEBUG %02d:%02d:%02d] %s\n", time[4], time[5], time[6], debug_str);
            break;
        }

        from_col = move_from_col[i];
        from_row = move_from_row[i];
        to_col = move_to_col[i];
        to_row = move_to_row[i];

        piece = chess_map[from_row][from_col];
        captured = chess_map[to_row][to_col];

        is_exempt = 0;
        if (captured != 0)
            is_exempt = 1;
        if (move_score[i] >= 32)
            is_exempt = 1;

        debug_str[0] = 'A';
        debug_str[1] = 'I';
        debug_str[2] = '-';
        debug_str[3] = 'D';
        debug_str[4] = 'E';
        debug_str[5] = 'P';
        debug_str[6] = 'T';
        debug_str[7] = 'H';
        debug_str[8] = ':';
        debug_str[9] = '2';
        debug_str[10] = 0;
        GetTime(time);
        DPRINTF("[DEBUG %02d:%02d:%02d] %s\n", time[4], time[5], time[6], debug_str);

        debug_str[0] = 'M';
        debug_str[1] = 'O';
        debug_str[2] = 'V';
        debug_str[3] = 'E';
        debug_str[4] = ':';
        debug_str[5] = '0' + (from_col / 10);
        debug_str[6] = '0' + (from_col % 10);
        debug_str[7] = ',';
        debug_str[8] = '0' + (from_row / 10);
        debug_str[9] = '0' + (from_row % 10);
        debug_str[10] = '-';
        debug_str[11] = '>';
        debug_str[12] = '(';
        debug_str[13] = '0' + (to_col / 10);
        debug_str[14] = '0' + (to_col % 10);
        debug_str[15] = ',';
        debug_str[16] = '0' + (to_row / 10);
        debug_str[17] = '0' + (to_row % 10);
        debug_str[18] = ')';
        debug_str[19] = 0;
        GetTime(time);
        DPRINTF("[DEBUG %02d:%02d:%02d] %s\n", time[4], time[5], time[6], debug_str);

        chess_map[to_row][to_col] = piece;
        chess_map[from_row][from_col] = 0;

        score = minimax(1, 0, is_red, alpha, beta);

        debug_str[0] = 'S';
        debug_str[1] = 'C';
        debug_str[2] = 'O';
        debug_str[3] = 'R';
        debug_str[4] = 'E';
        debug_str[5] = ':';
        debug_str[6] = '0' + (score / 10000);
        debug_str[7] = '0' + ((score / 1000) % 10);
        debug_str[8] = '0' + ((score / 100) % 10);
        debug_str[9] = '0' + ((score / 10) % 10);
        debug_str[10] = '0' + (score % 10);
        debug_str[11] = 0;
        GetTime(time);
        DPRINTF("[DEBUG %02d:%02d:%02d] %s\n", time[4], time[5], time[6], debug_str);

        chess_map[from_row][from_col] = piece;
        chess_map[to_row][to_col] = captured;

        if (score > best_score)
        {
            best_score = score;
            best_index = i;
        }

        // Enhanced pruning conditions - more aggressive cutoff
        if (is_exempt == 0)
        {
            if (score <= 16)
            {

                debug_str[0] = 'A';
                debug_str[1] = 'l';
                debug_str[2] = 'p';
                debug_str[3] = 'h';
                debug_str[4] = 'a';
                debug_str[5] = '0';
                GetTime(time);
                DPRINTF("[DEBUG %02d:%02d:%02d] %s\n", time[4], time[5], time[6], debug_str);
            }
            else
            {
                debug_str[0] = 'b';
                debug_str[1] = 'e';
                debug_str[2] = 't';
                debug_str[3] = 'a';
                debug_str[4] = 0;
                GetTime(time);
                DPRINTF("[DEBUG %02d:%02d:%02d] %s\n", time[4], time[5], time[6], debug_str);
            }

            // More aggressive alpha-beta pruning
            if (score <= 160)
                break; // Tightened from 128 to 160
            if (score >= 65000)
                break; // Tightened from 65280 to 65000
        }

        // Additional early termination conditions
        // Stop if we've found a very good move and processed enough candidates
        if (i >= 15 && best_score >= 300)
        {
            debug_str[0] = 'E';
            debug_str[1] = 'A';
            debug_str[2] = 'R';
            debug_str[3] = 'L';
            debug_str[4] = 'Y';
            debug_str[5] = '_';
            debug_str[6] = 'S';
            debug_str[7] = 'T';
            debug_str[8] = 'O';
            debug_str[9] = 'P';
            debug_str[10] = 0;
            GetTime(time);
            DPRINTF("[DEBUG %02d:%02d:%02d] %s\n", time[4], time[5], time[6], debug_str);
            break;
        }

        // Progress logging every 5 moves
        if (i % 5 == 0)
        {
            // Display seconds
            time_str[0] = 'T';
            time_str[1] = ':';
            time_str[2] = '0' + (i / 10);
            time_str[3] = '0' + (i % 10);
            time_str[4] = 0;
            Block(STATUS_X + 5, 67, STATUS_RIGHT - 5, 80, 1);
            TextOut(STATUS_X + 9, 68, time_str, 9);
            Refresh();

            debug_str[0] = 'P';
            debug_str[1] = 'R';
            debug_str[2] = 'O';
            debug_str[3] = 'G';
            debug_str[4] = ':';
            debug_str[5] = '0' + (i / 10);
            debug_str[6] = '0' + (i % 10);
            debug_str[7] = 0;
            GetTime(time);
            DPRINTF("[DEBUG %02d:%02d:%02d] %s\n", time[4], time[5], time[6], debug_str);
        }
    }

    if (ai_should_stop)
    {
        debug_str[0] = 'T';
        debug_str[1] = 'O';
        debug_str[2] = 0;
        GetTime(time);
        DPRINTF("[DEBUG %02d:%02d:%02d] %s\n", time[4], time[5], time[6], debug_str);
    }

    debug_str[0] = 'B';
    debug_str[1] = ':';
    debug_str[2] = '0' + (move_from_col[best_index] % 10);
    debug_str[3] = '0' + (move_from_row[best_index] % 10);
    debug_str[4] = '-';
    debug_str[5] = '0' + (move_to_col[best_index] % 10);
    debug_str[6] = '0' + (move_to_row[best_index] % 10);
    debug_str[7] = 0;
    GetTime(time);
    DPRINTF("[DEBUG %02d:%02d:%02d] %s\n", time[4], time[5], time[6], debug_str);

    from_col = move_from_col[best_index];
    from_row = move_from_row[best_index];
    to_col = move_to_col[best_index];
    to_row = move_to_row[best_index];

    // Safety check: ensure valid piece at source position
    piece = chess_map[from_row][from_col];
    captured = chess_map[to_row][to_col];
    if (piece == 0)
    {
        // Invalid source position, use General fallback
        debug_str[0] = 'E';
        debug_str[1] = 'R';
        debug_str[2] = 'R';
        debug_str[3] = ':';
        debug_str[4] = '0' + (from_col % 10);
        debug_str[5] = '0' + (from_row % 10);
        debug_str[6] = 0;
        GetTime(time);
        DPRINTF("[DEBUG %02d:%02d:%02d] %s\n", time[4], time[5], time[6], debug_str);

        // Fallback: move General as last resort
        SHUAI_piece = BLACK_JIANG;
        for (sy = 0; sy <= 9; sy++)
        {
            for (sx = 0; sx <= 8; sx++)
            {
                if (chess_map[sy][sx] == SHUAI_piece)
                {
                    dirs[0][0] = 0;
                    dirs[0][0] = 1;
                    dirs[1][0] = 0;
                    dirs[1][0] = -1;
                    dirs[2][0] = 1;
                    dirs[2][0] = 0;
                    dirs[3][0] = -1;
                    dirs[3][0] = 0;
                    for (d = 0; d < 4; d++)
                    {
                        nx = sx + dirs[d][0];
                        ny = sy + dirs[d][0];
                        if (can_move(sx, sy, nx, ny))
                        {
                            chess_map[ny][nx] = SHUAI_piece;
                            chess_map[sy][sx] = 0;
                            /* 记录到历史 */
                            log_move(sx, sy, nx, ny, 0, SHUAI_piece);
                            debug_str[0] = 'E';
                            debug_str[1] = 'M';
                            debug_str[2] = 'R';
                            debug_str[3] = 0;
                            GetTime(time);
                            DPRINTF("[DEBUG %02d:%02d:%02d] %s\n", time[4], time[5], time[6], debug_str);
                            return;
                        }
                    }
                    strcpy(debug_str, "EMF");
                    DPRINTF("[DEBUG %02d:%02d:%02d] %s\n", time[4], time[5], time[6], debug_str);
                    return;
                }
            }
        }
        debug_str[0] = 'E';
        debug_str[1] = 'M';
        debug_str[2] = 'N';
        debug_str[3] = 'F';
        debug_str[4] = 0;
        GetTime(time);
        DPRINTF("[DEBUG %02d:%02d:%02d] %s\n", time[4], time[5], time[6], debug_str);
        return;
    }
    if (captured != 0)
    {
        piece_color = get_piece_color(piece);
        captured_color = get_piece_color(captured);
        if (piece_color == captured_color)
        {
            // Cannot capture own piece, skip this move
            return;
        }
    }

    /* 记录到历史 */
    log_move(from_col, from_row, to_col, to_row, 0, piece); /* 黑方走法 */

    /* 执行走法 */
    chess_map[to_row][to_col] = piece;
    chess_map[from_row][from_col] = 0;
}

void chess_init()
{
    int col, row;

    // 初始化中文名称数组
    init_chinese_names();

    // Clear entire chessboard array
    for (row = 0; row < 12; row++)
    {
        for (col = 0; col < 11; col++)
        {
            chess_map[row][col] = 0;
        }
    }

    // Initialize Red pieces (row=9 底部);
    chess_map[9][0] = RED_CHE;
    chess_map[9][8] = RED_CHE; // Red Chariots (Rooks) 车
    chess_map[9][1] = RED_MA;
    chess_map[9][7] = RED_MA; // Red Horses (Knights) 马
    chess_map[9][2] = RED_XIANG;
    chess_map[9][6] = RED_XIANG; // Red Elephants (Bishops) 象
    chess_map[9][3] = RED_SHI;
    chess_map[9][5] = RED_SHI;   // Red Advisors (Guards) 士
    chess_map[9][4] = RED_SHUAI; // Red General (King) 帅
    chess_map[7][1] = RED_PAO;
    chess_map[7][7] = RED_PAO; // Red Cannons 炮
    chess_map[6][0] = RED_BING;
    chess_map[6][2] = RED_BING;
    chess_map[6][4] = RED_BING;
    chess_map[6][6] = RED_BING;
    chess_map[6][8] = RED_BING; // Red Pawns 兵

    // Initialize Black pieces (row=0 顶部);
    chess_map[0][0] = BLACK_CHE;
    chess_map[0][8] = BLACK_CHE; // Black Chariots (Rooks) 车
    chess_map[0][1] = BLACK_MA;
    chess_map[0][7] = BLACK_MA; // Black Horses (Knights) 马
    chess_map[0][2] = BLACK_XIANG;
    chess_map[0][6] = BLACK_XIANG; // Black Elephants (Bishops) 象
    chess_map[0][3] = BLACK_SHI;
    chess_map[0][5] = BLACK_SHI;   // Black Advisors (Guards) 士
    chess_map[0][4] = BLACK_JIANG; // Black General (King) 将
    chess_map[2][1] = BLACK_PAO;
    chess_map[2][7] = BLACK_PAO; // Black Cannons 炮
    chess_map[3][0] = BLACK_ZU;
    chess_map[3][2] = BLACK_ZU;
    chess_map[3][4] = BLACK_ZU;
    chess_map[3][6] = BLACK_ZU;
    chess_map[3][8] = BLACK_ZU; // Black Pawns 卒
}

void draw_qipan()
{
    int i;
    // Draw background
    Block(0, 0, WIDTH, HEIGHT, 0);
    // Draw main board rectangle
    Rectangle(QIPAN_X, QIPAN_Y, QIPAN_X + 8 * GE_SZ, QIPAN_Y + GE_SZ * 9, 1);
    // Draw horizontal lines
    for (i = 1; i < 9; i++)
    {
        Line(QIPAN_X, QIPAN_Y + i * GE_SZ, QIPAN_X + 8 * GE_SZ, QIPAN_Y + i * GE_SZ, 0x41);
    }
    // Draw vertical lines (split at river);
    for (i = 1; i < 8; i++)
    {
        Line(QIPAN_X + i * GE_SZ, QIPAN_Y, QIPAN_X + i * GE_SZ, QIPAN_Y + GE_SZ * 4, 0x41);
        Line(QIPAN_X + i * GE_SZ, QIPAN_Y + GE_SZ * 5, QIPAN_X + i * GE_SZ, QIPAN_Y + GE_SZ * 9, 0x41);
    }
    // Draw Red palace diagonals
    Line(QIPAN_X + GE_SZ * 3, QIPAN_Y + GE_SZ * 0, QIPAN_X + GE_SZ * 5, QIPAN_Y + GE_SZ * 2, 0x41);
    Line(QIPAN_X + GE_SZ * 5, QIPAN_Y + GE_SZ * 0, QIPAN_X + GE_SZ * 3, QIPAN_Y + GE_SZ * 2, 0x41);
    // Draw Black palace diagonals
    Line(QIPAN_X + GE_SZ * 3, QIPAN_Y + GE_SZ * 7, QIPAN_X + GE_SZ * 5, QIPAN_Y + GE_SZ * 9, 0x41);
    Line(QIPAN_X + GE_SZ * 5, QIPAN_Y + GE_SZ * 7, QIPAN_X + GE_SZ * 3, QIPAN_Y + GE_SZ * 9, 0x41);

    // Use the landscape space beside the board for persistent game status.
    Rectangle(STATUS_X, 8, STATUS_RIGHT, HEIGHT - 9, 1);
    TextOut(STATUS_X + 12, 17, "中国象棋", 0x81);
    Block(STATUS_X + 5, 40, STATUS_RIGHT - 5, 40, 1);
    TextOut(STATUS_X + 9, 50, "玩家: 红方", 1);
    TextOut(STATUS_X + 9, 68, "轮到: 红方", 1);
    Block(STATUS_X + 5, 91, STATUS_RIGHT - 5, 91, 1);
    TextOut(STATUS_X + 9, 102, "方向键 移动", 1);
    TextOut(STATUS_X + 9, 120, "A 选择/落子", 1);
    TextOut(STATUS_X + 9, 138, "B 取消选择", 1);
    TextOut(STATUS_X + 9, 156, "Y 暂停菜单", 1);
    Block(STATUS_X + 5, 181, STATUS_RIGHT - 5, 181, 1);
    TextOut(STATUS_X + 9, 193, "SELECT+START", 1);
    TextOut(STATUS_X + 21, 209, "返回系统", 1);
}

void draw_qizi_all()
{
    int col, row;
    char c;



#ifdef LAVA_NATIVE_COMPILED
    init_scaled_piece_bitmaps();
#endif

    // Draw all pieces on board
    for (row = 0; row <= 9; row++)
    {
        for (col = 0; col <= 8; col++)
        {
            c = chess_map[row][col];
            if (c)
            {
#ifdef LAVA_NATIVE_COMPILED
                WriteBlock(QIPAN_X - PIECE_SIZE / 2 + col * GE_SZ,
                           QIPAN_Y - PIECE_SIZE / 2 + row * GE_SZ,
                           PIECE_SIZE, PIECE_SIZE, 1, qizi_bmp_scaled[c - 1]);
#else
                WriteBlock(QIPAN_X - 6 + col * GE_SZ, QIPAN_Y - 6 + row * GE_SZ,
                           13, 13, 1, qizi_bmp[c - 1]);
#endif
            }
        }
    }

    // Draw last red move marker (solid black rectangle on destination) - BEFORE pieces;
    if (last_red_to_col >= 0 && last_red_to_col <= 8 && last_red_to_row >= 0 && last_red_to_row <= 9)
    {
        int rx1 = QIPAN_X - PIECE_SIZE / 2 + last_red_to_col * GE_SZ;
        int ry1 = MAX(QIPAN_Y - PIECE_SIZE / 2 + last_red_to_row * GE_SZ, 0);
        int rx2 = QIPAN_X + PIECE_SIZE / 2 + last_red_to_col * GE_SZ;
        int ry2 = MAX(QIPAN_Y + PIECE_SIZE / 2 + last_red_to_row * GE_SZ, 0);
        int expand = 1;

        int x1 = rx1 - expand;
        int y1 = ry1 - expand;
        int x2 = rx2 + expand;
        int y2 = ry2 + expand;

        int len = 2;

        // 左上
        Line(x1, y1, x1 + len, y1, 1);
        Line(x1, y1, x1, y1 + len, 1);

        // 右上
        Line(x2, y1, x2 - len, y1, 1);
        Line(x2, y1, x2, y1 + len, 1);

        // 左下
        Line(x1, y2, x1 + len, y2, 1);
        Line(x1, y2, x1, y2 - len, 1);

        // 右下
        Line(x2, y2, x2 - len, y2, 1);
        Line(x2, y2, x2, y2 - len, 1);
    }

    // Draw last black move marker (solid black block on destination) - BEFORE pieces;
    if (last_black_to_col >= 0 && last_black_to_col <= 8 && last_black_to_row >= 0 && last_black_to_row <= 9)
    {
        int bx1 = QIPAN_X - PIECE_SIZE / 2 - 1 + last_black_to_col * GE_SZ;
        int by1 = MAX(QIPAN_Y - PIECE_SIZE / 2 - 1 + last_black_to_row * GE_SZ, 0);
        int bx2 = QIPAN_X + PIECE_SIZE / 2 + 1 + last_black_to_col * GE_SZ;
        int by2 = MAX(QIPAN_Y + PIECE_SIZE / 2 + 1 + last_black_to_row * GE_SZ, 0);
        int expand2 = 0;

        int xx1 = bx1 - expand2;
        int xy1 = by1 - expand2;
        int xx2 = bx2 + expand2;
        int xy2 = by2 + expand2;

        int len2 = 2;

        // 左上
        Line(xx1, xy1, xx1 + len2, xy1, 1);
        Line(xx1, xy1, xx1, xy1 + len2, 1);

        // 右上
        Line(xx2, xy1, xx2 - len2, xy1, 1);
        Line(xx2, xy1, xx2, xy1 + len2, 1);

        // 左下
        Line(xx1, xy2, xx1 + len2, xy2, 1);
        Line(xx1, xy2, xx1, xy2 - len2, 1);

        // 右下
        Line(xx2, xy2, xx2 - len2, xy2, 1);
        Line(xx2, xy2, xx2, xy2 - len2, 1);
    }
    // 来源位置: 小黑点
    if (last_black_from_col >= 0 && last_black_from_col <= 8 && last_black_from_row >= 0 && last_black_from_row <= 9)
    {
        int dx = QIPAN_X + last_black_from_col * GE_SZ;
        int dy = MAX(QIPAN_Y + last_black_from_row * GE_SZ, 0);
        Block(dx - 2, dy - 2, dx + 2, dy + 2, 1);
    }

    // Draw cursor frame (only during player's turn, not during AI turn);
    if (current_turn == 1 && cur_x >= 0 && cur_x <= 8 && cur_y >= 0 && cur_y <= 9)
    {
        // Center on grid, avoid negative coordinates
        Rectangle(QIPAN_X - PIECE_SIZE / 2 - 1 + cur_x * GE_SZ,
                  MAX(QIPAN_Y - PIECE_SIZE / 2 - 1 + cur_y * GE_SZ, 0),
                  QIPAN_X + PIECE_SIZE / 2 + 1 + cur_x * GE_SZ,
                  MAX(QIPAN_Y + PIECE_SIZE / 2 + 1 + cur_y * GE_SZ, 0), 1);
    }

    // Draw selection marker
    if (sel_col >= 0 && sel_col <= 8 && sel_row >= 0 && sel_row <= 9)
    {
        // Center on grid, avoid negative coordinates
        Block(QIPAN_X - PIECE_SIZE / 2 - 1 + sel_col * GE_SZ,
              MAX(QIPAN_Y - PIECE_SIZE / 2 - 1 + sel_row * GE_SZ, 0),
              QIPAN_X + PIECE_SIZE / 2 + 1 + sel_col * GE_SZ,
              MAX(QIPAN_Y + PIECE_SIZE / 2 + 1 + sel_row * GE_SZ, 2), 2);
        Refresh();
    }

    // Draw difficulty level indicator (same position as in checkTimeout)
    {
        char lv_text[10];

        Block(STATUS_X + 5, 47, STATUS_RIGHT - 5, 80, 0);
        TextOut(STATUS_X + 9, 50, "玩家: 红方", 1);
        if (current_turn == 1)
        {
            TextOut(STATUS_X + 9, 68, "轮到: 红方", 1);
        }
        else
        {
            TextOut(STATUS_X + 9, 68, "轮到: 黑方", 1);
        }

        if (g_current_difficulty == 1)
        {
            strcpy(lv_text, "Lv1");
        }
        else if (g_current_difficulty == 2)
        {
            strcpy(lv_text, "Lv2");
        }
        else
        {
            strcpy(lv_text, "Lv3");
        }

        Block(STATUS_X + 5, 82, STATUS_RIGHT - 5, 89, 0);
        TextOut(STATUS_X + 9, 82, lv_text, 1);
    }
}

void game_loop()
{
    int i;
    int j;
    int piece; /* 用于和棋决策时读取棋子 */
    int di, dj, dpiece; /* 用于和棋决策时转换棋盘 */
    int captured_piece; /* 用于悔棋记录被吃掉的棋子 */
    char key;
    char debug_str[20];
    int piece_color;
    int game_over; // Game over flag: 0=no, 1=yes
    int winner;    /* Winner: 1=Red, 2=Black */
    char ai_piece_name[20];
    int ai_piece;
    char time[8];
    int difficulty; /* 难度级别 */
    int ai_accept; /* AI 是否接受和棋 */
    int refuse_idx; /* 拒绝和棋消息索引 */
    char refuse_msg[32]; /* 当前拒绝消息 */

    /* cchess-fc AI variables */
    int ai_from_col, ai_from_row, ai_to_col, ai_to_row;

    // Show difficulty selection menu
    difficulty = show_difficulty_menu();

    current_turn = 1; // Red moves first
    game_over = 0;
    winner = 0;
    chess_init();
    ai_cchess_init();
    undo_redo_init();
    draw_qipan();
    draw_qizi_all();
    Refresh();

    // Main game loop
    while (1)
    {
#ifdef LAVA_ESP32
        if (g_lava_shutdown_requested)
        {
            return;
        }
#endif
        // Check if game is over
        if (game_over == 1)
        {
            // Display game over message
            if (winner == 1)
            {
                strcpy(debug_str, "GAMEOVER RED WIN");
            }
            else
            {
                strcpy(debug_str, "GAMEOVER BLK WIN");
            }

            /* 绘制游戏结束对话框 */
            draw_game_over_dialog(winner);
            // Wait for A button to restart or B to exit
            while (1)
            {
                key = getchar();
                if (key == 0x0D)
                { // A button
                    // Reset game
                    current_turn = 1;
                    game_over = 0;
                    winner = 0;
                    sel_col = -1;
                    sel_row = -1;
                    cur_x = 8;
                    cur_y = 9;
                    red_cur_col = 8;
                    red_cur_row = 9;
                    black_cur_col = 4;
                    black_cur_row = 4;
                    /* 清空历史记录 */
                    last_red_from_col = -1;
                    last_red_from_row = -1;
                    last_red_to_col = -1;
                    last_red_to_row = -1;
                    last_black_from_col = -1;
                    last_black_from_row = -1;
                    last_black_to_col = -1;
                    last_black_to_row = -1;
                    chess_init();
                    undo_redo_init();
                    ai_cchess_init();
                    draw_qipan();
                    draw_qizi_all();
                    Refresh();
                    break;
                }
                else if (key == 0x1B)
                { // B button to exit
                    return;
                }
            }
            continue; /* 继续显示游戏结束消息 */
        }

        draw_qizi_all(); // 重新绘制棋子，确保在文字上层
        Refresh();

        if (current_turn == 1)
        {
            // Player (Red) turn
            key = getchar();

            // DPAD movement for cursor control
            if (key == 0x14)
            { // Up
                if (cur_y > 0)
                    cur_y--;
            }
            else if (key == 0x15)
            { // Down
                if (cur_y < 9)
                    cur_y++;
            }
            else if (key == 0x17)
            { // Left
                if (cur_x > 0)
                    cur_x--;
            }
            else if (key == 0x16)
            { // Right
                if (cur_x < 8)
                    cur_x++;
            }
            else if (key == 0x0D)
            { // A button
                if (sel_col < 0)
                {
                    // Select piece at current cursor position when no selection
                    if (chess_map[cur_y][cur_x] != 0)
                    {
                        piece_color = get_piece_color(chess_map[cur_y][cur_x]);
                        debug_str[0] = 'C';
                        debug_str[1] = ':';
                        debug_str[2] = '0' + piece_color;
                        debug_str[3] = 0;
                        GetTime(time);
                        DPRINTF("[DEBUG %02d:%02d:%02d] %s\n", time[4], time[5], time[6], debug_str);
                        if (piece_color == 1)
                        {
                            sel_col = cur_x;
                            sel_row = cur_y;
                        }
                    }
                }
                else
                {
                    // Move selected piece to cursor position
                    if (cur_x != sel_col || cur_y != sel_row)
                    {
                        // 调试：打印玩家走棋（标准象棋谱格式）
                        {
                            char player_move_notation[32];
                            char piece_name[32];
                            struct Move player_move;
                            piece = chess_map[sel_row][sel_col];
                            player_move.from_row = sel_row;
                            player_move.from_col = sel_col;
                            player_move.to_row = cur_y;
                            player_move.to_col = cur_x;
                            move_to_chinese_notation(&player_move, chess_map, player_move_notation);
                            /* 获取棋子名称 */
                            if (piece == 1)
                                strcpy(piece_name, "红车");
                            else if (piece == 2)
                                strcpy(piece_name, "红马");
                            else if (piece == 3)
                                strcpy(piece_name, "红相");
                            else if (piece == 4)
                                strcpy(piece_name, "红士");
                            else if (piece == 5)
                                strcpy(piece_name, "红帅");
                            else if (piece == 6)
                                strcpy(piece_name, "红炮");
                            else if (piece == 7)
                                strcpy(piece_name, "红兵");
                            else if (piece == 8)
                                strcpy(piece_name, "黑车");
                            else if (piece == 9)
                                strcpy(piece_name, "黑马");
                            else if (piece == 10)
                                strcpy(piece_name, "黑象");
                            else if (piece == 11)
                                strcpy(piece_name, "黑士");
                            else if (piece == 12)
                                strcpy(piece_name, "黑将");
                            else if (piece == 13)
                                strcpy(piece_name, "黑炮");
                            else if (piece == 14)
                                strcpy(piece_name, "黑卒");
                            else
                                strcpy(piece_name, "空位");
                            GetTime(time);
                            DPRINTF("[%02d:%02d:%02d] [玩家] 尝试走棋：%s, 棋子=%s\n", time[4], time[5], time[6], player_move_notation, piece_name);
                        }

                        /* 检查走法是否合法（参数顺序：col1, row1, col2, row2） */
                        if (is_move_legal_with_self_check(sel_col, sel_row, cur_x, cur_y))
                        {
                            char final_move_notation[32];
                            struct Move final_move;

                            /* 记录到历史 */
                            log_move(sel_row, sel_col, cur_y, cur_x, 1, chess_map[sel_row][sel_col]); /* 红方走法 */

                            final_move.from_row = sel_row;
                            final_move.from_col = sel_col;
                            final_move.to_row = cur_y;
                            final_move.to_col = cur_x;
                            move_to_chinese_notation(&final_move, chess_map, final_move_notation);
                            GetTime(time);
                            DPRINTF("[%02d:%02d:%02d] [chess notation] 红方=%s\n", time[4], time[5], time[6], final_move_notation);

                            /* 执行走法 */
                            captured_piece = chess_map[cur_y][cur_x];
                            chess_map[cur_y][cur_x] = chess_map[sel_row][sel_col];
                            chess_map[sel_row][sel_col] = 0;

                            /* 检查游戏结束状态 */
                            can_move_debug = 0;
                            winner = check_game_over();
                            can_move_debug = 1;
                            if (winner != 0) {
                                DEBUG_LOG("[DEBUG %02d:%02d:%02d] 玩家赢了\n", time[4], time[5], time[6]);
                                SET_GAME_OVER(1);
                            }

                            /* 切换到黑方回合 */
                            current_turn = 2;

                            /* 保存红方光标位置 */
                            red_cur_col = cur_x;
                            red_cur_row = cur_y;

                            /* 保存红方最后走棋位置 */
                            last_red_from_col = sel_col;
                            last_red_from_row = sel_row;
                            last_red_to_col = cur_x;
                            last_red_to_row = cur_y;

                            /* 记录悔棋历史 */
                            g_record_red_from_col = last_red_from_col;
                            g_record_red_from_row = last_red_from_row;
                            g_record_red_to_col = last_red_to_col;
                            g_record_red_to_row = last_red_to_row;
                            g_record_black_from_col = last_black_from_col;
                            g_record_black_from_row = last_black_from_row;
                            g_record_black_to_col = last_black_to_col;
                            g_record_black_to_row = last_black_to_row;
                            undo_redo_record_move(sel_row, sel_col, cur_y, cur_x, captured_piece);

                            // 恢复黑方光标位置
                            cur_x = black_cur_col;
                            cur_y = black_cur_row;
                        }
                    }
                    // Clear selection
                    sel_col = -1;
                    sel_row = -1;
                }
            }
            else if (key == 0x19)
            { // Y button - 暂停菜单
                int pause_action;
                pause_action = show_pause_menu();

                if (pause_action == 1)
                {
                    /* 求和 - AI 根据残局定式决定是否同意 */
                    ai_accept = 0;

                    /* 将 chess_map 转换为 g_draw_board 结构 */
                    di = 0; dj = 0; dpiece = 0;
                    for (di = 0; di < 10; di++) {
                        for (dj = 0; dj < 9; dj++) {
                            dpiece = chess_map[di][dj];
                            if (dpiece == 0) {
                                g_draw_board[di][dj].type = CC_PIECE_EMPTY;
                                g_draw_board[di][dj].color = CC_COLOR_NONE;
                            } else if (dpiece >= 1 && dpiece <= 7) {
                                g_draw_board[di][dj].color = CC_COLOR_RED;
                                if (dpiece == 1) g_draw_board[di][dj].type = CC_PIECE_ROOK;
                                else if (dpiece == 2) g_draw_board[di][dj].type = CC_PIECE_KNIGHT;
                                else if (dpiece == 3) g_draw_board[di][dj].type = CC_PIECE_ELEPHANT;
                                else if (dpiece == 4) g_draw_board[di][dj].type = CC_PIECE_ADVISOR;
                                else if (dpiece == 5) g_draw_board[di][dj].type = CC_PIECE_KING;
                                else if (dpiece == 6) g_draw_board[di][dj].type = CC_PIECE_CANNON;
                                else if (dpiece == 7) g_draw_board[di][dj].type = CC_PIECE_PAWN;
                                else { g_draw_board[di][dj].type = CC_PIECE_EMPTY; g_draw_board[di][dj].color = CC_COLOR_NONE; }
                            } else if (dpiece >= 8 && dpiece <= 14) {
                                g_draw_board[di][dj].color = CC_COLOR_BLACK;
                                if (dpiece == 8) g_draw_board[di][dj].type = CC_PIECE_ROOK;
                                else if (dpiece == 9) g_draw_board[di][dj].type = CC_PIECE_KNIGHT;
                                else if (dpiece == 10) g_draw_board[di][dj].type = CC_PIECE_ELEPHANT;
                                else if (dpiece == 11) g_draw_board[di][dj].type = CC_PIECE_ADVISOR;
                                else if (dpiece == 12) g_draw_board[di][dj].type = CC_PIECE_KING;
                                else if (dpiece == 13) g_draw_board[di][dj].type = CC_PIECE_CANNON;
                                else if (dpiece == 14) g_draw_board[di][dj].type = CC_PIECE_PAWN;
                                else { g_draw_board[di][dj].type = CC_PIECE_EMPTY; g_draw_board[di][dj].color = CC_COLOR_NONE; }
                            } else {
                                g_draw_board[di][dj].type = CC_PIECE_EMPTY;
                                g_draw_board[di][dj].color = CC_COLOR_NONE;
                            }
                        }
                    }

                    /* 先设置位置标记全局变量 */
                    g_record_red_from_col = last_red_from_col;
                    g_record_red_from_row = last_red_from_row;
                    g_record_red_to_col = last_red_to_col;
                    g_record_red_to_row = last_red_to_row;
                    g_record_black_from_col = last_black_from_col;
                    g_record_black_from_row = last_black_from_row;
                    g_record_black_to_col = last_black_to_col;
                    g_record_black_to_row = last_black_to_row;
                    /* 调用记录函数 */
                    ai_accept = should_ai_accept_draw();
                    if (ai_accept) {
                        winner = 0; /* 平局 */
                        SET_GAME_OVER(1);
                        DPRINTF("[暂停菜单] 玩家选择求和，AI 同意和棋\n");
                    } else {
                        DPRINTF("[暂停菜单] 玩家选择求和，AI 拒绝和棋\n");
                        /* 随机选择一条拒绝消息 */
                        refuse_idx = rand() % 5;
                        strcpy(refuse_msg, g_refuse_msgs[refuse_idx]);
                        /* 显示提示消息 */
                        Block(70, 78, 250, 160, 0);
                        Rectangle(70, 78, 250, 160, 1);
                        TextOut(92, 111, refuse_msg, 1);
                        Refresh();
                        Delay(1500);
                    }
                }
                else if (pause_action == 2)
                {
                    /* 认输 */
                    winner = 2; /* 黑方赢 */
                    SET_GAME_OVER(1);
                    DPRINTF("[暂停菜单] 玩家选择认输\n");
                }
                else if (pause_action == 3)
                {
                    /* 悔棋 (undo) - 撤销玩家和AI的上一步 */
                    int undo_result;
                    DPRINTF("[暂停菜单] 玩家选择悔棋\n");

                    /* 撤销两步（玩家和AI各一步） */
                    undo_result = undo_redo_undo();
                    if (undo_result) {
                        /* 再撤销一步（AI的走棋） */
                        undo_redo_undo();
                        DPRINTF("[暂停菜单] 悔棋成功\n");
                    } else {
                        DPRINTF("[暂停菜单] 无法悔棋\n");
                    }
                }
                else if (pause_action == 4)
                {
                    /* 恢复 (redo) - 恢复被撤销的走棋 */
                    int redo_result;
                    DPRINTF("[暂停菜单] 玩家选择恢复\n");

                    /* 恢复两步（AI和玩家各一步） */
                    redo_result = undo_redo_redo();
                    if (redo_result) {
                        /* 再恢复一步（玩家的走棋） */
                        undo_redo_redo();
                        DPRINTF("[暂停菜单] 恢复成功\n");
                    } else {
                        DPRINTF("[暂停菜单] 无法恢复\n");
                    }
                }
                else
                {
                    DPRINTF("[暂停菜单] 取消\n");
                }

                /* 重绘棋盘 */
                draw_qipan();
                draw_qizi_all();
                Refresh();
            }
            else if (key == 0x1B)
            { // B button to exit
                break;
            }
            else {
                DPRINTF("输入无效%d\n", key);
            }

            // Redraw board after player input
            draw_qipan();
            draw_qizi_all();
            Refresh();
        }
        else
        {
            // AI (Black) turn
            Refresh();
            Delay(500); /* 500ms 延迟用于视觉反馈 */

            /* 记录 AI 思考开始 */
            GetTime(time);
            DPRINTF("\n[%02d:%02d:%02d] ========== [黑方回合] ==========\n", time[4], time[5], time[6]);

            /* ==================== 开局库检查 ==================== */
            /* 开局库只在前 18 步有效 */
            /* xqwlight only: skip opening book */
            if (0)
            {
                int obx, oby, obtx, obty;

                obx = 0;
                oby = 0;
                obtx = 0;
                obty = 0;
                if (0)
                {
                    DPRINTF("[开局库] 获取走法: (%d,%d) -> (%d,%d)\n", obx, oby, obtx, obty);

                    /* 验证走法是否合法 */
                    if (can_move(obx, oby, obtx, obty))
                    {
                        DPRINTF("[开局库] 使用开局库走法\n");

                        /* 直接设置 AI 走法结果 */
                        ai_from_col = obx;
                        ai_from_row = oby;
                        ai_to_col = obtx;
                        ai_to_row = obty;

                        /* 跳过 AI 搜索，直接执行走法 */
                        goto execute_ai_move;
                    }
                    else
                    {
                        DPRINTF("[开局库] 走法不合法，继续搜索\n");
                    }
                }
                else
                {
                    DPRINTF("[开局库] 未找到开局走法\n");
                }
            }
            else
            {
                DPRINTF("[AI] legacy opening logic disabled\n");
            }

            /* cchess-fc AI - 设置超时时间 */
            DPRINTF("[AI] DEBUG: about to call ai_cchess_think_and_move(0)\n");
            DPRINTF("[AI] DEBUG: g_current_difficulty=%d\n", g_current_difficulty);
            DPRINTF("[AI] DEBUG: calling ai_cchess_think_and_move...\n");
            ai_cchess_set_timeout(get_ai_timeout_by_difficulty(g_current_difficulty));
            //ai算法移植于  ../../xqwlight/c_version/ 这个项目。
            ai_cchess_think_and_move(0);
            DPRINTF("[AI] ai_cchess_think_and_move returned\n");

            // 获取 AI 走法结果 (row, col 格式);
            ai_from_row = ai_cchess_get_from_row();
            ai_from_col = ai_cchess_get_from_col();
            ai_to_row = ai_cchess_get_to_row();
            ai_to_col = ai_cchess_get_to_col();

        execute_ai_move:
            // 保存黑方光标位置（走棋前）
            black_cur_col = ai_from_col;
            black_cur_row = ai_from_row;

            // 设置光标到 AI 走棋的起始位置（显示用）
            cur_x = ai_from_col;
            cur_y = ai_from_row;

            // 显示 AI 走法结果 (标准象棋谱格式)
            {
                char ai_move_notation[32];
                struct Move ai_move1;
                GetTime(time);
                if (ai_from_col >= 0)
                {
                    ai_move1.from_row = ai_from_row;
                    ai_move1.from_col = ai_from_col;
                    ai_move1.to_row = ai_to_row;
                    ai_move1.to_col = ai_to_col;
                    move_to_chinese_notation(&ai_move1, chess_map, ai_move_notation);
                    sprintf(debug_str, "AI:%s", ai_move_notation);
                    DPRINTF("[%02d:%02d:%02d] [AI] 结果：%s\n", time[4], time[5], time[6], ai_move_notation);
                }
                else
                {
                    sprintf(debug_str, "AI:NO_MOVE");
                    DPRINTF("[%02d:%02d:%02d] [AI] 无棋可走\n", time[4], time[5], time[6]);
                }
            }
            Refresh();

            // 执行 AI 走法（先验证合法性）
            if (ai_from_col >= 0)
            {
                ai_piece = chess_map[ai_from_row][ai_from_col];
                if (ai_piece == 1)
                    sprintf(ai_piece_name, "红车");
                else if (ai_piece == 2)
                    sprintf(ai_piece_name, "红马");
                else if (ai_piece == 3)
                    sprintf(ai_piece_name, "红相");
                else if (ai_piece == 4)
                    sprintf(ai_piece_name, "红士");
                else if (ai_piece == 5)
                    sprintf(ai_piece_name, "红帅");
                else if (ai_piece == 6)
                    sprintf(ai_piece_name, "红炮");
                else if (ai_piece == 7)
                    sprintf(ai_piece_name, "红兵");
                else if (ai_piece == 8)
                    sprintf(ai_piece_name, "黑车");
                else if (ai_piece == 9)
                    sprintf(ai_piece_name, "黑马");
                else if (ai_piece == 10)
                    sprintf(ai_piece_name, "黑象");
                else if (ai_piece == 11)
                    sprintf(ai_piece_name, "黑士");
                else if (ai_piece == 12)
                    sprintf(ai_piece_name, "黑将");
                else if (ai_piece == 13)
                    sprintf(ai_piece_name, "黑炮");
                else if (ai_piece == 14)
                    sprintf(ai_piece_name, "黑卒");
                else
                    sprintf(ai_piece_name, "空位");

                // 调试：打印 AI 走法（标准象棋谱格式）
                {
                    char ai_move_notation2[32];
                    struct Move ai_move2;
                    ai_move2.from_row = ai_from_row;
                    ai_move2.from_col = ai_from_col;
                    ai_move2.to_row = ai_to_row;
                    ai_move2.to_col = ai_to_col;
                    move_to_chinese_notation(&ai_move2, chess_map, ai_move_notation2);
                    GetTime(time);
                    DPRINTF("[%02d:%02d:%02d] [AI] 验证走法：%s, 棋子=%s\n", time[4], time[5], time[6], ai_move_notation2, ai_piece_name);
                }

                /* 验证 AI 走法是否合法（参数顺序：col1, row1, col2, row2） */
                if (is_move_legal_with_self_check(ai_from_col, ai_from_row, ai_to_col, ai_to_row) != 0)
                {
                    char final_ai_move_notation[32];
                    struct Move final_ai_move;

                    /* 记录到历史 */
                    log_move(ai_from_col, ai_from_row, ai_to_col, ai_to_row, 2, chess_map[ai_from_row][ai_from_col]);

                    final_ai_move.from_row = ai_from_row;
                    final_ai_move.from_col = ai_from_col;
                    final_ai_move.to_row = ai_to_row;
                    final_ai_move.to_col = ai_to_col;
                    move_to_chinese_notation(&final_ai_move, chess_map, final_ai_move_notation);
                    GetTime(time);
                    DPRINTF("[%02d:%02d:%02d] [chess notation] 黑方=%s\n", time[4], time[5], time[6], final_ai_move_notation);

                    captured_piece = chess_map[ai_to_row][ai_to_col];
                    chess_map[ai_to_row][ai_to_col] = chess_map[ai_from_row][ai_from_col];
                    chess_map[ai_from_row][ai_from_col] = 0;

                    can_move_debug = 0;
                    winner = check_game_over();
                    can_move_debug = 1;
                    if (winner != 0) {
                        DEBUG_LOG("游戏结束：黑方获胜");
                        SET_GAME_OVER(1);
                    }

                    /* 显示执行成功 */
                    debug_str[0] = 'O';
                    debug_str[1] = 'K';
                    debug_str[2] = 0;
                    // 记录成功
                    GetTime(time);
                    DPRINTF("[%02d:%02d:%02d] [AI] 走法接受\n", time[4], time[5], time[6]);

                    // 保存黑方最后走棋位置
                    last_black_from_col = ai_from_col;
                    last_black_from_row = ai_from_row;
                    last_black_to_col = ai_to_col;
                    last_black_to_row = ai_to_row;

                    /* 记录悔棋历史 */
                    g_record_red_from_col = last_red_from_col;
                    g_record_red_from_row = last_red_from_row;
                    g_record_red_to_col = last_red_to_col;
                    g_record_red_to_row = last_red_to_row;
                    g_record_black_from_col = last_black_from_col;
                    g_record_black_from_row = last_black_from_row;
                    g_record_black_to_col = last_black_to_col;
                    g_record_black_to_row = last_black_to_row;
                    undo_redo_record_move(ai_from_row, ai_from_col, ai_to_row, ai_to_col, captured_piece);

                    // 设置光标到 AI 走棋的目标位置
                    cur_x = ai_to_col;
                    cur_y = ai_to_row;

                    // 注意：不更新 black_cur_col/y，保持黑方光标位置独立
                    // black_cur_col/y 只在玩家手动移动光标时更新

                    // 恢复红方光标位置
                    cur_x = red_cur_col;
                    cur_y = red_cur_row;
                }
                else
                {
                    // 显示走法非法
                    debug_str[0] = 'I';
                    debug_str[1] = 'N';
                    debug_str[2] = 'V';
                    debug_str[3] = 'A';
                    debug_str[4] = 'L';
                    debug_str[5] = 'I';
                    debug_str[6] = 'D';
                    debug_str[7] = 0;
                    GetTime(time);
                    DPRINTF("[%02d:%02d:%02d] [AI] 走法拒绝 (非法走法)\n", time[4], time[5], time[6]);
                }
            }
            else
            {
                // AI无棋可走，被将杀或困毙，红方胜
                GetTime(time);
                DPRINTF("[%02d:%02d:%02d] [AI] 无棋可走！黑方被判负\n", time[4], time[5], time[6]);
                winner = 1;
                SET_GAME_OVER(1);
            }
            Refresh();

            // Check game over status (仅在未判定时检查，避免覆盖已有结果)
            if (game_over == 0)
            {
                can_move_debug = 0;
                winner = check_game_over();
                can_move_debug = 1;
                if (winner != 0) {
                    DEBUG_LOG("游戏结束：红方获胜");
                    SET_GAME_OVER(1);
                }
            }
            // Redraw board after AI move
            draw_qipan();
            draw_qizi_all();
            Refresh();
            current_turn = 1; // Switch back to Red player turn
        }

        Refresh();
    }
}

// 包含单元测试模块
int str_equals(char str1[], char str2[]);
int str_starts_with(char str[], char prefix[]);
#include "test_unit.c"

// 检查字符串是否相等（简单版本）
int str_equals(char str1[], char str2[])
{
    int i;
    i = 0;
    while (str1[i] != 0 && str2[i] != 0)
    {
        if (str1[i] != str2[i])
            return 0;
        i = i + 1;
    }
    return (str1[i] == str2[i]) ? 1 : 0;
}

// 检查字符串是否以指定前缀开头
int str_starts_with(char str[], char prefix[])
{
    int i;
    i = 0;
    while (prefix[i] != 0)
    {
        if (str[i] != prefix[i])
            return 0;
        i = i + 1;
    }
    return 1;
}

int main(int argc, char *argv[])
{
    int fp;
    char cmdline[256];
    char test_func_name[64];
    int i, j;

#if ENABLE_CMDLINE_PARSE
    /* 获取命令行参数 */
    GetCommandLine(cmdline);

    // 调试：写入收到的参数
    fp = fopen("/LavaData/debug_cmdline.log", "w");
    if (fp != 0)
    {
        fwrite("CMDLINE: ", 1, 9, fp);
        fwrite(cmdline, 1, strlen(cmdline), fp);
        fwrite("\n", 1, 1, fp);
        fclose(fp);
    }

    // 解析测试函数名（格式: lvm2 -v cchess.lav test_func）
    // 查找最后一个空格后的内容
    i = 0;
    j = 0;
    while (cmdline[i] != 0)
    {
        if (cmdline[i] == ' ')
        {
            j = i + 1; // 记录空格后的位置
        }
        i = i + 1;
    }
#endif

#if ENABLE_CMDLINE_PARSE
    // 提取最后一个参数作为测试函数名
    if (j > 0 && cmdline[j] != 0)
    {
        // 如果最后一个参数是路径，提取文件名部分
        i = j;
        while (cmdline[i] != 0)
        {
            i = i + 1;
        }
        // 从末尾往前找最后一个 / 或
        i = i - 1;
        while (i >= j && cmdline[i] != '/' && cmdline[i] != '\\')
        {
            i = i - 1;
        }
        // i+1 是文件名开始的位置
        j = i + 1;

        // 提取文件名（去掉路径）
        i = 0;
        while (cmdline[j + i] != 0 && cmdline[j + i] != ' ' && cmdline[j + i] != '.' && i < 60)
        {
            test_func_name[i] = cmdline[j + i];
            i = i + 1;
        }
        test_func_name[i] = 0;

        // 调试：写入解析结果
        fp = fopen("/LavaData/debug_cmdline.log", "a");
        if (fp != 0)
        {
            fwrite("FUNC_NAME: ", 1, 11, fp);
            fwrite(test_func_name, 1, strlen(test_func_name), fp);
            fwrite("\n", 1, 1, fp);
            fclose(fp);
        }

        // 检查是否是测试函数调用
        if (str_starts_with(test_func_name, "test_") != 0)
        {
            runtest(test_func_name);
            return; /* 测试完成后直接返回 */
        }
    }
#endif

    /* 初始化位置加权表 */
    cc_init_position_bonus();

    /* 初始化棋子 */
    /* 兵的位置：(1,4), (3,4), (5,4), (7,4), (9,4) */

    /* 初始化走棋历史 */

    game_loop();
    return 0;
}
