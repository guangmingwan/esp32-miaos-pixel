/**
 * @file myctype.h
 * @brief LavaX 虚拟机字符类型判断函数库
 *
 * 本文件提供了字符类型判断和转换的内联函数和宏定义。
 * 这是一个精简版的 C 语言 ctype 库，专为嵌入式环境优化。
 *
 * 主要功能：
 * - 字符类型判断（字母、数字、空白、控制字符等）
 * - 大小写转换
 * - ASCII 字符处理
 *
 * 注意：与标准 C 库不同，本实现不处理 EOF 情况。
 *
 * 移植自 Linux 内核源码的 ctype 实现。
 */

#ifndef _LINUX_CTYPE_H
#define _LINUX_CTYPE_H

/*
 * 注意！此 ctype 实现不处理 EOF，
 * 这与标准 C 库的要求不同。
 */

/* ==================== 字符类型掩码定义 ==================== */

/**
 * @defgroup CharTypeMasks 字符类型掩码
 * @{
 */

/**
 * @brief 大写字母掩码
 *
 * 用于标识大写字母 A-Z
 */
#define _U    0x01    /* upper */

/**
 * @brief 小写字母掩码
 *
 * 用于标识小写字母 a-z
 */
#define _L    0x02    /* lower */

/**
 * @brief 数字掩码
 *
 * 用于标识数字 0-9
 */
#define _D    0x04    /* digit */

/**
 * @brief 控制字符掩码
 *
 * 用于标识控制字符（ASCII 0-31, 127）
 */
#define _CT    0x08    /* cntrl */

/**
 * @brief 标点符号掩码
 *
 * 用于标识标点符号字符
 */
#define _P    0x10    /* punct */

/**
 * @brief 空白字符掩码
 *
 * 用于标识空白字符（空格、换行、制表符）
 */
#define _S    0x20    /* white space (space/lf/tab) */

/**
 * @brief 十六进制数字掩码
 *
 * 用于标识十六进制数字 0-9, A-F, a-f
 */
#define _X_DIGIT    0x40    /* hex digit */

/**
 * @brief 硬空格掩码
 *
 * 用于标识普通空格字符（ASCII 0x20）
 */
#define _SP    0x80    /* hard space (0x20) */

/** @} */ /* CharTypeMasks */

/* ==================== 外部数据声明 ==================== */

/**
 * @brief 字符类型查找表
 *
 * 256 字节的查找表，每个字节对应一个 ASCII 字符的类型掩码。
 * 通过查表方式快速判断字符类型，避免复杂的条件判断。
 *
 * 使用方法：_ctype[(int)(unsigned char)c] 获取字符 c 的类型掩码
 */
extern const unsigned char _ctype[];

/**
 * @brief 获取字符的类型掩码
 * @param x 字符（自动转换为 int）
 * @return 该字符的类型掩码
 */
#define __ismask(x) (_ctype[(int)(unsigned char)(x)])

/* ==================== 字符类型判断宏 ==================== */

/**
 * @brief 判断是否为字母或数字
 * @param c 待判断的字符
 * @return 非零值表示是字母或数字，0 表示不是
 */
#define isalnum(c)    ((__ismask(c)&(_U|_L|_D)) != 0)

/**
 * @brief 判断是否为字母
 * @param c 待判断的字符
 * @return 非零值表示是字母，0 表示不是
 */
#define isalpha(c)    ((__ismask(c)&(_U|_L)) != 0)

/**
 * @brief 判断是否为控制字符
 * @param c 待判断的字符
 * @return 非零值表示是控制字符，0 表示不是
 */
#define iscntrl(c)    ((__ismask(c)&(_CT)) != 0)

/**
 * @brief 判断是否为十进制数字
 * @param c 待判断的字符
 * @return 非零值表示是数字（0-9），0 表示不是
 */
#define isdigit(c)    ((__ismask(c)&(_D)) != 0)

/**
 * @brief 判断是否为可打印字符（不含空格）
 * @param c 待判断的字符
 * @return 非零值表示是可打印字符，0 表示不是
 */
#define isgraph(c)    ((__ismask(c)&(_P|_U|_L|_D)) != 0)

/**
 * @brief 判断是否为小写字母
 * @param c 待判断的字符
 * @return 非零值表示是小写字母，0 表示不是
 */
#define islower(c)    ((__ismask(c)&(_L)) != 0)

/**
 * @brief 判断是否为可打印字符（含空格）
 * @param c 待判断的字符
 * @return 非零值表示是可打印字符，0 表示不是
 */
#define isprint(c)    ((__ismask(c)&(_P|_U|_L|_D|_SP)) != 0)

/**
 * @brief 判断是否为标点符号
 * @param c 待判断的字符
 * @return 非零值表示是标点符号，0 表示不是
 */
#define ispunct(c)    ((__ismask(c)&(_P)) != 0)

/**
 * @brief 判断是否为空白字符
 * @param c 待判断的字符
 * @return 非零值表示是空白字符，0 表示不是
 *
 * 注意：isspace() 对 NUL 终止符返回 false
 */
#define isspace(c)    ((__ismask(c)&(_S)) != 0)

/**
 * @brief 判断是否为大写字母
 * @param c 待判断的字符
 * @return 非零值表示是大写字母，0 表示不是
 */
#define isupper(c)    ((__ismask(c)&(_U)) != 0)

/**
 * @brief 判断是否为十六进制数字
 * @param c 待判断的字符
 * @return 非零值表示是十六进制数字（0-9, A-F, a-f），0 表示不是
 */
#define isxdigit(c)    ((__ismask(c)&(_D|_X_DIGIT)) != 0)

/**
 * @brief 判断是否为 ASCII 字符
 * @param c 待判断的字符
 * @return 非零值表示是 ASCII 字符（0-127），0 表示不是
 */
#ifndef isascii
#define isascii(c) (((unsigned char)(c))<=0x7f)
#endif

/**
 * @brief 转换为 ASCII 字符
 * @param c 输入字符
 * @return 截断后的 ASCII 字符（低 7 位）
 */
#ifndef toascii
#define toascii(c) (((unsigned char)(c))&0x7f)
#endif

/* ==================== 大小写转换函数 ==================== */

/**
 * @brief 转换为小写字母（内联函数）
 * @param c 输入字符
 * @return 如果是大写字母返回对应的小写字母，否则返回原字符
 *
 * 通过减去 'A'-'a' 的差值（32）实现大小写转换
 */
static inline unsigned char __tolower(unsigned char c)
{
    if (isupper(c))
        c -= 'A'-'a';
    return c;
}

/**
 * @brief 转换为大写字母（内联函数）
 * @param c 输入字符
 * @return 如果是小写字母返回对应的大写字母，否则返回原字符
 *
 * 通过加上 'a'-'A' 的差值（-32）实现大小写转换
 */
static inline unsigned char __toupper(unsigned char c)
{
    if (islower(c))
        c -= 'a'-'A';
    return c;
}

/**
 * @brief 转换为小写字母（宏）
 * @param c 输入字符
 * @return 对应的小写字母或原字符
 */
#define tolower(c) __tolower(c)

/**
 * @brief 转换为大写字母（宏）
 * @param c 输入字符
 * @return 对应的大写字母或原字符
 */
#define toupper(c) __toupper(c)

#endif /* _LINUX_CTYPE_H */