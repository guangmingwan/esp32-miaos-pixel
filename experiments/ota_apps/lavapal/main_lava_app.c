/* Native PAL source bundle used by the ESP32 OTA application. */

#include "main.c"
#define malloc PAL_TMP_ALLOC
#define free PAL_TMP_FREE
#include "yj1.c"
#undef free
#undef malloc
#include "lava_fight.c"
#include "lava_uibattle.c"
#include "lava_battle.c"
