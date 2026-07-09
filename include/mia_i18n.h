#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
enum class MiaLanguage : uint8_t {
    English = 0,
    Chinese = 1,
};
#else
typedef uint8_t MiaLanguage;
#define MIA_LANG_ENGLISH 0
#define MIA_LANG_CHINESE 1
#endif

MiaLanguage miaLanguage(void);
const char* miaLanguageName(MiaLanguage lang);
void miaCycleLanguage(void);
const char* miaTr(const char* key);

/* When enabled, skip NVS read and stay on default (English). */
void miaI18nSkipPersisted(void);

#ifdef __cplusplus
}
#endif
