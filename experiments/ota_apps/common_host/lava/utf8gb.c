/**
 * @file utf8gb.c
 * @brief UTF-8 与 GBK 编码转换模块
 *
 * 本文件实现了 UTF-8 编码与 GBK 编码之间的相互转换功能。
 * 主要用于 LavaX 虚拟机中的中文字符处理，确保在不同编码
 * 环境下正确显示中文文本。
 *
 * 编码转换流程：
 * - UTF-8 -> GBK: UTF-8 -> UTF-16 -> GBK
 * - GBK -> UTF-8: GBK -> UTF-16 -> UTF-8
 *
 * 数据表依赖：
 * - utf16togb.h: UTF-16 到 GBK 的映射表
 * - mb_gbk2uni.h: GBK 到 Unicode (UTF-16) 的映射表
 *
 * @author LavaX Team
 * @date 2024
 */

/* 定义无符号短整型别名，用于 UTF-16 字符处理 */
typedef unsigned short u16;

/* 引入编码转换映射表 */
#include "utf16togb.h"    /* UTF-16 到 GBK 编码映射表 */
#include "mb_gbk2uni.h"   /* GBK 到 Unicode 编码映射表 */

/**
 * @brief 将 UTF-16 编码转换为 GBK 编码
 *
 * 这是一个内部辅助函数，用于 UTF-8 到 GBK 转换过程中的中间步骤。
 *
 * 转换规则：
 * - UTF-16 值 < 256: 直接作为单字节 GBK 字符
 * - UTF-16 值 >= 256: 通过查表转换为双字节 GBK 字符
 *
 * @param utf16 UTF-16 编码的字符值
 * @param d 输出缓冲区，用于存储转换后的 GBK 字符（最多 2 字节）
 * @return 转换后的字节数（1 或 2）
 *
 * @note GBK 编码中，ASCII 字符（0-127）为单字节，
 *       中文字符为双字节（第一字节 0x81-0xFE，第二字节 0x40-0xFE）
 */
static int utf16_to_gb(u16 utf16, char* d)
{
	/* ASCII 字符（0-255）直接输出单字节 */
	if (utf16 < 256) {
		d[0] = (char)utf16;
		return 1;
	}

	/* 通过查找表获取 GBK 编码的高字节 */
	d[0] = (char)utf16togb[utf16 * 2];

	/* 获取 GBK 编码的低字节 */
	char c = (char)utf16togb[utf16 * 2 + 1];

	/* 如果低字节不为 0，表示这是一个双字节 GBK 字符 */
	if (c != 0) {
		d[1] = c;
		return 2;
	}
	return 1;
}

/**
 * @brief 将 UTF-16 编码转换为 UTF-8 编码
 *
 * 这是一个内部辅助函数，用于 GBK 到 UTF-8 转换过程中的中间步骤。
 *
 * UTF-8 编码规则：
 * - U+0000 ~ U+007F: 0xxxxxxx (1 字节)
 * - U+0080 ~ U+07FF: 110xxxxx 10xxxxxx (2 字节)
 * - U+0800 ~ U+FFFF: 1110xxxx 10xxxxxx 10xxxxxx (3 字节)
 *
 * @param utf16 UTF-16 编码的字符值
 * @param utf8 输出缓冲区，用于存储转换后的 UTF-8 字符（最多 3 字节）
 * @return 转换后的字节数（1、2 或 3）
 *
 * @note 本函数仅处理 BMP（基本多语言平面）内的字符，即 U+0000 ~ U+FFFF
 */
static int utf16_to_utf8(u16 utf16, char* utf8)
{
	/* 单字节序列：0xxxxxxx (U+0000 ~ U+007F) */
	if (utf16 < 0x80) {
		*utf8++ = (char)utf16;
		return 1;
	}
	/* 双字节序列：110xxxxx 10xxxxxx (U+0080 ~ U+07FF) */
	else if (utf16 < 0x800) {
		/* 第一字节：高 5 位，前缀 110 */
		*utf8++ = (utf16 >> 6) | 0xc0;
		/* 第二字节：低 6 位，前缀 10 */
		*utf8++ = (utf16 & 0x3f) | 0x80;
		return 2;
	}
	/* 三字节序列：1110xxxx 10xxxxxx 10xxxxxx (U+0800 ~ U+FFFF) */
	else {
		/* 第一字节：高 4 位，前缀 1110 */
		*utf8++ = (utf16 >> 12) | 0xe0;
		/* 第二字节：中间 6 位，前缀 10 */
		*utf8++ = ((utf16 >> 6) & 0x3F) | 0x80;
		/* 第三字节：低 6 位，前缀 10 */
		*utf8++ = (utf16 & 0x3f) | 0x80;
		return 3;
	}
}

/**
 * @brief 将 UTF-8 编码字符串转换为 GBK 编码字符串
 *
 * 逐字符解析 UTF-8 编码，转换为对应的 GBK 编码。
 * 转换过程：UTF-8 -> UTF-16 -> GBK
 *
 * UTF-8 字节序列判断规则：
 * - 0xxxxxxx: 单字节字符（ASCII）
 * - 110xxxxx: 双字节序列起始
 * - 1110xxxx: 三字节序列起始
 * - 10xxxxxx: 续字节（不应出现在序列开头）
 *
 * @param utf8 输入的 UTF-8 编码字符串（以 '\0' 结尾）
 * @param gb 输出缓冲区，用于存储转换后的 GBK 字符串
 * @return 成功返回 1，失败返回 0
 *
 * @note 输出缓冲区应足够大以容纳转换结果
 * @warning 如果遇到无效的 UTF-8 序列，转换将中止
 */
int utf8_to_gb(char* utf8, char* gb)
{
	unsigned char c, c2, c3;
	unsigned short utf16;
	int len;

	for (;;) {
		c = *utf8++;

		/* 单字节字符（ASCII）：0xxxxxxx (0x00-0x7F) */
		if (c < 0x80) {
			*gb++ = c;
			/* 遇到字符串结束符，转换完成 */
			if (c == 0) {
				return 1;
			}
		}
		/* 续字节（10xxxxxx）出现在序列开头，非法序列 */
		else if (c < 0xc0) {
			break;
		}
		/* 双字节序列：110xxxxx 10xxxxxx */
		else if (c < 0xe0) {
			c2 = *utf8++;
			/* 检查续字节有效性 */
			if (c2 < 0x80 || c2 >= 0xc0) {
				break;
			}
			/* 解码 UTF-16 值：取第一字节低 5 位和第二字节低 6 位 */
			utf16 = ((c & 0x1f) << 6) | (c2 & 0x3f);
			/* 将 UTF-16 转换为 GBK */
			len = utf16_to_gb(utf16, gb);
			gb += len;
		}
		/* 三字节序列：1110xxxx 10xxxxxx 10xxxxxx */
		else if (c < 0xf0) {
			c2 = *utf8++;
			/* 检查第一个续字节有效性 */
			if (c2 < 0x80 || c2 >= 0xc0) {
				break;
			}
			c3 = *utf8++;
			/* 检查第二个续字节有效性 */
			if (c3 < 0x80 || c3 >= 0xc0) {
				break;
			}
			/* 解码 UTF-16 值：取三个字节的相应位 */
			utf16 = ((c & 0xf) << 12) | ((c2 & 0x3f) << 6) | (c3 & 0x3f);
			/* 将 UTF-16 转换为 GBK */
			len = utf16_to_gb(utf16, gb);
			gb += len;
		}
		/* 四字节及以上序列（超出 BMP），暂不支持 */
		else {
			break;
		}
	}

	/* 转换失败，确保输出字符串正确终止 */
	*gb = 0;
	return 0;
}

/**
 * @brief 将 GBK 编码字符串转换为 UTF-8 编码字符串
 *
 * 逐字符解析 GBK 编码，转换为对应的 UTF-8 编码。
 * 转换过程：GBK -> UTF-16 -> UTF-8
 *
 * GBK 编码规则：
 * - 单字节字符：0x00-0x80（ASCII 兼容）
 * - 双字节字符：第一字节 0x81-0xFE，第二字节 0x40-0xFE
 *
 * @param gb 输入的 GBK 编码字符串（以 '\0' 结尾）
 * @param utf8 输出缓冲区，用于存储转换后的 UTF-8 字符串
 * @return 成功返回 1，失败返回 0
 *
 * @note 输出缓冲区应足够大（UTF-8 可能比 GBK 占用更多字节）
 * @warning 如果遇到无效的 GBK 序列，转换将中止
 */
int gb_to_utf8(char* gb, char* utf8)
{
	unsigned char c, c2;
	unsigned short utf16;
	int len;

	for (;;) {
		c = *gb++;

		/* 单字节字符（ASCII 兼容区）：0x00-0x80 */
		if (c < 0x81) {
			*utf8++ = c;
			/* 遇到字符串结束符，转换完成 */
			if (c == 0) {
				return 1;
			}
		}
		/* 双字节 GBK 字符 */
		else {
			c2 = *gb++;
			/* 检查第二字节有效性（GBK 第二字节范围：0x40-0xFE） */
			if (c2 >= 0x40) {
				/* 计算 GBK 字符在查找表中的索引 */
				c -= 0x81;
				c2 -= 0x40;

				/* 检查索引范围是否有效 */
				/* 第一字节范围：0x81-0xFE (共 126 个，即 0x7E+1)
				 * 第二字节范围：0x40-0xFE (共 191 个，即 0xBF)
				 * 但实际上 GB2312 的第一字节范围是 0xA1-0xF7 */
				if (c <= 0x7d && c2 <= 0xbe) {
					/* 通过查找表获取 Unicode (UTF-16) 值 */
					utf16 = mb_gb2uni_table[c * 0xbf + c2];
					/* 将 UTF-16 转换为 UTF-8 */
					len = utf16_to_utf8(utf16, utf8);
					utf8 += len;
				}
				else {
					break;
				}
			}
			else {
				break;
			}
		}
	}

	/* 转换失败，确保输出字符串正确终止 */
	*utf8++ = 0;
	return 0;
}

/**
 * @brief UTF-8 到 GBK 转换的静态缓冲区版本
 *
 * 提供一个使用静态缓冲区的便捷接口，主要用于 printf 宏等
 * 需要直接返回转换结果字符串的场景。
 *
 * @param utf8_str 输入的 UTF-8 编码字符串
 * @return 指向静态缓冲区中 GBK 编码字符串的指针
 *
 * @warning 此函数使用静态缓冲区，非线程安全
 * @warning 静态缓冲区大小为 4096 字节，长字符串将被截断
 * @warning 连续调用会覆盖之前的结果
 *
 * @example
 * printf("中文: %s\n", utf8_to_gb_static("中文测试"));
 */
char* utf8_to_gb_static(const char* utf8_str)
{
	/* 静态缓冲区，用于存储转换结果 */
	static char gb_buf[4096];

	/* 执行转换 */
	utf8_to_gb((char*)utf8_str, gb_buf);

	return gb_buf;
}