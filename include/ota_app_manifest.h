#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MIA_APP_MANIFEST_V1_MAGIC 0x3141494D  /* "MIA1" */
#define MIA_APP_MANIFEST_MAGIC 0x3241494D     /* "MIA2" */
#define MIA_MANIFEST_CATEGORY_SIZE 16
#define MIA_MANIFEST_NAME_SIZE 32

typedef struct __attribute__((packed)) {
  uint32_t magic;
  char category[MIA_MANIFEST_CATEGORY_SIZE];
  char name[MIA_MANIFEST_NAME_SIZE];
  uint64_t build_epoch;
  uint32_t image_size;
  uint32_t image_crc;
  uint32_t crc;
} OtaAppManifest;

typedef struct __attribute__((packed)) {
  uint32_t magic;
  char category[MIA_MANIFEST_CATEGORY_SIZE];
  char name[MIA_MANIFEST_NAME_SIZE];
  uint32_t crc;
} OtaAppManifestV1;

#define MIA_MANIFEST_TRAILER_SIZE ((int)sizeof(OtaAppManifest))

/* Returns CRC32 of every manifest field before crc. */
static inline uint32_t miaManifestComputeCrc(const OtaAppManifest *m) {
  uint32_t crc = 0xFFFFFFFF;
  const uint8_t *p = (const uint8_t *)m;
  unsigned int len = offsetof(OtaAppManifest, crc);
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

static inline uint32_t miaManifestV1ComputeCrc(const OtaAppManifestV1 *m) {
  uint32_t crc = 0xFFFFFFFF;
  const uint8_t *p = (const uint8_t *)m;
  for (unsigned int i = 0; i < offsetof(OtaAppManifestV1, crc); ++i) {
    crc ^= p[i];
    for (int bit = 0; bit < 8; ++bit)
      crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320 : crc >> 1;
  }
  return crc ^ 0xFFFFFFFF;
}

static inline int miaManifestV1IsValid(const OtaAppManifestV1 *m) {
  return m && m->magic == MIA_APP_MANIFEST_V1_MAGIC &&
         miaManifestV1ComputeCrc(m) == m->crc;
}

#ifdef __cplusplus
}
#endif
