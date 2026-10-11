#include <universal/com_math_anglevectors.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <initializer_list>
#if defined(BO1_MATH_WINDOWS_ORACLE)
#include "legacy_axis_oracle.inc"
// MSVC resolves this unused external before discarding AnglesToQuat. The
// bounded probe does not exercise quaternion conversion; make accidental use
// fail explicitly. This harness definition is never linked into engine/APK.
void __cdecl AxisToQuat(const float (*)[3], float *)
{
    std::fputs("Unsupported in the axis probe: AxisToQuat\n", stderr);
    std::abort();
}
#endif
// A failing engine assertion must terminate the standalone probe.
bool Assert_MyHandler(const char *file, int line, int, const char *, ...)
{
    std::fprintf(stderr, "Engine assertion: %s:%d\n", file, line);
    std::abort();
}
static void test(const float *input)
{
    float axis[3][3], output[3];
    AnglesToAxis(input, axis);
    AxisToSignedAngles(axis, output);
    for (int i = 0; i < 3; ++i) {
        if (!std::isfinite(output[i]) || std::fabs(std::remainder(output[i] - input[i], 360.f)) > 0.002f) {
            std::fprintf(stderr, "Axis roundtrip failed [%f %f %f] -> [%f %f %f]\n",
                input[0], input[1], input[2], output[0], output[1], output[2]);
            std::abort();
        }
    }
#if defined(BO1_MATH_WINDOWS_ORACLE)
    float original[3];
    LegacyAxisToSignedAngles(axis, original);
    for (int i = 0; i < 3; ++i) if (output[i] != original[i]) std::abort();
#endif
}
int main()
{
    unsigned int cases = 0;
    for (float pitch : {-89.f, -45.f, 0.f, 45.f, 89.f})
        for (int yaw = -180; yaw <= 180; yaw += 30)
            for (int roll = -180; roll <= 180; roll += 30) {
                const float input[] = {pitch, static_cast<float>(yaw), static_cast<float>(roll)};
                test(input); ++cases;
            }
    uint32_t seed = 0x12345678;
    for (int n = 0; n < 5000; ++n) {
        float input[3];
        for (int i = 0; i < 3; ++i) {
            seed = 1664525u * seed + 1013904223u;
            const float width = i ? 360.f : 178.f;
            input[i] = static_cast<float>(seed >> 8) / 16777216.f * width - width / 2.f;
        }
        test(input); ++cases;
    }
    std::printf("PASS actual axis maths: %u angle/axis roundtrips (0.002 degree tolerance)", cases);
#if defined(BO1_MATH_WINDOWS_ORACLE)
    std::printf("; bit-identical to original Windows AxisToSignedAngles");
#endif
    std::puts("");
}
