#pragma once

#include "hal/wdt_hal.h"
#include "soc/timer_group_struct.h"

struct ScopedIntWdtPause {
  wdt_hal_context_t ctx{};
  bool active;
  ScopedIntWdtPause() : active(false) {
    ctx.inst = WDT_MWDT1;
    ctx.mwdt_dev = &TIMERG1;
    wdt_hal_write_protect_disable(&ctx);
    wdt_hal_disable(&ctx);
    wdt_hal_write_protect_enable(&ctx);
    active = true;
  }
  ~ScopedIntWdtPause() {
    if (active) {
      wdt_hal_write_protect_disable(&ctx);
      wdt_hal_enable(&ctx);
      wdt_hal_feed(&ctx);
      wdt_hal_write_protect_enable(&ctx);
    }
  }
  ScopedIntWdtPause(const ScopedIntWdtPause &) = delete;
  ScopedIntWdtPause &operator=(const ScopedIntWdtPause &) = delete;
};
