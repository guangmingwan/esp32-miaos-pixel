#include "mia_text_api.h"

#include "droid_gbk_renderer.h"

static const MiaTextApi MIA_TEXT_API = {
    .abi_version = MIA_TEXT_ABI_VERSION,
    .struct_size = sizeof(MiaTextApi),
    .draw_text = droid_gbk_draw_text,
};

__attribute__((visibility("default")))
const MiaTextApi *mia_text_get_api(uint32_t requested_version) {
  return requested_version == MIA_TEXT_ABI_VERSION ? &MIA_TEXT_API : 0;
}
