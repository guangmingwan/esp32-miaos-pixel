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
// Portions based on PalLibrary by Lou Yihua <louyihua@21cn.com>.
// Copyright (c) 2006-2007, Lou Yihua.
//
// Ported to C from C++ and modified for compatibility with Big-Endian
// by Wei Mingzhi <whistler_wmz@users.sf.net>.
//

#include "common.h"

struct _YJOneTreeNode
{
	BYTE   value;
	BYTE   leaf;
	WORD   level;
	DWORD  weight;

	addr parent;
	addr left;
	addr right;
};
#define YJOneTreeNode struct _YJOneTreeNode

struct _YJOneFileHeader
{
	DWORD Signature;          // 'YJ_1'
	DWORD UncompressedLength; // size before compression
	DWORD CompressedLength;   // size after compression
	WORD  BlockCount;         // number of blocks
	BYTE  Unknown;
	BYTE  HuffmanTreeLength;  // length of huffman tree
};
#define YJOneFileHeader struct _YJOneFileHeader

struct _YJOneBlockHeader
{
	WORD UncompressedLength; // maximum 0x4000
	WORD CompressedLength;   // including the header
	WORD LZSSRepeatTable[4];
	BYTE LZSSOffsetCodeLengthTable[4];
	BYTE LZSSRepeatCodeLengthTable[3];
	BYTE CodeCountCodeLengthTable[3];
	BYTE CodeCountTable[2];
};
#define YJOneBlockHeader struct _YJOneBlockHeader

#define YJ1_FILE_SIGNATURE_OFFSET 0
#define YJ1_FILE_UNCOMPRESSED_LENGTH_OFFSET 4
#define YJ1_FILE_BLOCK_COUNT_OFFSET 12
#define YJ1_FILE_HUFFMAN_TREE_LENGTH_OFFSET 15

#define YJ1_BLOCK_UNCOMPRESSED_LENGTH_OFFSET 0
#define YJ1_BLOCK_COMPRESSED_LENGTH_OFFSET 2
#define YJ1_BLOCK_REPEAT_TABLE_OFFSET 4
#define YJ1_BLOCK_OFFSET_CODE_LENGTH_OFFSET 12
#define YJ1_BLOCK_REPEAT_CODE_LENGTH_OFFSET 16
#define YJ1_BLOCK_CODE_COUNT_CODE_LENGTH_OFFSET 19
#define YJ1_BLOCK_CODE_COUNT_TABLE_OFFSET 22

WORD yj1_read_word(addr src)
{
	return (WORD)(PAL_U8(src[0]) | (PAL_U8(src[1]) << 8));
	return 0;
}

DWORD yj1_read_dword(addr src)
{
	return (DWORD)(PAL_U8(src[0]) |
		(PAL_U8(src[1]) << 8) |
		(PAL_U8(src[2]) << 16) |
		(PAL_U8(src[3]) << 24));
	return 0;
}

WORD yj_one_get_bits(
	addr src,
	addr bitptr_addr,
	WORD count)
{
	DWORD *bitptr;
	BYTE *data;
	DWORD base;
	DWORD value;
	DWORD word;
	DWORD bit_in_word;
	int i;

	bitptr = (DWORD *)bitptr_addr;
	data = (BYTE *)src;
	base = *bitptr;
	value = 0;
	for (i = 0; i < count; i++)
	{
		word = (base + i) >> 4;
		word = word << 1;
		bit_in_word = (base + i) & 15;
		word = (data[word] & 255) | ((data[word + 1] & 255) * 256);
		value = value << 1;
		value = value | ((word >> (15 - bit_in_word)) & 1);
	}
	*bitptr = *bitptr + count;
	return (WORD)value;
}

DWORD g_yj1_bitptr;
DWORD g_yj1_temp;
DWORD g_yj1_bptr;
DWORD g_yj1_word0;
DWORD g_yj1_word1;
DWORD g_yj1_mask;
BYTE *g_yj1_src;
BYTE *g_yj1_flag;
BYTE *g_yj1_header;
BYTE *g_yj1_hdr;
BYTE *g_yj1_dest;
BYTE *g_yj1_dest_start;
BYTE *g_yj1_dest_end;
int g_yj1_i;
int g_yj1_left_tmp;
int g_yj1_tree_node;
WORD g_yj1_word_tmp;
WORD g_yj1_tree_len;
WORD g_yj1_hul;
WORD g_yj1_loop;
DWORD g_yj1_pos;
DWORD g_yj1_count;
DWORD g_yj1_file_signature;
DWORD g_yj1_uncompressed_length;
int g_yj1_left[1024];
int g_yj1_right[1024];
int g_yj1_value[1024];
int g_yj1_leaf[1024];

WORD yj_one_get_bits_g(
	addr src,
	WORD count)
{
	g_yj1_temp = (g_yj1_bitptr >> 4) << 1;
	g_yj1_bptr = g_yj1_bitptr & 0xf;
	g_yj1_bitptr = g_yj1_bitptr + count;

	g_yj1_word0 = (PAL_U8(src[g_yj1_temp]) | (PAL_U8(src[g_yj1_temp + 1]) << 8));
	g_yj1_word1 = (PAL_U8(src[g_yj1_temp + 2]) | (PAL_U8(src[g_yj1_temp + 3]) << 8));
	if (count > 16 - g_yj1_bptr)
	{
		count = count + g_yj1_bptr - 16;
		g_yj1_mask = 0xffff >> g_yj1_bptr;
		return (WORD)(((g_yj1_word0 & g_yj1_mask) << count) | (g_yj1_word1 >> (16 - count)));
	}
	return (WORD)(((g_yj1_word0 << g_yj1_bptr) & 0xffff) >> (16 - count));
}

WORD yj_one_get_loop_g(
	addr src,
	addr header_addr)
{
	g_yj1_header = (BYTE *)header_addr;
	if (yj_one_get_bits_g(src, 1))
		return g_yj1_header[YJ1_BLOCK_CODE_COUNT_TABLE_OFFSET + 0] & 255;

	g_yj1_temp = yj_one_get_bits_g(src, 2);
	if (g_yj1_temp)
		return yj_one_get_bits_g(src, g_yj1_header[YJ1_BLOCK_CODE_COUNT_CODE_LENGTH_OFFSET + g_yj1_temp - 1] & 255);
	return g_yj1_header[YJ1_BLOCK_CODE_COUNT_TABLE_OFFSET + 1] & 255;
}

WORD yj_one_get_count_g(
	addr src,
	addr header_addr)
{
	g_yj1_header = (BYTE *)header_addr;
	g_yj1_word_tmp = yj_one_get_bits_g(src, 2);
	if (g_yj1_word_tmp != 0)
	{
		if (yj_one_get_bits_g(src, 1))
			return yj_one_get_bits_g(src, g_yj1_header[YJ1_BLOCK_REPEAT_CODE_LENGTH_OFFSET + g_yj1_word_tmp - 1] & 255);
		return (WORD)(PAL_U8(g_yj1_header[YJ1_BLOCK_REPEAT_TABLE_OFFSET + g_yj1_word_tmp * 2]) |
			(PAL_U8(g_yj1_header[YJ1_BLOCK_REPEAT_TABLE_OFFSET + g_yj1_word_tmp * 2 + 1]) << 8));
	}
	return (WORD)(PAL_U8(g_yj1_header[YJ1_BLOCK_REPEAT_TABLE_OFFSET]) |
		(PAL_U8(g_yj1_header[YJ1_BLOCK_REPEAT_TABLE_OFFSET + 1]) << 8));
}

int yj_one_build_tree_index(addr source, int tree_len)
{
	if (tree_len <= 0 || tree_len >= 1024)
		return FALSE;

	g_yj1_src = (BYTE *)source;
	g_yj1_flag = g_yj1_src + 16 + tree_len;
	g_yj1_bitptr = 0;

	g_yj1_leaf[0] = 0;
	g_yj1_value[0] = 0;
	g_yj1_left[0] = 1;
	g_yj1_right[0] = 2;

	for (g_yj1_i = 1; g_yj1_i <= tree_len; g_yj1_i++)
	{
		g_yj1_leaf[g_yj1_i] = yj_one_get_bits_g((addr)g_yj1_flag, 1) ? 0 : 1;
		g_yj1_value[g_yj1_i] = g_yj1_src[15 + g_yj1_i] & 255;
		if (g_yj1_leaf[g_yj1_i])
		{
			g_yj1_left[g_yj1_i] = 0;
			g_yj1_right[g_yj1_i] = 0;
		}
		else
		{
			g_yj1_left_tmp = (g_yj1_value[g_yj1_i] & 255) * 2 + 1;
			if (g_yj1_left_tmp < 0 || g_yj1_left_tmp + 1 > tree_len || g_yj1_left_tmp + 1 >= 1024)
			{
				return FALSE;
			}
			g_yj1_left[g_yj1_i] = g_yj1_left_tmp;
			g_yj1_right[g_yj1_i] = g_yj1_left_tmp + 1;
		}
	}

	return TRUE;
}

WORD yj_one_get_loop(
	addr src,
	addr bitptr_addr,
	addr header_addr)
{
	DWORD *bitptr;
	BYTE *header;
	DWORD temp;

	bitptr = (DWORD *)bitptr_addr;
	header = (BYTE *)header_addr;
	if (yj_one_get_bits(src, bitptr, 1))
		return header[YJ1_BLOCK_CODE_COUNT_TABLE_OFFSET + 0] & 255;
	else
	{
		temp = yj_one_get_bits(src, bitptr, 2);
		if (temp)
			return yj_one_get_bits(src, bitptr, header[YJ1_BLOCK_CODE_COUNT_CODE_LENGTH_OFFSET + temp - 1] & 255);
		else
			return header[YJ1_BLOCK_CODE_COUNT_TABLE_OFFSET + 1] & 255;
	}
	return 0;
}

WORD yj_one_get_count(
	addr src,
	addr bitptr_addr,
	addr header_addr)
{
	DWORD *bitptr;
	BYTE *header;
	WORD temp;

	bitptr = (DWORD *)bitptr_addr;
	header = (BYTE *)header_addr;
	temp = yj_one_get_bits(src, bitptr, 2);
	if (temp != 0)
	{
		if (yj_one_get_bits(src, bitptr, 1))
			return yj_one_get_bits(src, bitptr, header[YJ1_BLOCK_REPEAT_CODE_LENGTH_OFFSET + temp - 1] & 255);
		else
			return (WORD)(PAL_U8(header[YJ1_BLOCK_REPEAT_TABLE_OFFSET + temp * 2]) |
			   (PAL_U8(header[YJ1_BLOCK_REPEAT_TABLE_OFFSET + temp * 2 + 1]) << 8));
	}
	else
		return (WORD)(PAL_U8(header[YJ1_BLOCK_REPEAT_TABLE_OFFSET]) |
		   (PAL_U8(header[YJ1_BLOCK_REPEAT_TABLE_OFFSET + 1]) << 8));
	return 0;
}

long YJOne_Decompress(
	addr          Source,
	addr          Destination,
	long          DestSize)
{
	g_yj1_hdr = (BYTE *)Source;
	g_yj1_src = (BYTE *)Source;

	if (Source == NULL)
	{
		printf("[LAVA][YJ1] fail=1\n");
		return -101;
	}
	g_yj1_file_signature = (DWORD)(g_yj1_hdr[YJ1_FILE_SIGNATURE_OFFSET] & 255) |
		((DWORD)(g_yj1_hdr[YJ1_FILE_SIGNATURE_OFFSET + 1] & 255) << 8) |
		((DWORD)(g_yj1_hdr[YJ1_FILE_SIGNATURE_OFFSET + 2] & 255) << 16) |
		((DWORD)(g_yj1_hdr[YJ1_FILE_SIGNATURE_OFFSET + 3] & 255) << 24);
	if (g_yj1_file_signature != 0x315f4a59)
	{
		printf("[LAVA][YJ1] fail=2\n");
		return -102;
	}
	g_yj1_uncompressed_length = (DWORD)(g_yj1_hdr[YJ1_FILE_UNCOMPRESSED_LENGTH_OFFSET] & 255) |
		((DWORD)(g_yj1_hdr[YJ1_FILE_UNCOMPRESSED_LENGTH_OFFSET + 1] & 255) << 8) |
		((DWORD)(g_yj1_hdr[YJ1_FILE_UNCOMPRESSED_LENGTH_OFFSET + 2] & 255) << 16) |
		((DWORD)(g_yj1_hdr[YJ1_FILE_UNCOMPRESSED_LENGTH_OFFSET + 3] & 255) << 24);
	if (g_yj1_uncompressed_length > (DWORD)DestSize)
	{
		printf("[LAVA][YJ1] fail=3\n");
		return -103;
	}

	do
	{
		g_yj1_tree_len = (g_yj1_hdr[YJ1_FILE_HUFFMAN_TREE_LENGTH_OFFSET] & 255) * 2;
		if (g_yj1_tree_len <= 0 || g_yj1_tree_len >= 1024)
		{
			printf("[LAVA][YJ1] fail=4 tree_len=%d\n", g_yj1_tree_len);
			return -104;
		}

		g_yj1_flag = g_yj1_src + 16 + g_yj1_tree_len;
		g_yj1_bitptr = 0;
		g_yj1_leaf[0] = 0;
		g_yj1_value[0] = 0;
		g_yj1_left[0] = 1;
		g_yj1_right[0] = 2;
		for (g_yj1_i = 1; g_yj1_i <= g_yj1_tree_len; g_yj1_i++)
		{
			g_yj1_leaf[g_yj1_i] = yj_one_get_bits_g((addr)g_yj1_flag, 1) ? 0 : 1;
			g_yj1_value[g_yj1_i] = g_yj1_src[15 + g_yj1_i] & 255;
			if (g_yj1_leaf[g_yj1_i])
			{
				g_yj1_left[g_yj1_i] = 0;
				g_yj1_right[g_yj1_i] = 0;
			}
			else
			{
				g_yj1_left_tmp = (g_yj1_value[g_yj1_i] & 255) * 2 + 1;
				if (g_yj1_left_tmp < 0 || g_yj1_left_tmp + 1 > g_yj1_tree_len || g_yj1_left_tmp + 1 >= 1024)
				{
					printf("[LAVA][YJ1] fail=4 tree_len=%d\n", g_yj1_tree_len);
					return -104;
				}
				g_yj1_left[g_yj1_i] = g_yj1_left_tmp;
				g_yj1_right[g_yj1_i] = g_yj1_left_tmp + 1;
			}
		}

		if (0)
		{
			printf("[LAVA][YJ1] fail=4 tree_len=%d\n", g_yj1_tree_len);
			return -104;
		}
		g_yj1_src += 16 + g_yj1_tree_len + (((g_yj1_tree_len & 0xf) ? (g_yj1_tree_len >> 4) + 1 : (g_yj1_tree_len >> 4)) << 1);
	} while (0);

	g_yj1_dest = (BYTE *)Destination;
	g_yj1_dest_start = g_yj1_dest;
	g_yj1_dest_end = g_yj1_dest_start + DestSize;

	for (g_yj1_i = 0;
	     g_yj1_i < (int)(PAL_U8(g_yj1_hdr[YJ1_FILE_BLOCK_COUNT_OFFSET]) |
	        (PAL_U8(g_yj1_hdr[YJ1_FILE_BLOCK_COUNT_OFFSET + 1]) << 8));
	     g_yj1_i++)
	{
		g_yj1_header = g_yj1_src;
		g_yj1_src += 4;
		if (!(PAL_U8(g_yj1_header[YJ1_BLOCK_COMPRESSED_LENGTH_OFFSET]) |
		   (PAL_U8(g_yj1_header[YJ1_BLOCK_COMPRESSED_LENGTH_OFFSET + 1]) << 8)))
		{
			g_yj1_hul = (WORD)(PAL_U8(g_yj1_header[YJ1_BLOCK_UNCOMPRESSED_LENGTH_OFFSET]) |
			   (PAL_U8(g_yj1_header[YJ1_BLOCK_UNCOMPRESSED_LENGTH_OFFSET + 1]) << 8));
			while (g_yj1_hul--)
			{
				if (g_yj1_dest >= g_yj1_dest_end)
				{
					printf("[LAVA][YJ1] fail=6 block=%d ofs=%ld\n", g_yj1_i, (long)(g_yj1_dest - g_yj1_dest_start));
					return -106;
				}
				*g_yj1_dest++ = *g_yj1_src++;
			}
			continue;
		}
		g_yj1_src += 20;
		g_yj1_bitptr = 0;
		for (;;)
		{
			if (yj_one_get_bits_g((addr)g_yj1_src, 1))
				g_yj1_loop = g_yj1_header[YJ1_BLOCK_CODE_COUNT_TABLE_OFFSET + 0] & 255;
			else
			{
				g_yj1_temp = yj_one_get_bits_g((addr)g_yj1_src, 2);
				if (g_yj1_temp)
					g_yj1_loop = yj_one_get_bits_g((addr)g_yj1_src,
						g_yj1_header[YJ1_BLOCK_CODE_COUNT_CODE_LENGTH_OFFSET + g_yj1_temp - 1] & 255);
				else
					g_yj1_loop = g_yj1_header[YJ1_BLOCK_CODE_COUNT_TABLE_OFFSET + 1] & 255;
			}
			if (g_yj1_loop == 0)
				break;

			while (g_yj1_loop--)
			{
				if (g_yj1_dest >= g_yj1_dest_end)
				{
					printf("[LAVA][YJ1] fail=7 block=%d ofs=%ld\n", g_yj1_i, (long)(g_yj1_dest - g_yj1_dest_start));
					return -107;
				}
				g_yj1_tree_node = 0;
				for (; !g_yj1_leaf[g_yj1_tree_node];)
				{
					if (g_yj1_tree_node < 0 || g_yj1_tree_node > g_yj1_tree_len)
					{
						printf("[LAVA][YJ1] fail=8 block=%d ofs=%ld\n", g_yj1_i, (long)(g_yj1_dest - g_yj1_dest_start));
						return -108;
					}
					if (yj_one_get_bits_g((addr)g_yj1_src, 1))
						g_yj1_tree_node = g_yj1_right[g_yj1_tree_node];
					else
						g_yj1_tree_node = g_yj1_left[g_yj1_tree_node];
				}
				*g_yj1_dest++ = g_yj1_value[g_yj1_tree_node];
			}

			if (yj_one_get_bits_g((addr)g_yj1_src, 1))
				g_yj1_loop = g_yj1_header[YJ1_BLOCK_CODE_COUNT_TABLE_OFFSET + 0] & 255;
			else
			{
				g_yj1_temp = yj_one_get_bits_g((addr)g_yj1_src, 2);
				if (g_yj1_temp)
					g_yj1_loop = yj_one_get_bits_g((addr)g_yj1_src,
						g_yj1_header[YJ1_BLOCK_CODE_COUNT_CODE_LENGTH_OFFSET + g_yj1_temp - 1] & 255);
				else
					g_yj1_loop = g_yj1_header[YJ1_BLOCK_CODE_COUNT_TABLE_OFFSET + 1] & 255;
			}
			if (g_yj1_loop == 0)
				break;

			while (g_yj1_loop--)
			{
				g_yj1_word_tmp = yj_one_get_bits_g((addr)g_yj1_src, 2);
				if (g_yj1_word_tmp != 0)
				{
					if (yj_one_get_bits_g((addr)g_yj1_src, 1))
						g_yj1_count = yj_one_get_bits_g((addr)g_yj1_src,
							g_yj1_header[YJ1_BLOCK_REPEAT_CODE_LENGTH_OFFSET + g_yj1_word_tmp - 1] & 255);
					else
						g_yj1_count = (DWORD)(PAL_U8(g_yj1_header[YJ1_BLOCK_REPEAT_TABLE_OFFSET + g_yj1_word_tmp * 2]) |
							(PAL_U8(g_yj1_header[YJ1_BLOCK_REPEAT_TABLE_OFFSET + g_yj1_word_tmp * 2 + 1]) << 8));
				}
				else
				{
					g_yj1_count = (DWORD)(PAL_U8(g_yj1_header[YJ1_BLOCK_REPEAT_TABLE_OFFSET]) |
						(PAL_U8(g_yj1_header[YJ1_BLOCK_REPEAT_TABLE_OFFSET + 1]) << 8));
				}
				g_yj1_pos = yj_one_get_bits_g((addr)g_yj1_src, 2);
				g_yj1_pos = yj_one_get_bits_g((addr)g_yj1_src, g_yj1_header[YJ1_BLOCK_OFFSET_CODE_LENGTH_OFFSET + g_yj1_pos] & 255);
				if (g_yj1_pos == 0 || g_yj1_pos > (DWORD)(g_yj1_dest - g_yj1_dest_start))
				{
					printf("[LAVA][YJ1] fail=9 block=%d ofs=%d pos=%d count=%d\n",
					       g_yj1_i, (int)(g_yj1_dest - g_yj1_dest_start), (int)g_yj1_pos, (int)g_yj1_count);
					return -109;
				}
				while (g_yj1_count--)
				{
					if (g_yj1_dest >= g_yj1_dest_end)
					{
						printf("[LAVA][YJ1] fail=10 block=%d ofs=%ld\n", g_yj1_i, (long)(g_yj1_dest - g_yj1_dest_start));
						return -110;
					}
					*g_yj1_dest = *(g_yj1_dest - g_yj1_pos);
					g_yj1_dest++;
				}
			}
		}
		g_yj1_src = g_yj1_header + (PAL_U8(g_yj1_header[YJ1_BLOCK_COMPRESSED_LENGTH_OFFSET]) |
		   (PAL_U8(g_yj1_header[YJ1_BLOCK_COMPRESSED_LENGTH_OFFSET + 1]) << 8));
	}

	return (long)(g_yj1_dest - g_yj1_dest_start);
}

/* ============================================================================================================================================= */

struct _YJTwoTreeNode
{
	WORD      weight;
	WORD      value;
	addr      parent;
	addr      left;
	addr      right;
};
#define YJTwoTreeNode struct _YJTwoTreeNode

struct _YJTwoTree
{
	addr             node;
	addr             list;
};
#define YJTwoTree struct _YJTwoTree

addr yj_two_list_get(addr list_addr, WORD index)
{
	DWORD *list;
	list = (DWORD *)list_addr;
	return list[index];
	return 0;
}

void yj_two_list_set(addr list_addr, WORD index, addr value)
{
	DWORD *list;
	list = (DWORD *)list_addr;
	list[index] = value;
}

BYTE yj_two_data_one[0x100] =
{
	0x3f, 0x0b, 0x17, 0x03, 0x2f, 0x0a, 0x16, 0x00, 0x2e, 0x09, 0x15, 0x02, 0x2d, 0x01, 0x08, 0x00,
	0x3e, 0x07, 0x14, 0x03, 0x2c, 0x06, 0x13, 0x00, 0x2b, 0x05, 0x12, 0x02, 0x2a, 0x01, 0x04, 0x00,
	0x3d, 0x0b, 0x11, 0x03, 0x29, 0x0a, 0x10, 0x00, 0x28, 0x09, 0x0f, 0x02, 0x27, 0x01, 0x08, 0x00,
	0x3c, 0x07, 0x0e, 0x03, 0x26, 0x06, 0x0d, 0x00, 0x25, 0x05, 0x0c, 0x02, 0x24, 0x01, 0x04, 0x00,
	0x3b, 0x0b, 0x17, 0x03, 0x23, 0x0a, 0x16, 0x00, 0x22, 0x09, 0x15, 0x02, 0x21, 0x01, 0x08, 0x00,
	0x3a, 0x07, 0x14, 0x03, 0x20, 0x06, 0x13, 0x00, 0x1f, 0x05, 0x12, 0x02, 0x1e, 0x01, 0x04, 0x00,
	0x39, 0x0b, 0x11, 0x03, 0x1d, 0x0a, 0x10, 0x00, 0x1c, 0x09, 0x0f, 0x02, 0x1b, 0x01, 0x08, 0x00,
	0x38, 0x07, 0x0e, 0x03, 0x1a, 0x06, 0x0d, 0x00, 0x19, 0x05, 0x0c, 0x02, 0x18, 0x01, 0x04, 0x00,
	0x37, 0x0b, 0x17, 0x03, 0x2f, 0x0a, 0x16, 0x00, 0x2e, 0x09, 0x15, 0x02, 0x2d, 0x01, 0x08, 0x00,
	0x36, 0x07, 0x14, 0x03, 0x2c, 0x06, 0x13, 0x00, 0x2b, 0x05, 0x12, 0x02, 0x2a, 0x01, 0x04, 0x00,
	0x35, 0x0b, 0x11, 0x03, 0x29, 0x0a, 0x10, 0x00, 0x28, 0x09, 0x0f, 0x02, 0x27, 0x01, 0x08, 0x00,
	0x34, 0x07, 0x0e, 0x03, 0x26, 0x06, 0x0d, 0x00, 0x25, 0x05, 0x0c, 0x02, 0x24, 0x01, 0x04, 0x00,
	0x33, 0x0b, 0x17, 0x03, 0x23, 0x0a, 0x16, 0x00, 0x22, 0x09, 0x15, 0x02, 0x21, 0x01, 0x08, 0x00,
	0x32, 0x07, 0x14, 0x03, 0x20, 0x06, 0x13, 0x00, 0x1f, 0x05, 0x12, 0x02, 0x1e, 0x01, 0x04, 0x00,
	0x31, 0x0b, 0x11, 0x03, 0x1d, 0x0a, 0x10, 0x00, 0x1c, 0x09, 0x0f, 0x02, 0x1b, 0x01, 0x08, 0x00,
	0x30, 0x07, 0x0e, 0x03, 0x1a, 0x06, 0x0d, 0x00, 0x19, 0x05, 0x0c, 0x02, 0x18, 0x01, 0x04, 0x00
};
BYTE yj_two_data_two[0x10] =
{
	0x08, 0x05, 0x06, 0x04, 0x07, 0x05, 0x06, 0x03, 0x07, 0x05, 0x06, 0x04, 0x07, 0x04, 0x05, 0x03
};

void yj_two_adjust_tree(YJTwoTree *tree, WORD value)
{
	YJTwoTreeNode* node;
	YJTwoTreeNode tmp;
	YJTwoTreeNode* tmp1;
	YJTwoTreeNode* temp;

	node = yj_two_list_get(tree->list, value);
	while (node->value != 0x280)
	{
		temp = node + 1;
		while (node->weight == temp->weight)
			temp++;
		temp--;
		if (temp != node)
		{
			tmp1 = node->parent;
			node->parent = temp->parent;
			temp->parent = tmp1;
			if (node->value > 0x140)
			{
				tmp1 = node->left;
				tmp1->parent = (addr)temp;
				tmp1 = node->right;
				tmp1->parent = (addr)temp;
			}
			else
				yj_two_list_set(tree->list, node->value, (addr)temp);
			if (temp->value > 0x140)
			{
				tmp1 = temp->left;
				tmp1->parent = (addr)node;
				tmp1 = temp->right;
				tmp1->parent = (addr)node;
			}
			else
				yj_two_list_set(tree->list, temp->value, (addr)node);
			tmp.weight = node->weight;
			tmp.value = node->value;
			tmp.parent = node->parent;
			tmp.left = node->left;
			tmp.right = node->right;
			node->weight = temp->weight;
			node->value = temp->value;
			node->parent = temp->parent;
			node->left = temp->left;
			node->right = temp->right;
			temp->weight = tmp.weight;
			temp->value = tmp.value;
			temp->parent = tmp.parent;
			temp->left = tmp.left;
			temp->right = tmp.right;
			node = temp;
		}
		node->weight++;
		node = node->parent;
	}
	node->weight++;
}

int yj_two_build_tree(YJTwoTree *tree)
{
	int i, ptr;
	addr list;
	YJTwoTreeNode* cur;
	YJTwoTreeNode* cur1;
	YJTwoTreeNode* cur2;
	YJTwoTreeNode* node;
	list = malloc(sizeof(DWORD) * 321);
	tree->list = list;
	if (list == NULL)
		return 0;
	node = malloc(sizeof(YJTwoTreeNode) * 641);
	tree->node = (addr)node;
	if (node == NULL)
	{
		free(list);
		return 0;
	}
	memset((BYTE *)list, 0, 321 * sizeof(DWORD));
	memset(node, 0, 641 * sizeof(YJTwoTreeNode));
	for (i = 0; i <= 0x140; i++)
		yj_two_list_set(list, i, (addr)(node + i));
	for (i = 0; i <= 0x280; i++)
	{
		cur = node + i;
		cur->value = i;
		cur->weight = 1;
	}
	cur = node + 0x280;
	cur->parent = (addr)(node + 0x280);
	for (i = 0, ptr = 0x141; ptr <= 0x280; i += 2, ptr++)
	{
		cur = node + ptr;
		cur->left = (addr)(node + i);
		cur->right = (addr)(node + i + 1);
		cur1 = node + i;
		cur1->parent = (addr)(node + ptr);
		cur1 = node + i + 1;
		cur1->parent = (addr)(node + ptr);
		cur2 = node + i;
		cur1 = node + i + 1;
		cur->weight = cur2->weight + cur1->weight;
	}
	return 1;
}

int yj_two_bt(BYTE* data, DWORD pos)
{
	return (data[pos >> 3] & (BYTE)(1 << (pos & 0x7))) >> (pos & 0x7);
}


long YJTwo_Decompress(
	addr          Source,
	addr          Destination,
	long          DestSize)
{
	int Length;
	int i;
	DWORD len;
	DWORD ptr;
	DWORD temp;
	DWORD tmp;
	DWORD pos;
	BYTE* src;
	BYTE* dest;
	BYTE* pre;
	YJTwoTree tree;
	YJTwoTreeNode* tree_node;
	YJTwoTreeNode* node;
	YJTwoTreeNode* list_node;
	WORD val;
	BYTE *length_src;

	len = 0;
	ptr = 0;
	temp = 0;
	tmp = 0;
	pos = 0;
	src = (BYTE*)Source + 4;

	if (Source == NULL)
		return -1;

	if (!yj_two_build_tree(&tree))
		return -1;
	tree_node = tree.node;

	length_src = (BYTE *)Source;
	Length = (int)((DWORD)(length_src[0] & 255) |
		((DWORD)(length_src[1] & 255) << 8) |
		((DWORD)(length_src[2] & 255) << 16) |
		((DWORD)(length_src[3] & 255) << 24));
	if (Length > DestSize)
		return -1;
	dest = (BYTE*)Destination;

	while (1)
	{
		node = tree_node + 0x280;
		while (node->value > 0x140)
		{
			if (yj_two_bt(src, ptr))
				node = node->right;
			else
				node = node->left;
			ptr++;
		}
		val = node->value;
		list_node = tree_node + 0x280;
		if (list_node->weight == 0x8000)
		{
			for (i = 0; i < 0x141; i++)
			{
				list_node = yj_two_list_get(tree.list, i);
				if (list_node->weight & 0x1)
					yj_two_adjust_tree(&tree, i);
			}
			for (i = 0; i <= 0x280; i++)
			{
				list_node = tree_node + i;
				list_node->weight >>= 1;
			}
		}
		yj_two_adjust_tree(&tree, val);
		if (val > 0xff)
		{
			temp = 0;
			for (i = 0; i < 8; i++, ptr++)
				temp |= (DWORD)yj_two_bt(src, ptr) << i;
			tmp = temp & 0xff;
			for (; i < yj_two_data_two[tmp & 0xf] + 6; i++, ptr++)
				temp |= (DWORD)yj_two_bt(src, ptr) << i;
			temp >>= yj_two_data_two[tmp & 0xf];
			pos = (temp & 0x3f) | ((DWORD)yj_two_data_one[tmp] << 6);
			if (pos == 0xfff)
				break;
			pre = dest - pos - 1;
			for (i = 0; i < val - 0xfd; i++)
				*dest++ = *pre++;
			len += val - 0xfd;
		}
		else
		{
			*dest++ = (BYTE)val;
			len++;
		}
	}

	free(tree.list);
	free(tree_node);
	return Length;
}
