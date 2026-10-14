#pragma once
#include <cstddef>
#include <cstdint>

namespace bo1::android {
// Real mapped virtual address space, including reserved mappings, not RSS.
bool ProcessVirtualBytes(std::uint64_t *bytes);
// Dynamic ELF symbol when available; otherwise real module + native offset.
bool ProfileSymbolName(const void *address, char *name, std::size_t capacity);
}
