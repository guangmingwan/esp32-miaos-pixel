#pragma once

#include <stddef.h>
#include <stdint.h>

struct GbkUnicodePair {
  uint16_t unicode;
  uint16_t gbk;
};

extern const GbkUnicodePair GBK_UNICODE_PAIRS[];
extern const size_t GBK_UNICODE_PAIR_COUNT;
