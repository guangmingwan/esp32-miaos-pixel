#include "mia_nes_contract.h"

#include <assert.h>
#include <string.h>

int main(void) {
    uint8_t ines[16] = {'N', 'E', 'S', 0x1a, 1, 1, 0, 0};
    assert(mia_nes_validate_extension("game.nes") == MIA_NES_OK);
    assert(mia_nes_validate_extension("game.fds") == MIA_NES_OK);
    assert(mia_nes_validate_extension("game.zip") == MIA_NES_UNSUPPORTED_FILE);
    assert(mia_nes_validate_image(ines, sizeof(ines), 0) == MIA_NES_OK);
    ines[6] = 0xd0;
    assert(mia_nes_validate_image(ines, sizeof(ines), 0) == MIA_NES_UNSUPPORTED_MAPPER);
    memcpy(ines, "FDS\x1a", 4);
    assert(mia_nes_validate_image(ines, sizeof(ines), 0) == MIA_NES_FDS_BIOS_REQUIRED);
    assert(mia_nes_validate_image(ines, sizeof(ines), 1) == MIA_NES_OK);
    memset(ines, 0, sizeof(ines));
    assert(mia_nes_validate_image(ines, sizeof(ines), 0) == MIA_NES_INVALID_HEADER);
    return 0;
}
