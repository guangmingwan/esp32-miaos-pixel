#include "mia_snes_contract.h"

#include <string.h>

int mia_snes_extension_supported(const char *path) {
    const char *dot = path == NULL ? NULL : strrchr(path, '.');
    return dot != NULL && (strcmp(dot + 1, "sfc") == 0 || strcmp(dot + 1, "smc") == 0);
}

void mia_snes_pacing_record(MiaSnesPacing *pacing, uint32_t elapsed_us, uint32_t budget_us, int rendered) {
    if (pacing == NULL) return;
    pacing->frames++;
    if (elapsed_us > budget_us) pacing->late_frames++;
    if (!rendered) pacing->skipped_frames++;
}
