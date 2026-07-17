/**
 * @file dialog_file.c
 * @brief SDL2 文件选择对话框实现
 * @author QWen 3.5 千问大模型
 * @date 2026
 */

#include "xiangqi.h"
#include <SDL2/SDL_ttf.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 扫描目录中的.pgn 和.txt 文件
static void scan_pgn_files(const char* directory, DialogState* state) {
    if (!state) return;

    state->file_count = 0;

    DIR* dir = opendir(directory);
    if (!dir) {
        return;
    }

    struct dirent* entry;
    struct stat st;
    char full_path[512];

    while ((entry = readdir(dir)) != NULL && state->file_count < MAX_FILES) {
        // 跳过隐藏文件和目录
        if (entry->d_name[0] == '.') continue;

        // 构造完整路径
        snprintf(full_path, sizeof(full_path), "%s/%s", directory, entry->d_name);

        // 检查是否是普通文件
        if (stat(full_path, &st) != 0 || !S_ISREG(st.st_mode)) continue;

        // 检查扩展名
        const char* filename = entry->d_name;
        size_t len = strlen(filename);

        bool is_pgn = (len > 4 && strcmp(filename + len - 4, ".pgn") == 0);
        bool is_txt = (len > 4 && strcmp(filename + len - 4, ".txt") == 0);
        bool is_dhtml = (len > 6 && strcmp(filename + len - 6, ".dhtml") == 0);

        if (is_pgn || is_txt || is_dhtml) {
            strncpy(state->file_list[state->file_count], filename, MAX_FILENAME_LENGTH - 1);
            state->file_list[state->file_count][MAX_FILENAME_LENGTH - 1] = '\0';
            state->file_count++;
        }
    }

    closedir(dir);

    // 按文件名排序（简单冒泡排序）
    char temp[MAX_FILENAME_LENGTH];
    for (int i = 0; i < state->file_count - 1; i++) {
        for (int j = 0; j < state->file_count - 1 - i; j++) {
            if (strcmp(state->file_list[j], state->file_list[j + 1]) > 0) {
                // 使用 strcpy 替代 strncpy 避免截断警告
                strcpy(temp, state->file_list[j]);
                strcpy(state->file_list[j], state->file_list[j + 1]);
                strcpy(state->file_list[j + 1], temp);
            }
        }
    }

    state->selected_file_index = 0;
    state->scroll_offset = 0;
}

// 显示文件选择对话框
void show_file_dialog(DialogState* state, const char* title, const char* directory) {
    if (!state) return;

    strncpy(state->file_dialog_title, title, sizeof(state->file_dialog_title) - 1);
    state->file_dialog_title[sizeof(state->file_dialog_title) - 1] = '\0';

    // 扫描目录中的文件
    scan_pgn_files(directory, state);

    if (state->file_count > 0) {
        state->file_dialog_state = FILE_DIALOG_VISIBLE;
    }
}

// 隐藏文件选择对话框
void hide_file_dialog(DialogState* state) {
    if (!state) return;
    state->file_dialog_state = FILE_DIALOG_HIDDEN;
}

// 处理文件选择对话框事件
// 返回值：0=无操作，1=选择文件并关闭，2=取消/关闭
int handle_file_dialog_event(DialogState* state, SDL_Event* event, SDL_Renderer* renderer, void* font) {
    (void)renderer; // 保留用于未来扩展
    if (!state || state->file_dialog_state != FILE_DIALOG_VISIBLE) return 0;

    TTF_Font* ttf_font = (TTF_Font*)font;
    (void)ttf_font; // 保留用于未来扩展

    // 对话框位置和尺寸
    int dialog_x = (WINDOW_WIDTH - 400) / 2;
    int dialog_y = (WINDOW_HEIGHT - 350) / 2;
    int dialog_width = 400;
    int dialog_height = 350;

    // 文件列表区域
    int list_x = dialog_x + 20;
    int list_y = dialog_y + 60;
    int list_width = dialog_width - 40;
    int list_height = 220;
    int item_height = 25;
    int visible_items = list_height / item_height;

    if (event->type == SDL_MOUSEBUTTONDOWN && event->button.button == SDL_BUTTON_LEFT) {
        int mouse_x = event->button.x;
        int mouse_y = event->button.y;

        // 检查是否点击了文件列表
        if (mouse_x >= list_x && mouse_x < list_x + list_width &&
            mouse_y >= list_y && mouse_y < list_y + list_height) {

            // 检查是否点击了滚动条
            if (state->file_count > visible_items) {
                int scrollbar_width = 12;
                int scrollbar_x = list_x + list_width - scrollbar_width;

                if (mouse_x >= scrollbar_x && mouse_x < scrollbar_x + scrollbar_width) {
                    // 计算滚动块位置
                    float scroll_ratio = (float)visible_items / state->file_count;
                    int thumb_height = (int)(list_height * scroll_ratio);
                    if (thumb_height < 20) thumb_height = 20;

                    float scroll_pos = (float)state->scroll_offset / (state->file_count - visible_items);
                    int thumb_y = list_y + (int)(scroll_pos * (list_height - thumb_height));

                    if (mouse_y >= thumb_y && mouse_y < thumb_y + thumb_height) {
                        // 点击了滚动块，可以拖动（暂不实现）
                        return 0;
                    } else if (mouse_y < thumb_y) {
                        // 点击了滚动块上方，向上翻页
                        state->scroll_offset -= visible_items;
                        if (state->scroll_offset < 0) state->scroll_offset = 0;
                        return 0;
                    } else {
                        // 点击了滚动块下方，向下翻页
                        state->scroll_offset += visible_items;
                        if (state->scroll_offset > state->file_count - visible_items) {
                            state->scroll_offset = state->file_count - visible_items;
                        }
                        return 0;
                    }
                }
            }

            // 计算点击的文件索引
            int clicked_index = (mouse_y - list_y) / item_height + state->scroll_offset;

            if (clicked_index >= 0 && clicked_index < state->file_count) {
                state->selected_file_index = clicked_index;
                // 点击即选择
                return 1;  // 返回 1 表示选择文件
            }
        }

        // 检查是否点击了关闭按钮区域
        if (mouse_x >= dialog_x + dialog_width - 40 && mouse_x < dialog_x + dialog_width - 10 &&
            mouse_y >= dialog_y + 10 && mouse_y < dialog_y + 40) {
            hide_file_dialog(state);
            return 2;  // 返回 2 表示取消
        }

        // 检查是否点击了"取消"按钮
        int cancel_x = dialog_x + dialog_width / 2 + 10;
        int cancel_y = dialog_y + dialog_height - 50;
        int cancel_w = 100, cancel_h = 35;
        if (mouse_x >= cancel_x && mouse_x < cancel_x + cancel_w &&
            mouse_y >= cancel_y && mouse_y < cancel_y + cancel_h) {
            hide_file_dialog(state);
            return 2;  // 返回 2 表示取消
        }

        // 检查是否点击了"确定"按钮
        int ok_x = dialog_x + dialog_width / 2 - 110;
        int ok_y = dialog_y + dialog_height - 50;
        int ok_w = 100, ok_h = 35;
        if (mouse_x >= ok_x && mouse_x < ok_x + ok_w &&
            mouse_y >= ok_y && mouse_y < ok_y + ok_h) {
            return 1;  // 返回 1 表示选择文件
        }
    }

    // 处理滚轮
    if (event->type == SDL_MOUSEWHEEL && state->file_count > visible_items) {
        if (event->wheel.y > 0) {
            // 向上滚动
            if (state->scroll_offset > 0) {
                state->scroll_offset--;
            }
        } else if (event->wheel.y < 0) {
            // 向下滚动
            if (state->scroll_offset < state->file_count - visible_items) {
                state->scroll_offset++;
            }
        }
    }

    return 0;
}

// 渲染文件选择对话框
void render_file_dialog(const DialogState* state, SDL_Renderer* renderer, void* font) {
    if (!state || state->file_dialog_state != FILE_DIALOG_VISIBLE || !renderer || !font) return;

    TTF_Font* ttf_font = (TTF_Font*)font;

    // 对话框位置和尺寸
    int dialog_x = (WINDOW_WIDTH - 400) / 2;
    int dialog_y = (WINDOW_HEIGHT - 350) / 2;
    int dialog_width = 400;
    int dialog_height = 350;

    // 绘制半透明背景遮罩
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 150);
    SDL_Rect overlay_rect = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
    SDL_RenderFillRect(renderer, &overlay_rect);

    // 绘制对话框背景
    SDL_SetRenderDrawColor(renderer, 240, 240, 240, 255);
    SDL_Rect dialog_rect = {dialog_x, dialog_y, dialog_width, dialog_height};
    SDL_RenderFillRect(renderer, &dialog_rect);

    // 绘制对话框边框
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_RenderDrawRect(renderer, &dialog_rect);

    // 绘制标题
    SDL_Color text_color = {0, 0, 0, 255};
    SDL_Surface* title_surface = TTF_RenderUTF8_Solid(ttf_font, state->file_dialog_title, text_color);
    if (title_surface) {
        SDL_Texture* title_texture = SDL_CreateTextureFromSurface(renderer, title_surface);
        if (title_texture) {
            int title_x = dialog_x + (dialog_width - title_surface->w) / 2;
            int title_y = dialog_y + 15;
            SDL_Rect title_rect = {title_x, title_y, title_surface->w, title_surface->h};
            SDL_RenderCopy(renderer, title_texture, NULL, &title_rect);
            SDL_DestroyTexture(title_texture);
        }
        SDL_FreeSurface(title_surface);
    }

    // 绘制关闭按钮（X）
    SDL_SetRenderDrawColor(renderer, 200, 50, 50, 255);
    SDL_Rect close_rect = {dialog_x + dialog_width - 35, dialog_y + 12, 25, 25};
    SDL_RenderFillRect(renderer, &close_rect);
    SDL_Surface* close_surface = TTF_RenderUTF8_Solid(ttf_font, "×", text_color);
    if (close_surface) {
        SDL_Texture* close_texture = SDL_CreateTextureFromSurface(renderer, close_surface);
        if (close_texture) {
            int close_x = dialog_x + dialog_width - 30;
            int close_y = dialog_y + 14;
            SDL_Rect close_rect2 = {close_x, close_y, close_surface->w, close_surface->h};
            SDL_RenderCopy(renderer, close_texture, NULL, &close_rect2);
            SDL_DestroyTexture(close_texture);
        }
        SDL_FreeSurface(close_surface);
    }

    // 文件列表区域
    int list_x = dialog_x + 20;
    int list_y = dialog_y + 60;
    int list_width = dialog_width - 40;
    int list_height = 220;
    int item_height = 25;

    // 绘制列表背景
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Rect list_rect = {list_x, list_y, list_width, list_height};
    SDL_RenderFillRect(renderer, &list_rect);
    SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
    SDL_RenderDrawRect(renderer, &list_rect);

    // 绘制文件列表
    int visible_items = list_height / item_height;
    int start_index = state->scroll_offset;
    int end_index = (start_index + visible_items < state->file_count) ?
                    (start_index + visible_items) : state->file_count;

    for (int i = start_index; i < end_index; i++) {
        int display_index = i - start_index;
        int item_y = list_y + display_index * item_height;

        // 选中项高亮
        if (i == state->selected_file_index) {
            SDL_SetRenderDrawColor(renderer, 200, 220, 255, 255);
            SDL_Rect highlight_rect = {list_x, item_y, list_width, item_height};
            SDL_RenderFillRect(renderer, &highlight_rect);
        }

        // 绘制文件名
        SDL_Surface* file_surface = TTF_RenderUTF8_Solid(ttf_font, state->file_list[i], text_color);
        if (file_surface) {
            SDL_Texture* file_texture = SDL_CreateTextureFromSurface(renderer, file_surface);
            if (file_texture) {
                int file_x = list_x + 10;
                int file_y = item_y + 4;
                SDL_Rect file_rect = {file_x, file_y, file_surface->w, file_surface->h};
                SDL_RenderCopy(renderer, file_texture, NULL, &file_rect);
                SDL_DestroyTexture(file_texture);
            }
            SDL_FreeSurface(file_surface);
        }
    }

    // 绘制滚动条（仅在文件数超过可见区域时）
    if (state->file_count > visible_items) {
        int scrollbar_width = 12;
        int scrollbar_x = list_x + list_width - scrollbar_width;

        // 滚动条背景
        SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
        SDL_Rect scrollbar_bg_rect = {scrollbar_x, list_y, scrollbar_width, list_height};
        SDL_RenderFillRect(renderer, &scrollbar_bg_rect);

        // 计算滚动块位置和大小
        float scroll_ratio = (float)visible_items / state->file_count;
        int thumb_height = (int)(list_height * scroll_ratio);
        if (thumb_height < 20) thumb_height = 20;

        float scroll_pos = (float)state->scroll_offset / (state->file_count - visible_items);
        int thumb_y = list_y + (int)(scroll_pos * (list_height - thumb_height));

        // 滚动块
        SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
        SDL_Rect scrollbar_thumb_rect = {scrollbar_x, thumb_y, scrollbar_width, thumb_height};
        SDL_RenderFillRect(renderer, &scrollbar_thumb_rect);

        // 滚动条边框
        SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
        SDL_RenderDrawRect(renderer, &scrollbar_bg_rect);
        SDL_RenderDrawRect(renderer, &scrollbar_thumb_rect);
    }

    // 绘制"确定"和"取消"按钮
    int button_w = 100, button_h = 35;
    int ok_x = dialog_x + dialog_width / 2 - 110;
    int cancel_x = dialog_x + dialog_width / 2 + 10;
    int button_y = dialog_y + dialog_height - 50;

    // 确定按钮
    SDL_SetRenderDrawColor(renderer, 100, 200, 100, 255);
    SDL_Rect ok_rect = {ok_x, button_y, button_w, button_h};
    SDL_RenderFillRect(renderer, &ok_rect);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_RenderDrawRect(renderer, &ok_rect);
    SDL_Surface* ok_surface = TTF_RenderUTF8_Solid(ttf_font, "确定", text_color);
    if (ok_surface) {
        SDL_Texture* ok_texture = SDL_CreateTextureFromSurface(renderer, ok_surface);
        if (ok_texture) {
            int ok_text_x = ok_x + (button_w - ok_surface->w) / 2;
            int ok_text_y = button_y + (button_h - ok_surface->h) / 2;
            SDL_Rect ok_text_rect = {ok_text_x, ok_text_y, ok_surface->w, ok_surface->h};
            SDL_RenderCopy(renderer, ok_texture, NULL, &ok_text_rect);
            SDL_DestroyTexture(ok_texture);
        }
        SDL_FreeSurface(ok_surface);
    }

    // 取消按钮
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_Rect cancel_rect = {cancel_x, button_y, button_w, button_h};
    SDL_RenderFillRect(renderer, &cancel_rect);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
    SDL_RenderDrawRect(renderer, &cancel_rect);
    SDL_Surface* cancel_surface = TTF_RenderUTF8_Solid(ttf_font, "取消", text_color);
    if (cancel_surface) {
        SDL_Texture* cancel_texture = SDL_CreateTextureFromSurface(renderer, cancel_surface);
        if (cancel_texture) {
            int cancel_text_x = cancel_x + (button_w - cancel_surface->w) / 2;
            int cancel_text_y = button_y + (button_h - cancel_surface->h) / 2;
            SDL_Rect cancel_text_rect = {cancel_text_x, cancel_text_y, cancel_surface->w, cancel_surface->h};
            SDL_RenderCopy(renderer, cancel_texture, NULL, &cancel_text_rect);
            SDL_DestroyTexture(cancel_texture);
        }
        SDL_FreeSurface(cancel_surface);
    }

    // 如果没有文件，显示提示信息
    if (state->file_count == 0) {
        const char* no_file_msg = "目录中没有找到.pgn、.txt 或.dhtml 棋谱文件";
        SDL_Surface* no_file_surface = TTF_RenderUTF8_Solid(ttf_font, no_file_msg, text_color);
        if (no_file_surface) {
            SDL_Texture* no_file_texture = SDL_CreateTextureFromSurface(renderer, no_file_surface);
            if (no_file_texture) {
                int no_file_x = dialog_x + (dialog_width - no_file_surface->w) / 2;
                int no_file_y = dialog_y + (dialog_height - no_file_surface->h) / 2;
                SDL_Rect no_file_rect = {no_file_x, no_file_y, no_file_surface->w, no_file_surface->h};
                SDL_RenderCopy(renderer, no_file_texture, NULL, &no_file_rect);
                SDL_DestroyTexture(no_file_texture);
            }
            SDL_FreeSurface(no_file_surface);
        }
    }
}
