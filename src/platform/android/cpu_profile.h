#pragma once
#include <cstddef>
#include <cstdint>
namespace bo1::android {
struct CpuThread;
// Samples own an open proc task descriptor and verify kernel start-time identity.
CpuThread *OpenCpuThread(unsigned tid);
void CloseCpuThread(CpuThread *thread);
bool ReadCpuThread(CpuThread *thread, std::uint64_t *nanoseconds);
bool CpuThreadName(CpuThread *thread, char *name, std::size_t capacity);
bool EnumerateCpuThreads(unsigned *ids, std::size_t capacity, std::size_t *count);
bool ProcessCpuNanoseconds(std::uint64_t *nanoseconds);
bool CurrentCpuNanoseconds(std::uint64_t *nanoseconds);
}
