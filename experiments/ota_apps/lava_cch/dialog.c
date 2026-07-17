/**
 * @file dialog.c
 * @brief LavaX 对话框实现
 * @author QWen
 * @date 2026
 */

#include "xiangqi.h"

/**
 * @brief 绘制游戏结束对话框
 * @param winner 获胜方: 1=红方, 2=黑方
 */
void draw_game_over_dialog(int winner)
{
    /* 绘制对话框背景 */
    Block(70, 68, 250, 170, 0);
    Rectangle(70, 68, 250, 170, 1);

    /* 标题 */
    TextOut(136, 80, "游戏结束", 1);

    /* 获胜方 */
    if(winner == 0) {
        TextOut(142, 105, "平局!", 1);
    }else if (winner == 1) {
        TextOut(136, 105, "红方赢!", 1);
    } else {
        TextOut(136, 105, "黑方赢!", 1);
    }


    /* 提示 */
    TextOut(112, 138, "A:重开  B:退出", 1);

    Refresh();
}
