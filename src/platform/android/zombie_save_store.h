#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace bo1::android::save_store {
// Storage envelope only; callers still validate their payload/schema/assets.
// Diagnostic payloads must never be offered by the normal Resume flow.
constexpr std::uint32_t DiagnosticPayload = 1;
constexpr std::uint32_t SoloResumePayload = 2;
constexpr std::size_t MaxMetadata = 16384;
constexpr std::size_t MaxGame = 0x200000;
constexpr std::size_t MaxClient = 0x140000;
struct Snapshot {
    std::uint64_t generation{};
    std::uint32_t flags{DiagnosticPayload};
    std::vector<unsigned char> metadata, game, client;
};
// Single writer, outside gameplay locks. root is the installed app user root.
// Alternating generations retain the last valid copy during interrupted writes.
bool Write(const char *root, const Snapshot &snapshot, std::string &error);
// Validates both generations and returns the newest intact one. Does not mutate
// gameplay or install data; output is unchanged on failure.
bool Read(const char *root, Snapshot &snapshot, std::string &error);
}
