#include "mia_snes_contract.h"

#include <stdbool.h>
#include <string.h>
#include <strings.h>

int mia_snes_extension_supported(const char *path) {
    const char *dot = path == NULL ? NULL : strrchr(path, '.');
    return dot != NULL && (strcasecmp(dot + 1, "sfc") == 0 || strcasecmp(dot + 1, "smc") == 0);
}

void mia_snes_pacing_record(MiaSnesPacing *pacing, uint32_t elapsed_us, uint32_t budget_us, int rendered) {
    if (pacing == NULL) return;
    pacing->frames++;
    if (elapsed_us > budget_us) pacing->late_frames++;
    if (!rendered) pacing->skipped_frames++;
}

uint8_t mia_snes_adjust_frameskip(uint8_t frameskip, uint32_t emulated_frames,
                                  uint32_t target_frames, uint32_t late_frames,
                                  uint32_t display_busy_frames) {
    if (frameskip < 1u) frameskip = 1u;
    if (frameskip > 5u) frameskip = 5u;
    if (target_frames == 0u) return frameskip;

    const bool below_realtime = emulated_frames * 100u < target_frames * 96u;
    const bool overloaded = late_frames * 4u > target_frames || display_busy_frames > 1u;
    if ((below_realtime || overloaded) && frameskip < 5u) return frameskip + 1u;

    const bool at_realtime = emulated_frames * 100u >= target_frames * 99u;
    const bool has_headroom = late_frames * 10u < target_frames && display_busy_frames == 0u;
    if (at_realtime && has_headroom && frameskip > 1u) return frameskip - 1u;
    return frameskip;
}
