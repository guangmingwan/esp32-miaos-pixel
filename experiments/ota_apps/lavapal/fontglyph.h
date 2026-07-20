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
// fontglyph.h: Unicode font glyph definition file, converted from GNU Unifont.
//     @Author: Lou Yihua <louyihua@21cn.com>, 2015-06-07.
//

#ifndef _FONTGLYPH_H
#define _FONTGLYPH_H

/* On ESP32-S3 with ESP-IDF 4.4, the BSS segment cannot be placed in PSRAM,
 * so the 2 MB unicode_font + 64 KB font_width tables would overflow
 * internal DRAM. The tables are heap-allocated from PSRAM at startup
 * (see font.c PAL_FontManagerInit). */
#define UNICODE_FONT_SIZE 65536

#ifndef _FONT_C
#error "This file should only be included inside font.c!"
#endif

extern unsigned char (*unicode_font)[32];
extern unsigned char *font_width;

static const int unicode_lower_top  = 0xd800;
static const int unicode_upper_base = 0xf900;
static const int unicode_upper_top  = 65534;

#endif
