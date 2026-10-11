#pragma once

// T5 assumes the Windows CRT's per-thread 15-bit stream. Keep libc/vendor RNG
// separate on Android; explicit calls avoid altering system declarations.
#if defined(__ANDROID__)
#include <platform/android/engine_random.h>
inline int BO1_CrtRand() { return bo1::android::CrtRand(); }
inline void BO1_CrtSrand(unsigned int seed) { bo1::android::CrtSrand(seed); }
#else
#include <cstdlib>
inline int BO1_CrtRand() { return std::rand(); }
inline void BO1_CrtSrand(unsigned int seed) { std::srand(seed); }
#endif
