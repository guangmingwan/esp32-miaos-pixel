#pragma once

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define MIA_HOST_LAUNCH_CONTEXT_PATH "/sd/MiaOS/.launch"
#define MIA_HOST_LAUNCH_ARG_SIZE 256

/* Consume a launch argument written by the launcher for this app. The first
 * line identifies the target app; an unrelated app leaves the marker intact. */
static inline int mia_host_consume_launch_arg(const char *expected_app,
                                              char *out_path, size_t out_size) {
    if (expected_app == NULL || out_path == NULL || out_size == 0) return 0;
    out_path[0] = '\0';

    FILE *file = fopen(MIA_HOST_LAUNCH_CONTEXT_PATH, "rb");
    if (file == NULL) return 0;

    char target[64] = {0};
    char path[MIA_HOST_LAUNCH_ARG_SIZE] = {0};
    const int target_ok = fgets(target, sizeof(target), file) != NULL;
    const int path_ok = fgets(path, sizeof(path), file) != NULL;
    fclose(file);

    if (!target_ok || !path_ok) return 0;
    target[strcspn(target, "\r\n")] = '\0';
    path[strcspn(path, "\r\n")] = '\0';
    if (strcmp(target, expected_app) != 0 || path[0] == '\0') return 0;

    remove(MIA_HOST_LAUNCH_CONTEXT_PATH);
    if (snprintf(out_path, out_size, "%s", path) >= (int)out_size) {
        out_path[0] = '\0';
        return 0;
    }
    return 1;
}
