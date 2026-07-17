/**
 * @file xiangqi.h
 * @brief 中国象棋核心数据结构和常量定义（统一头文件）
 * @author QWen 3.5 千问大模型
 * @date 2026
 *
 * 项目：中国象棋 (FC Chess)
 * 描述：基于 SDL2 的中国象棋游戏，包含 AI 对战、棋谱加载/导出等功能
 *
 * 本文件包含：
 * - 核心数据结构和常量
 * - GUI 相关定义
 * - AI 相关函数声明
 * - 对话框相关定义
 * - 走法解析器相关定义
 * - 工具宏定义
 */

#ifndef XIANGQI_H
#define XIANGQI_H

/* ==================== 编译参数控制 ==================== */
/* 是否启用命令行参数解析（用于运行测试） */
/* 改为 1 启用，0 禁用（默认） */

#define ENABLE_CMDLINE_PARSE 0

/* ==================== 调试开关 ==================== */
/* 定义 ENABLE_DEBUG_LOG 时输出 printf 日志，注释掉则关闭 */

#if defined(__LAVA__) || defined(LAVA_ESP32)
    /* LavaX 如果用printf，满屏信息乱飞，禁用调试日志 */
    #undef ENABLE_DEBUG_LOG
#else
    /* 非 LavaX 环境，启用调试日志 */
    #define ENABLE_DEBUG_LOG
#endif

int can_move_debug = 0; /* 是否输出 can_move 调试日志 */
#ifdef ENABLE_DEBUG_LOG
    #define DPRINTF printf
    #define DEBUG_LOG printf
#else
    #define DPRINTF(...) ((void)0)
    #define DEBUG_LOG(...) ((void)0)
#endif

/* ==================== 基础类型定义 ==================== */
#define bool int
#define true 1
#define false 0

/* ==================== 棋盘和搜索常量 ==================== */
#define BOARD_ROWS 10   /* 棋盘行数（中国象棋棋盘为 10 行） */
#define BOARD_COLS 9    /* 棋盘列数（中国象棋棋盘为 9 列） */
#define MAX_MOVES 200   /* 单步搜索中最大生成走法数 */
#define MAX_DEPTH 5     /* 最大搜索深度（根据 FC 硬件限制，最大 5 层） */
#define MAX_HISTORY 200 /* 最大走棋历史记录数（用于悔棋功能） */

/* ==================== 杀手走法和历史启发相关常量 ==================== */
#define KILLER_MOVES_PER_DEPTH 2           /* 每层搜索存储的杀手走法数量（2 个） */
#define HISTORY_TABLE_SIZE 14 * 9 * 10 * 9 /* 历史启发表大小（棋子类型*起始位置*目标位置） */

/* ==================== 棋子类型枚举 ==================== */
#define CC_PIECE_EMPTY 0  /* 空位（无棋子） */
#define CC_PIECE_KING 1   /* 将/帅 */
#define CC_PIECE_ROOK 2   /* 车 */
#define CC_PIECE_CANNON 3 /* 炮 */
#define CC_PIECE_KNIGHT 4 /* 马 */
#define CC_PIECE_ELEPHANT 5 /* 象/相 */
#define CC_PIECE_ADVISOR 6 /* 士/仕 */
#define CC_PIECE_PAWN 7   /* 卒/兵 */

/* ==================== 棋子颜色枚举 ==================== */
#define CC_COLOR_NONE -1 /* 无颜色（无效值） */
#define CC_COLOR_RED 0   /* 红方 */
#define CC_COLOR_BLACK 1 /* 黑方 */

/* ==================== 棋子结构体 ==================== */
struct Piece {
    int type; /* 棋子类型（车马炮等） */
    int color;    /* 棋子颜色（红方/黑方） */
};

/* ==================== 走法结构体 ==================== */
struct Move {
    int from_row, from_col; /* 起始位置的行、列坐标 */
    int to_row, to_col;     /* 目标位置的行、列坐标 */
};

/* ==================== 悔棋记录结构体（包含吃子信息） ==================== */
struct MoveRecord {
    struct Move move;            /* 走法信息 */
    struct Piece captured_piece; /* 被吃掉的棋子（用于悔棋时恢复） */
};

/* ==================== 位置历史记录常量 ==================== */
#define MAX_POSITION_HISTORY 1000 /* 最大位置历史记录数（用于检测长将/长捉） */

/* ==================== 位置记录结构体 ==================== */
struct PositionRecord {
    long hash;     /* 棋盘位置的哈希值 */
    bool is_check; /* 是否将军状态 */
    bool is_chase; /* 是否捉子状态 */
    struct Move move;     /* 导致该位置的走法 */
};

/* ==================== 游戏状态结构体 ==================== */
struct GameState {
    struct Piece board[10][9];                                    /* 当前棋盘状态 */
    int current_player;                                  /* 当前走棋方 */
    bool in_check;                                         /* 当前是否处于被将军状态 */
    int move_count;                                        /* 总走棋回合数 */
    bool game_over;                                        /* 游戏是否结束 */
    bool has_last_move;                                    /* 是否有最后一步走法 */
    struct Move last_move;                                        /* 最后一步走法 */
    bool has_prev_move;                                    /* 是否有上一步走法（用于显示双方走棋标记） */
    struct Move prev_move;                                        /* 上一步走法 */
    struct MoveRecord move_history[MAX_HISTORY];                  /* 走棋历史记录数组（包含吃子信息） */
    int history_count;                                     /* 历史记录数量 */
    int current_history_index;                             /* 当前悔棋位置索引（支持向前/向后悔棋） */
    struct PositionRecord position_history[MAX_POSITION_HISTORY]; /* 位置历史记录数组 */
    int position_history_count;                            /* 位置历史记录数量 */
};

/* ==================== 杀手走法结构体 ==================== */
struct KillerMove {
    struct Move moves[KILLER_MOVES_PER_DEPTH]; /* 杀手走法数组 */
    int count;                          /* 杀手走法数量 */
};

/* ==================== 历史启发表条目结构体 ==================== */
struct HistoryEntry {
    int from_row, from_col, to_row, to_col; /* 走法坐标 */
    int value;                              /* 历史启发值 */
};

/* ==================== GUI 相关定义 ==================== */

// GUI 窗口尺寸定义
#include "screen_config.h"
#define WINDOW_WIDTH LAVA_CCH_SCREEN_WIDTH
#define WINDOW_HEIGHT LAVA_CCH_SCREEN_HEIGHT
#define BOARD_OFFSET_X 110
#define BOARD_OFFSET_Y 51
#define CELL_SIZE_X 63
#define CELL_SIZE_Y 50
#define PIECE_RADIUS 25
#define GUI_ENABLED

// 难度选择状态
#define DIFFICULTY_MENU_ACTIVE 0
#define DIFFICULTY_MENU_HIDDEN 1

// 难度级别
#define DIFFICULTY_BEGINNER 1
#define DIFFICULTY_INTERMEDIATE 2
#define DIFFICULTY_ADVANCED 3

// GUI 状态结构体
struct GuiState {
    long game_state; // GameState* 改为 long
    int selected_row, selected_col;
    bool has_selection;
    bool ai_thinking;
    int ai_timeout_seconds;
    int ai_think_start_time;
    int menu_state;
    int selected_difficulty;
    int hovered_difficulty;
    long dialog_state; // DialogState* 改为 long
    bool quit_flag;
    char debug_lines[20][256];
    int debug_line_count;
    bool undo_hovered;
    bool redo_hovered;
    bool draw_hovered;
    bool review_hovered;
    bool load_pgn_hovered;
    bool surrender_hovered;
    bool quit_hovered;
    bool first_hovered;
    bool last_hovered;
    bool play_hovered;
    bool debug_hovered;
    bool start_game_hovered;
    bool is_playing;
    int play_current_step;
    int play_last_move_time;
    bool debug_menu_visible;
    int debug_menu_selected;
};

/* 定义棋盘类型 */
#define Board struct Piece[BOARD_ROWS][BOARD_COLS]

/* ==================== 棋子价值表（外部定义） ==================== */
// extern int PIECE_VALUES_PY[];    /* Python 版本棋子价值表 */
// extern int PIECE_VALUES_FC[]; /* FC 版本棋子价值表 */

/* ==================== 棋子中文名称（外部定义） ==================== */
// extern char *PIECE_NAMES[]; /* 棋子中文名称数组 */

/* ==================== 位置加成表（外部定义） ==================== */
// extern int POSITION_BONUS[BOARD_ROWS][BOARD_COLS]; /* 各位置的战略价值加成表 */

/* ==================== 基础走法验证函数 ==================== */
/**
 * @brief 获取对方的颜色
 * @param color 当前颜色
 * @return 对方颜色
 */
int get_opponent(int color);

/**
 * @brief 检查坐标是否在九宫格内
 * @param row 行坐标
 * @param col 列坐标
 * @param color 所属方颜色
 * @return 是否在九宫格内
 */
bool is_in_palace(int row, int col, int color);

/**
 * @brief 检查指定颜色的棋子是否能攻击到目标位置
 * @param board 棋盘
 * @param row 目标行坐标
 * @param col 目标列坐标
 * @param attacker_color 攻击方颜色
 * @return 是否能攻击到
 */
bool can_attack_square(struct Piece *board, int row, int col, int attacker_color);

/* ==================== 局面评估函数 ==================== */
/**
 * @brief 评估当前局面的分数
 * @param board 棋盘
 * @param color 评估方颜色
 * @return 局面评估分数
 */
int evaluate_position(struct Piece *board, int color);

/**
 * @brief 初始化位置加成表
 */
void initialize_position_bonus();

/**
 * @brief 获取棋子的字符表示
 * @param piece 棋子
 * @return 棋子字符
 */
char *get_piece_char(struct Piece *piece);

/* ==================== 将军和威胁检测函数 ==================== */
/**
 * @brief 检查指定颜色是否被将军
 * @param board 棋盘
 * @param color 检查方颜色
 * @return 是否被将军
 */
bool is_in_check(struct Piece *board, int color);

/**
 * @brief 检查是否有棋子被对方威胁
 * @param board 棋盘
 * @param color 检查方颜色
 * @return 是否有棋子被威胁
 */
bool has_piece_under_threat(struct Piece *board, int color);

/**
 * @brief 检查是否有吃子机会
 * @param board 棋盘
 * @param color 检查方颜色
 * @return 是否有吃子机会
 */
bool has_capture_opportunity(struct Piece *board, int color);

/**
 * @brief 检查是否有将军机会
 * @param board 棋盘
 * @param color 检查方颜色
 * @return 是否有将军机会
 */
bool has_check_opportunity(struct Piece *board, int color);

/* ==================== 特殊规则检测函数 ==================== */
/**
 * @brief 检查双方将帅是否对面（飞将）
 * @param board 棋盘
 * @return 是否对面
 */
bool is_facing_kings(struct Piece *board);

/**
 * @brief 查找指定颜色的将/帅位置
 * @param board 棋盘
 * @param color 棋子颜色
 * @param row 输出：将/帅行坐标
 * @param col 输出：将/帅列坐标
 * @return 是否找到
 */
bool find_king(struct Piece *board, int color, int *row, int *col);

/**
 * @brief 检查走法是否符合基本规则（不考虑将军）
 * @param board 棋盘
 * @param from_row 起始行
 * @param from_col 起始列
 * @param to_row 目标行
 * @param to_col 目标列
 * @return 是否是合法走法
 */
bool is_valid_move_basic(struct Piece *board, int from_row, int from_col, int to_row, int to_col);

/* ==================== 走法生成和执行函数 ==================== */
/**
 * @brief 生成指定颜色的所有合法走法
 * @param board 棋盘
 * @param color 走棋方颜色
 * @param moves 输出：走法数组
 * @param max_moves 最大走法数
 * @return 生成的走法数量
 */
int generate_moves(struct Piece *board, int color, struct Move *moves, int max_moves);

/**
 * @brief 检查走法是否会导致捉子
 * @param board 棋盘
 * @param move 走法
 * @param color 走棋方颜色
 * @return 是否会导致捉子
 */
bool move_causes_chase(struct Piece *board, struct Move *move, int color);

/**
 * @brief 执行走法（修改棋盘）
 * @param board 棋盘
 * @param from_row 起始行
 * @param from_col 起始列
 * @param to_row 目标行
 * @param to_col 目标列
 * @return 是否成功执行
 */
bool make_move(struct Piece *board[10][9], int from_row, int from_col, int to_row, int to_col);

/* ==================== 走法记谱函数 ==================== */
/**
 * @brief 将走法转换为代数记谱法
 * @param move 走法
 * @param buffer 输出缓冲区
 * @return 记谱字符串
 */
char *move_to_notation(struct Move *move, char *buffer);

/**
 * @brief 将走法转换为中文记谱法（标准棋谱记法）
 * @param move 走法
 * @param board 棋盘
 * @param buffer 输出缓冲区
 * @return 中文记谱字符串
 */
char *move_to_chinese_notation(struct Move *move, char board[12][11], char buffer[]);

/**
 * @brief 检查走法是否完全合法（考虑将军）
 * @param board 棋盘
 * @param from_row 起始行
 * @param from_col 起始列
 * @param to_row 目标行
 * @param to_col 目标列
 * @return 是否是合法走法
 */
bool is_valid_move(struct Piece *board, int from_row, int from_col, int to_row, int to_col);

/* ==================== 棋盘初始化函数 ==================== */
/**
 * @brief 初始化棋盘为开局状态
 * @param board 棋盘
 */
void init_board(struct Piece *board[10][9]);

/**
 * @brief 打印棋盘到控制台（调试用）
 * @param board 棋盘
 */
void print_board(struct Piece *board);

/* ==================== 棋盘哈希函数 ==================== */
/**
 * @brief 计算当前棋盘的哈希值
 * @param board 棋盘
 * @return 哈希值
 */
long compute_board_hash(struct Piece *board);

/* ==================== 游戏状态管理函数 ==================== */
/**
 * @brief 初始化游戏状态
 * @param state 游戏状态
 */
void init_game_state(struct GameState *state);

/**
 * @brief 初始化位置历史记录
 * @param state 游戏状态
 */
void init_position_history(struct GameState *state);

/**
 * @brief 检查坐标是否是有效位置
 * @param row 行坐标
 * @param col 列坐标
 * @return 是否有效
 */
bool is_valid_position(int row, int col);

/**
 * @brief 记录走法到历史记录
 * @param state 游戏状态
 * @param move 走法
 */
void record_move_to_history(struct GameState *state, struct Move *move);

/* ==================== 长将长捉检测函数 ==================== */
/**
 * @brief 检查是否长将（连续将军）
 * @param state 游戏状态
 * @return 是否长将
 */
bool is_perpetual_check(struct GameState *state);

/**
 * @brief 检查是否长捉（连续捉子）
 * @param state 游戏状态
 * @return 是否长捉
 */
bool is_perpetual_chase(struct GameState *state);

/* ==================== 走法历史记录函数 ==================== */
/**
 * @brief 添加走法到历史记录
 * @param move 走法
 */
void add_to_move_history(struct Move *move);

/**
 * @brief 重置搜索历史（调试用）
 */
void reset_search_history(void);

/* ==================== AI 走法函数 ==================== */
/**
 * @brief AI 计算并返回最佳走法
 * @param state 游戏状态
 * @param max_depth 最大搜索深度
 * @param timeout_seconds 超时时间（秒）
 * @return AI 选择的走法
 * 注：LavaX 编译器不支持函数返回结构体，此函数已禁用
 */
/* struct Move ai_move(struct GameState *state, int max_depth, int timeout_seconds); */

/* ==================== 求和判断函数 ==================== */
/**
 * @brief 判断 AI 是否应该接受求和
 * @return 是否接受求和
 * @note 使用前需先将棋盘数据复制到 g_draw_board 全局变量
 */
bool should_ai_accept_draw();

/* ==================== 和棋决策全局变量 ==================== */
/**
 * @brief 用于和棋决策的棋盘数据
 * @note LavaX 不支持结构体参数，使用全局变量传递棋盘
 */
// extern struct Piece g_draw_board[10][9];  /* 在 ai_draw_decision.c 中定义 */

/* ==================== 悔棋功能函数 ==================== */
/**
 * @brief 撤销一步走法
 * @param state 游戏状态
 */
void undo_move_game(struct GameState *state);

/**
 * @brief 恢复一步走法（向前）
 * @param state 游戏状态
 */
void redo_move_game(struct GameState *state);

/* ==================== 历史记录长度重置 ==================== */
/**
 * @brief 重置历史记录长度
 */
void reset_history_length(void);

/* ==================== 杀手走法和历史启发管理函数 ==================== */
/**
 * @brief 初始化杀手走法表
 */
void init_killer_moves(void);

/**
 * @brief 初始化历史启发表
 */
void init_history_table(void);

/**
 * @brief 添加杀手走法到指定深度
 * @param move 走法
 * @param depth 搜索深度
 */
void add_killer_move(struct Move *move, int depth);

/**
 * @brief 更新历史启发表
 * @param move 走法
 * @param depth 搜索深度
 * @param bonus 奖励值
 */
void update_history_table(struct Move *move, int depth, int bonus);

/**
 * @brief 获取走法的历史启发值
 * @param move 走法
 * @return 历史启发值
 */
int get_history_value(struct Move *move);

/**
 * @brief 获取指定深度的杀手走法
 * @param depth 搜索深度
 * @return 杀手走法指针
 */
// struct KillerMove *get_killer_moves_at_depth(int depth);

/* ==================== SEE 静态交换评估 ==================== */
/**
 * @brief SEE（静态交换评估）计算走法的得失
 * @param board 棋盘
 * @param move 走法
 * @param color 走棋方颜色
 * @return 评估分数
 */
int see_evaluate(struct Piece *board, struct Move *move, int color);

/* ==================== 动态棋子价值评估 ==================== */
/**
 * @brief 根据开局阶段动态获取棋子价值
 * @param type 棋子类型
 * @param move_count 当前回合数
 * @return 棋子动态价值
 */
// int get_piece_value_dynamic(int type, int move_count);

/* ==================== GUI 函数声明 ==================== */
void render_gui(struct GuiState *gui);
void handle_mouse_click(struct GuiState *gui, int mouse_x, int mouse_y);
void handle_right_click_remove_piece(struct GuiState *gui, int mouse_x, int mouse_y);
void handle_ai_turn(struct GuiState *gui);
void add_debug_message(struct GuiState *gui, char *message);
void handle_difficulty_menu_mouse_event(struct GuiState *gui, int mouse_x, int mouse_y);

/* ==================== AI 相关函数声明 ==================== */
// 开局库函数
long positionBookMoveSimple();

// AI 初始化
void ai_init_structures();
void ai_copy_board();
void ai_generate_moves(int color);
int ai_make_move(int from_x, int from_y, int to_x, int to_y);
void ai_undo_move(int from_x, int from_y, int to_x, int to_y, int captured);
void ai_get_best_move(int color, int depth);
int ai_evaluate_board(int color);
int ai_alpha_beta(int depth, int color, int alpha, int beta, int is_maximizing);
void ai_think_and_move(int is_red);
void do_move(int from_x, int from_y, int to_x, int to_y);

// cchess-fc AI 模块
void ai_cchess_init();
void ai_cchess_set_timeout(int timeout_seconds);
void ai_cchess_think_and_move(int is_red);
void ai_copy_board_from_chess_map();
void ai_generate_all_moves(int color);
int ai_evaluate_position(int color);
int ai_alpha_beta_search(int depth, int color, int alpha, int beta, int is_maximizing);
int ai_make_move_internal(int from_x, int from_y, int to_x, int to_y);
void ai_undo_move_internal(int from_x, int from_y, int to_x, int to_y, int captured);
int ai_is_valid_move(int from_x, int from_y, int to_x, int to_y, int color);
int ai_is_in_check(int color);

/* ==================== 对话框函数声明 ==================== */
/**
 * @brief 绘制游戏结束对话框
 * @param winner 获胜方：1=红方，2=黑方
 */
void draw_game_over_dialog(int winner);

/**
 * @brief 设置游戏结束标记（方便调试追踪）
 * @param state 游戏状态指针
 * @param value 设置的值
 */
void set_game_over(long state, int value);

/* ==================== 走法解析器函数声明 ==================== */
/**
 * @brief 解析并执行中文象棋走法字符串
 * @param gui GUI 状态指针
 * @param move_str 中文走法字符串，如"车九平八"
 * @param color 当前走棋方颜色
 * @return 执行成功返回 true，失败返回 false
 */
bool parse_and_execute_move(long gui, char *move_str, int color);

/**
 * @brief 解析并执行中文象棋走法字符串（适配 GameState 版本）
 * @param state 游戏状态指针
 * @param move_str 中文走法字符串，如"车九平八"
 * @param color 当前走棋方颜色
 * @return 执行成功返回 true，失败返回 false
 */
bool parse_and_execute_move_gamestate(long state, char *move_str, int color);

/* ==================== GUI 辅助函数声明 ==================== */
/* 注意：ClearScreen, Rectangle, TextOut, Block, Refresh 是 LavaX 内置函数，无需声明 */

/* ==================== 初始化函数声明 ==================== */
/**
 * @brief 初始化中文名称数组
 */
void init_chinese_names();

#endif // XIANGQI_H
