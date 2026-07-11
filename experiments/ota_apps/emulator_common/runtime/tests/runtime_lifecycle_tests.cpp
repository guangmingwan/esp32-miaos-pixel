#include "test_support.h"

int main() {
    MiaRuntimeLifecycle lifecycle{};
    FakeLifecycleApp app{};
    app.steps_before_exit = 3;

    require_status(mia_runtime_lifecycle_init(&lifecycle), MIA_RUNTIME_OK);
    require_true(lifecycle.state == MIA_RUNTIME_LIFECYCLE_READY, "lifecycle should start ready");
    require_status(mia_runtime_lifecycle_run(&lifecycle, fake_lifecycle_hooks(&app)), MIA_RUNTIME_OK);
    require_true(lifecycle.state == MIA_RUNTIME_LIFECYCLE_EXITED, "clean run should reach exited state");
    require_true(lifecycle.frame_count == 3, "run loop should count executed frames");
    require_true(app.exits == 1, "clean exit hook should run once");

    MiaRuntimeLifecycle failing{};
    FakeLifecycleApp bad{};
    bad.steps_before_exit = 2;
    bad.step_status = mia_runtime_error(MIA_RUNTIME_ERR_LIFECYCLE, "injected step failure");
    require_status(mia_runtime_lifecycle_init(&failing), MIA_RUNTIME_OK);
    require_status(mia_runtime_lifecycle_run(&failing, fake_lifecycle_hooks(&bad)), MIA_RUNTIME_ERR_LIFECYCLE);
    require_true(failing.state == MIA_RUNTIME_LIFECYCLE_RUNNING, "failed app should not be marked cleanly exited");
    require_true(bad.exits == 0, "clean exit hook should not run after step failure");
}
