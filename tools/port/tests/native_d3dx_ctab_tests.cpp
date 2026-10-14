#if defined(__ANDROID__)
#include <platform/android/d3dx_ctab.h>
#else
#include <d3dx9.h>
#endif
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>
static void require(bool value, const char *message) {
    if (!value) { std::fprintf(stderr, "FAIL %s\n", message); std::abort(); }
}
int main() {
    std::vector<DWORD> shader(26);
    shader[0] = D3DVS_VERSION(3,0); shader[1] = (23u << 16) | D3DSIO_COMMENT;
    shader[2] = 0x42415443; shader[25] = D3DSIO_END;
    unsigned char *data = reinterpret_cast<unsigned char *>(shader.data() + 3);
    _D3DXSHADER_CONSTANTTABLE header{28,70,shader[0],1,28,0,78};
    _D3DXSHADER_CONSTANTINFO constant{64,2,0,4,0,48,0};
    const WORD type[] = {3,3,4,4,1,0,0,0};
    std::memcpy(data, &header, sizeof(header)); std::memcpy(data + 28, &constant, sizeof(constant));
    std::memcpy(data + 48, type, sizeof(type));
    std::memcpy(data + 64, "world", 6); std::memcpy(data + 70, "fixture", 8); std::memcpy(data + 78, "vs_3_0", 7);
    unsigned char expected[88]; std::memcpy(expected, data, sizeof(expected));
    ID3DXConstantTable *table = nullptr;
    require(D3DXGetShaderConstantTable(shader.data(), &table) == S_OK && table, "real CTAB owner");
    require(table->GetBufferSize() == sizeof(expected), "exact CTAB size");
    require(!std::memcmp(table->GetBufferPointer(), expected, sizeof(expected)), "exact CTAB contents");
    std::memset(data, 0x5a, sizeof(expected));
    require(!std::memcmp(table->GetBufferPointer(), expected, sizeof(expected)), "independent ownership");
    require(table->AddRef() == 2 && table->Release() == 1, "retained lifetime");
    void *identity = nullptr;
    require(table->QueryInterface(IID_IUnknown, &identity) == S_OK && identity, "IUnknown identity");
    static_cast<IUnknown *>(identity)->Release();
    require(table->Release() == 0, "final release");
    DWORD empty[] = {D3DVS_VERSION(3,0), D3DSIO_END};
    table = reinterpret_cast<ID3DXConstantTable *>(static_cast<std::uintptr_t>(1));
    const HRESULT noTable = D3DXGetShaderConstantTable(empty, &table);
    std::printf("no CTAB hr=0x%08x null=%d\n", static_cast<unsigned>(noTable), !table);
    std::fflush(stdout);
    require(noTable == static_cast<HRESULT>(0x88760b59u) && !table, "absence contract");
#if defined(__ANDROID__)
    std::memcpy(data, expected, sizeof(expected));
    require(bo1::android::ShaderConstantTable(shader.data(), 2, &table) < 0 && !table, "bounded truncated comment");
    constant.Name = 88; std::memcpy(data + 28, &constant, sizeof(constant));
    require(bo1::android::ShaderConstantTable(shader.data(), shader.size(), &table) < 0 && !table, "invalid string offset");
    constant.Name = 64; constant.TypeInfo = 88; std::memcpy(data + 28, &constant, sizeof(constant));
    require(bo1::android::ShaderConstantTable(shader.data(), shader.size(), &table) < 0 && !table, "invalid type offset");
#endif
    std::puts("PASS CTAB byte identity, independent ownership, references and absence");
}
