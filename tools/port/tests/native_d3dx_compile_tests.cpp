#if defined(__ANDROID__)
#include <platform/win32_compat/native_d3dx9_api.h>
#else
#include <d3dx9.h>
#endif
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
static void require(bool condition, const char *message) {
    if (!condition) { std::fprintf(stderr, "FAIL %s\n", message); std::abort(); }
}
static void release(ID3DXBuffer *buffer) { if (buffer) buffer->Release(); }
struct Type { WORD kind, type, rows, columns, elements, members; DWORD memberInfo; };
static void matrix(DWORD packing) {
    const char *source = "float4x3 transform; float4 shade(float4 p:POSITION):POSITION{return float4(mul(p,transform),1);}";
    ID3DXBuffer *shader = nullptr, *messages = nullptr; ID3DXConstantTable *table = nullptr;
    HRESULT hr = D3DXCompileShader(source, static_cast<UINT>(std::strlen(source)), nullptr, nullptr,
        "shade", "vs_3_0", 2u | 65536u | packing, &shader, &messages, &table);
    if (hr < 0 && messages) std::fprintf(stderr, "%s\n", static_cast<char *>(messages->GetBufferPointer()));
    require(hr == S_OK && shader && table, "real vertex compile and CTAB");
    require(*static_cast<DWORD *>(shader->GetBufferPointer()) == D3DVS_VERSION(3,0), "actual vertex profile");
    const auto *data = static_cast<const unsigned char *>(table->GetBufferPointer());
    _D3DXSHADER_CONSTANTTABLE header; std::memcpy(&header, data, sizeof(header));
    require(header.Constants == 1, "matrix constant count");
    _D3DXSHADER_CONSTANTINFO constant; std::memcpy(&constant, data + header.ConstantInfo, sizeof(constant));
    Type type; std::memcpy(&type, data + constant.TypeInfo, sizeof(type));
    require(!std::strcmp(reinterpret_cast<const char *>(data + constant.Name), "transform"), "actual constant name");
    require(type.kind == ((packing & 8u) ? 2 : 3) && type.rows == 4 && type.columns == 3, "packing and matrix dimensions");
    std::printf("matrix packing=%u class=%u rows=%u columns=%u registers=%u\n", packing, type.kind, type.rows, type.columns, constant.RegisterCount);
    release(shader); release(messages);
    require(!std::strcmp(reinterpret_cast<const char *>(data + constant.Name), "transform"), "table independent of shader ownership");
    table->Release();
}
class Includes final : public ID3DXInclude {
public:
    unsigned opens = 0, closes = 0, local = 0, system = 0;
    const void *outer = nullptr;
    HRESULT STDMETHODCALLTYPE Open(D3DXINCLUDE_TYPE type, const char *name, const void *parent, const void **data, UINT *bytes) override {
        const char *source;
        if (!std::strcmp(name, "outer.h")) { source = "#include <inner.h>\n"; require(type == D3DXINC_LOCAL, "local include type"); ++local; }
        else if (!std::strcmp(name, "inner.h")) { source = "float4 tint;\n"; require(type == D3DXINC_SYSTEM && parent == outer, "nested parent/type"); ++system; }
        else return E_FAIL;
        *bytes = static_cast<UINT>(std::strlen(source));
        void *memory = std::malloc(*bytes); require(memory != nullptr, "include allocation");
        std::memcpy(memory, source, *bytes); *data = memory;
        if (!std::strcmp(name, "outer.h")) outer = memory;
        ++opens; return S_OK;
    }
    HRESULT STDMETHODCALLTYPE Close(const void *data) override { std::free(const_cast<void *>(data)); ++closes; return S_OK; }
};
int main() {
    matrix(0); matrix(8u); matrix(16u);
    const char *source = "#include \"outer.h\"\nfloat4 shade(float2 uv:TEXCOORD0):COLOR0{return tint*SCALE;}";
    const D3DXMACRO macros[] = {{"SCALE","2.0"},{nullptr,nullptr}};
    Includes includes; ID3DXBuffer *shader = nullptr, *messages = nullptr; ID3DXConstantTable *table = nullptr;
    HRESULT hr = D3DXCompileShader(source, static_cast<UINT>(std::strlen(source)), macros, &includes,
        "shade", "ps_3_0", 2u | 65536u, &shader, &messages, &table);
    if (hr < 0 && messages) std::fprintf(stderr, "%s\n", static_cast<char *>(messages->GetBufferPointer()));
    require(hr == S_OK && shader && table, "real macro/include pixel compile");
    require(*static_cast<DWORD *>(shader->GetBufferPointer()) == D3DPS_VERSION(3,0), "actual pixel profile");
    require(includes.opens == 2 && includes.closes == 2 && includes.local == 1 && includes.system == 1, "balanced nested include lifecycle");
    release(shader); release(messages); table->Release();
    shader = messages = nullptr; table = nullptr;
    const char *bad = "float4 shade( broken syntax";
    hr = D3DXCompileShader(bad, static_cast<UINT>(std::strlen(bad)), nullptr, nullptr, "shade", "ps_3_0", 2u, &shader, &messages, &table);
    require(hr < 0 && !shader && !table && messages && messages->GetBufferSize() > 1, "real syntax diagnostics and null outputs");
    release(messages); messages = nullptr;
#if defined(__ANDROID__)
    hr = D3DXCompileShader(source, static_cast<UINT>(std::strlen(source)), macros, nullptr, "shade", "ps_3_0", 2u, &shader, &messages, &table);
    require(hr < 0 && !shader && !table && messages, "missing include callback rejects include");
    release(messages); messages = nullptr;
    hr = D3DXCompileShader(bad, static_cast<UINT>(std::strlen(bad)), nullptr, nullptr, "shade", "ps_3_0", 1u, &shader, &messages, &table);
    require(hr == E_NOTIMPL && !shader && !table && messages, "unsupported compiler option explicit");
    release(messages);
#endif
    std::puts("PASS real D3DX shader compilation, matrix packing, macros/includes, ownership and failure cleanup");
}
