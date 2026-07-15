/**
 * @file vdc.c
 * @brief LavaX 虚拟机图形库核心实现
 *
 * 本文件实现了 LavaX 虚拟机的核心图形绘制功能，来源于 lvc 图形库。
 * 主要功能包括：
 *   - VDC（虚拟显示上下文）管理
 *   - 基本图元绘制（点、线、矩形、圆、椭圆）
 *   - 位块传送（BitBlt）操作
 *   - 文本输出
 *   - 剪切区域（Clipping）支持
 *
 * 绘图模式说明：
 *   - DRAW_COPY: 直接复制，用前景色绘制
 *   - DRAW_XOR: 异或模式，用于橡皮筋效果
 *   - DRAW_BROKEN: 虚线模式
 *   - DRAW_SPRITE: 精灵模式，透明色处理
 *   - DRAW_NOT: 取反标志，可与上述模式组合使用
 *
 * VDC 结构体包含：
 *   - mem: 显存指针（32位像素，BGRA格式）
 *   - width/height: 宽度和高度
 *   - clip: 剪切区域
 *   - fgcolor/bgcolor: 前景色/背景色
 *   - keycolor: 透明色（用于精灵绘制）
 *   - org_x/org_y: 原点偏移
 *   - draw_mode: 绘图模式
 *   - font: 字体属性
 *
 * @author LavaX Team
 */

#include "vdc.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*============================================================================
 * 字体数据内存布局（从 ram.h 移植）
 *============================================================================*/

/* 字体数据基地址 */
#define base        g_font_data

/* ASCII 8x8 点阵字体 */
#define ascii       (byte*)(base)

/* ASCII 8x16 点阵字体 */
#define ascii8      (byte*)(base+0x600)

/* GB2312 汉字 16x16 点阵字体 */
#define gbfont      (byte*)(base+0xe00)

/* GB2312 扩展区 */
#define gbfont16    (byte*)(base+0x2d7d0)

/*============================================================================
 * 全局字体数据指针
 *============================================================================*/

/**
 * @brief 字体数据指针（通过 vw_SetFontData 设置）
 */
static const byte *g_font_data = NULL;

/**
 * @brief 设置字体数据源
 * @param data 字体数据指针
 */
void vw_SetFontData(const byte *data)
{
    g_font_data = data;
}

/*============================================================================
 * 宏定义
 *============================================================================*/

/**
 * @brief XOR 模式使用的颜色值
 * 用于异或操作时产生可见效果
 */
#define XOR_COLOR 0xffffff

/**
 * @brief 为 X 坐标添加原点偏移
 * @param x X 坐标
 */
#define DC_ADDORGX(x) x+=dc->org_x

/**
 * @brief 为 Y 坐标添加原点偏移
 * @param y Y 坐标
 */
#define DC_ADDORGY(y) y+=dc->org_y

/**
 * @brief 为源 VDC 的 X 坐标添加原点偏移
 * @param x X 坐标
 */
#define DC_ADDORGXS(x) x+=dcs->org_x

/**
 * @brief 为源 VDC 的 Y 坐标添加原点偏移
 * @param y Y 坐标
 */
#define DC_ADDORGYS(y) y+=dcs->org_y

/**
 * @brief 为坐标添加原点偏移（X 和 Y）
 * @param x X 坐标
 * @param y Y 坐标
 */
#define DC_ADDORG(x,y) DC_ADDORGX(x);DC_ADDORGY(y)

/**
 * @brief 为源 VDC 的坐标添加原点偏移（X 和 Y）
 * @param x X 坐标
 * @param y Y 坐标
 */
#define DC_ADDORGS(x,y) DC_ADDORGXS(x);DC_ADDORGYS(y)

/*============================================================================
 * 全局变量
 *============================================================================*/

/** @brief 字体渲染的最大尺寸 */
#define MAX_FONT_SIZE 32

/**
 * @brief 字体渲染专用 VDC
 * 用于在将字符绘制到目标 VDC 之前，先在临时缓冲区中渲染字符
 * 支持字体效果处理（粗体、斜体、边框等）
 */
VDC FontDc;

/**
 * @brief 字体渲染临时缓冲区
 * 存储单个字符的渲染结果，用于字体效果处理后再传送到目标 VDC
 */
static u32 FontMem[MAX_FONT_SIZE * MAX_FONT_SIZE];

/*============================================================================
 * 静态函数 - 内部辅助函数
 *============================================================================*/

/**
 * @brief 初始化字体渲染专用 VDC
 *
 * 设置 FontDc 的基本属性，包括剪切区域和显存指针
 * 用于后续的字符渲染操作
 */
static void FontDcInit()
{
	memset(&FontDc, 0, sizeof(VDC));
	FontDc.clip.x1 = MAX_FONT_SIZE - 1;  /* 设置剪切区域右边界 */
	FontDc.clip.y1 = MAX_FONT_SIZE - 1;  /* 设置剪切区域下边界 */
	FontDc.width = MAX_FONT_SIZE;         /* 设置宽度 */
	FontDc.mem = FontMem;                 /* 绑定显存缓冲区 */
}

/**
 * @brief 用指定值填充 32 位整数数组
 *
 * 类似于标准库的 memset，但操作的是 32 位整数
 *
 * @param p    目标数组指针
 * @param t    填充值
 * @param size 填充元素个数
 */
static void wmemset(u32* p, u32 t, int size)
{
	while (size-- > 0) {
		*p++ = t;
	}
}

/**
 * @brief 内部位块传送函数（不处理原点偏移）
 *
 * 将源 VDC 的矩形区域复制到目标 VDC 的指定位置
 * 支持多种绘制模式，并自动进行剪切区域检测
 *
 * @param dc   目标 VDC
 * @param xd   目标 X 坐标
 * @param yd   目标 Y 坐标
 * @param w    宽度
 * @param h    高度
 * @param dcs  源 VDC
 * @param xs   源 X 坐标
 * @param ys   源 Y 坐标
 * @param mode 绘制模式（DRAW_COPY, DRAW_XOR, DRAW_AND, DRAW_OR, DRAW_SPRITE, DRAW_NOT）
 *
 * 绘制模式说明：
 *   - DRAW_COPY: 直接复制像素
 *   - DRAW_XOR: 目标与源进行异或操作
 *   - DRAW_AND: 目标与源进行与操作
 *   - DRAW_OR:  目标与源进行或操作
 *   - DRAW_SPRITE: 精灵模式，跳过透明色像素
 *   - DRAW_NOT: 取反标志，与上述模式组合使用
 */
static void vw_bitblt(VDC* dc, int xd, int yd, int w, int h, VDC* dcs, int xs, int ys, int mode)
{
	int x0, y0, x1, y1, ww, ddd, dds;
	u32* ss, * dd, keycolor;

	/* 计算目标矩形边界 */
	x0 = xd;
	y0 = yd;
	x1 = x0 + w - 1;
	y1 = y0 + h - 1;

	/* 快速剔除完全在剪切区域外的情况 */
	if (w<1 || x0>dc->clip.x1 || x1 < dc->clip.x0) return;
	if (h<1 || y0>dc->clip.y1 || y1 < dc->clip.y0) return;

	/* 裁剪到剪切区域内 */
	if (x0 < dc->clip.x0) x0 = dc->clip.x0;
	if (x1 > dc->clip.x1) x1 = dc->clip.x1;
	if (y0 < dc->clip.y0) y0 = dc->clip.y0;
	if (y1 > dc->clip.y1) y1 = dc->clip.y1;

	/* 调整源坐标以匹配裁剪后的目标区域 */
	xs += x0 - xd;
	ys += y0 - yd;
	w = x1 - x0 + 1;

	/* DRAW_COPY 模式直接跳转到优化的内存复制代码 */
	if (mode == DRAW_COPY)
		goto xxx;

	/* 计算行扫描后的偏移量 */
	ddd = dc->width - w;   /* 目标 VDC 的行偏移 */
	dds = dcs->width - w;  /* 源 VDC 的行偏移 */
	dd = &dc->mem[y0 * dc->width + x0];
	ss = &dcs->mem[ys * dcs->width + xs];

	/* 处理带 DRAW_NOT 标志的模式 */
	if (mode & DRAW_NOT) {
		mode &= ~DRAW_NOT;  /* 清除 DRAW_NOT 标志 */
		switch (mode) {
		case DRAW_XOR:
			/* 异或模式：目标 ^= 源 ^ XOR_COLOR */
			while (y0++ <= y1) {
				ww = w;
				while (ww--)
					*dd++ ^= *ss++ ^ XOR_COLOR;
				dd += ddd;
				ss += dds;
			}
			break;
		case DRAW_AND:
			/* 与模式：目标 &= 源 ^ XOR_COLOR */
			while (y0++ <= y1) {
				ww = w;
				while (ww--)
					*dd++ &= *ss++ ^ XOR_COLOR;
				dd += ddd;
				ss += dds;
			}
			break;
		case DRAW_OR:
			/* 或模式：目标 |= 源 ^ XOR_COLOR */
			while (y0++ <= y1) {
				ww = w;
				while (ww--)
					*dd++ |= *ss++ ^ XOR_COLOR;
				dd += ddd;
				ss += dds;
			}
			break;
		case DRAW_SPRITE:
			/* 精灵模式：跳过透明色（keycolor）像素 */
			keycolor = dcs->keycolor;
			while (y0++ <= y1) {
				ww = w;
				while (ww--) {
					if (*ss != keycolor)
						*dd++ = *ss++ ^ XOR_COLOR;
					else {
						dd++;  /* 跳过透明像素 */
						ss++;
					}
				}
				dd += ddd;
				ss += dds;
			}
			break;
		default:
			/* 默认：直接复制并取反 */
			while (y0++ <= y1) {
				ww = w;
				while (ww--)
					*dd++ = *ss++ ^ XOR_COLOR;
				dd += ddd;
				ss += dds;
			}
		}
	}
	else {
		/* 处理不带 DRAW_NOT 标志的模式 */
		switch (mode) {
		case DRAW_XOR:
			/* 异或模式：目标 ^= 源 */
			while (y0++ <= y1) {
				ww = w;
				while (ww--)
					*dd++ ^= *ss++;
				dd += ddd;
				ss += dds;
			}
			break;
		case DRAW_AND:
			/* 与模式：目标 &= 源 */
			while (y0++ <= y1) {
				ww = w;
				while (ww--)
					*dd++ &= *ss++;
				dd += ddd;
				ss += dds;
			}
			break;
		case DRAW_OR:
			/* 或模式：目标 |= 源 */
			while (y0++ <= y1) {
				ww = w;
				while (ww--)
					*dd++ |= *ss++;
				dd += ddd;
				ss += dds;
			}
			break;
		case DRAW_SPRITE:
			/* 精灵模式：跳过透明色像素 */
			keycolor = dcs->keycolor;
			while (y0++ <= y1) {
				ww = w;
				while (ww--) {
					if (*ss != keycolor)
						*dd++ = *ss++;
					else {
						dd++;  /* 跳过透明像素 */
						ss++;
					}
				}
				dd += ddd;
				ss += dds;
			}
			break;
		default:
xxx:
			/* 默认/复制模式：使用 memcpy 进行高效复制 */
			while (y0 <= y1) {
				memcpy(&dc->mem[y0 * dc->width + x0], &dcs->mem[ys * dcs->width + xs], w << 2);
				y0++;
				ys++;
			}
		}
	}
}

/*============================================================================
 * 公共 API 函数
 *============================================================================*/

/**
 * @brief 位块传送函数（处理原点偏移）
 *
 * 将源 VDC 的矩形区域复制到目标 VDC 的指定位置
 * 此函数是 vw_bitblt 的公共接口，会自动处理原点偏移
 *
 * @param dc   目标 VDC
 * @param xd   目标 X 坐标
 * @param yd   目标 Y 坐标
 * @param w    宽度
 * @param h    高度
 * @param dcs  源 VDC
 * @param xs   源 X 坐标
 * @param ys   源 Y 坐标
 * @param mode 绘制模式
 *
 * @see vw_bitblt
 */
void vw_BitBlt(VDC* dc, int xd, int yd, int w, int h, VDC* dcs, int xs, int ys, int mode)
{
	DC_ADDORG(xd, yd);    /* 添加目标原点偏移 */
	DC_ADDORGS(xs, ys);  /* 添加源原点偏移 */
	vw_bitblt(dc, xd, yd, w, h, dcs, xs, ys, mode);
}

/**
 * @brief 内部绘制像素点函数（不处理原点偏移）
 *
 * 在指定位置绘制一个像素点，根据绘图模式进行相应处理
 *
 * @param dc 目标 VDC
 * @param x  X 坐标
 * @param y  Y 坐标
 *
 * 绘图模式处理：
 *   - DRAW_XOR: 异或操作，用于橡皮筋效果
 *   - DRAW_BROKEN: 虚线效果，交替绘制
 *   - 默认: 使用前景色绘制
 */
static void vw_point_xy(VDC* dc, int x, int y)
{
	/* 检查是否在剪切区域内 */
	if (x<dc->clip.x0 || x>dc->clip.x1 ||
		y<dc->clip.y0 || y>dc->clip.y1)
		return;

	/* 根据绘图模式处理 */
	switch (dc->draw_mode) {
	case DRAW_XOR:
		/* 异或模式：与 XOR_COLOR 异或，产生可见效果 */
		dc->mem[y * dc->width + x] ^= XOR_COLOR;
		break;
	case DRAW_BROKEN:
		/* 虚线模式：(x+y) 为奇数时跳过 */
		if ((x + y) & 1)
			break;
		/* 向下执行默认绘制 */
	default:
		/* 默认模式：使用前景色绘制 */
		dc->mem[y * dc->width + x] = dc->fgcolor;
	}
}

/**
 * @brief 获取指定位置的像素颜色
 *
 * @param dc 目标 VDC
 * @param x  X 坐标
 * @param y  Y 坐标
 * @return 像素颜色值，如果坐标超出剪切区域则返回 0
 */
u32 vw_GetPixel(VDC* dc, int x, int y)
{
	DC_ADDORG(x, y);  /* 添加原点偏移 */

	/* 检查是否在剪切区域内 */
	if (x<dc->clip.x0 || x>dc->clip.x1 ||
		y<dc->clip.y0 || y>dc->clip.y1)
		return 0;

	return dc->mem[y * dc->width + x];
}

/**
 * @brief 绘制像素点
 *
 * 在指定位置绘制一个像素点
 *
 * @param dc 目标 VDC
 * @param x  X 坐标
 * @param y  Y 坐标
 */
void vw_DrawPixel(VDC* dc, int x, int y)
{
	DC_ADDORG(x, y);  /* 添加原点偏移 */
	vw_point_xy(dc, x, y);
}

/**
 * @brief 绘制垂直线
 *
 * 绘制一条垂直线（X 坐标相同）
 *
 * @param dc 目标 VDC
 * @param x  X 坐标
 * @param y0 起点 Y 坐标
 * @param y1 终点 Y 坐标
 */
static void vw_vline(VDC* dc, int x, int y0, int y1)
{
	/* 根据方向逐点绘制 */
	if (y0 < y1)
		while (y0 <= y1) {
			vw_point_xy(dc, x, y0++);
		}
	else
		while (y1 <= y0) {
			vw_point_xy(dc, x, y1++);
		}
}

/**
 * @brief 绘制水平线（内部函数）
 *
 * 绘制一条水平线（Y 坐标相同）
 * 调用前必须确保 x0 <= x1 且 y 在剪切区域内
 *
 * @param dc 目标 VDC
 * @param x0 起点 X 坐标
 * @param x1 终点 X 坐标
 * @param y  Y 坐标
 */
static void vw_hline(VDC* dc, int x0, int x1, int y)
{
	int width, broken_line;
	u32* p;

	width = x1 - x0 + 1;
	p = &dc->mem[y * dc->width + x0];

	/* 根据绘图模式处理整行 */
	switch (dc->draw_mode) {
	case DRAW_XOR:
		/* 异或模式：逐像素异或 */
		while (width--) {
			*p ^= XOR_COLOR;
			p++;
		}
		break;
	case DRAW_BROKEN:
		/* 虚线模式：交替绘制 */
		broken_line = (x0 + y) & 1;
		while (width--) {
			if (!broken_line)
				*p = dc->fgcolor;
			p++;
			broken_line ^= 1;
		}
		break;
	default:
		/* 默认模式：使用 wmemset 快速填充 */
		wmemset(p, dc->fgcolor, width);
	}
}

/**
 * @brief 用背景色清除水平线
 *
 * 用背景色填充一条水平线
 * 调用前必须确保 x0 <= x1 且 y 在剪切区域内
 *
 * @param dc 目标 VDC
 * @param x0 起点 X 坐标
 * @param x1 终点 X 坐标
 * @param y  Y 坐标
 */
static void vw_clearline(VDC* dc, int x0, int x1, int y)
{
	u32 *p = &dc->mem[y * dc->width + x0];
	for (int i = 0; i <= x1 - x0; i++) {
		*p++ = dc->bgcolor;
	}
}

/**
 * @brief 绘制水平线（带剪切检查）
 *
 * 绘制一条水平线，自动进行剪切区域检测
 *
 * @param dc 目标 VDC
 * @param x0 起点 X 坐标
 * @param x1 终点 X 坐标
 * @param y  Y 坐标
 */
static void vw_hline_check(VDC* dc, int x0, int x1, int y)
{
	int t;

	/* 检查 Y 是否在剪切区域内 */
	if (y<dc->clip.y0 || y>dc->clip.y1) return;

	/* 确保 x0 <= x1 */
	if (x0 > x1) {
		t = x0;
		x0 = x1;
		x1 = t;
	}

	/* 检查 X 是否在剪切区域内 */
	if (x0 > dc->clip.x1 || x1 < dc->clip.x0) return;

	/* 裁剪到剪切区域 */
	if (x0 < dc->clip.x0) x0 = dc->clip.x0;
	if (x1 > dc->clip.x1) x1 = dc->clip.x1;

	vw_hline(dc, x0, x1, y);
}

/**
 * @brief 内部绘制直线函数（不处理原点偏移）
 *
 * 使用 Bresenham 算法绘制直线
 * 支持任意角度的直线，自动进行剪切检测
 *
 * @param dc 目标 VDC
 * @param x0 起点 X 坐标
 * @param y0 起点 Y 坐标
 * @param x1 终点 X 坐标
 * @param y1 终点 Y 坐标
 */
static void vw_line(VDC* dc, int x0, int y0, int x1, int y1)
{
	unsigned short delta_x, delta_y, distance, tt, xerr, yerr;
	int t;
	int incy;

	/* 特殊情况：垂直线 */
	if (x0 == x1) {
		vw_vline(dc, x0, y0, y1);
		return;
	}
	/* 特殊情况：水平线 */
	if (y0 == y1) {
		vw_hline_check(dc, x0, x1, y0);
		return;
	}

	/* 确保从左到右绘制 */
	if (x1 < x0) {
		t = x0;
		x0 = x1;
		x1 = t;
		t = y0;
		y0 = y1;
		y1 = t;
	}

	/* Bresenham 算法参数计算 */
	delta_x = x1 - x0;
	delta_y = abs(y1 - y0);
	if (y1 > y0) incy = 1;
	else incy = -1;
	distance = (delta_x > delta_y) ? delta_x : delta_y;
	tt = 0;
	xerr = 0;
	yerr = 0;

	/* Bresenham 直线绘制主循环 */
	for (;;) {
		vw_point_xy(dc, x0, y0);
		xerr += delta_x;
		yerr += delta_y;
		if (xerr >= distance) {
			xerr -= distance;
			x0++;
		}
		if (yerr >= distance) {
			yerr -= distance;
			y0 += incy;
		}
		tt++;
		if (distance < tt) break;
	}
}

/**
 * @brief 绘制直线
 *
 * 在两点之间绘制一条直线
 *
 * @param dc 目标 VDC
 * @param x0 起点 X 坐标
 * @param y0 起点 Y 坐标
 * @param x1 终点 X 坐标
 * @param y1 终点 Y 坐标
 */
void vw_DrawLine(VDC* dc, int x0, int y0, int x1, int y1)
{
	DC_ADDORG(x0, y0);  /* 添加起点原点偏移 */
	DC_ADDORG(x1, y1);  /* 添加终点原点偏移 */
	vw_line(dc, x0, y0, x1, y1);
}

/**
 * @brief 填充矩形
 *
 * 用前景色填充一个矩形区域
 *
 * @param dc 目标 VDC
 * @param x0 矩形左上角 X 坐标
 * @param y0 矩形左上角 Y 坐标
 * @param x1 矩形右下角 X 坐标
 * @param y1 矩形右下角 Y 坐标
 */
void vw_FillRect(VDC* dc, int x0, int y0, int x1, int y1)
{
	int t;

	DC_ADDORG(x0, y0);  /* 添加起点原点偏移 */
	DC_ADDORG(x1, y1);  /* 添加终点原点偏移 */

	/* 规范化坐标：确保 x0 <= x1, y0 <= y1 */
	if (y0 > y1) {
		t = y0;
		y0 = y1;
		y1 = t;
	}
	if (x0 > x1) {
		t = x0;
		x0 = x1;
		x1 = t;
	}

	/* 快速剔除完全在剪切区域外的情况 */
	if (x0 > dc->clip.x1 || x1 < dc->clip.x0) return;
	if (y0 > dc->clip.y1 || y1 < dc->clip.y0) return;

	/* 裁剪到剪切区域 */
	if (x0 < dc->clip.x0) x0 = dc->clip.x0;
	if (x1 > dc->clip.x1) x1 = dc->clip.x1;
	if (y0 < dc->clip.y0) y0 = dc->clip.y0;
	if (y1 > dc->clip.y1) y1 = dc->clip.y1;

	/* 逐行填充 */
	while (y0 <= y1) {
		vw_hline(dc, x0, x1, y0);
		y0++;
	}
}

/**
 * @brief 绘制矩形边框
 *
 * 绘制矩形的四条边（不填充）
 *
 * @param dc 目标 VDC
 * @param x0 矩形左上角 X 坐标
 * @param y0 矩形左上角 Y 坐标
 * @param x1 矩形右下角 X 坐标
 * @param y1 矩形右下角 Y 坐标
 */
void vw_DrawRect(VDC* dc, int x0, int y0, int x1, int y1)
{
	DC_ADDORG(x0, y0);  /* 添加起点原点偏移 */
	DC_ADDORG(x1, y1);  /* 添加终点原点偏移 */

	/* 绘制四条边 */
	vw_hline_check(dc, x0, x1, y0);  /* 上边 */
	vw_hline_check(dc, x0, x1, y1);  /* 下边 */
	vw_vline(dc, x0, y0, y1);         /* 左边 */
	vw_vline(dc, x1, y0, y1);         /* 右边 */
}

/**
 * @brief 填充圆形
 *
 * 用前景色填充一个圆形区域
 * 使用优化的扫描线填充算法
 *
 * @param dc 目标 VDC
 * @param x0 圆心 X 坐标
 * @param y0 圆心 Y 坐标
 * @param r  半径
 */
void vw_FillCircle(VDC* dc, int x0, int y0, int r)
{
	int i;
	int imax = r * 707 / 1000 + 1;    /* 最大迭代次数，约 r * sqrt(2)/2 */
	int sqmax = r * r + r / 2;         /* 半径平方的阈值 */
	int x = r;

	DC_ADDORG(x0, y0);

	/* 先绘制中间的水平线 */
	vw_hline_check(dc, x0 - r, x0 + r, y0);

	/* 从内向外逐层扫描 */
	for (i = 1;i <= imax;i++) {
		if ((i * i + x * x) > sqmax) {
			/* 绘制外层水平线 */
			if (x > imax) {
				vw_hline_check(dc, x0 - i + 1, x0 + i - 1, y0 + x);
				vw_hline_check(dc, x0 - i + 1, x0 + i - 1, y0 - x);
			}
			x--;
		}
		/* 绘制内层水平线（从中心向外） */
		vw_hline_check(dc, x0 - x, x0 + x, y0 + i);
		vw_hline_check(dc, x0 - x, x0 + x, y0 - i);
	}
}

/**
 * @brief 绘制圆的 8 个对称点
 *
 * 利用圆的八对称性，从一点生成八个点
 * 这是 Bresenham 圆算法的核心
 *
 * @param dc   目标 VDC
 * @param x0   圆心 X 坐标
 * @param y0   圆心 Y 坐标
 * @param xoff X 偏移量
 * @param yoff Y 偏移量
 */
static void Draw8Point(VDC* dc, int x0, int y0, int xoff, int yoff)
{
	/* 第一象限 */
	vw_point_xy(dc, x0 + xoff, y0 + yoff);
	vw_point_xy(dc, x0 - xoff, y0 + yoff);
	/* 利用对称性绘制其他点 */
	vw_point_xy(dc, x0 + yoff, y0 + xoff);
	vw_point_xy(dc, x0 + yoff, y0 - xoff);
	/* 当 yoff 不为 0 时，绘制剩余四个点 */
	if (yoff) {
		vw_point_xy(dc, x0 + xoff, y0 - yoff);
		vw_point_xy(dc, x0 - xoff, y0 - yoff);
		vw_point_xy(dc, x0 - yoff, y0 + xoff);
		vw_point_xy(dc, x0 - yoff, y0 - xoff);
	}
}

/**
 * @brief 绘制圆形边框
 *
 * 使用 Bresenham 算法绘制圆形边框
 *
 * @param dc 目标 VDC
 * @param x0 圆心 X 坐标
 * @param y0 圆心 Y 坐标
 * @param r  半径
 */
void vw_DrawCircle(VDC* dc, int x0, int y0, int r)
{
	int i;
	int imax = r * 707 / 1000 + 1;    /* 最大迭代次数 */
	int sqmax = r * r + r / 2;         /* 半径平方的阈值 */
	int y = r;

	DC_ADDORG(x0, y0);

	/* 绘制起始点（右端点） */
	Draw8Point(dc, x0, y0, r, 0);

	/* 从上到下逐点绘制 */
	for (i = 1;i <= imax;i++) {
		if ((i * i + y * y) > sqmax) {
			Draw8Point(dc, x0, y0, i, y);
			y--;
		}
		Draw8Point(dc, x0, y0, i, y);
	}
}

/**
 * @brief 填充椭圆
 *
 * 用前景色填充一个椭圆区域
 *
 * @param dc 目标 VDC
 * @param x0 椭圆中心 X 坐标
 * @param y0 椭圆中心 Y 坐标
 * @param rx X 方向半轴长度
 * @param ry Y 方向半轴长度
 */
void vw_FillEllipse(VDC* dc, int x0, int y0, int rx, int ry)
{
	int OutConst, Sum, SumY;
	int x, y;
	unsigned int _rx = rx;
	unsigned int _ry = ry;

	DC_ADDORG(x0, y0);

	/* 计算椭圆方程的常数项 */
	OutConst = _rx * _rx * _ry * _ry       /* 常数项 */
		+ (_rx * _rx * _ry >> 1);           /* 补偿四舍五入 */
	x = rx;

	/* 从上到下逐行扫描 */
	for (y = 0;y <= ry;y++) {
		SumY = ((rx * rx)) * ((y * y));     /* Y 相关项，循环内不变 */
		/* 找到当前 y 值对应的 x 边界 */
		while (Sum = SumY + ((ry * ry)) * ((x * x)),
			(x > 0) && (Sum > OutConst)) {
			x--;
		}
		/* 绘制水平线 */
		vw_hline_check(dc, x0 - x, x0 + x, y0 + y);
		if (y)
			vw_hline_check(dc, x0 - x, x0 + x, y0 - y);
	}
}

/**
 * @brief 绘制椭圆的 4 个对称点
 *
 * 利用椭圆的四对称性，从一点生成四个点
 *
 * @param dc 目标 VDC
 * @param x0 椭圆中心 X 坐标
 * @param y0 椭圆中心 Y 坐标
 * @param x  X 偏移量
 * @param y  Y 偏移量
 */
static void vw_put_dot4(VDC* dc, int x0, int y0, int x, int y)
{
	if (x == 0) {
		/* X 为 0 时，只有上下两点 */
		vw_point_xy(dc, x0, y0 + y);
		vw_point_xy(dc, x0, y0 - y);
	}
	else if (y == 0) {
		/* Y 为 0 时，只有左右两点 */
		vw_point_xy(dc, x0 + x, y0);
		vw_point_xy(dc, x0 - x, y0);
	}
	else {
		/* 一般情况：四个对称点 */
		vw_point_xy(dc, x0 + x, y0 + y);
		vw_point_xy(dc, x0 - x, y0 + y);
		vw_point_xy(dc, x0 + x, y0 - y);
		vw_point_xy(dc, x0 - x, y0 - y);
	}
}

/**
 * @brief 绘制椭圆边框
 *
 * 使用中点椭圆算法绘制椭圆边框
 *
 * @param dc 目标 VDC
 * @param x0 椭圆中心 X 坐标
 * @param y0 椭圆中心 Y 坐标
 * @param r1 X 方向半轴长度
 * @param r2 Y 方向半轴长度
 */
void vw_DrawEllipse(VDC* dc, int x0, int y0, int r1, int r2)
{
	int fxy, fx, fy, incx, incy, temp_x, temp_y;
	unsigned short delta_x, delta_y, distant_a, distant_b, circle_r, dot_start;

	DC_ADDORG(x0, y0);

	distant_a = r1;  /* 长轴 */
	distant_b = r2;  /* 短轴 */
	dot_start = 0;

	/* 特殊情况：点 */
	if (distant_a == 0 && distant_a == 0) {
		vw_point_xy(dc, x0, y0);
		return;
	}

	/* 初始化中点椭圆算法参数 */
	circle_r = (distant_a > distant_b) ? distant_a : distant_b;
	incx = -1;
	incy = 1;
	fy = 1;
	fx = 1 - 2 * circle_r;
	fxy = 0;
	delta_x = 0;
	delta_y = 0;
	temp_x = distant_a;
	temp_y = 0;

	/* 绘制起始点 */
	vw_put_dot4(dc, x0, y0, temp_x, temp_y);

	/* 中点椭圆算法主循环 */
	do {
		if (fxy >= 0) {
			delta_x += distant_a;
			if (delta_x >= circle_r) {
				temp_x += incx;
				delta_x -= circle_r;
				if (temp_x + 1 != distant_a)
					vw_put_dot4(dc, x0, y0, temp_x, temp_y);
			}
			fxy -= abs(fx);
			fx += 2;
			if (fx < 0 || fx >= 3) continue;
			incy = -incy;
			fy = -fy + 2;
			fxy = -fxy;
		}
		else {
			delta_y += distant_b;
			if (delta_y >= circle_r) {
				delta_y -= circle_r;
				temp_y += incy;
				if ((temp_y == 1 || temp_y == 2) && dot_start == 0) {
					vw_put_dot4(dc, x0, y0, distant_a, temp_y);
				}
				else {
					dot_start = 1;
					vw_put_dot4(dc, x0, y0, temp_x, temp_y);
				}
			}
			fxy = fxy + abs(fy);
			fy += 2;
			if (fy < 0 || fy>2) continue;
			incx = -incx;
			fx = -fx + 2;
			fxy = -fxy;
		}
	} while (temp_x);
}

/**
 * @brief 显示字符位图到字体缓冲区
 *
 * 将字符的位图数据渲染到 FontMem 缓冲区
 *
 * @param x         缓冲区中的起始 X 坐标
 * @param y         缓冲区中的起始 Y 坐标
 * @param color     字符颜色
 * @param font_data 字体位图数据指针
 * @param size      位图数据字节数
 * @param w         字符宽度
 * @param h         字符高度
 */
static void showchar(int x, int y, unsigned int color, unsigned char* font_data, int size, int w, int h)
{
	int row, col;
	unsigned short pattern; // Changed to short to handle 2-byte rows
	int x1, y1;

	/* For each row */
	for (row = 0; row < h; row++) {
		/* Determine how many bytes per row based on width */
		if (w <= 8) {
			// 1 byte per row for 8 pixels or less
			pattern = font_data[row];
		} else {
			// 2 bytes per row for more than 8 pixels (Chinese characters)
			pattern = (font_data[row * 2] << 8) | font_data[row * 2 + 1];
		}
		y1 = y + row;
		x1 = x;
		/* For each column in this row */
		for (col = 0; col < w; col++) {
			/* Check the correct bit */
			int bit_pos = (w <= 8) ? (7 - col) : (15 - col);
			if (pattern & (1 << bit_pos)) {
				FontMem[y1 * MAX_FONT_SIZE + x1] = color;
			}
			x1++;
		}
	}
}

/**
 * @brief 为字符添加边框效果
 *
 * 在已渲染的字符周围添加边框
 * 用于 FONT_BORDER 字体效果
 *
 * @param w        字符宽度
 * @param h        字符高度
 * @param fgcolor  前景色（字符颜色）
 * @param border   边框颜色
 */
static void drawborderchar(int w, int h, unsigned int fgcolor, unsigned int border)
{
	int x, y;
	unsigned int* p;

	/* 扫描每个字符像素 */
	for (y = 1;y <= h;y++) {
		for (x = 1;x <= w;x++) {
			if (FontMem[y * MAX_FONT_SIZE + x] == fgcolor) {
				/* 在字符像素周围添加边框像素 */
				/* 上边三个像素 */
				p = &FontMem[(y - 1) * MAX_FONT_SIZE + (x - 1)];
				if (*p != fgcolor) *p = border;
				if (*++p != fgcolor) *p = border;
				if (*++p != fgcolor) *p = border;
				/* 左右两个像素 */
				p = &FontMem[(y)*MAX_FONT_SIZE + (x - 1)];
				if (*p != fgcolor) *p = border;
				p += 2;
				if (*p != fgcolor) *p = border;
				/* 下边三个像素 */
				p = &FontMem[(y + 1) * MAX_FONT_SIZE + (x - 1)];
				if (*p != fgcolor) *p = border;
				if (*++p != fgcolor) *p = border;
				if (*++p != fgcolor) *p = border;
			}
		}
	}
}

/**
 * @brief 斜体字的倾斜偏移表
 *
 * 用于实现斜体效果，根据行号确定水平偏移量
 * 表中的值使得字符呈现向右倾斜的效果
 */
static const unsigned char tilt_table[32] = {
	0,0,0,0,0,1,1,1,1,2,2,2,2,2,3,3,
	3,3,4,4,4,4,5,5,5,5,5,6,6,6,6,7
};

/**
 * @brief 获取字体高度
 *
 * @param font 字体类型（目前固定返回 16）
 * @return 字体高度
 */
static int GetFontHeight_os(int font)
{
	if (font == FONT_SMALL) {
		return 12;
	}
	return 16;
}

/**
 * @brief 获取字符的字体数据
 *
 * 根据 GB2312 编码获取字符的位图数据
 *
 * @param c          字符编码（ASCII 或 GB2312）
 * @param font       字体类型
 * @param ppCharData 输出：字体数据指针
 * @param width      输出：字符宽度
 * @param height     输出：字符高度
 * @return 字体数据的字节数
 */
static unsigned char small_font_buf[24];
static int GetFontData_os(unsigned short c, int font, unsigned char** ppCharData, int* width, int* height)
{
	unsigned char* p;
	unsigned char c1, c2;
	int index, i;
	// fprintf(stderr, "[GetFontData_os] c=0x%04X (%c), font=%d\n", c, (c < 128) ? (char)c : '?', font);

	if (font == FONT_SMALL) {
		*height = 12;
		if (c < 0x80) {
			p = ascii + c * 12;
			*width = 6;
			*ppCharData = p;
			// fprintf(stderr, "[GetFontData_os] FONT_SMALL: ASCII c=%d (0x%02X), data: ", c, c);
			// for (i = 0; i < 12; i++) fprintf(stderr, "%02X ", p[i]);
			// fprintf(stderr, "\n");
			return 12;
		} else {
			c1 = c >> 8;
			c2 = (unsigned char)c;
			if (c1 < 0xa1 || c1 > 0xfe || c2 < 0xa1 || c2 > 0xfe) {
				*width = 6;
				*ppCharData = ascii + '?' * 12;
				// fprintf(stderr, "[GetFontData_os] FONT_SMALL: Invalid GB c1=0x%02X, c2=0x%02X\n", c1, c2);
				return 12;
			}
			if (c1 < 0xb0) {
				index = (c1 - 0xa1) * 94 + (c2 - 0xa1);
			} else {
				index = (c1 - 0xa7) * 94 + (c2 - 0xa1);
			}
			p = gbfont + index * 24;
			*width = 12;
			*ppCharData = p;
			// fprintf(stderr, "[GetFontData_os] FONT_SMALL: GB c1=0x%02X, c2=0x%02X, index=%d, data: ", c1, c2, index);
			//for (i = 0; i < 24; i++) fprintf(stderr, "%02X ", p[i]);
			//fprintf(stderr, "\n");
			return 24;
		}
	} else {
		*height = 16;
		if (c < 0x80) {
			p = ascii8 + c * 16;
			*width = 8;
			*ppCharData = p;
			// fprintf(stderr, "[GetFontData_os] FONT_MEDIUM: ASCII c=%d (0x%02X), p=%p, data: ", c, c, p);
			// for (i = 0; i < 16; i++) fprintf(stderr, "%02X ", p[i]);
			// fprintf(stderr, "\n");
			return 16;
		} else {
			c1 = c >> 8;
			c2 = (unsigned char)c;
			if (c1 < 0xa1 || c1 > 0xfe || c2 < 0xa1 || c2 > 0xfe) {
				*width = 8;
				*ppCharData = ascii8 + '?' * 16;
				// fprintf(stderr, "[GetFontData_os] FONT_MEDIUM: Invalid GB c1=0x%02X, c2=0x%02X\n", c1, c2);
				return 16;
			}
			if (c1 < 0xb0) {
				index = (c1 - 0xa1) * 94 + (c2 - 0xa1);
			} else {
				index = (c1 - 0xa7) * 94 + (c2 - 0xa1);
			}
			p = gbfont16 + index * 32;
			*width = 16;
			*ppCharData = p;
			// fprintf(stderr, "[GetFontData_os] FONT_MEDIUM: GB c1=0x%02X, c2=0x%02X, index=%d, p=%p, data: ", c1, c2, index, p);
			// for (i = 0; i < 32; i++) fprintf(stderr, "%02X ", p[i]);
			// fprintf(stderr, "\n");
			return 32;
		}
	}
}

/**
 * @brief 渲染单个字符到目标 VDC
 *
 * 将字符渲染到目标 VDC 的指定位置
 * 支持多种字体效果（粗体、斜体、下划线、边框）
 *
 * @param dc    目标 VDC
 * @param c     字符编码（ASCII 或 GB2312）
 * @param x     目标 X 坐标
 * @param y     目标 Y 坐标
 * @param mode  绘制模式
 * @param pw    输出：字符宽度
 */
static void DispFont(VDC* dc, unsigned short c, int x, int y, int mode, int* pw)
{
	unsigned int fgcolor, bgcolor;
	int i, w, h, num, font, hx;
	unsigned char* addr;

	fgcolor = dc->fgcolor;
	bgcolor = dc->bgcolor;
	font = dc->font;

	/* 精灵模式或边框字体：选择一个不与前景色/边框色冲突的背景色 */
	if (mode == DRAW_SPRITE || (font & FONT_BORDER)) {
		for (i = 0;i < 0x100;i++) {
			if (i != fgcolor && i != dc->bordercolor) {
				bgcolor = i;
				break;
			}
		}
		FontDc.keycolor = bgcolor;
	}

	/* 获取字体信息 */
	hx = GetFontHeight_os(font);
	num = GetFontData_os(c, font, &addr, &w, &h);

	/* 清空字体缓冲区并渲染字符 */
	wmemset(&FontMem[MAX_FONT_SIZE], bgcolor, hx * MAX_FONT_SIZE);
	showchar(1, hx - h + 1, fgcolor, addr, num, w, h);

	/* 处理粗体效果：再次渲染一个像素偏移 */
	if (font & FONT_BOLD) {
		showchar(2, hx - h + 1, fgcolor, addr, num, w, h);
		w++;
	}
	*pw = w;

	/* 处理边框效果 */
	if (font & FONT_BORDER) {
		FontDc.keycolor = bgcolor;
		/* 清空边框区域 */
		wmemset(FontMem, bgcolor, MAX_FONT_SIZE);
		wmemset(&FontMem[(hx + 1) * MAX_FONT_SIZE], bgcolor, MAX_FONT_SIZE);
		/* 绘制边框 */
		drawborderchar(w, hx, fgcolor, dc->bordercolor);
		/* 输出到目标 VDC */
		if (font & FONT_ITALIC) {
			/* 斜体+边框：逐行倾斜输出 */
			for (i = 0;i < hx + 2;i++)
				vw_bitblt(dc, x + tilt_table[hx + 1 - i] - 1, y + i - 1, w + 2, 1, &FontDc, 0, i, DRAW_SPRITE);
		}
		else
			vw_bitblt(dc, x - 1, y - 1, w + 2, hx + 2, &FontDc, 0, 0, DRAW_SPRITE);
	}
	else {
		/* 无边框模式 */
		/* 处理下划线效果 */
		if (font & FONT_UNDERLINE)
			wmemset(&FontMem[(hx - 1) * MAX_FONT_SIZE + 1], fgcolor, w);

		/* 输出到目标 VDC */
		if (font & FONT_ITALIC) {
			/* 斜体：逐行倾斜输出 */
			for (i = 0;i < hx;i++)
				vw_bitblt(dc, x + tilt_table[hx - i], y + i, w, 1, &FontDc, 1, i + 1, mode);
		}
		else
			vw_bitblt(dc, x, y, w, hx, &FontDc, 1, 1, mode);
	}
}

/**
 * @brief 从 GB2312 编码字符串中获取一个字符
 *
 * 支持混合 ASCII 和 GB2312 编码的字符串
 *
 * @param ps 指向字符串指针的指针（会被更新）
 * @return 字符编码（ASCII 或 GB2312）
 */
static unsigned short i_GetUniFromGbx(const char** ps)
{
	unsigned char* p, c, c2;

	p = (unsigned char*)*ps;
	c = *p++;
	if (c == 0)
		return 0;

	/* ASCII 字符 */
	if (c < 0x80) {
		(*ps)++;
		return c;
	}

	/* GB2312 汉字 */
	c2 = *p++;
	if (c >= 0xa1 && c <= 0xfe && c2 >= 0xa1 && c2 <= 0xfe) {
		(*ps) += 2;
		return (c << 8) | c2;
	}

	/* 无效编码：返回问号 */
	(*ps)++;
	return '?';
}

/**
 * @brief 输出文本字符串
 *
 * 在指定位置输出文本字符串
 * 支持混合 ASCII 和 GB2312 编码
 *
 * @param dc     目标 VDC
 * @param font_x 起始 X 坐标
 * @param font_y 起始 Y 坐标
 * @param p      文本字符串指针
 * @param num    字符数限制（-1 表示不限制）
 * @param mode   绘制模式
 */
void vw_TextOut(VDC* dc, int font_x, int font_y, const char* p, int num, int mode)
{
	const char* pe;
	int w;
	unsigned int original_font = dc->font; /* Save original font */
	//fprintf(stderr, "[vw_TextOut] font_x=%d, font_y=%d, str=\"%s\", num=%d, mode=0x%02X\n", font_x, font_y, p, num, mode);

	DC_ADDORG(font_x, font_y);

	/* Select font size based on mode bit 0x80 (like lavaVM):
	 * - (mode & 0x80) == 0 → FONT_SMALL (6x12/12x12)
	 * - (mode & 0x80) != 0 → FONT_MEDIUM (8x16/16x16)
	 */
	if (mode & 0x80) {
		dc->font = FONT_MEDIUM;
		//fprintf(stderr, "[vw_TextOut] mode bit 0x80 set: using FONT_MEDIUM\n");
	} else {
		dc->font = FONT_SMALL;
		//fprintf(stderr, "[vw_TextOut] mode bit 0x80 not set: using FONT_SMALL\n");
	}

	/* 确定字符串结束位置 */
	if (num >= 0)
		pe = p + num;
	else
		pe = (char*)-1;

	/* 逐字符输出 */
	while (*p && p < pe) {
		if (font_x >= dc->width) {
			//fprintf(stderr, "[vw_TextOut] font_x (%d) >= dc->width (%d): breaking\n", font_x, dc->width);
			break;
		}
		unsigned short ch = i_GetUniFromGbx(&p);
		//fprintf(stderr, "[vw_TextOut] Drawing character 0x%04X at (%d, %d)\n", ch, font_x, font_y);
		DispFont(dc, ch, font_x, font_y, mode, &w);
		//fprintf(stderr, "[vw_TextOut] Character width: %d\n", w);
		font_x += w;
	}

	dc->font = original_font; /* Restore original font */
	//fprintf(stderr, "[vw_TextOut] Done\n");
}

/**
 * @brief 内部清除矩形区域函数
 *
 * 用背景色填充矩形区域
 *
 * @param dc 目标 VDC
 * @param x0 矩形左上角 X 坐标
 * @param y0 矩形左上角 Y 坐标
 * @param x1 矩形右下角 X 坐标
 * @param y1 矩形右下角 Y 坐标
 */
static void vw_clearrect(VDC* dc, int x0, int y0, int x1, int y1)
{
	int t;

	/* 规范化坐标 */
	if (y0 > y1) {
		t = y0;
		y0 = y1;
		y1 = t;
	}
	if (x0 > x1) {
		t = x0;
		x0 = x1;
		x1 = t;
	}

	/* 剪切区域检测 */
	if (x0 > dc->clip.x1 || x1 < dc->clip.x0) return;
	if (y0 > dc->clip.y1 || y1 < dc->clip.y0) return;

	/* 裁剪到剪切区域 */
	if (x0 < dc->clip.x0) x0 = dc->clip.x0;
	if (x1 > dc->clip.x1) x1 = dc->clip.x1;
	if (y0 < dc->clip.y0) y0 = dc->clip.y0;
	if (y1 > dc->clip.y1) y1 = dc->clip.y1;

	/* 逐行清除 */
	while (y0 <= y1) {
		vw_clearline(dc, x0, x1, y0);
		y0++;
	}
}

/**
 * @brief 清除矩形区域
 *
 * 用背景色填充指定的矩形区域
 *
 * @param dc 目标 VDC
 * @param x0 矩形左上角 X 坐标
 * @param y0 矩形左上角 Y 坐标
 * @param x1 矩形右下角 X 坐标
 * @param y1 矩形右下角 Y 坐标
 */
void vw_ClearRect(VDC* dc, int x0, int y0, int x1, int y1)
{
	DC_ADDORG(x0, y0);
	DC_ADDORG(x1, y1);
	vw_clearrect(dc, x0, y0, x1, y1);
}

/**
 * @brief 清除剪切区域
 *
 * 用背景色填充整个剪切区域
 *
 * @param dc 目标 VDC
 */
void vw_ClearArea(VDC* dc)
{
	int x0, y0, x1, y1;

	x0 = dc->clip.x0;
	y0 = dc->clip.y0;
	x1 = dc->clip.x1;
	y1 = dc->clip.y1;

	while (y0 <= y1) {
		vw_clearline(dc, x0, x1, y0);
		y0++;
	}
}

/**
 * @brief 设置原点偏移
 *
 * 设置 VDC 的原点坐标，后续所有绘图操作的坐标都会加上此偏移
 * 用于实现窗口滚动等效果
 *
 * @param dc 目标 VDC
 * @param x  原点 X 偏移
 * @param y  原点 Y 偏移
 */
void vw_SetOrg(VDC* dc, int x, int y)
{
	dc->org_x = x;
	dc->org_y = y;
}

/**
 * @brief 设置绘图区域
 *
 * 设置 VDC 的原点和剪切区域
 * 原点设为 (x0, y0)，剪切区域为 (x0, y0) 到 (x1, y1)
 *
 * @param dc 目标 VDC
 * @param x0 区域左上角 X 坐标
 * @param y0 区域左上角 Y 坐标
 * @param x1 区域右下角 X 坐标
 * @param y1 区域右下角 Y 坐标
 */
void vw_SetArea(VDC* dc, int x0, int y0, int x1, int y1)
{
	int t;

	/* 规范化坐标 */
	if (y0 > y1) {
		t = y0;
		y0 = y1;
		y1 = t;
	}
	if (x0 > x1) {
		t = x0;
		x0 = x1;
		x1 = t;
	}

	/* 设置原点 */
	dc->org_x = x0;
	dc->org_y = y0;

	/* 检查是否完全在 VDC 外 */
	if (x1 < 0 || x0 >= dc->width || y1 < 0 || y0 >= dc->height) {
		dc->clip.x0 = dc->clip.x1 = dc->clip.y0 = dc->clip.y1 = 0;
		return;
	}

	/* 裁剪到 VDC 边界 */
	if (x0 < 0) x0 = 0;
	if (x1 >= dc->width) x1 = dc->width - 1;
	if (y0 < 0) y0 = 0;
	if (y1 >= dc->height) y1 = dc->height - 1;

	/* 设置剪切区域 */
	dc->clip.x0 = x0;
	dc->clip.x1 = x1;
	dc->clip.y0 = y0;
	dc->clip.y1 = y1;
}

/**
 * @brief 初始化图形库
 *
 * 初始化字体渲染专用 VDC
 * 在使用图形库前必须调用
 */
void vw_Init()
{
	FontDcInit();
}

/**
 * @brief 设置前景色
 *
 * @param dc    目标 VDC
 * @param color 新的前景色
 * @return 旧的前景色
 */
int vw_SetFgColor(VDC* dc, u32 color)
{
	u32 t = dc->fgcolor;
	dc->fgcolor = color;
	return t;
}

/**
 * @brief 设置背景色
 *
 * @param dc    目标 VDC
 * @param color 新的背景色
 * @return 旧的背景色
 */
int vw_SetBgColor(VDC* dc, u32 color)
{
	u32 t = dc->bgcolor;
	dc->bgcolor = color;
	return t;
}

/**
 * @brief 设置透明色
 *
 * 透明色用于精灵模式（DRAW_SPRITE）绘制
 *
 * @param dc    目标 VDC
 * @param color 新的透明色
 * @return 旧的透明色
 */
int vw_SetKeyColor(VDC* dc, u32 color)
{
	u32 t = dc->keycolor;
	dc->keycolor = color;
	return t;
}

/**
 * @brief 设置绘图模式
 *
 * @param dc   目标 VDC
 * @param mode 绘图模式（DRAW_COPY, DRAW_XOR, DRAW_BROKEN 等）
 */
void vw_SetDrawMode(VDC* dc, int mode)
{
	dc->draw_mode = mode;
}

/**
 * @brief 设置字体属性
 *
 * 字体属性可以是以下标志的组合：
 *   - FONT_MEDIUM: 中等字体
 *   - FONT_SMALL:  小字体
 *   - FONT_LARGE:  大字体
 *   - FONT_BOLD:   粗体
 *   - FONT_ITALIC: 斜体
 *   - FONT_UNDERLINE: 下划线
 *   - FONT_BORDER: 边框
 *
 * @param dc   目标 VDC
 * @param font 字体属性
 */
void vw_SetFont(VDC* dc, int font)
{
	dc->font = font;
}
