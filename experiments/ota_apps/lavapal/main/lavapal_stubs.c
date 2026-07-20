/*
 * lavapal_stubs.c — provides empty bodies for SDLPAL entry points whose
 * real implementations (aviplay.c, scandir) are excluded from this build.
 */
#include "common.h"
#include <dirent.h>

PAL_C_LINKAGE_BEGIN

/* AVI playback is disabled; stub the public API consumed by main/ending/ui. */
VOID PAL_AVIInit(VOID) {}
VOID PAL_AVIShutdown(VOID) {}
BOOL PAL_PlayAVI(const char *lpszPath) { (void)lpszPath; return FALSE; }

/* ESP-IDF newlib lacks scandir/alphasort; util.c uses them for save listing.
 * Return -1 (enumeration unsupported) so callers fall back gracefully. */
int scandir(const char *dir, struct dirent ***namelist,
            int (*selector)(const struct dirent *),
            int (*cmp)(const struct dirent **, const struct dirent **)) {
	(void)dir; (void)namelist; (void)selector; (void)cmp;
	return -1;
}
int alphasort(const struct dirent **a, const struct dirent **b) {
	(void)a; (void)b;
	return 0;
}

PAL_C_LINKAGE_END
