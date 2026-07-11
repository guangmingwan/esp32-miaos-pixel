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
    return 0;
}
