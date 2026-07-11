#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "mia_storage.h"

void require_true(bool condition, const char *message);
void require_status(MiaStorageStatus status, MiaStorageStatusCode code);
std::filesystem::path make_temp_tree(const char *name);
std::vector<MiaStoragePickerEntry> picker_entries(const MiaStoragePickerResult &result);
std::vector<uint8_t> bytes(std::string_view value);
