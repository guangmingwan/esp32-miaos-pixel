/**
 * @file py2gb.c
 * @brief 拼音输入法核心模块 - 拼音转汉字功能
 *
 * 本文件实现了拼音到汉字的转换功能，是 LavaX 虚拟机拼音输入法的核心组件。
 * 通过拼音串查找对应的 GBK 编码汉字列表。
 *
 * 主要功能：
 * - 根据拼音串查找对应的汉字列表
 * - 支持一次返回多个候选汉字
 * - 使用折半查找优化搜索效率
 *
 * 数据结构说明：
 * pinyin_data[] 数组是一个复合数据表，包含三部分：
 * - [0x0000-0x3959]: GBK 汉字数据区（汉字的 GBK 编码列表）
 * - [0x395A-0x3C9D]: 拼音索引表（每个拼音对应的汉字起止位置）
 * - [0x3C9E-末尾]: 拼音字符串表（按序号排列的拼音字符串）
 *
 * @author LavaX Team
 * @date 2024
 */

#include <string.h>
#include "lava_rt.h"

typedef u32 a32;

/**
 * @defgroup PinyinConstants 拼音输入法常量定义
 * @{
 */

/**
 * @brief 单次读取的最大候选汉字数
 *
 * 每次调用 GetGBCodeByPY 返回的候选汉字最大数量。
 * 用于限制返回结果的大小，避免缓冲区溢出。
 */
#define	ONCEREAD		9				/* 单次最多返回 9 个候选字 */

/**
 * @brief 最大拼音序列号
 *
 * 拼音字符串表中的拼音总数。
 * 用于折半查找的上界。
 */
#define MAX_SEQNO		416

/**
 * @brief 拼音字符串表偏移量
 *
 * 在 pinyin_data 数组中，拼音字符串表的起始位置。
 * 每个拼音占用 6 字节（最多 6 个字符，不足补零）。
 */
#define	PY_TXT_OFS		0x3c9e

/**
 * @brief 拼音汉字索引表偏移量
 *
 * 在 pinyin_data 数组中，拼音索引表的起始位置。
 * 每个拼音对应 4 字节（2 个 16 位索引，分别表示起止位置）。
 */
#define	PYHZ_IDX_OFS	0x395a

/**
 * @brief 汉字数据区偏移量
 *
 * 在 pinyin_data 数组中，汉字 GBK 编码数据的起始位置。
 * 通常为 0，表示从数组开头开始。
 */
#define	PYHZ_DAT_OFS	0x0

/** @} */ /* PinyinConstants */

/**
 * @brief 引用外部拼音数据表
 *
 * 该数据表定义在 pinyin.c 文件中，包含完整的拼音-汉字映射数据。
 */
extern unsigned char pinyin_data[];

/**
 * @brief 指向拼音数据表的指针
 *
 * 用于方便地访问拼音数据表中的数据。
 */
byte *pinyin=pinyin_data;

/**
 * @brief 折半查找输入拼音串的序列号
 *
 * 在拼音字符串表中使用折半查找算法，查找输入拼音串对应的序列号。
 * 该序列号用于后续从索引表和数据区获取对应的汉字列表。
 *
 * 查找算法说明：
 * - 拼音字符串表按字典序排列，支持折半查找
 * - 每个拼音固定占用 6 字节，不足补 '\0'
 * - 输入串会被转换为小写进行比较
 *
 * @param InputBuffer 输入的拼音字符串（如 "zhong"）
 * @return 找到返回对应的序列号（0-415），未找到返回 0xFFFF
 *
 * @note 输入串最多取前 6 个字符进行比较
 * @note 查找时会自动将大写字母转换为小写
 *
 * @example
 * word seq = get_seq_no("zhong");  // 返回 "zhong" 对应的序列号
 */
word get_seq_no(byte *InputBuffer)
{
	byte *ofs;
	int i;
	int top,mid,bot;
	byte ibuf[10],obuf[10];

	/* 提取输入串的前 6 个字符，并转换为小写 */
	for( i = 0; i < 6; i++ ) {   /* 无论输入多少,取前6个字符 */
		ibuf[i] = *( InputBuffer + i );
		if( ibuf[i] == '\0')
			break;
		else
			ibuf[i] |= 0x20;			/* 通过置位第5位转换为小写 (A-Z -> a-z) */
	}
	ibuf[i] = '\0';

	/* 折半查找初始化 */
	top = 0;
	bot = MAX_SEQNO;

	/* 折半查找主循环 */
	while( top <= bot ) {
		mid = ( top + bot ) / 2;

		/* 计算中间位置的拼音字符串偏移 */
		ofs = pinyin+ PY_TXT_OFS + 6 * mid;
		memcpy(obuf,ofs,6);

		/* 字符串比较 */
		i = strncmp( (char*)ibuf, (char*)obuf, 6 );

		if( i == 0 )
			return mid;        /* 找到匹配，返回序列号 */
		else if( i > 0 )
			top = mid + 1;     /* 输入串较大，搜索上半部分 */
		else
			bot = mid - 1;     /* 输入串较小，搜索下半部分 */
	}

	/* 未找到匹配的拼音 */
	return 0xffff;
}

/**
 * @brief 根据拼音获取对应的 GBK 编码汉字列表
 *
 * 这是拼音输入法的主要接口函数。根据输入的拼音串，
 * 获取对应位置的汉字候选列表。
 *
 * 工作流程：
 * 1. 验证输入有效性
 * 2. 通过 get_seq_no 获取拼音序列号
 * 3. 从索引表获取该拼音对应的汉字起止位置
 * 4. 从数据区读取汉字的 GBK 编码
 *
 * @param pos 期望获取的起始位置（从 0 开始）
 * @param InputBuffer 输入的拼音字符串
 * @param OutBuffer 输出缓冲区，用于存储候选汉字的 GBK 编码
 * @return 低 16 位为本次返回的候选字数，高 16 位为总候选数
 *         如果出错，返回的两个 16 位值都是 0xFFFF
 *
 * @note 每次最多返回 ONCEREAD(9) 个候选汉字
 * @note OutBuffer 需要足够大以容纳 ONCEREAD * 2 + 1 字节
 *
 * @example
 * byte buffer[20];
 * unsigned long result = GetGBCodeByPY(0, "zhong", buffer);
 * int count = result & 0xFFFF;      // 本次返回的字数
 * int total = (result >> 16) & 0xFFFF;  // 总候选数
 */
unsigned long GetGBCodeByPY( unsigned int pos, byte *InputBuffer, byte *OutBuffer )
{
	word sum;			/* 该拼音对应的汉字总数 */
	word gbidx[2];		/* 汉字索引 [0]=起始位置, [1]=结束位置 */
	word seqno;			/* 拼音序列号 */
	byte *gbofs;		/* 指向汉字数据区的指针 */
	byte tmpidx[5];		/* 临时存储索引数据 */

	/* 使用联合体返回两个 16 位值 */
	union {
		a32 value;       /* 32 位返回值 */
		word chr[2];     /* 两个 16 位值 [0]=本次字数, [1]=总数 */
	} rtn;

	/* 检查输入串是否为空 */
	if( *InputBuffer == 0) {
		rtn.chr[0] = 0xffff;
		rtn.chr[1] = 0xffff;
		return rtn.value;
	}

	/* 查找拼音序列号 */
	seqno = get_seq_no( InputBuffer );
	if( seqno == 0xffff ) {
		rtn.chr[0] = 0xffff;
		rtn.chr[1] = 0xffff;
		return rtn.value;
	}

	/* 从索引表获取该拼音对应的汉字起止位置 */
	gbofs = pinyin + PYHZ_IDX_OFS + 2 * seqno;
	memcpy(tmpidx,gbofs,4);

	/* 解析索引值（小端格式） */
	gbidx[0] = ( tmpidx[1] << 8 ) + tmpidx[0];  /* 起始位置 */
	gbidx[1] = ( tmpidx[3] << 8 ) + tmpidx[2];  /* 结束位置 */

	/* 计算汉字总数（每个汉字占 2 字节） */
	sum = ( gbidx[1] - gbidx[0] ) >> 1;

	/* 检查请求的位置是否超出范围 */
	if (pos>=sum) {
		rtn.chr[0] = 0xffff;
		rtn.chr[1] = 0xffff;
		return rtn.value;
	}

	/* 计算数据读取位置和数量 */
	gbofs = pinyin + PYHZ_DAT_OFS + gbidx[0] + (pos<<1);

	/* 确定本次返回的字数（不超过 ONCEREAD） */
	seqno = ( sum - pos >= ONCEREAD ) ? ONCEREAD : sum - pos;

	/* 复制汉字 GBK 编码到输出缓冲区 */
	memcpy(OutBuffer,gbofs,2 * seqno);

	/* 添加字符串结束标记 */
	OutBuffer[2 * seqno]=0;

	/* 设置返回值 */
	rtn.chr[0] = seqno;  /* 本次返回的字数 */
	rtn.chr[1] = sum;    /* 该拼音对应的汉字总数 */

	return rtn.value;
}