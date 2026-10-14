#include <cfenv>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#pragma STDC FENV_ACCESS ON
int MSG_RoundFloatToInt(float value);
static void require(bool condition, const char *message) {
    if (!condition) { std::fprintf(stderr, "FAIL %s\n", message); std::abort(); }
}
int main() {
    const int modes[] = {FE_TONEAREST, FE_UPWARD, FE_DOWNWARD, FE_TOWARDZERO};
    const float input[] = {0.25f, -0.25f, 1.75f, -1.75f};
    const int expected[4][4] = {{0,0,2,-2}, {1,0,2,-1}, {0,-1,1,-2}, {0,0,1,-1}};
    for (int mode = 0; mode < 4; ++mode) {
        require(std::fesetround(modes[mode]) == 0, "set rounding mode");
        for (int i = 0; i < 4; ++i) require(MSG_RoundFloatToInt(input[i]) == expected[mode][i], "current mode conversion");
    }
    require(std::fesetround(FE_TONEAREST) == 0, "restore nearest");
    const float halves[] = {0.5f,1.5f,2.5f,-0.5f,-1.5f,-2.5f};
    const int ties[] = {0,2,2,0,-2,-2};
    for (int i = 0; i < 6; ++i) require(MSG_RoundFloatToInt(halves[i]) == ties[i], "nearest even ties");
    require(MSG_RoundFloatToInt(2147483520.0f) == 2147483520, "largest valid positive float");
    std::feclearexcept(FE_ALL_EXCEPT);
    require(MSG_RoundFloatToInt(-2147483648.0f) == INT_MIN && !(std::fetestexcept(FE_INVALID)), "valid negative limit");
    const float invalid[] = {INFINITY, -INFINITY, NAN, 2147483648.0f, -4294967296.0f};
    for (float value : invalid) {
        std::feclearexcept(FE_ALL_EXCEPT);
        require(MSG_RoundFloatToInt(value) == INT_MIN && std::fetestexcept(FE_INVALID), "invalid sentinel and exception");
    }
    std::feclearexcept(FE_ALL_EXCEPT);
    require(MSG_RoundFloatToInt(1.25f) == 1 && std::fetestexcept(FE_INEXACT), "fractional inexact exception");
    std::puts("PASS original message rounding: four modes, even ties, limits, invalid sentinel and FP exceptions");
}
