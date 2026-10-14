#pragma once
#include <cstddef>
#include <cstdint>
namespace bo1::android {
struct ServerProfileSample { std::uintptr_t pc; const char *scriptPosition; int category; };
void BeginServerSamples(int mode, int scriptCategory);
void SetServerSampleCategory(int category);
void PublishServerScriptPosition(int instance, const char *position);
// Stops delivery, destroys the thread-owned timer and copies captured samples.
std::size_t EndServerSamples(ServerProfileSample *output, std::size_t capacity, unsigned *dropped);
}
