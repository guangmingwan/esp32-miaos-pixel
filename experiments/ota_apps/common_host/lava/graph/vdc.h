/**
 * @file vdc.h
 * @brief VDC (Virtual Device Context) 图形设备上下文
 *
 * 从 lavaVM/graph_vw.h 抽离，移除虚拟机依赖
 * VDC 是独立的图形抽象层，可被双模式共用
 */

#ifndef VDC_H
#define VDC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 基础类型 ==================== */

typedef uint8_t  byte;
typedef uint16_t word;
typedef uint32_t u32;

/* ==================== 矩形结构 ==================== */

typedef struct {
    short x0, y0;   /* 左上角 */
    short x1, y1;   /* 右下角 */
} VRECT;

/* ==================== VDC 结构 ==================== */

/**
 * @brief 虚拟设备上下文
 *
 * 包含绘图操作所需的所有状态信息
 */
typedef struct {
    u32 *mem;           /* 像素数据 (BGRA 格式) */
    int width;          /* 宽度 */
    int height;         /* 高度 */
    int depth;          /* 颜色深度 */

    /* 绘图状态 */
    u32 font;           /* 字体样式 */
    u32 fgcolor;        /* 前景色 */
    u32 bgcolor;        /* 背景色 */
    u32 keycolor;       /* 关键色（透明） */
    u32 bordercolor;    /* 边框色 */

    /* 坐标系统 */
    int org_x, org_y;   /* 原点偏移 */

    /* 裁剪区域 */
    VRECT clip;         /* 裁剪矩形 */

    /* 绘图模式 */
    word draw_mode;     /* 绘图模式 */
} VDC;

/* ==================== 绘图模式 ==================== */

enum {
    DRAW_COPY = 0,      /* 复制模式 */
    DRAW_BROKEN,        /* 虚线模式 */
    DRAW_XOR,           /* 异或模式 */
    DRAW_AND,           /* 与模式 */
    DRAW_OR,            /* 或模式 */
    DRAW_SPRITE,        /* 精灵模式（透明） */
    DRAW_NOT = 8        /* 取反标志 */
};

/* ==================== 字体样式 ==================== */

enum {
    FONT_MEDIUM = 0,    /* 中等字体 */
    FONT_SMALL  = 1,    /* 小字体 */
    FONT_LARGE  = 2,    /* 大字体 */
    FONT_BOLD   = 4,    /* 粗体 */
    FONT_ITALIC = 8,    /* 斜体 */
    FONT_UNDERLINE = 16,/* 下划线 */
    FONT_BORDER = 32    /* 描边 */
};

/* ==================== 颜色宏 ==================== */

#ifdef RGB
#undef RGB
#endif

#define RGB(r,g,b)   ((byte)(r) | ((byte)(g) << 8) | ((byte)(b) << 16) | 0xFF000000)
#define RGBA(r,g,b,a) ((byte)(r) | ((byte)(g) << 8) | ((byte)(b) << 16) | ((byte)(a) << 24))

/* 预定义颜色 */
#define COLOR_BLACK   RGB(0,0,0)
#define COLOR_WHITE   RGB(255,255,255)
#define COLOR_RED     RGB(255,0,0)
#define COLOR_GREEN   RGB(0,255,0)
#define COLOR_BLUE    RGB(0,0,255)
#define COLOR_YELLOW  RGB(255,255,0)

/* ==================== 图形函数声明 ==================== */

/* 初始化 */
void vw_Init(void);

/* 像素操作 */
void vw_DrawPixel(VDC *dc, int x, int y);
u32  vw_GetPixel(VDC *dc, int x, int y);

/* 图元绘制 */
void vw_DrawLine(VDC *dc, int x0, int y0, int x1, int y1);
void vw_DrawRect(VDC *dc, int x0, int y0, int x1, int y1);
void vw_FillRect(VDC *dc, int x0, int y0, int x1, int y1);
void vw_DrawCircle(VDC *dc, int x0, int y0, int r);
void vw_FillCircle(VDC *dc, int x0, int y0, int r);
void vw_DrawEllipse(VDC *dc, int x0, int y0, int r1, int r2);
void vw_FillEllipse(VDC *dc, int x0, int y0, int rx, int ry);

/* 区域操作 */
void vw_ClearArea(VDC *dc);
void vw_ClearRect(VDC *dc, int x0, int y0, int x1, int y1);
void vw_SetArea(VDC *dc, int x0, int y0, int x1, int y1);

/* 位块传送 */
void vw_BitBlt(VDC *dc, int xd, int yd, int w, int h,
               VDC *dcs, int xs, int ys, int mode);

/* 状态设置 */
void vw_SetDrawMode(VDC *dc, int mode);
int  vw_SetFgColor(VDC *dc, u32 color);
int  vw_SetBgColor(VDC *dc, u32 color);
int  vw_SetKeyColor(VDC *dc, u32 color);
void vw_SetFont(VDC *dc, int font);
void vw_SetOrg(VDC *dc, int x, int y);

/* 文本输出 */
void vw_TextOut(VDC *dc, int x, int y, const char *str, int num, int mode);

/* ==================== 字体接口 ==================== */

/**
 * @brief 设置字体数据源
 * @param data 字体数据指针
 */
void vw_SetFontData(const byte *data);

#ifdef __cplusplus
}
#endif

#endif /* VDC_H */
