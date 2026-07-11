#include "test_support.h"

int main() {
    FakeClock clock{{1000, 2000, 1500, 4000}, {}, false, 0};
    MiaRuntimeClock monotonic = fake_clock(&clock);
    uint64_t now = 0;
    require_status(mia_runtime_clock_now(&monotonic, &now), MIA_RUNTIME_OK);
    require_true(now == 1000, "first monotonic timestamp should pass through");
    require_status(mia_runtime_clock_now(&monotonic, &now), MIA_RUNTIME_OK);
    require_true(now == 2000, "second monotonic timestamp should pass through");
    require_status(mia_runtime_clock_now(&monotonic, &now), MIA_RUNTIME_ERR_CLOCK_BACKWARDS);

    FakeClock pacing_clock{{1000, 5000, 17000}, {}, false, 0};
    MiaRuntimeFramePacer pacer{};
    require_status(mia_runtime_frame_pacer_init(&pacer, fake_clock(&pacing_clock), 60), MIA_RUNTIME_OK);
    uint64_t waited = 0;
    require_status(mia_runtime_frame_pacer_step(&pacer, &waited), MIA_RUNTIME_OK);
    require_true(waited == 0, "first frame should not wait");
    require_status(mia_runtime_frame_pacer_step(&pacer, &waited), MIA_RUNTIME_OK);
    require_true(waited == 12666, "second frame should wait until the 60 Hz deadline");
    require_true(pacing_clock.waits == std::vector<uint64_t>{12666}, "fake clock should record frame delay");

    FakeClock failing_wait{{1000, 2000}, {}, false, 0};
    failing_wait.fail_wait = true;
    require_status(mia_runtime_frame_pacer_init(&pacer, fake_clock(&failing_wait), 60), MIA_RUNTIME_OK);
    require_status(mia_runtime_frame_pacer_step(&pacer, &waited), MIA_RUNTIME_OK);
    require_status(mia_runtime_frame_pacer_step(&pacer, &waited), MIA_RUNTIME_ERR_CLOCK_WAIT_FAILED);
}
