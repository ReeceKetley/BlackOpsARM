#include <xanim/xmodel.h>
#include <xanim/dobj.h>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace {
struct Output {
    std::uint64_t before[4];
    float axis[4][3];
    std::uint64_t after[4];
};
int checks;

bool run(const char *name, const float quat[4], const float translation[3], const float expected[4][3])
{
    DObjAnimMat input{};
    std::memcpy(input.quat, quat, sizeof(input.quat));
    std::memcpy(input.trans, translation, sizeof(input.trans));
    input.transWeight = 2.0f;
    const DObjAnimMat unchanged = input;
    Output output;
    std::memset(&output, 0xA5, sizeof(output));
    ConvertQuatToInverseMat(&input, output.axis);
    for (unsigned row = 0; row < 4; ++row) {
        for (unsigned column = 0; column < 3; ++column) {
            const float value = output.axis[row][column];
            if (!std::isfinite(value) || std::fabs(value - expected[row][column]) > 2.0e-6f) {
                std::printf("FAIL %s [%u][%u] actual=%g expected=%g\n", name, row, column, value, expected[row][column]);
                return false;
            }
            ++checks;
        }
    }
    for (unsigned i = 0; i < 4; ++i) {
        if (output.before[i] != UINT64_C(0xA5A5A5A5A5A5A5A5) || output.after[i] != UINT64_C(0xA5A5A5A5A5A5A5A5)) {
            std::printf("FAIL %s output canary\n", name);
            return false;
        }
    }
    ++checks;
    if (std::memcmp(&input, &unchanged, sizeof(input)) != 0) {
        std::printf("FAIL %s input changed\n", name);
        return false;
    }
    ++checks;
    return true;
}
}

int main()
{
    // Expectations use geometric inverse rotations and inverse translated origin,
    // independent of the production quaternion product formulas.
    const float identity[4] = {0, 0, 0, 1};
    const float translationA[3] = {3, -5, 7};
    const float expectedA[4][3] = {{1,0,0}, {0,1,0}, {0,0,1}, {-3,5,-7}};
    const float z90[4] = {0, 0, 0.7071067811865475f, 0.7071067811865475f};
    const float translationB[3] = {2, -3, 5};
    const float expectedB[4][3] = {{0,-1,0}, {1,0,0}, {0,0,1}, {3,2,-5}};
    // 120 degrees around (1,1,1) cyclically permutes coordinate axes.
    const float cyclic120[4] = {0.5f, 0.5f, 0.5f, 0.5f};
    const float translationC[3] = {-4, 6, 9};
    const float expectedC[4][3] = {{0,0,1}, {1,0,0}, {0,1,0}, {-6,-9,4}};
    if (!run("identity", identity, translationA, expectedA)
        || !run("z90", z90, translationB, expectedB)
        || !run("cyclic120", cyclic120, translationC, expectedC))
        return 1;
    std::printf("PASS %d actual inverse matrix numerical/input/canary checks\n", checks);
    return 0;
}
