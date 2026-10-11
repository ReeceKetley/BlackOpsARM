#pragma once
#if !(defined(__ANDROID__) && defined(__aarch64__)) && !defined(BO1_CRT_RNG_HOST_TEST)
#error Android engine RNG requires ARM64 or the explicit standalone host oracle
#endif
namespace bo1::android {
int CrtRand();
void CrtSrand(unsigned int seed);
}
