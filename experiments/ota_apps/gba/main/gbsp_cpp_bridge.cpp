extern "C" {
#include "common.h"
}

extern "C" void mia_gbsp_execute_arm(u32 cycles) {
    execute_arm(cycles);
}
