#pragma once

#include <stdbool.h>

#include "ota_app_manifest.h"

struct LauncherReturnContext {
  char category[MIA_MANIFEST_CATEGORY_SIZE];
  char name[MIA_MANIFEST_NAME_SIZE];
};

bool miaLauncherReturnContextSave(const char *category, const char *name);
bool miaLauncherReturnContextLoad(LauncherReturnContext *context);
bool miaLauncherReturnContextClear();
