/**
 * @file dialog_pause.c
 * @brief LavaX 游戏暂停菜单
 * @author QWen
 * @date 2026
 */

#include "xiangqi.h"

#define PAUSE_MENU_X 88
#define PAUSE_MENU_Y 50
#define PAUSE_MENU_W 144
#define PAUSE_MENU_H 132
#define PAUSE_OPT_H 24
#define PAUSE_OPT_Y(i) (PAUSE_MENU_Y + 28 + (i) * PAUSE_OPT_H)

/* 当前选中的菜单项 (0=求和, 1=认输, 2=悔棋, 3=恢复) */
int g_pause_menu_selected = 0;

/* 菜单选项文本 */
char* g_pause_menu_texts[] = {
    "1. 求和",
    "2. 认输",
    "3. 悔棋",
    "4. 恢复"
};

#define PAUSE_MENU_ITEMS 4

/**
 * @brief 绘制暂停菜单
 */
void draw_pause_menu()
{
    int i;
    char text[32];

    /* 绘制半透明背景 */
    //Block(0, 0, 128, 128, 0);

    /* 绘制对话框 */
    Block(PAUSE_MENU_X, PAUSE_MENU_Y, PAUSE_MENU_X + PAUSE_MENU_W, PAUSE_MENU_Y + PAUSE_MENU_H, 0);
    Rectangle(PAUSE_MENU_X, PAUSE_MENU_Y, PAUSE_MENU_X + PAUSE_MENU_W, PAUSE_MENU_Y + PAUSE_MENU_H, 1);

    /* 标题 */
    TextOut(PAUSE_MENU_X + 40, PAUSE_MENU_Y + 7, "暂停菜单", 1);

    /* 菜单选项 */
    for (i = 0; i < PAUSE_MENU_ITEMS; i++)
    {
        int y = PAUSE_OPT_Y(i);

        if (i == g_pause_menu_selected)
        {
            /* 选中项 */
            Block(PAUSE_MENU_X + 2, y, PAUSE_MENU_X + PAUSE_MENU_W - 2, y + PAUSE_OPT_H, 1);
            strcpy(text, g_pause_menu_texts[i]);
            TextOut(PAUSE_MENU_X + 38, y + 6, text, 9);
        }
        else
        {
            /* 未选中项 */
            strcpy(text, g_pause_menu_texts[i]);
            TextOut(PAUSE_MENU_X + 38, y + 6, text, 1);
        }
    }

    Refresh();
}

/**
 * @brief 处理暂停菜单键盘事件
 * @param key 按键值
 * @return 返回选择的操作: 0=无操作/取消, 1=求和, 2=认输, 3=悔棋, 4=恢复
 */
int handle_pause_menu_key(char key)
{
    int result = 0;

    if (key == 0x14)
    { /* 上 */
        if (g_pause_menu_selected > 0)
        {
            g_pause_menu_selected--;
        }
        else
        {
            g_pause_menu_selected = PAUSE_MENU_ITEMS - 1;
        }
        draw_pause_menu();
    }
    else if (key == 0x15)
    { /* 下 */
        if (g_pause_menu_selected < PAUSE_MENU_ITEMS - 1)
        {
            g_pause_menu_selected++;
        }
        else
        {
            g_pause_menu_selected = 0;
        }
        draw_pause_menu();
    }
    else if (key == 0x0D)
    { /* A 按钮 - 确认 */
        result = g_pause_menu_selected + 1;
    }
    else if (key == 0x1B)
    { /* B 按钮 - 取消 */
        result = 0;
    }
    else if (key == 0x19)
    { /* Y 按钮 - 再次按下关闭暂停菜单 */
        result = 0;
    }
    else {
        DPRINTF("输入无效%d\n", key);
    }

    return result;
}

/**
 * @brief 显示暂停菜单并处理用户输入
 * @return 返回选择的操作: 0=取消, 1=求和, 2=认输, 3=悔棋, 4=恢复
 */
int show_pause_menu()
{
    char key;
    int result = 0;

    /* 初始化选中项 */
    g_pause_menu_selected = 0;

    /* 绘制菜单 */
    draw_pause_menu();

    /* 等待用户输入 */
    while (1)
    {
        key = getchar();
        result = handle_pause_menu_key(key);

        if (result != 0 || key == 0x1B || key == 0x19)
        {
            break;
        }
    }

    return result;
}
