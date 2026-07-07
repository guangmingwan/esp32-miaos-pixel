#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MIA_APP_MANIFEST_MAGIC 0x3141494D  /* "MIA1" */
#define MIA_MANIFEST_CATEGORY_SIZE 16
#define MIA_MANIFEST_NAME_SIZE 32

typedef struct __attribute__((packed)) {
  uint32_t magic;
  char category[MIA_MANIFEST_CATEGORY_SIZE];
  char name[MIA_MANIFEST_NAME_SIZE];
  uint32_t crc;
} OtaAppManifest;

#define MIA_MANIFEST_TRAILER_SIZE ((int)sizeof(OtaAppManifest))

/* Returns CRC32 of the manifest payload (magic + category + name). */
static inline uint32_t miaManifestComputeCrc(const OtaAppManifest *m) {
  uint32_t crc = 0xFFFFFFFF;
  const uint8_t *p = (const uint8_t *)m;
  unsigned int len = sizeof(uint32_t) + MIA_MANIFEST_CATEGORY_SIZE + MIA_MANIFEST_NAME_SIZE;
  for (unsigned int i = 0; i < len; ++i) {
    crc ^= p[i];
    for (int bit = 0; bit < 8; ++bit) {
      if (crc & 1)
        crc = (crc >> 1) ^ 0xEDB88320;
      else
        crc >>= 1;
    }
  }
  return crc ^ 0xFFFFFFFF;
}

static inline int miaManifestIsValid(const OtaAppManifest *m) {
  if (!m) return 0;
  if (m->magic != MIA_APP_MANIFEST_MAGIC) return 0;
  return miaManifestComputeCrc(m) == m->crc ? 1 : 0;
}

#ifdef __cplusplus
}
#endif
