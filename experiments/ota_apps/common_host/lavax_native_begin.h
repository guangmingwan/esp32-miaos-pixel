//
//  @file lavax_native_begin.h
//  @brief LavaX 原生编译类型兼容头文件（开始部分）
//
//  说明：
//  C 语言不能直接重定义关键字类型，因此这里采用“别名 + 宏映射”的方式，
//  只在 begin/end 包裹的用户代码区内，把 LavaX 基本类型映射到固定宽度类型。
//
//  要求：
//  1. 先包含运行时/标准库头文件，再包含本文件。
//  2. 用户 LavaX 风格源码放在 begin/end 之间。
//

#ifndef LAVAX_NATIVE_BEGIN_H
#define LAVAX_NATIVE_BEGIN_H

#include <limits.h>
#include <stdint.h>

typedef uint8_t lavax_char;
typedef int16_t lavax_int;
typedef int32_t lavax_long;
typedef float lavax_float;
typedef uint8_t* lavax_addr;

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(CHAR_BIT == 8, "LavaX native bridge requires 8-bit bytes");
_Static_assert(sizeof(lavax_char) == 1, "lavax_char must be 8-bit");
_Static_assert(sizeof(lavax_int) == 2, "lavax_int must be 16-bit");
_Static_assert(sizeof(lavax_long) == 4, "lavax_long must be 32-bit");
_Static_assert(sizeof(lavax_float) == 4, "lavax_float must be 32-bit IEEE float");
_Static_assert(sizeof(lavax_addr) == sizeof(void*), "lavax_addr must match pointer size");
#endif

/*
 * 仅重写用户源码区域里的类型关键字。
 * 这要求系统头和运行时头在本文件之前完成包含，避免污染外部声明。
 */
#ifndef LAVA_ESP32
#define char lavax_char
#define int lavax_int
#define long lavax_long
#define float lavax_float
#endif
#define addr lavax_addr

#endif /* LAVAX_NATIVE_BEGIN_H */
