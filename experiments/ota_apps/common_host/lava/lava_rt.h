/**
 * @file lava_rt.h
 * @brief Lava Runtime 统一头文件
 *
 * Lava Runtime 是 LavaX 的共用运行时库，提供：
 * - 图形绘制 API（基于 VDC）
 * - 点阵字库支持
 * - 双模式兼容（字节码模式 + 原生模式）
 *
 * 设计原则：
 * - 无虚拟机依赖，可独立编译
 * - 接口与原 LAVA API 兼容
 * - 支持跨平台（Linux/Windows）
 */

#ifndef LAVA_RT_H
#define LAVA_RT_H

#include <stdint.h>
#include <stdio.h>

/* Undefine ctype macros to avoid conflicts */
#undef isalnum
#undef isalpha
#undef iscntrl
#undef isdigit
#undef isgraph
#undef islower
#undef isprint
#undef ispunct
#undef isspace
#undef isupper
#undef isxdigit
#undef tolower
#undef toupper

#include "myctype.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 类型定义 ==================== */

typedef uint8_t  byte;
typedef uint16_t word;
typedef uint32_t u32;
typedef int32_t  lava_long;
typedef uint32_t addr;

/* ==================== 运行时上下文 ==================== */

/**
 * @brief Lava Runtime 上下文结构
 *
 * 封装运行时所需的所有状态，替代原有的全局变量依赖
 */
typedef struct {
    /* 图形状态 */
    void *screen_vdc;       /* 屏幕 VDC 指针 */
    u32 palette[256];       /* 256色调色板（32位颜色） */
    byte *index_buf;        /* lavaVM 兼容像素索引缓冲 */

    /* 屏幕配置 */
    int screen_width;
    int screen_height;
    int color_depth;

    /* 图形状态 */
    word fgcolor;           /* 前景色 */
    word bgcolor;           /* 背景色 */
    word graph_mode;        /* 图形模式 */

    /* 字体 */
    const byte *font_data;  /* 字体数据指针 */

    /* 文本模式状态 */
    byte screen_mode;       /* 0=大字体, 1=小字体 */
    byte text_cols;
    byte text_rows;
    byte cursor_x;
    byte cursor_y;

    /* 用户数据（预留扩展） */
    void *user_data;
} LavaRuntime;

/* ==================== 初始化/销毁 ==================== */

/**
 * @brief 创建运行时上下文
 * @param width 屏幕宽度
 * @param height 屏幕高度
 * @param font_data 字体数据（NULL 使用内置字体）
 * @return 运行时上下文指针，失败返回 NULL
 */
LavaRuntime* lrt_create(int width, int height, const byte *font_data);

/**
 * @brief 销毁运行时上下文
 * @param rt 运行时上下文
 */
void lrt_destroy(LavaRuntime *rt);

/* ==================== 图形 API（VDC 封装）==================== */

#include "graph/vdc.h"

 /**
  * @brief 获取屏幕 VDC
  * @param rt 运行时上下文
  * @return 屏幕 VDC 指针
  */
 VDC* lrt_get_screen(LavaRuntime *rt);

 /**
  * @brief 获取全局单例运行时上下文
  * @details 用于原生 bridge 模式下刷新显示
  * @return 全局运行时指针
  */
 LavaRuntime* lrt_get_global(void);

 /**
  * @brief 设置调色板
 * @param rt 运行时上下文
 * @param index 调色板索引
 * @param r 红色分量
 * @param g 绿色分量
 * @param b 蓝色分量
 */
void lrt_set_palette(LavaRuntime *rt, int index, int r, int g, int b);

/**
 * @brief 设置前景色
 * @param rt 运行时上下文
 * @param color 颜色索引
 */
void lrt_set_fgcolor(LavaRuntime *rt, int color);

/**
 * @brief 设置背景色
 * @param rt 运行时上下文
 * @param color 颜色索引
 */
void lrt_set_bgcolor(LavaRuntime *rt, int color);

/* 全局版本（原生模式使用，不需要传递 rt 参数） */
void lrt_set_fgcolor_global(int color);
void lrt_set_bgcolor_global(int color);
void lrt_set_palette_block(int start, int count, unsigned char *data);
int lrt_set_palette_vm(int start, int count, const unsigned char *data);

/* ==================== LAVA 兼容 API ==================== */

/**
 * 以下函数保持与原 LAVA 虚拟机 API 兼容的签名
 * 便于字节码模式和原生模式共用
 */

/* 绘图开始/结束钩子 */
void lrt_begin_draw(void);
void lrt_end_draw(void);

/* 画点 */
void lrt_point(int x, int y, int type);

/* 画线 */
void lrt_line(int x0, int y0, int x1, int y1, int type);

/* 画矩形 */
void lrt_box(int x0, int y0, int x1, int y1, int fill, int type);

/* 画圆 */
void lrt_circle(int x, int y, int r, int fill, int type);

/* 画椭圆 */
void lrt_ellipse(int x, int y, int a, int b, int fill, int type);

/* 文本输出 */
void lrt_textout(int x, int y, const char *str, int mode);

/* 清屏 */
void lrt_clear_screen(void);

/* 刷新屏幕 */
void lrt_refresh(void);

/* 设置屏幕模式 */
void lrt_set_screen(int mode);

/* native 后端 printf 包装 */
int lrt_stack_printf(long long n, long long *stack);

/* 从 LAV 数据段初始化 gvar_data */
void lrt_init_data(const unsigned char *data, int len);

/* 写入图形块 */
void lrt_write_block(int x, int y, int w, int h, int mode, const byte *data);

/* 获取图形块 */
void lrt_get_block(int x, int y, int w, int h, int mode, byte *data);

/* 获取像素点 */
u32 lrt_get_pixel(int x, int y);
int lrt_get_point(int x, int y);

/* 设置图形模式 */
int lrt_set_graph_mode(int mode);

/* 设置文本屏模式 */
void lrt_locate(int y, int x);

/* 16色初始化 */
void lrt_Color16Init(void);

/* 256色初始化 */
void lrt_Color256Init(void);

/* 淡入淡出 */
void lrt_fade(int bright);

/* ==================== 帮助面板 API ==================== */

/**
 * @brief 设置按键映射帮助面板显示状态
 * @param visible 0=隐藏, 非0=显示
 */
void lrt_set_keymap_overlay_visible(int visible);

/**
 * @brief 切换按键映射帮助面板显示状态
 * @return 切换后的状态（0=隐藏, 1=显示）
 */
int lrt_toggle_keymap_overlay(void);

/**
 * @brief 获取按键映射帮助面板显示状态
 * @return 0=隐藏, 1=显示
 */
int lrt_is_keymap_overlay_visible(void);

/**
 * @brief 在 ARGB8888 帧缓冲上绘制按键映射帮助面板
 * @param pixels 目标像素缓冲区
 * @param width 缓冲区宽度（像素）
 * @param height 缓冲区高度（像素）
 * @param pitch 每行字节数
 */
void lrt_draw_keymap_overlay_u32(u32 *pixels, int width, int height, int pitch);

/* ==================== 字体 API ==================== */

/**
 * @brief 获取字符点阵数据
 * @param rt 运行时上下文
 * @param ch 字符编码（ASCII 或 GB2312）
 * @param buf 输出缓冲区
 * @param buf_size 缓冲区大小
 * @return 点阵宽度（8 或 16），失败返回 0
 */
int lrt_get_font_bitmap(LavaRuntime *rt, int ch, byte *buf, int buf_size);

/**
 * @brief 按字体规格获取字符点阵数据
 * @param rt 运行时上下文
 * @param ch 字符编码（ASCII 或 GB2312）
 * @param font 字体规格（FONT_SMALL 或 FONT_MEDIUM）
 * @param buf 输出缓冲区
 * @param buf_size 缓冲区大小
 * @return 点阵宽度，失败返回 0
 */
int lrt_get_font_bitmap_ex(LavaRuntime *rt, int ch, int font, byte *buf, int buf_size);

/* ==================== 编码转换 API ==================== */

/**
 * @brief 将 UTF-8 编码字符串转换为 GBK 编码字符串
 * @param utf8 输入的 UTF-8 编码字符串（以 '\0' 结尾）
 * @param gb 输出缓冲区，用于存储转换后的 GBK 字符串
 * @return 成功返回 1，失败返回 0
 */
int utf8_to_gb(char* utf8, char* gb);

/**
 * @brief 将 GBK 编码字符串转换为 UTF-8 编码字符串
 * @param gb 输入的 GBK 编码字符串（以 '\0' 结尾）
 * @param utf8 输出缓冲区，用于存储转换后的 UTF-8 字符串
 * @return 成功返回 1，失败返回 0
 */
int gb_to_utf8(char* gb, char* utf8);

/**
 * @brief UTF-8 到 GBK 转换的静态缓冲区版本
 * @param utf8_str 输入的 UTF-8 编码字符串
 * @return 指向静态缓冲区中 GBK 编码字符串的指针
 * @warning 非线程安全，连续调用会覆盖之前的结果
 */
char* utf8_to_gb_static(const char* utf8_str);

/* ==================== 标准 I/O API ==================== */

int lrt_putchar(int c);
int lrt_getchar(void);
int lrt_inkey(void);
int lrt_checkkey(int key);
void lrt_releasekey(int key);
int lrt_platform_key_down(int key);

/* ==================== 时间 API ==================== */

void lrt_delay(unsigned int ms);
unsigned int lrt_getms(void);
void lrt_gettime(uint8_t *t);
void lrt_settime(uint8_t y, uint8_t m, uint8_t d, uint8_t h, uint8_t mi, uint8_t s);

/* ==================== 随机数 API ==================== */

int lrt_random(void);
void lrt_srandom(unsigned int seed);

/* ==================== 字符串 API ==================== */

lava_long lrt_strlen(addr str);
lava_long lrt_strcmp(addr str1, addr str2);
addr lrt_strcpy(addr dst, addr src);
addr lrt_strcat(addr dst, addr src);
addr lrt_strchr(addr str, int c);
addr lrt_strstr(addr haystack, addr needle);

/* ==================== 字符判断 API ==================== */

int lrt_isalnum(int c);
int lrt_isalpha(int c);
int lrt_iscntrl(int c);
int lrt_isdigit(int c);
int lrt_isgraph(int c);
int lrt_islower(int c);
int lrt_isprint(int c);
int lrt_ispunct(int c);
int lrt_isspace(int c);
int lrt_isupper(int c);
int lrt_isxdigit(int c);
int lrt_tolower(int c);
int lrt_toupper(int c);

/* ==================== 文件 API ==================== */

addr lrt_fopen(addr path, addr mode);
FILE* lrt_fopen_native(const char *path, const char *mode);
int lrt_fopen_native_int(const char *path, const char *mode);
int lrt_fclose(addr fp);
lava_long lrt_fread(addr buf, lava_long size, lava_long nmemb, addr fp);
lava_long lrt_fwrite(addr buf, lava_long size, lava_long nmemb, addr fp);
int lrt_fseek(addr fp, lava_long offset, int whence);
lava_long lrt_ftell(addr fp);
int lrt_feof(addr fp);
void lrt_rewind(addr fp);
int lrt_fgetc(addr fp);
int lrt_fputc(int c, addr fp);

/* ==================== 配置文件读取 API ==================== */

#define MAX_CONFIG_SIZE 4096

typedef struct {
    char name[64];
    char val[260];
} config_t;

char *skip_space(char *p);
char *get_string(char *p,char *buf,int blen);
char *to_next_line(char *p);
void set_config(char *name,char *val);
int ConfigKey(char *name, char *val);
void ReadConfig(char *fname);

/* ==================== 路径处理 API ==================== */

extern char ExePath[260];
void GetExePath();

FILE* native_fopen(const char* path, const char* mode);

#ifdef __cplusplus
}
#endif

#endif /* LAVA_RT_H */
