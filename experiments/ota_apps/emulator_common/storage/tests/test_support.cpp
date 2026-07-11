#include "test_support.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string_view>

void require_true(bool condition, const char *message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

void require_status(MiaStorageStatus status, MiaStorageStatusCode code) {
    if (status.code != code) {
        std::cerr << "expected status " << code << ", got " << status.code << ": " << status.message << '\n';
        std::exit(1);
    }
}

std::filesystem::path make_temp_tree(const char *name) {
    auto root = std::filesystem::temp_directory_path() / (std::string("mia-storage-") + name + "-XXXXXX");
    std::string pattern = root.string();
    char *created = mkdtemp(pattern.data());
    require_true(created != nullptr, "mkdtemp should create a unique root");
    return std::filesystem::path(created);
}

std::vector<MiaStoragePickerEntry> picker_entries(const MiaStoragePickerResult &result) {
    return std::vector<MiaStoragePickerEntry>(result.entries, result.entries + result.count);
}

std::vector<uint8_t> bytes(std::string_view value) {
    return std::vector<uint8_t>(value.begin(), value.end());
}
