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

#ifndef PAL_MIDI_H
#define PAL_MIDI_H

#include "common.h"
#include "native_midi/native_midi.h"

/*++
  作用:

    设置MIDI音乐的音量。

  参数:

    [输入]  iVolume - 音量，范围 0-PAL_MAX_VOLUME。

  返回值:

    无。

--*/
PAL_C_LINKAGE
void
MIDI_SetVolume(
	int       iVolume
);

/*++
  作用:

    开始播放指定的MIDI格式音乐。

  参数:

    [输入]  iNumRIX - 音乐编号。为0时停止当前播放的音乐。

    [输入]  fLoop - 音乐是否应该循环播放。

  返回值:

    无。

--*/
PAL_C_LINKAGE
void
MIDI_Play(
	int       iNumRIX,
	BOOL      fLoop
);

PAL_C_LINKAGE
void
MIDI_FillBuffer(
    LPBYTE      stream,
    INT         len
);

PAL_C_LINKAGE
void
MIDI_Shutdown(
);
#endif
