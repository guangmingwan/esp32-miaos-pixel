#include "mia_host_abi.h"
#include "display_host.h"

#include <array>
#include <cstdlib>
#include <cstdint>
#include <vector>

struct Capture {
  std::vector<uint8_t> bytes;
  std::vector<uint32_t> starts;
  std::vector<uint32_t> rows;
  const uint16_t *source = nullptr;
  bool retained_source = false;
  int fail_at = -1;
};

static void require(bool condition) {
  if (!condition) std::exit(1);
}

static int32_t capture_chunk(uint32_t y, uint32_t rows, const uint16_t *pixels, size_t count,
                             void *context) {
  Capture &capture = *static_cast<Capture *>(context);
  if (capture.fail_at == static_cast<int>(capture.starts.size())) return MIA_HOST_RESULT_IO;
  capture.starts.push_back(y);
  capture.rows.push_back(rows);
  capture.retained_source = capture.retained_source || pixels == capture.source;
  const uint8_t *bytes = reinterpret_cast<const uint8_t *>(pixels);
  capture.bytes.insert(capture.bytes.end(), bytes, bytes + count * sizeof(uint16_t));
  return MIA_HOST_RESULT_OK;
}

int main() {
  std::vector<uint16_t> padded(322 * 240, 0x1234);
  padded[0] = 0xF800;
  padded[1] = 0x07E0;
  padded[2] = 0x001F;
  padded[320] = 0xDEAD;
  padded[321] = 0xBEEF;
  const std::vector<uint16_t> original = padded;
  std::array<uint16_t, 320 * 7> staging{};
  Capture capture;
  capture.source = padded.data();

  require(display_host_test_transport_rgb565(nullptr, 320, 240, 640, 1, staging.data(), 8,
                                   capture_chunk, &capture) == MIA_HOST_RESULT_INVALID_ARGUMENT);
  require(display_host_test_transport_rgb565(reinterpret_cast<const uint16_t *>(
                                       reinterpret_cast<const uint8_t *>(padded.data()) + 1),
                                   320, 240, 644, 1, staging.data(), 8, capture_chunk,
                                   &capture) == MIA_HOST_RESULT_INVALID_ARGUMENT);
  require(display_host_test_transport_rgb565(padded.data(), 319, 240, 644, 1, staging.data(), 8,
                                   capture_chunk, &capture) == MIA_HOST_RESULT_INVALID_ARGUMENT);
  require(display_host_test_transport_rgb565(padded.data(), 320, 239, 644, 1, staging.data(), 8,
                                   capture_chunk, &capture) == MIA_HOST_RESULT_INVALID_ARGUMENT);
  require(display_host_test_transport_rgb565(padded.data(), 320, 240, 639, 1, staging.data(), 8,
                                   capture_chunk, &capture) == MIA_HOST_RESULT_INVALID_ARGUMENT);
  require(display_host_test_transport_rgb565(padded.data(), 320, 240, 638, 1, staging.data(), 8,
                                   capture_chunk, &capture) == MIA_HOST_RESULT_INVALID_ARGUMENT);
  require(display_host_test_transport_rgb565(padded.data(), 320, 240, 644, 0, staging.data(), 8,
                                   capture_chunk, &capture) == MIA_HOST_RESULT_NOT_READY);
  require(display_host_test_transport_rgb565(padded.data(), 320, 240, 644, 1, nullptr, 7,
                                              capture_chunk, &capture) ==
          MIA_HOST_RESULT_NOT_READY);
  require(display_host_test_transport_rgb565(padded.data(), 320, 240, 644, 1, staging.data(), 0,
                                              capture_chunk, &capture) ==
          MIA_HOST_RESULT_NOT_READY);
  require(display_host_test_transport_rgb565(padded.data(), 320, 240, 644, 1, staging.data(), 7,
                                              nullptr, &capture) == MIA_HOST_RESULT_NOT_READY);

  require(display_host_test_transport_rgb565(padded.data(), 320, 240, 644, 1, staging.data(), 7,
                                   capture_chunk, &capture) == MIA_HOST_RESULT_OK);
  require(capture.starts.size() == 35 && capture.starts.front() == 0 &&
          capture.starts.back() == 238);
  for (size_t chunk = 0; chunk < capture.starts.size(); ++chunk) {
    require(capture.starts[chunk] == chunk * 7);
  }
  require(capture.rows.front() == 7 && capture.rows.back() == 2);
  require(capture.bytes.size() == 320u * 240u * 2u);
  require(capture.bytes[0] == 0xF8 && capture.bytes[1] == 0x00);
  require(capture.bytes[2] == 0x07 && capture.bytes[3] == 0xE0);
  require(capture.bytes[4] == 0x00 && capture.bytes[5] == 0x1F);
  require(capture.bytes[6] == 0x12 && capture.bytes[7] == 0x34);
  require(capture.bytes[640] == 0x12 && capture.bytes[641] == 0x34);
  require(padded == original);
  require(!capture.retained_source);
  padded.assign(padded.size(), 0);
  require(capture.bytes[0] == 0xF8);

  Capture failure;
  failure.fail_at = 1;
  require(display_host_test_transport_rgb565(padded.data(), 320, 240, 644, 1, staging.data(), 7,
                                   capture_chunk, &failure) == MIA_HOST_RESULT_IO);
  require(failure.starts.size() == 1);
  return 0;
}
