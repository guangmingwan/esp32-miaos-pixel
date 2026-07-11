#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "mia_hardware_status.h"

void require_true(bool condition, const char *message);
void require_status(MiaHardwareStatus status, MiaHardwareStatusCode code);
std::string read_text_file(const char *relative_path);
std::vector<uint16_t> pixels(std::size_t count, uint16_t value);
