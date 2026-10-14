#if defined(__ANDROID__)
#include <platform/win32_compat/native_d3dx9_api.h>
#else
#include <d3dx9.h>
#endif
#include <cstdio>
#include <cstdlib>
#include <cstring>
static void require(bool condition, const char *message) {
    if (!condition) { std::fprintf(stderr, "FAIL %s\n", message); std::abort(); }
}
static DWORD reg(unsigned type, unsigned index) {
    return 0x800f0000u | ((type & 7u) << 28) | ((type & 24u) << 8) | index;
}
static void inspect(const char *name, const DWORD *shader, bool output) {
    _D3DXSEMANTIC values[MAXD3DDECLLENGTH]{};
    UINT count = 999, queried = 999;
    auto function = output ? D3DXGetShaderOutputSemantics : D3DXGetShaderInputSemantics;
    require(function(shader, values, &count) == S_OK, "valid shader");
    require(function(shader, nullptr, &queried) == S_OK && queried == count, "count-only query");
    std::printf("%s count=%u", name, count);
    for (UINT i = 0; i < count; ++i) std::printf(" %u:%u", values[i].Usage, values[i].UsageIndex);
    std::puts("");
}
int main() {
    const DWORD vertex[] = {
        D3DVS_VERSION(3,0), 0x0003fffe, 0x42415443, 0x800f0803, 0xffff0000,
        0x0200001f, 0x80000000, reg(D3DSPR_INPUT,0),
        0x0200001f, 0x80010005, reg(D3DSPR_INPUT,1),
        0x0200001f, 0x80000000, reg(D3DSPR_OUTPUT,0),
        0x0200001f, 0x80030005, reg(D3DSPR_OUTPUT,1),
        0x02000001, reg(D3DSPR_OUTPUT,0), reg(D3DSPR_INPUT,0), D3DSIO_END};
    const DWORD pixel[] = {
        D3DPS_VERSION(3,0), 0x0200001f, 0x80030005, reg(D3DSPR_INPUT,0),
        0x05000051, reg(D3DSPR_CONST,0), 0x800f0803, 0, 0, 0,
        0x02000001, reg(D3DSPR_COLOROUT,2), reg(D3DSPR_INPUT,0),
        0x02000001, reg(D3DSPR_COLOROUT,0), reg(D3DSPR_INPUT,0),
        0x02000001, reg(D3DSPR_COLOROUT,2), reg(D3DSPR_INPUT,0),
        0x02000001, reg(D3DSPR_DEPTHOUT,0), reg(D3DSPR_TEMP,0), D3DSIO_END};
    inspect("vs3 input", vertex, false); inspect("vs3 output", vertex, true);
    inspect("ps3 input", pixel, false); inspect("ps3 output", pixel, true);
    UINT untouched = 123;
    require(D3DXGetShaderInputSemantics(nullptr, nullptr, &untouched) == D3DERR_INVALIDCALL && untouched == 123, "null shader output unchanged");
    std::puts("PASS shader-model3 declarations, comments, literals, outputs and count-only queries");
}
