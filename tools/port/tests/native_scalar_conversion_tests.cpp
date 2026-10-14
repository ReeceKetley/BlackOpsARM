#include <platform/android/scalar_conversion.h>
#include <cstdio>
#include <cstdlib>

static int checks;
static void check(double input, std::int32_t expected, bool invalid = false) {
    volatile double observed = input;
    std::feclearexcept(FE_ALL_EXCEPT);
    const auto result = bo1::android::RoundedDoubleToInt32(observed);
    const bool raised = (std::fetestexcept(FE_INVALID) != 0);
    if (result != expected || raised != invalid) {
        std::fprintf(stderr, "conversion failed input=%.17g actual=%d expected=%d invalid=%d/%d\n",
            input, result, expected, raised, invalid);
        std::exit(1);
    }
    ++checks;
}
int main() {
    const int prior = std::fegetround();
    const int modes[] = {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO};
    const std::int32_t positives[] = {2, 1, 2, 1};
    const std::int32_t negatives[] = {-2, -2, -1, -1};
    for (int i = 0; i < 4; ++i) {
        if (std::fesetround(modes[i]) != 0) return 2;
        check(1.5, positives[i]);
        check(-1.5, negatives[i]);
        check(0.0, 0);
        check(-0.0, 0);
        check(2147483647.0, 2147483647);
        check(-2147483648.0, INT32_MIN);
        check(2147483648.0, INT32_MIN, true);
        check(-2147483649.0, INT32_MIN, true);
        check(std::numeric_limits<double>::infinity(), INT32_MIN, true);
        check(-std::numeric_limits<double>::infinity(), INT32_MIN, true);
        check(std::numeric_limits<double>::quiet_NaN(), INT32_MIN, true);
    }
    if (std::fesetround(FE_TONEAREST) != 0) return 2;
    check(2.5, 2);
    check(3.5, 4);
    check(-2.5, -2);
    check(-3.5, -4);
    check(1.5 + 9.313225746154785e-10, 2);
    check(-1.5 + 9.313225746154785e-10, -1);
    check(2147483647.5, INT32_MIN, true);
    check(-2147483648.5, INT32_MIN);
    if (std::fesetround(prior) != 0) return 2;
    std::printf("native scalar conversion: %d checks passed\n", checks);
}
