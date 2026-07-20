/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2026, SDLPAL development team.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 3
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef FONT_H
#define FONT_H

#include "common.h"
#include "palcommon.h"
#include "palcfg.h"

PAL_C_LINKAGE_BEGIN

extern char *font_offset_x;
extern char *font_offset_y;

/*++
 作用:

 初始化font 子系统.

 参数:

 [输入] cfg - Pointerconfiguration object.

 返回值:

 0 = success, -1 = failure.
--*/
int
PAL_InitFont(
	const CONFIGURATION* cfg
);

void
PAL_FreeFont(
	void
);

/*++
 作用:

 绘制一个Unicode character在一个surface.

 参数:

 [输入] wChar - unicode characterbe drawn.

 [输出] lpSurface - destination surface.

 [输入] pos - destination location的surface.

 [输入] bColor - color的character.

 返回值:

 无。

--*/
void
PAL_DrawCharOnSurface(
	uint16_t                 wChar,
	SDL_Surface             *lpSurface,
	PAL_POS                  pos,
	uint8_t                  bColor,
	BOOL                     fUse8x8Font
);

/*++
 作用:

 获取text width的一个character.

 参数:

 [输入] wChar - unicode character用于width calculation.

 返回值:

 The width的character在pixels, 16用于full-width char和8用于half-width char.

--*/
int
PAL_CharWidth(
	uint16_t                 wChar
);

/*++
 作用:

 获取height的currently used font.

 参数:

 无。

 返回值:

 The height的font在pixels.

--*/
int
PAL_FontHeight(
	void
);

PAL_C_LINKAGE_END

#endif
