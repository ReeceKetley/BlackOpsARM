#pragma once
#include <cstddef>
namespace bo1::android {
// Bounded named-zone owner, not the legacy PMem low/high pool or tracker.
// Main-thread only. Begin -> DB allocations -> End -> clear streams/blocks -> Release.
void ZoneMemoryBegin(const char *name, unsigned allocType, int capacity);
void ZoneMemoryEnd();
void ZoneMemoryRelease(const char *name, unsigned allocType);
std::size_t ZoneMemoryCount();
std::size_t ZoneMemoryAllocationCount();
std::size_t ZoneMemoryRequestedBytes();
}
