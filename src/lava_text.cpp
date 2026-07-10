#include "lava_text.h"

#include <algorithm>
#include <cstring>

#include <Preferences.h>

#include "gbk_unicode_map.h"
#include "droid_gbk_index.h"

#include "int_wdt_guard.h"
#include "lava_font.h"
#include "launcher_log.h"

static constexpr uint8_t LAVA_FONT_MAX_HEIGHT = 32;
static constexpr uint8_t LAVA_FONT_FALLBACK_WIDTH = 8;
static constexpr uint8_t LAVA_DRAW_CHARS_PER_YIELD = 8;
static constexpr const char *LAVA_TEXT_NAMESPACE = "lava-text";
static constexpr const char *LAVA_TEXT_FONT_KEY = "font";
static constexpr size_t DROID_GBK12_DATA_BYTES = 456429;

static const uint8_t *droidFontDataEnd() {
  return fontDroidGbk12.data + DROID_GBK12_DATA_BYTES;
}

extern "C" void esp32_task_wdt_reset(void);

static LavaFontFace g_fontFace = LavaFontFace::DroidGbk12;
static bool g_fontLoaded = false;
static bool g_fontSkipPersisted = false;

static constexpr uint32_t DROID_GBK12_NO_PAGE = 0xFFFFFFFFu;
static constexpr uint32_t DROID_GBK12_PAGE_OFFSETS[256] = {
    0x00000u, 0x00A00u, 0x00ADDu, 0x00B21u, 0x00D61u, 0x01079u, 0x01079u, 0x01079u,
    0x01079u, 0x01079u, 0x01079u, 0x01079u, 0x01079u, 0x01079u, 0x01079u, 0x01079u,
    0x01079u, 0x01079u, 0x01079u, 0x01079u, 0x01079u, 0x01079u, 0x01079u, 0x01079u,
    0x01079u, 0x01079u, 0x01079u, 0x01079u, 0x01079u, 0x01079u, 0x01079u, 0x01079u,
    0x01079u, 0x01139u, 0x012DDu, 0x01499u, 0x014A5u, 0x016FDu, 0x01DC9u, 0x01E05u,
    0x01E05u, 0x01E05u, 0x01E05u, 0x01E05u, 0x01E05u, 0x01E05u, 0x01E05u, 0x01E05u,
    0x01E05u, 0x02C61u, 0x02EDDu, 0x02FFEu, 0x030D3u, 0x030D3u, 0x030D3u, 0x030D3u,
    0x030D3u, 0x030D3u, 0x030D3u, 0x030D3u, 0x030D3u, 0x030D3u, 0x030D3u, 0x030D3u,
    0x030D3u, 0x030D3u, 0x030D3u, 0x030D3u, 0x030D3u, 0x030D3u, 0x030D3u, 0x030D3u,
    0x030D3u, 0x030D3u, 0x030D3u, 0x030D3u, 0x030D3u, 0x030D3u, 0x030D3u, 0x04559u,
    0x05C0Fu, 0x072C2u, 0x087F1u, 0x09CF6u, 0x0B153u, 0x0C5F2u, 0x0DABCu, 0x0EF7Au,
    0x1045Fu, 0x11960u, 0x12E4Cu, 0x1435Eu, 0x15853u, 0x16D3Bu, 0x181E4u, 0x197C4u,
    0x1ACC2u, 0x1C243u, 0x1D7C4u, 0x1ECDBu, 0x2020Cu, 0x21731u, 0x22C4Du, 0x240F1u,
    0x255F6u, 0x26B06u, 0x28016u, 0x29535u, 0x2AA55u, 0x2BEFBu, 0x2D3B6u, 0x2E87Cu,
    0x2FD6Fu, 0x312D9u, 0x3285Fu, 0x33DDEu, 0x352BBu, 0x36787u, 0x37C70u, 0x3919Au,
    0x3A667u, 0x3BC94u, 0x3D1E5u, 0x3E6F3u, 0x3FBF4u, 0x410F5u, 0x42607u, 0x43AFAu,
    0x44F8Cu, 0x464E8u, 0x47A7Eu, 0x48F9Du, 0x4A4A3u, 0x4B9B2u, 0x4CECAu, 0x4E3BDu,
    0x4F8C1u, 0x50DB6u, 0x522A4u, 0x537A6u, 0x54C9Bu, 0x56163u, 0x57679u, 0x58B5Cu,
    0x5A045u, 0x5B4F3u, 0x5C9C7u, 0x5DE8Bu, 0x5F3ABu, 0x609ECu, 0x61F74u, 0x63444u,
    0x6494Cu, 0x65E73u, 0x67469u, 0x689B9u, 0x69FD9u, 0x6B5C4u, 0x6CB1Cu, 0x6E06Au,
    0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu,
    0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu,
    0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu,
    0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu,
    0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu,
    0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu,
    0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu,
    0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu,
    0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu,
    0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu, 0x6EE1Cu,
    0x6EE1Cu, 0x6EE1Cu, 0x6EE84u, 0x6EFD4u, 0x6EFD4u, 0x6EFD4u, 0x6EFD4u, 0x6F1CEu,
};

static const uint8_t kFont5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00},  {0x00, 0x00, 0x5F, 0x00, 0x00},
    {0x00, 0x07, 0x00, 0x07, 0x00},  {0x14, 0x7F, 0x14, 0x7F, 0x14},
    {0x24, 0x2A, 0x7F, 0x2A, 0x12},  {0x23, 0x13, 0x08, 0x64, 0x62},
    {0x36, 0x49, 0x55, 0x22, 0x50},  {0x00, 0x05, 0x03, 0x00, 0x00},
    {0x00, 0x1C, 0x22, 0x41, 0x00},  {0x00, 0x41, 0x22, 0x1C, 0x00},
    {0x14, 0x08, 0x3E, 0x08, 0x14},  {0x08, 0x08, 0x3E, 0x08, 0x08},
    {0x00, 0x50, 0x30, 0x00, 0x00},  {0x08, 0x08, 0x08, 0x08, 0x08},
    {0x00, 0x60, 0x60, 0x00, 0x00},  {0x20, 0x10, 0x08, 0x04, 0x02},
    {0x3E, 0x51, 0x49, 0x45, 0x3E},  {0x00, 0x42, 0x7F, 0x40, 0x00},
    {0x42, 0x61, 0x51, 0x49, 0x46},  {0x21, 0x41, 0x45, 0x4B, 0x31},
    {0x18, 0x14, 0x12, 0x7F, 0x10},  {0x27, 0x45, 0x45, 0x45, 0x39},
    {0x3C, 0x4A, 0x49, 0x49, 0x30},  {0x01, 0x71, 0x09, 0x05, 0x03},
    {0x36, 0x49, 0x49, 0x49, 0x36},  {0x06, 0x49, 0x49, 0x29, 0x1E},
    {0x00, 0x36, 0x36, 0x00, 0x00},  {0x00, 0x56, 0x36, 0x00, 0x00},
    {0x00, 0x08, 0x14, 0x22, 0x41},  {0x14, 0x14, 0x14, 0x14, 0x14},
    {0x41, 0x22, 0x14, 0x08, 0x00},  {0x02, 0x01, 0x51, 0x09, 0x06},
    {0x32, 0x49, 0x79, 0x41, 0x3E},  {0x7E, 0x11, 0x11, 0x11, 0x7E},
    {0x7F, 0x49, 0x49, 0x49, 0x36},  {0x3E, 0x41, 0x41, 0x41, 0x22},
    {0x7F, 0x41, 0x41, 0x22, 0x1C},  {0x7F, 0x49, 0x49, 0x49, 0x41},
    {0x7F, 0x09, 0x09, 0x01, 0x01},  {0x3E, 0x41, 0x41, 0x51, 0x32},
    {0x7F, 0x08, 0x08, 0x08, 0x7F},  {0x00, 0x41, 0x7F, 0x41, 0x00},
    {0x20, 0x40, 0x41, 0x3F, 0x01},  {0x7F, 0x08, 0x14, 0x22, 0x41},
    {0x7F, 0x40, 0x40, 0x40, 0x40},  {0x7F, 0x02, 0x04, 0x02, 0x7F},
    {0x7F, 0x04, 0x08, 0x10, 0x7F},  {0x3E, 0x41, 0x41, 0x41, 0x3E},
    {0x7F, 0x09, 0x09, 0x09, 0x06},  {0x3E, 0x41, 0x51, 0x21, 0x5E},
    {0x7F, 0x09, 0x19, 0x29, 0x46},  {0x46, 0x49, 0x49, 0x49, 0x31},
    {0x01, 0x01, 0x7F, 0x01, 0x01},  {0x3F, 0x40, 0x40, 0x40, 0x3F},
    {0x1F, 0x20, 0x40, 0x20, 0x1F},  {0x7F, 0x20, 0x18, 0x20, 0x7F},
    {0x63, 0x14, 0x08, 0x14, 0x63},  {0x03, 0x04, 0x78, 0x04, 0x03},
    {0x61, 0x51, 0x49, 0x45, 0x43},  {0x00, 0x7F, 0x41, 0x41, 0x00},
    {0x02, 0x04, 0x08, 0x10, 0x20},  {0x00, 0x41, 0x41, 0x7F, 0x00},
    {0x04, 0x02, 0x01, 0x02, 0x04},  {0x40, 0x40, 0x40, 0x40, 0x40},
    {0x00, 0x01, 0x02, 0x04, 0x00},  {0x20, 0x54, 0x54, 0x54, 0x78},
    {0x7F, 0x48, 0x44, 0x44, 0x38},  {0x38, 0x44, 0x44, 0x44, 0x20},
    {0x38, 0x44, 0x44, 0x48, 0x7F},  {0x38, 0x54, 0x54, 0x54, 0x18},
    {0x08, 0x7E, 0x09, 0x01, 0x02},  {0x08, 0x14, 0x54, 0x54, 0x3C},
    {0x7F, 0x08, 0x04, 0x04, 0x78},  {0x00, 0x44, 0x7D, 0x40, 0x00},
    {0x20, 0x40, 0x44, 0x3D, 0x00},  {0x00, 0x7F, 0x10, 0x28, 0x44},
    {0x00, 0x41, 0x7F, 0x40, 0x00},  {0x7C, 0x04, 0x18, 0x04, 0x78},
    {0x7C, 0x08, 0x04, 0x04, 0x78},  {0x38, 0x44, 0x44, 0x44, 0x38},
    {0x7C, 0x14, 0x14, 0x14, 0x08},  {0x08, 0x14, 0x14, 0x18, 0x7C},
    {0x7C, 0x08, 0x04, 0x04, 0x08},  {0x48, 0x54, 0x54, 0x54, 0x20},
    {0x04, 0x3F, 0x44, 0x40, 0x20},  {0x3C, 0x40, 0x40, 0x20, 0x7C},
    {0x1C, 0x20, 0x40, 0x20, 0x1C},  {0x3C, 0x40, 0x30, 0x40, 0x3C},
    {0x44, 0x28, 0x10, 0x28, 0x44},  {0x0C, 0x50, 0x50, 0x50, 0x3C},
    {0x44, 0x64, 0x54, 0x4C, 0x44},  {0x00, 0x08, 0x36, 0x41, 0x00},
    {0x00, 0x00, 0x7F, 0x00, 0x00},  {0x00, 0x41, 0x36, 0x08, 0x00},
    {0x02, 0x01, 0x02, 0x04, 0x02},
};

static void loadFontFace() {
  if (g_fontLoaded) {
    return;
  }
  if (g_fontSkipPersisted) {
    g_fontFace = LavaFontFace::Basic8;
    g_fontLoaded = true;
    return;
  }
  Preferences prefs;
  if (prefs.begin(LAVA_TEXT_NAMESPACE, false)) {
    const uint8_t value = prefs.getUChar(LAVA_TEXT_FONT_KEY, static_cast<uint8_t>(LavaFontFace::Basic8));
    g_fontFace = value <= static_cast<uint8_t>(LavaFontFace::DroidGbk12)
                     ? static_cast<LavaFontFace>(value)
                     : LavaFontFace::Basic8;
    prefs.end();
  }
  g_fontLoaded = true;
}

static int decodeUtf8(const char **ptr) {
  if (ptr == nullptr || *ptr == nullptr || **ptr == '\0') {
    return 0;
  }

  const uint8_t first = static_cast<uint8_t>(**ptr);
  int codepoint = 0;
  size_t extraBytes = 0;
  *ptr += 1;

  if ((first & 0x80) == 0x00) {
    return first;
  }
  if ((first & 0xE0) == 0xC0) {
    codepoint = first & 0x1F;
    extraBytes = 1;
  } else if ((first & 0xF0) == 0xE0) {
    codepoint = first & 0x0F;
    extraBytes = 2;
  } else if ((first & 0xF8) == 0xF0) {
    codepoint = first & 0x07;
    extraBytes = 3;
  } else {
    return -1;
  }

  for (size_t i = 0; i < extraBytes; ++i) {
    const uint8_t next = static_cast<uint8_t>((*ptr)[i]);
    if ((next & 0xC0) != 0x80) {
      return -1;
    }
    codepoint <<= 6;
    codepoint += next & 0x3F;
  }
  *ptr += extraBytes;
  return codepoint;
}

static const uint8_t *glyphSearchStart(const LavaFont &font, int codepoint) {
  if (&font != &fontDroidGbk12 || codepoint < 0 || codepoint > 0xFFFF) {
    return font.data;
  }
  const uint32_t offset = DROID_GBK12_PAGE_OFFSETS[static_cast<uint16_t>(codepoint) >> 8];
  return offset == DROID_GBK12_NO_PAGE ? nullptr : font.data + offset;
}

static const uint8_t *glyphSearchEnd(const LavaFont &font, int codepoint) {
  if (&font != &fontDroidGbk12 || codepoint < 0 || codepoint > 0xFFFF) {
    return nullptr;
  }
  const uint8_t page = static_cast<uint16_t>(codepoint) >> 8;
  for (uint16_t nextPage = static_cast<uint16_t>(page) + 1; nextPage < 256; ++nextPage) {
    const uint32_t nextOffset = DROID_GBK12_PAGE_OFFSETS[nextPage];
    if (nextOffset != DROID_GBK12_NO_PAGE) {
      return font.data + nextOffset;
    }
  }
  return nullptr;
}

static uint16_t glyphCode(const uint8_t *glyph) {
  return static_cast<uint16_t>(glyph[0]) | (static_cast<uint16_t>(glyph[1]) << 8);
}

static uint8_t glyphY(const uint8_t *glyph) { return glyph[2]; }
static uint8_t glyphWidth(const uint8_t *glyph) { return glyph[3]; }
static uint8_t glyphHeight(const uint8_t *glyph) { return glyph[4]; }

static bool glyphLooksValid(const uint8_t *glyph) {
  if (glyph == nullptr) {
    return false;
  }
  const uint8_t height = glyphHeight(glyph);
  const uint8_t width = glyphWidth(glyph);
  return height > 0 && height <= LAVA_FONT_MAX_HEIGHT && width > 0 && width <= 31;
}
static uint8_t glyphXOffset(const uint8_t *glyph) { return glyph[5]; }
static uint8_t glyphXDelta(const uint8_t *glyph) { return glyph[6]; }
static const uint8_t *glyphData(const uint8_t *glyph) { return glyph + 7; }

static size_t glyphByteSize(const uint8_t *glyph) {
  const uint8_t width = glyphWidth(glyph);
  if (width == 0) {
    return 7;
  }
  return 7 + (((width * glyphHeight(glyph)) - 1) / 8) + 1;
}

static constexpr uint32_t DROID_ASCII_OFFSETS[] = {
    0x00000000u, 0x00000007u, 0x00000010u, 0x00000019u, 0x00000028u, 0x00000036u, 0x00000046u, 0x00000056u,
    0x0000005Eu, 0x0000006Au, 0x00000076u, 0x00000082u, 0x0000008Du, 0x00000095u, 0x0000009Du, 0x000000A5u,
    0x000000B1u, 0x000000BEu, 0x000000C9u, 0x000000D6u, 0x000000E3u, 0x000000F2u, 0x000000FFu, 0x0000010Cu,
    0x00000119u, 0x00000126u, 0x00000133u, 0x0000013Bu, 0x00000145u, 0x00000150u, 0x0000015Au, 0x00000165u,
    0x00000172u, 0x00000185u, 0x00000195u, 0x000001A3u, 0x000001B1u, 0x000001C0u, 0x000001CDu, 0x000001DAu,
    0x000001E9u, 0x000001F8u, 0x00000204u, 0x00000210u, 0x0000021Eu, 0x0000022Bu, 0x0000023Du, 0x0000024Cu,
    0x0000025Cu, 0x0000026Au, 0x0000027Cu, 0x0000028Au, 0x00000297u, 0x000002A6u, 0x000002B5u, 0x000002C4u,
    0x000002D8u, 0x000002E7u, 0x000002F6u, 0x00000304u, 0x00000310u, 0x0000031Cu, 0x00000328u, 0x00000334u,
    0x0000033Cu, 0x00000344u, 0x00000350u, 0x0000035Fu, 0x0000036Au, 0x00000378u, 0x00000384u, 0x00000392u,
    0x000003A0u, 0x000003AEu, 0x000003B7u, 0x000003C3u, 0x000003D1u, 0x000003DAu, 0x000003E9u, 0x000003F5u,
    0x00000402u, 0x00000411u, 0x0000041Fu, 0x0000042Au, 0x00000435u, 0x00000441u, 0x0000044Du, 0x0000045Au,
    0x00000469u, 0x00000476u, 0x00000485u, 0x00000491u, 0x0000049Eu, 0x000004A7u, 0x000004B4u,
};

static bool unicodeToGbkCode(uint16_t unicode, uint16_t &gbk) {
  size_t low = 0;
  size_t high = GBK_UNICODE_PAIR_COUNT;
  while (low < high) {
    const size_t mid = low + ((high - low) / 2);
    const uint16_t midUnicode = GBK_UNICODE_PAIRS[mid].unicode;
    if (midUnicode == unicode) {
      gbk = GBK_UNICODE_PAIRS[mid].gbk;
      return true;
    }
    if (midUnicode < unicode) {
      low = mid + 1;
    } else {
      high = mid;
    }
  }
  return false;
}

static const uint8_t *droidGlyphByGbkCode(int codepoint) {
  if (codepoint < 0 || codepoint > 0xFFFF) {
    return nullptr;
  }

  if (codepoint >= 0x20 && codepoint <= 0x7E) {
    return fontDroidGbk12.data + DROID_ASCII_OFFSETS[codepoint - 0x20];
  }
  if (codepoint < 0x80) {
    return nullptr;
  }

  uint16_t gbk = 0;
  if (!unicodeToGbkCode(static_cast<uint16_t>(codepoint), gbk)) {
    return nullptr;
  }

  const uint32_t offset = DROID_GBK12_GLYPH_OFFSETS[gbk];
  return offset == DROID_GBK12_NO_GLYPH || offset > DROID_GBK12_DATA_BYTES - 7
             ? nullptr
             : fontDroidGbk12.data + offset;
}

static uint8_t glyphAdvance(const LavaFont &font, uint8_t points, uint32_t *rows, int codepoint) {
  if (codepoint == '\r' || codepoint == '\n' || codepoint == 0) {
    return 0;
  }

  if (&font != &fontDroidGbk12 && codepoint > 0x00FF) {
    const uint8_t height = std::min<uint8_t>(points, LAVA_FONT_MAX_HEIGHT);
    if (rows != nullptr) {
      memset(rows, 0, sizeof(uint32_t) * height);
    }
    const uint8_t boxWidth = font.width == 0 ? LAVA_FONT_FALLBACK_WIDTH : font.width;
    if (rows != nullptr) {
      const uint32_t mask = boxWidth >= 31 ? 0x7FFFFFFE : ~((0xFFFFFFFFu << (boxWidth - 1)) | 1u);
      for (uint8_t i = 0; i < height; ++i) {
        rows[i] = (0xAAAAAAAAu << (i & 1u)) & mask;
      }
    }
    return boxWidth;
  }

  const uint8_t *matchedGlyph = nullptr;
  const LavaFont *effectiveFont = &font;
  if (&font == &fontDroidGbk12) {
    matchedGlyph = droidGlyphByGbkCode(codepoint);
  }

  if (matchedGlyph == nullptr && &font != &fontDroidGbk12) {
    const uint8_t *ptr = glyphSearchStart(*effectiveFont, codepoint);
    const uint8_t *end = glyphSearchEnd(*effectiveFont, codepoint);
    esp32_task_wdt_reset();
    for (size_t i = 0; ptr != nullptr && (end == nullptr || ptr < end) &&
                       i < effectiveFont->chars && glyphCode(ptr) != 0; ++i) {
      if (glyphCode(ptr) == codepoint) {
        matchedGlyph = ptr;
        break;
      }
      ptr += glyphByteSize(ptr);
      if ((i & 0x0Fu) == 0x0Fu) {
        esp32_task_wdt_reset();
      }
    }
    esp32_task_wdt_reset();
  }

  const uint8_t height = std::min<uint8_t>(points, LAVA_FONT_MAX_HEIGHT);
  if (rows != nullptr) {
    memset(rows, 0, sizeof(uint32_t) * height);
  }

  if (matchedGlyph != nullptr && glyphWidth(matchedGlyph) == 0 &&
      glyphHeight(matchedGlyph) == 0 && glyphXDelta(matchedGlyph) > 0) {
    const uint8_t advance = glyphXDelta(matchedGlyph);
    return points == effectiveFont->height
               ? advance
               : std::max<uint8_t>(1, (advance * points) / effectiveFont->height);
  }

  if (matchedGlyph != nullptr && !glyphLooksValid(matchedGlyph)) {
    matchedGlyph = nullptr;
  }

  if (matchedGlyph == nullptr) {
    const uint8_t boxWidth = effectiveFont->width == 0 ? LAVA_FONT_FALLBACK_WIDTH : effectiveFont->width;
    if (rows != nullptr) {
      const uint32_t mask = boxWidth >= 31 ? 0x7FFFFFFE : ~((0xFFFFFFFFu << (boxWidth - 1)) | 1u);
      for (uint8_t i = 0; i < height; ++i) {
        rows[i] = (0xAAAAAAAAu << (i & 1u)) & mask;
      }
    }
    return boxWidth;
  }

  const uint8_t *glyph = matchedGlyph;
  const uint8_t rawXOffset = glyphXOffset(glyph);
  const int xOffset = static_cast<int8_t>(rawXOffset);
  const uint8_t drawHeight = glyphHeight(glyph);
  const uint8_t glyphW = glyphWidth(glyph);
  if (drawHeight == 0 || drawHeight > LAVA_FONT_MAX_HEIGHT || glyphW == 0 || glyphW > 31) {
    const uint8_t boxWidth = effectiveFont->width == 0 ? LAVA_FONT_FALLBACK_WIDTH : effectiveFont->width;
    if (rows != nullptr) {
      const uint32_t mask = boxWidth >= 31 ? 0x7FFFFFFE : ~((0xFFFFFFFFu << (boxWidth - 1)) | 1u);
      for (uint8_t i = 0; i < height; ++i) {
        rows[i] = (0xAAAAAAAAu << (i & 1u)) & mask;
      }
    }
    return boxWidth;
  }

  const uint8_t *data = glyphData(glyph);
  if (&font == &fontDroidGbk12 && (data < fontDroidGbk12.data || data >= droidFontDataEnd())) {
    const uint8_t boxWidth = effectiveFont->width == 0 ? LAVA_FONT_FALLBACK_WIDTH : effectiveFont->width;
    if (rows != nullptr) {
      const uint32_t mask = boxWidth >= 31 ? 0x7FFFFFFE : ~((0xFFFFFFFFu << (boxWidth - 1)) | 1u);
      for (uint8_t i = 0; i < height; ++i) {
        rows[i] = (0xAAAAAAAAu << (i & 1u)) & mask;
      }
    }
    return boxWidth;
  }

  if (rows != nullptr) {
    int byte = 0;
    int mask = 0x80;
    uint32_t nativeRows[LAVA_FONT_MAX_HEIGHT];
    memset(nativeRows, 0, sizeof(nativeRows));
    const size_t bitmapBytes =
        static_cast<size_t>(((drawHeight * glyphW) - 1) / 8) + 1;
    if (&font == &fontDroidGbk12 &&
        (data > droidFontDataEnd() ||
         static_cast<size_t>(droidFontDataEnd() - data) < bitmapBytes)) {
      launcherTracef("[lava] glyph U+%04X out of bounds", codepoint);
      const uint8_t boxWidth = effectiveFont->width == 0 ? LAVA_FONT_FALLBACK_WIDTH : effectiveFont->width;
      const uint32_t fallbackMask =
          boxWidth >= 31 ? 0x7FFFFFFE : ~((0xFFFFFFFFu << (boxWidth - 1)) | 1u);
      for (uint8_t i = 0; i < height; ++i) {
        rows[i] = (0xAAAAAAAAu << (i & 1u)) & fallbackMask;
      }
      return boxWidth;
    }
    for (uint8_t y = 0; y < drawHeight; ++y) {
      uint32_t row = 0;
      for (uint8_t x = 0; x < glyphW; ++x) {
        if (((x + (y * glyphW)) % 8) == 0) {
          mask = 0x80;
          byte = *data++;
        }
        const int bit = xOffset + x;
        if ((byte & mask) != 0 && bit >= 0 && bit < 32) {
          row |= 1u << bit;
        }
        mask >>= 1;
      }
      const uint8_t rowIndex = glyphY(glyph) + y;
      if (rowIndex < LAVA_FONT_MAX_HEIGHT) {
        nativeRows[rowIndex] = row;
      }
      esp32_task_wdt_reset();
    }
    if (points == effectiveFont->height) {
      memcpy(rows, nativeRows, sizeof(uint32_t) * height);
    } else {
      for (uint8_t row = 0; row < height; ++row) {
        rows[row] = nativeRows[(row * effectiveFont->height) / points];
      }
    }
  }
  const uint8_t advance = std::max<uint8_t>(glyphW, glyphXDelta(glyph));
  return points == effectiveFont->height ? advance : std::max<uint8_t>(1, (advance * points) / effectiveFont->height);
}

static void fillGlyphBackground(LavaSurface &surface, int16_t x, int16_t y,
                                uint8_t advance, uint8_t height, uint8_t bg) {
  for (uint8_t row = 0; row < height; ++row) {
    const int16_t py = y + row;
    if (py < 0 || py >= surface.h) {
      continue;
    }
    for (uint8_t col = 0; col < advance; ++col) {
      const int16_t px = x + col;
      if (px >= 0 && px < surface.w) {
        surface.pixels[py * surface.pitch + px] = bg;
      }
    }
    esp32_task_wdt_reset();
  }
}

static void drawSmallMissingGlyph(LavaSurface &surface, int16_t x, int16_t y, uint8_t fg, uint8_t bg) {
  for (uint8_t row = 0; row < 7; ++row) {
    const int16_t py = y + row;
    if (py < 0 || py >= surface.h) {
      continue;
    }
    for (uint8_t col = 0; col < 5; ++col) {
      const int16_t px = x + col;
      if (px >= 0 && px < surface.w) {
        const bool border = row == 0 || row == 6 || col == 0 || col == 4;
        surface.pixels[py * surface.pitch + px] = border ? fg : bg;
      }
    }
  }
}

static void drawSmallGlyph(LavaSurface &surface, int16_t x, int16_t y, int codepoint,
                           uint8_t fg, uint8_t bg) {
  fillGlyphBackground(surface, x, y, 6, 7, bg);
  if (codepoint < 32 || codepoint > 126) {
    drawSmallMissingGlyph(surface, x, y, fg, bg);
    return;
  }
  const uint8_t *glyph = kFont5x7[codepoint - 32];
  for (int16_t col = 0; col < 5; ++col) {
    const uint8_t bits = glyph[col];
    for (int16_t row = 0; row < 7; ++row) {
      const int16_t px = x + col;
      const int16_t py = y + row;
      if (px >= 0 && px < surface.w && py >= 0 && py < surface.h && ((bits >> row) & 1u) != 0) {
        surface.pixels[py * surface.pitch + px] = fg;
      }
    }
  }
}

static const LavaFont *currentFont() {
  switch (lavaFontFace()) {
    case LavaFontFace::Basic8:
    case LavaFontFace::Basic12:
    case LavaFontFace::Basic16:
      return &fontBasic8x8;
    case LavaFontFace::DejaVu12:
      return &fontDejaVu12;
    case LavaFontFace::DejaVu15:
      return &fontDejaVu15;
    case LavaFontFace::VeraBold11:
      return &fontVeraBold11;
    case LavaFontFace::VeraBold14:
      return &fontVeraBold14;
    case LavaFontFace::DroidGbk12:
      return &fontDroidGbk12;
    case LavaFontFace::Small5x7:
      return nullptr;
  }
  return &fontDroidGbk12;
}

static uint8_t currentFontPoints() {
  switch (lavaFontFace()) {
    case LavaFontFace::Small5x7:
      return 7;
    case LavaFontFace::Basic12:
      return 12;
    case LavaFontFace::Basic16:
      return 16;
    case LavaFontFace::Basic8:
      return 8;
    case LavaFontFace::DejaVu12:
    case LavaFontFace::DejaVu15:
    case LavaFontFace::VeraBold11:
    case LavaFontFace::VeraBold14:
    case LavaFontFace::DroidGbk12:
      return currentFont()->height;
  }
  return fontDroidGbk12.height;
}

LavaFontFace lavaFontFace() {
  loadFontFace();
  return g_fontFace;
}

void lavaSetFontFace(LavaFontFace face) {
  loadFontFace();
  g_fontFace = face;
  Preferences prefs;
  if (prefs.begin(LAVA_TEXT_NAMESPACE, false)) {
    prefs.putUChar(LAVA_TEXT_FONT_KEY, static_cast<uint8_t>(face));
    prefs.end();
  }
}

void lavaUseFontFaceForSession(LavaFontFace face) {
  loadFontFace();
  g_fontFace = face;
}

void lavaFontSkipPersisted(void) {
  g_fontSkipPersisted = true;
}

void lavaCycleFontFace() {
  const uint8_t next = static_cast<uint8_t>(lavaFontFace()) + 1;
  lavaSetFontFace(next > static_cast<uint8_t>(LavaFontFace::DroidGbk12)
                      ? LavaFontFace::Small5x7
                      : static_cast<LavaFontFace>(next));
}

const char *lavaFontName(LavaFontFace face) {
  switch (face) {
    case LavaFontFace::Small5x7:
      return "5x7 ASCII";
    case LavaFontFace::Basic8:
      return "Basic 8";
    case LavaFontFace::Basic12:
      return "Basic 12";
    case LavaFontFace::Basic16:
      return "Basic 16";
    case LavaFontFace::DejaVu12:
      return "DejaVu 12";
    case LavaFontFace::DejaVu15:
      return "DejaVu 15";
    case LavaFontFace::VeraBold11:
      return "Vera Bold 11";
    case LavaFontFace::VeraBold14:
      return "Vera Bold 14";
    case LavaFontFace::DroidGbk12:
      return "Droid GBK 12";
  }
  return "Droid GBK 12";
}

int16_t lavaFontHeight() {
  return currentFontPoints();
}

int16_t lavaTextWidth(const char *text) {
  if (text == nullptr) {
    return 0;
  }

  int16_t width = 0;
  for (const char *ptr = text; *ptr != '\0';) {
    const int codepoint = decodeUtf8(&ptr);
    const LavaFont *font = currentFont();
    width += font == nullptr ? 6 : glyphAdvance(*font, currentFontPoints(), nullptr, codepoint);
  }
  return width;
}

int16_t lavaDrawTextUtf8(LavaSurface &surface, int16_t x, int16_t y,
                         const char *text, uint8_t fg, uint8_t bg) {
  if (surface.pixels == nullptr || text == nullptr) {
    return 0;
  }

  int16_t cursor = x;
  uint16_t rendered = 0;
  if (lavaFontFace() == LavaFontFace::Small5x7) {
    for (const char *ptr = text; *ptr != '\0';) {
      drawSmallGlyph(surface, cursor, y, decodeUtf8(&ptr), fg, bg);
      cursor += 6;
      ++rendered;
      if ((rendered % LAVA_DRAW_CHARS_PER_YIELD) == 0) {
        esp32_task_wdt_reset();
      }
    }
    return cursor - x;
  }

  const LavaFont *font = currentFont();
  const uint8_t fontHeight = std::min<uint8_t>(currentFontPoints(), LAVA_FONT_MAX_HEIGHT);
  ScopedIntWdtPause wdtGuard;
  for (const char *ptr = text; *ptr != '\0';) {
    uint32_t rows[LAVA_FONT_MAX_HEIGHT];
    const int codepoint = decodeUtf8(&ptr);
    esp32_task_wdt_reset();
    const uint8_t advance = glyphAdvance(*font, fontHeight, rows, codepoint);
    fillGlyphBackground(surface, cursor, y, advance, fontHeight, bg);
    for (uint8_t row = 0; row < fontHeight; ++row) {
      const int16_t py = y + row;
      if (py < 0 || py >= surface.h) {
        continue;
      }
      for (uint8_t col = 0; col < advance; ++col) {
        const int16_t px = cursor + col;
        if (px >= 0 && px < surface.w && col < 32 && ((rows[row] >> col) & 1u) != 0) {
          surface.pixels[py * surface.pitch + px] = fg;
        }
      }
    }
    cursor += advance;
    ++rendered;
    if ((rendered % LAVA_DRAW_CHARS_PER_YIELD) == 0) {
      esp32_task_wdt_reset();
    }
  }
  return cursor - x;
}
