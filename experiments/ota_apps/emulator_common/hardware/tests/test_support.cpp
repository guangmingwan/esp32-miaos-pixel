#include "test_support.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

void require_true(bool condition, const char *message) {
    if (!condition) {
        std::cerr << message << std::endl;
        std::exit(1);
    }
}

void require_status(MiaHardwareStatus status, MiaHardwareStatusCode code) {
    if (status.code != code) {
        std::cerr << "expected status " << static_cast<int>(code) << " got " << static_cast<int>(status.code) << ": " << status.message << std::endl;
        std::exit(1);
    }
}

std::string read_text_file(const char *relative_path) {
    const char *root = std::getenv("MIA_HARDWARE_REPO_ROOT");
    require_true(root != nullptr, "MIA_HARDWARE_REPO_ROOT must be set");
    std::ifstream input(std::filesystem::path(root) / relative_path);
    require_true(input.good(), "expected template file to be readable");
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

std::vector<uint16_t> pixels(std::size_t count, uint16_t value) {
    return std::vector<uint16_t>(count, value);
}
