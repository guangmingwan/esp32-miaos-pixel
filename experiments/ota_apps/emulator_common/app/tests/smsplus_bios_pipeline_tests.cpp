#include "mia_emulator_smsplus.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>

struct PipelineState {
    MiaSmsPlusBiosStatus read_status{MIA_SMSPLUS_BIOS_OK};
    size_t read_size{MIA_SMSPLUS_COLECO_BIOS_SIZE};
    int hash_result{};
    std::array<uint8_t, MIA_SMSPLUS_SHA1_SIZE> digest{};
    bool hash_saw_read_data{};
    bool started{};
};

static void require_true(bool condition, const char *message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

static MiaSmsPlusBiosStatus synthetic_read(void *context, MiaSmsPlusBiosReadRequest *request) {
    auto &state = *static_cast<PipelineState *>(context);
    if (state.read_status != MIA_SMSPLUS_BIOS_OK) return state.read_status;
    require_true(request->capacity >= state.read_size, "reader receives bounded destination");
    std::fill_n(request->buffer, state.read_size, 0x5au);
    request->size = state.read_size;
    return MIA_SMSPLUS_BIOS_OK;
}

static int synthetic_hash(void *context, MiaSmsPlusBiosHashRequest *request) {
    auto &state = *static_cast<PipelineState *>(context);
    state.hash_saw_read_data = request->size == MIA_SMSPLUS_COLECO_BIOS_SIZE && request->buffer[0] == 0x5au;
    std::copy(state.digest.begin(), state.digest.end(), request->digest);
    return state.hash_result;
}

static void synthetic_start(void *context, uint8_t *bios) {
    auto &state = *static_cast<PipelineState *>(context);
    state.started = bios[0] == 0x5au;
}

static PipelineState canonical_state() {
    PipelineState state;
    state.digest = {0x45, 0xbe, 0xdc, 0x4c, 0xbd, 0xea, 0xc6, 0x6c, 0x7d, 0xf5,
                    0x9e, 0x9e, 0x59, 0x91, 0x95, 0xc7, 0x78, 0xd8, 0x6a, 0x92};
    return state;
}

static MiaSmsPlusBiosStatus run_pipeline(PipelineState &state) {
    std::array<uint8_t, MIA_SMSPLUS_COLECO_BIOS_SIZE + 1u> buffer{};
    const MiaSmsPlusBiosPipeline pipeline{buffer.data(), buffer.size(), synthetic_read,
                                          synthetic_hash, synthetic_start, &state};
    return mia_smsplus_load_validate_coleco_bios(&pipeline);
}

static void test_canonical_digest_starts_only_after_read_and_hash() {
    PipelineState state = canonical_state();
    require_true(run_pipeline(state) == MIA_SMSPLUS_BIOS_OK, "canonical pipeline succeeds");
    require_true(state.hash_saw_read_data, "hasher observes bytes produced by reader");
    require_true(state.started, "core start receives validated BIOS only after hashing");
}

static void test_noncanonical_digests_block_core_start() {
    PipelineState pal = canonical_state();
    pal.digest = {0x16, 0x00, 0x77, 0xaf, 0xb1, 0x39, 0x94, 0x37, 0x25, 0xc6,
                  0x34, 0xd6, 0x53, 0x98, 0x98, 0xdb, 0x59, 0xf3, 0x36, 0x57};
    require_true(run_pipeline(pal) == MIA_SMSPLUS_BIOS_CHECKSUM_INVALID, "PAL digest is rejected");
    require_true(pal.hash_saw_read_data && !pal.started, "PAL failure occurs after hash and before start");

    PipelineState mutation = canonical_state();
    mutation.digest.back() ^= 0x01u;
    require_true(run_pipeline(mutation) == MIA_SMSPLUS_BIOS_CHECKSUM_INVALID, "mutation is rejected");
    require_true(!mutation.started, "mutation never reaches core start");
}

static void test_read_and_hash_failures_are_typed_and_block_start() {
    PipelineState read_failure = canonical_state();
    read_failure.read_status = MIA_SMSPLUS_BIOS_IO_FAILED;
    require_true(run_pipeline(read_failure) == MIA_SMSPLUS_BIOS_IO_FAILED, "read failure remains typed");
    require_true(!read_failure.hash_saw_read_data && !read_failure.started, "read failure blocks hash and start");

    PipelineState hash_failure = canonical_state();
    hash_failure.hash_result = -1;
    require_true(run_pipeline(hash_failure) == MIA_SMSPLUS_BIOS_HASH_FAILED, "hash failure remains typed");
    require_true(hash_failure.hash_saw_read_data && !hash_failure.started, "hash failure follows read and blocks start");
}

int main() {
    test_canonical_digest_starts_only_after_read_and_hash();
    test_noncanonical_digests_block_core_start();
    test_read_and_hash_failures_are_typed_and_block_start();
    return 0;
}
