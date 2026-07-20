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

#ifndef UTIL_H
#define UTIL_H

#include "common.h"
#include "palcommon.h"

PAL_C_LINKAGE_BEGIN

void
UTIL_MsgBox(
   char *string
);

long
flength(
   FILE *fp
);

void
trim(
   char *str
);

char *
UTIL_GlobalBuffer(
	int         index
);
#define PAL_BUFFER_SIZE_ARGS(i) UTIL_GlobalBuffer(i), PAL_GLOBAL_BUFFER_SIZE

/*++
 作用:

 Does 一个varargs printf into user-supplied buffer,
	so we don't need tovarargs versions的all text functions.

 参数:

 buffer - user-supplied 缓冲区。
	buflen - size的缓冲区, including null-terminator.
 format - format string.

 返回值:

 The value的buffer 如果buffernon-NULL和buflen > 0, otherwise NULL.

--*/
char *
UTIL_va(
	char       *buffer,
	int         buflen,
	const char *format,
	...
);
#define PAL_va(i, fmt, ...) UTIL_va(UTIL_GlobalBuffer(i), PAL_GLOBAL_BUFFER_SIZE, fmt, __VA_ARGS__)

int
RandomLong(
   int from,
   int to
);

float
RandomFloat(
   float from,
   float to
);

void
UTIL_Delay(
   unsigned int ms
);

void
TerminateOnError(
   const char *fmt,
   ...
);

void *
UTIL_malloc(
   size_t               buffer_size
);

void *
UTIL_calloc(
   size_t               n,
   size_t               size
);

FILE *
UTIL_OpenRequiredFile(
   LPCSTR               lpszFileName
);

FILE *
UTIL_OpenRequiredFileForMode(
   LPCSTR               lpszFileName,
   LPCSTR               szMode
);

FILE *
UTIL_OpenFile(
   LPCSTR               lpszFileName
);

FILE *
UTIL_OpenFileForMode(
   LPCSTR               lpszFileName,
   LPCSTR               szMode
);

FILE *
UTIL_OpenFileAtPath(
	LPCSTR              lpszPath,
	LPCSTR              lpszFileName
);

/*++
 作用:

 打开一个file在desired mode在specific path.
	If fails, return NULL.

 参数:

 [输入] lpszPath - pathlocate 文件。
 [输入] lpszFileName - file nameopen.
 [输入] szMode - file open mode.

 返回值:

 Pointer文件。

--*/
FILE *
UTIL_OpenFileAtPathForMode(
	LPCSTR              lpszPath,
	LPCSTR              lpszFileName,
	LPCSTR              szMode
);

VOID
UTIL_CloseFile(
   FILE                *fp
);

/*++
 作用:

 Combine 'dir'和'file' part into 一个single path string.
	If 'dir'non-NULL, then it ensures that output string contains
	'/' between 'dir'和'file' (no matter whether 'file'NULL或not).

 参数:

 buffer - user-supplied 缓冲区。
	buflen - size的缓冲区, including null-terminator.
 dir - directory path.
	file - 文件路径。

 返回值:

 The value的buffer 如果buffernon-NULL和buflen > 0, otherwise NULL.

--*/
const char *
UTIL_CombinePath(
	char       *buffer,
	size_t      buflen,
	int         numentry,
	...
);
#define PAL_CombinePath(i, d, f) UTIL_CombinePath(UTIL_GlobalBuffer(i), PAL_GLOBAL_BUFFER_SIZE, 2, (d), (f))

BOOL
UTIL_IsFileExist(
    const char *path
);

const char *
UTIL_GetFullPathName(
	char       *buffer,
	size_t      buflen,
	const char *basepath,
	const char *subpath
);

PALFILE
UTIL_CheckResourceFiles(
	const char *path,
	const char *msgfile
);

char *UTIL_basename(const char *path);

/*
 * Platform-specific utilities
 */

BOOL
UTIL_GetScreenSize(
	DWORD *pdwScreenWidth,
	DWORD *pdwScreenHeight
);

BOOL
UTIL_IsAbsolutePath(
	const char *lpszFileName
);

int
UTIL_Platform_Startup(
	int   argc,
	char *argv[]
);

int
UTIL_Platform_Init(
	int   argc,
	char *argv[]
);

void
UTIL_Platform_Quit(
	void
);


/*
 * Logging utilities
 */

/*++
 作用:

 The 指向callback function that produces actual log output.

 参数:

 [输入] level - The log level的this output call.
	[输入] full_log - The full log string produced 幅度为UTIL_LogOutput.
	[输入] user_log - The log string produced 幅度为user-provided format.

 返回值:

 无。

--*/
typedef void(*LOGCALLBACK)(LOGLEVEL level, const char *full_log, const char *user_log);

/*++
 作用:

 Adds 一个log output callback.

 参数:

 [输入] callback - The callback functionbe added. Once added,
	 itbe called 幅度为UTIL_LogOutput.
 [输入] loglevel - The minimal log level that callback should
	 called. Any log whose level below this will
						ignored 幅度为callback.

 返回值:

 The slot id (>= 0), -1 如果all slotsused或callbackNULL.

--*/
int
UTIL_LogAddOutputCallback(
	LOGCALLBACK    callback,
	LOGLEVEL       loglevel
);

/*++
 作用:

 Removes 一个log output callback.

 参数:

 [输入] id - The id的callback functionbe removed.

 返回值:

 无

--*/
void
UTIL_LogRemoveOutputCallback(
	int            id
);

/*++
 作用:

 设置minimal log level that可以be output.
	Any level below this levelproduce no output.

 参数:

 [输入] minlevel - The minimal log level,必须be within the
	 range [LOGLEVEL_MIN, LOGLEVEL_MAX].

 返回值:

 无。

--*/
void
UTIL_LogOutput(
	LOGLEVEL       level,
	const char    *fmt,
	...
);

#ifdef PAL_ENABLE_TRACE_LOGS
#define PAL_TRACE_LOG(...) UTIL_LogOutput(LOGLEVEL_DEBUG, __VA_ARGS__)
#else
#define PAL_TRACE_LOG(...) ((void)0)
#endif

/*++
 作用:

 设置minimal log level that可以be output.
	Any level below this levelproduce no output.

 参数:

 [输入] minlevel - The minimal log level,必须be within the
	 range [LOGLEVEL_MIN, LOGLEVEL_MAX].

 返回值:

 无。

--*/
void
UTIL_LogSetLevel(
	LOGLEVEL       minlevel
);

void
UTIL_LogToFile(
	LOGLEVEL       _,
	const char    *string,
	const char    *__
);

void
UTIL_LogSetPrelude(
    const char    *prelude
);

PAL_C_LINKAGE_END

#endif
