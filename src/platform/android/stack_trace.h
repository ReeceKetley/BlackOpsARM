#pragma once
#include <cstddef>
namespace bo1::android {
std::size_t CaptureStackTrace(char *output, std::size_t capacity, unsigned skipFrames);
}
