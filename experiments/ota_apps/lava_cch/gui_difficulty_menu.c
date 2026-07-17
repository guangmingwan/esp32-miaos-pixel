/**
 * @file gui_difficulty_menu.c
 * @brief 难度选择菜单 GUI 模块（美化版）
 * @author QWen
 * @date 2026
 *
 * 功能：提供难度选择界面，支持三级难度选择
 * - 初级：AI 思考 5 秒
 * - 中级：AI 思考 15 秒
 * - 高级：AI 思考 45 秒
 * 美化：简约几何装饰（圆环、装饰线、三角箭头、分隔线）
 * 注意：LavaX 的 Line() 默认画到屏幕而非缓冲区，Refresh() 时会被覆盖
 *       所以装饰线用 Block(高1像素) 代替 Line，确保画到缓冲区
 */

/* ==================== 常量定义 ==================== */
/* 注意：WINDOW_WIDTH, WINDOW_HEIGHT 已在 xiangqi.h 中定义 */

/* 难度级别定义 */
#define DIFFICULTY_EASY   1    /* 初级：AI 思考 5 秒 */
#define DIFFICULTY_MEDIUM 2    /* 中级：AI 思考 15 秒 */
#define DIFFICULTY_HARD   3    /* 高级：AI 思考 45 秒 */

/* 菜单布局常量 */
#define MENU_W       200
#define MENU_H       164
#define MENU_X       ((WINDOW_WIDTH - MENU_W) / 2)
#define MENU_Y       28

/* 选项布局 */
#define OPT_X        (MENU_X + 20)
#define OPT_W        (MENU_W - 40)
#define OPT_H        28
#define OPT_Y(i)     (MENU_Y + 48 + (i) * 34)
#define OPT_TEXT_X   (MENU_X + 58)
#define MARQUEE_STEP 6

/* ==================== 全局变量（供 AI 模块使用） ==================== */
int g_current_difficulty = 0;      /* 当前难度级别 */

/* GUI 状态变量 */
int g_difficulty_selected = 0;     /* 当前选中的难度 (1/2/3) */
int g_difficulty_hovered = 0;      /* 鼠标悬停的难度 (1/2/3) */
int g_difficulty_confirmed = 0;    /* 是否已确认选择 */
int g_marquee_offset = 0;         /* 跑马灯滚动偏移 */

/* ==================== 文本缓冲区（全局数组） ==================== */
char difficulty_text_0[64] = "";
char difficulty_text_1[64] = "";
char difficulty_text_2[64] = "";
char gui_title[64] = "";
char gui_hint[64] = "";
char g_marquee_text[128] = "鸣谢：LEE，诗诺比，fix-eua.dax，Isword，曾半仙，SAILOR-HB，狐の嫁入り，移植者：大湾明仔";
int g_marquee_text_width = 0;

/* ==================== 初始化难度文本 ==================== */
void init_difficulty_texts()
{
    strcpy(difficulty_text_0, "1. 初级");
    strcpy(difficulty_text_1, "2. 中级");
    strcpy(difficulty_text_2, "3. 高级");
    strcpy(gui_title, "选择难度");
    strcpy(gui_hint, "上下选择 确定进入");
}

/* ==================== 获取难度选项文本 ==================== */
char* get_difficulty_text(int index)
{
    if (index == 0)
    {
        return difficulty_text_0;
    }
    if (index == 1)
    {
        return difficulty_text_1;
    }
    if (index == 2)
    {
        return difficulty_text_2;
    }
    return difficulty_text_0;
}

int get_marquee_char_bytes(char* text, int* char_width)
{
    int ch;

    ch = text[0] & 0xFF;
    if (ch == 0)
    {
        *char_width = 0;
        return 0;
    }

#ifdef LAVA_NATIVE_COMPILED
    if ((ch & 0x80) == 0)
    {
        *char_width = 6;
        return 1;
    }
    if ((ch & 0xE0) == 0xC0)
    {
        *char_width = 12;
        return 2;
    }
    if ((ch & 0xF0) == 0xE0)
    {
        *char_width = 12;
        return 3;
    }
    if ((ch & 0xF8) == 0xF0)
    {
        *char_width = 12;
        return 4;
    }
    *char_width = 6;
    return 1;
#else
    if (ch >= 0x80 && text[1] != 0)
    {
        *char_width = 12;
        return 2;
    }
    if (ch >= 0x80)
    {
        *char_width = 12;
        return 1;
    }
    *char_width = 6;
    return 1;
#endif
}

int calc_marquee_text_width()
{
    char* p;
    int total_width;
    int char_width;
    int char_bytes;

    p = g_marquee_text;
    total_width = 0;
    while (*p != 0)
    {
        char_bytes = get_marquee_char_bytes(p, &char_width);
        if (char_bytes <= 0)
        {
            break;
        }
        total_width = total_width + char_width;
        p = p + char_bytes;
    }
    return total_width;
}

/* ==================== 画三角箭头 ==================== */
/* 用 Block 逐行画实心三角 ▶，确保画到缓冲区 */
void draw_arrow(int x, int y)
{
    Block(x, y, x + 2, y + 1, 1);
    Block(x, y + 1, x + 4, y + 2, 1);
    Block(x, y + 2, x + 6, y + 3, 1);
    Block(x, y + 3, x + 8, y + 4, 1);
    Block(x, y + 4, x + 6, y + 5, 1);
    Block(x, y + 5, x + 4, y + 6, 1);
    Block(x, y + 6, x + 2, y + 7, 1);
}

/* ==================== 战马logo位图 (50x25像素) ==================== */
char g_horse_logo[] = {
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0xFF,0xFF,0xF0,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0xFF,0xFF,0xFF,0x0F,0xFF,0xF0,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x00,0xFF,0xFF,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0xFF,0xFF,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xF0,0xF0,0x0F,0x0F,0xF0,0x00,
0x00,0x00,0x00,0x00,0xFF,0xFF,0xF0,0x0F,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x0F,0xFF,0xFF,0xF0,0x0F,0xFF,0x00,0x00,0x00,0x00,
0x0F,0xF0,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x0F,0xFF,0xFF,0xFF,0xFF,0xFF,0xF0,0x0F,0x00,0xFF,0xFF,0x0F,0xF0,
0xFF,0xFF,0xFF,0xFF,0xFF,0xF0,0x0F,0xFF,0xFF,0xF0,0x0F,0xF0,0x00,0xFF,0xFF,0xFF,0x00,0xFF,0xF0,0xF0,0x00,0x00,0x0F,0xF0,0xFF,
0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0xFF,0xFF,0xFF,0x00,0xFF,0xFF,0x00,0x0F,0xFF,0xFF,0x00,0x0F,0x0F,0xF0,0x00,0x00,0x00,0xFF,0xF0,
0x00,0xFF,0xFF,0xFF,0x00,0x00,0xFF,0xFF,0xFF,0xF0,0xFF,0xFF,0xFF,0x00,0x0F,0xF0,0x00,0x00,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x0F,0xFF,0x00,0x00,0x00,0x0F,0xFF,0xFF,0xF0,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0xFF,0xFF,0xFF,0x0F,0xFF,0xFF,0xFF,0x00,0x0F,0xF0,0x00,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0x0F,0xFF,0xFF,0xFF,0x00,0xFF,0xF0,0x0F,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0xFF,0xF0,0xFF,0xFF,0xFF,0x00,0x00,0xFF,0xFF,0x0F,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0x00,0x0F,0xFF,0xFF,0xFF,0xFF,0xF0,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xF0,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xF0,0xF0,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0xFF,0xFF,0xF0,0x00,0x00,0x00,0x00,0xFF,0xF0,0x00,0x0F,0xFF,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xF0,0x00,0x0F,0x00,0x0F,0xFF,0xF0,0x00,0x0F,0xFF,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0xFF,0xFF,0x00,0xFF,0xFF,0xFF,0xFF,0x00,0xFF,0xFF,0xF0,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0x0F,0xFF,0xFF,0xF0,0x00,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0xFF,0xFF,0x00,0xFF,0xF0,0x00,0x00,0x0F,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xF0,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

/* ==================== 绘制难度选择菜单 ==================== */
void draw_difficulty_menu()
{
    int i;
    int title_x;
    int line_left, line_right;
    char* text;
    char* visible_text;
    int scroll_x;
    int text_width;
    int skip_pixels;
    int cx;

    ClearScreen();

    /* 1. 菜单外框 */
    Rectangle(MENU_X - 2, MENU_Y - 2, MENU_X + MENU_W + 2, MENU_Y + MENU_H, 1);

    /* 2. 标题区 "中国象棋" (大字体) */
    title_x = (WINDOW_WIDTH - 64) / 2;
    TextOut(title_x, MENU_Y + 5, "中国象棋", 0x81);

    /* 版本号 (小字体，标题右上角) */
    TextOut(WINDOW_WIDTH - 76, 10, "LavaX V1.0", 0x01);

    /* "选择难度" 副标题 */
    //TextOut(40, MENU_Y + 5, gui_title, 0x81);

    /* 副标题下分隔线 */
    Block(MENU_X + 10, MENU_Y + 34, MENU_X + MENU_W - 10, MENU_Y + 34, 1);

    /* 4. 三个选项 */
    for (i = 0; i < 3; i++)
    {
        int y = OPT_Y(i);
        int selected = ((i + 1) == g_difficulty_selected);

        if (selected)
        {
            /* 选中：黑底白字 + 三角箭头 */
            Block(OPT_X, y, OPT_X + OPT_W, y + OPT_H, 1);
            text = get_difficulty_text(i);
            TextOut(OPT_TEXT_X, y + 8, text, 0x09);
            draw_arrow(OPT_X - 12, y + 10);
        }
        else
        {
            /* 未选中：细框 + 普通文字 */
            Rectangle(OPT_X, y, OPT_X + OPT_W, y + OPT_H, 1);
            text = get_difficulty_text(i);
            TextOut(OPT_TEXT_X, y + 8, text, 0x01);
        }
    }

    /* 5. 选项下方分隔线 */
    Block(MENU_X + 4, OPT_Y(2) + OPT_H + 3, MENU_X + MENU_W - 4, OPT_Y(2) + OPT_H + 3, 1);

    /* 7. 底部跑马灯文字 */
    text_width = g_marquee_text_width;
    if (text_width <= 0)
    {
        text_width = calc_marquee_text_width();
    }

    /* 从右侧进入，向左滚动，滚到文字全部离开左侧后从右侧重新进入 */
    scroll_x = WINDOW_WIDTH - g_marquee_offset * MARQUEE_STEP;

    /* 清除底部文字区域 (从外框下方开始，避免遮住框线) */
    Block(0, MENU_Y + MENU_H + 4, WINDOW_WIDTH, WINDOW_HEIGHT, 0);

    /* 只在文字可见时绘制 (TextOut不支持负x坐标) */
    if (scroll_x > -text_width && scroll_x < WINDOW_WIDTH)
    {
        if (scroll_x >= 0)
        {
            TextOut(scroll_x, WINDOW_HEIGHT - 13, g_marquee_text, 0x01);
        }
        else
        {
            /* 负坐标: 跳过已滚出左侧的字符，从x=0开始绘制剩余部分 */
            skip_pixels = -scroll_x;
            cx = 0;
            visible_text = g_marquee_text;

            /* 逐字符计算宽度，跳过已滚出左侧的部分 */
            while (*visible_text != 0 && cx < skip_pixels)
            {
                int char_width;
                int char_bytes;

                char_bytes = get_marquee_char_bytes(visible_text, &char_width);
                if (char_bytes <= 0)
                {
                    break;
                }
                cx = cx + char_width;
                visible_text = visible_text + char_bytes;
            }

            /* 从剩余字符开始绘制 */
            if (*visible_text != 0)
            {
                TextOut(0, WINDOW_HEIGHT - 13, visible_text, 0x01);
            }
        }
    }

    /* 8. 战马logo (50x25像素，最后绘制覆盖在最上层) */
    WriteBlock(8, 6, 50, 25, 1, g_horse_logo);

    Refresh();
}

/* ==================== 处理键盘事件 ==================== */
int handle_difficulty_menu_key(char key)
{
    if(key == 0)
    {
        /* 无输入 */
        return 0;
    }
    if (key == 0x14)
    {
        /* Up */
        if (g_difficulty_selected > 1)
        {
            g_difficulty_selected--;
        }
        else
        {
            g_difficulty_selected = 3;
        }
        return 0;
    }
    else if (key == 0x15)
    {
        /* Down */
        if (g_difficulty_selected < 3)
        {
            g_difficulty_selected++;
        }
        else
        {
            g_difficulty_selected = 1;
        }
        return 0;
    }
    else if (key == 0x0D)
    {
        /* A/确认 */
        g_difficulty_confirmed = 1;
        return 1;
    }
    else if (key == 0x1B)
    {
        /* B/取消 - 默认中级 */
        if (g_difficulty_selected == 0)
        {
            g_difficulty_selected = DIFFICULTY_MEDIUM;
        }
        g_difficulty_confirmed = 1;
        return 1;
    }
    return 0;
}

/* ==================== 初始化菜单状态 ==================== */
void init_difficulty_menu()
{
    init_difficulty_texts();
    g_marquee_text_width = calc_marquee_text_width();
    g_difficulty_selected = DIFFICULTY_MEDIUM;
    g_difficulty_hovered = 0;
    g_difficulty_confirmed = 0;
    g_current_difficulty = DIFFICULTY_MEDIUM;
}

/* ==================== 获取选中的难度 ==================== */
int get_selected_difficulty()
{
    if (g_difficulty_selected == 0)
    {
        return DIFFICULTY_MEDIUM;
    }
    return g_difficulty_selected;
}

/* ==================== 根据难度获取搜索深度 ==================== */
int get_ai_search_depth_by_difficulty(int difficulty)
{
    if (difficulty == DIFFICULTY_EASY)
    {
        return 2;
    }
    else if (difficulty == DIFFICULTY_HARD)
    {
        return 4;
    }
    return 3;
}

/* ==================== 根据难度获取超时时间 ==================== */
int get_ai_timeout_by_difficulty(int difficulty)
{
    if (difficulty == DIFFICULTY_EASY)
    {
        return 5;
    }
    if (difficulty == DIFFICULTY_HARD)
    {
        return 45;
    }
    return 15;
}

/* ==================== 显示难度选择窗口 ==================== */
int show_difficulty_menu()
{
    char key;
    int result;
    int depth;
    int timeout;
    int marquee_cycle_width;
    int display_ready_logged;
    char* name;

    /* 初始化菜单状态 */
    init_difficulty_menu();
    g_marquee_offset = 0;
    display_ready_logged = 0;
    marquee_cycle_width = WINDOW_WIDTH + g_marquee_text_width;

    /* 绘制初始菜单 */
    draw_difficulty_menu();

    /* 等待用户选择，同时跑马灯滚动 */
    while (g_difficulty_confirmed == 0)
    {
        /* 非阻塞检查按键 */
        key = Inkey();

        if (key != 0)
        {
            handle_difficulty_menu_key(key);
        }

        /* 更新跑马灯偏移，完整滚出屏幕后再从右侧重新进入 */
        g_marquee_offset = g_marquee_offset + 1;
        if (display_ready_logged == 0 && g_marquee_offset >= 4)
        {
            printf("[lava_cch] UI ready: %dx%d\n", WINDOW_WIDTH, WINDOW_HEIGHT);
            display_ready_logged = 1;
        }
        if (g_marquee_offset * MARQUEE_STEP >= marquee_cycle_width)
        {
            g_marquee_offset = 0;
        }

        /* 重绘菜单 */
        draw_difficulty_menu();

        /* 等待500ms */
        Delay(500);
    }

    result = get_selected_difficulty();
    g_current_difficulty = result;

    depth = get_ai_search_depth_by_difficulty(result);
    timeout = get_ai_timeout_by_difficulty(result);
    name = (result == DIFFICULTY_EASY) ? "初级" :
           (result == DIFFICULTY_HARD) ? "高级" : "中级";
    DPRINTF("[难度选择] 难度=%s 思考时间=%d秒\n", name, timeout);

    return result;
}
