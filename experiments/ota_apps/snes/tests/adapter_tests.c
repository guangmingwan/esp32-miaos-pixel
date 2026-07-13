#include "mia_snes_contract.h"

#include <assert.h>

int main(void) {
    assert(mia_snes_extension_supported("game.sfc"));
    assert(mia_snes_extension_supported("game.smc"));
    assert(!mia_snes_extension_supported("game.zip"));
    MiaSnesPacing pacing = {0};
    mia_snes_pacing_record(&pacing, 17000, 16667, 1);
    mia_snes_pacing_record(&pacing, 10000, 16667, 0);
    assert(pacing.frames == 2);
    assert(pacing.late_frames == 1);
    assert(pacing.skipped_frames == 1);
    assert(mia_snes_adjust_frameskip(3, 60, 60, 0, 0) == 2);
    assert(mia_snes_adjust_frameskip(2, 55, 60, 0, 0) == 3);
    assert(mia_snes_adjust_frameskip(2, 60, 60, 16, 0) == 3);
    assert(mia_snes_adjust_frameskip(2, 60, 60, 0, 2) == 3);
    assert(mia_snes_adjust_frameskip(1, 60, 60, 0, 0) == 1);
    assert(mia_snes_adjust_frameskip(5, 40, 60, 30, 3) == 5);
    return 0;
}
