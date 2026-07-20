/*
 * Stub for SDLPAL's common.h unconditional include of "adplug/opltypes.h".
 * The MiaOS lavapal port uses the bundled vendor/rix_adplug emulator which
 * does not depend on upstream libadplug; this header provides the type and
 * constant names that palcfg.h / unix.cpp reference.
 */
#ifndef ADPLUG_OPLTYPES_H
#define ADPLUG_OPLTYPES_H

#include <stdint.h>

typedef enum {
	OPL_TYPE_OPL2 = 0,
	OPL_TYPE_OPL3 = 1,
	OPL_TYPE_DUAL_OPL2 = 2
} OPL_TYPE;

/* SDLPAL configuration enums consumed by palcfg.h and unix.cpp. */
typedef enum {
	OPLCORE_DBFLT = 0,
	OPLCORE_NUKED = 1,
	OPLCORE_MAX
} OPLCORE_TYPE;

typedef enum {
	OPLCHIP_OPL2 = 0,
	OPLCHIP_OPL3 = 1,
	OPLCHIP_MAX = 2
} OPLCHIP_TYPE;

#endif /* ADPLUG_OPLTYPES_H */
