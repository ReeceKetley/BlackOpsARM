#include "d3dx_ctab.h"
#include <vkd3d_shader.h>
#include <cstring>
#include <limits>
#include <vector>
#include <new>

namespace {
constexpr HRESULT invalidData = static_cast<HRESULT>(0x88760b59u);
HRESULT copy(const void *data, std::size_t bytes, ID3DXBuffer **out) {
    if (bytes > std::numeric_limits<DWORD>::max()) return E_OUTOFMEMORY;
    HRESULT hr = D3DXCreateBuffer(static_cast<DWORD>(bytes), out);
    if (hr >= 0 && bytes) std::memcpy((*out)->GetBufferPointer(), data, bytes);
    return hr;
}
HRESULT diagnostic(const char *message, HRESULT status, ID3DXBuffer **out) {
    if (!out) return status;
    if (*out) { (*out)->Release(); *out = nullptr; }
    HRESULT hr = copy(message, std::strlen(message) + 1, out);
    return hr < 0 ? hr : status;
}
struct Includes { ID3DXInclude *handler; HRESULT failure = S_OK; };
int openInclude(const char *name, bool local, const char *parent, void *context, vkd3d_shader_code *out) {
    auto &includes = *static_cast<Includes *>(context);
    if (!includes.handler) return VKD3D_ERROR_INVALID_ARGUMENT;
    const void *data = nullptr; UINT bytes = 0;
    const HRESULT hr = includes.handler->Open(local ? D3DXINC_LOCAL : D3DXINC_SYSTEM, name, parent, &data, &bytes);
    if (hr < 0) { includes.failure = hr; return VKD3D_ERROR; }
    if (!data && bytes) {
        includes.handler->Close(data); includes.failure = E_FAIL; return VKD3D_ERROR;
    }
    out->code = data; out->size = bytes; return VKD3D_OK;
}
void closeInclude(const vkd3d_shader_code *code, void *context) {
    auto &includes = *static_cast<Includes *>(context);
    const HRESULT hr = includes.handler->Close(code->code);
    if (hr < 0) includes.failure = hr;
}
struct Compilation {
    vkd3d_shader_code code{};
    char *messages = nullptr;
    ~Compilation() { vkd3d_shader_free_shader_code(&code); vkd3d_shader_free_messages(messages); }
};
}
extern "C" HRESULT WINAPI D3DXCompileShader(const char *source, UINT length,
    const D3DXMACRO *defines, ID3DXInclude *include, const char *entry,
    const char *profile, DWORD flags, ID3DXBuffer **shader,
    ID3DXBuffer **diagnostics, ID3DXConstantTable **constants) {
    if (shader) *shader = nullptr;
    if (diagnostics) *diagnostics = nullptr;
    if (constants) *constants = nullptr;
    if (!source || !length || !entry || !profile)
        return diagnostic("error: invalid native shader compiler arguments\n", D3DERR_INVALIDCALL, diagnostics);
    // Legacy DLL selection is replaced by this platform's real CPU compiler.
    // SKIPVALIDATION is permission to skip, not a requirement to disable the
    // native backend's validation. Preserve actual default matrix semantics.
    constexpr DWORD supported = 2u | 8u | 16u | 4096u | 65536u;
    if ((flags & ~supported) || ((flags & 24u) == 24u))
        return diagnostic("error: native shader compiler cannot honour these compiler options\n", E_NOTIMPL, diagnostics);
    try {
        std::vector<vkd3d_shader_macro> macros;
        if (defines) {
            for (unsigned i = 0; defines[i].Name; ++i) {
                if (i == 4096) return diagnostic("error: too many predefined shader macros\n", D3DERR_INVALIDCALL, diagnostics);
                macros.push_back({defines[i].Name, defines[i].Definition});
            }
        }
        Includes includes{include};
        vkd3d_shader_preprocess_info preprocessing{};
        preprocessing.type = VKD3D_SHADER_STRUCTURE_TYPE_PREPROCESS_INFO;
        preprocessing.macros = macros.data(); preprocessing.macro_count = static_cast<unsigned>(macros.size());
        preprocessing.pfn_open_include = openInclude; preprocessing.pfn_close_include = closeInclude;
        preprocessing.include_context = &includes;
        vkd3d_shader_hlsl_source_info hlsl{};
        hlsl.type = VKD3D_SHADER_STRUCTURE_TYPE_HLSL_SOURCE_INFO; hlsl.next = &preprocessing;
        hlsl.entry_point = entry; hlsl.profile = profile;
        vkd3d_shader_compile_option options[2] = {
            {VKD3D_SHADER_COMPILE_OPTION_PACK_MATRIX_ORDER, (flags & 8u)
                ? VKD3D_SHADER_COMPILE_OPTION_PACK_MATRIX_ROW_MAJOR : VKD3D_SHADER_COMPILE_OPTION_PACK_MATRIX_COLUMN_MAJOR},
            {VKD3D_SHADER_COMPILE_OPTION_BACKWARD_COMPATIBILITY, (flags & 4096u)
                ? VKD3D_SHADER_COMPILE_OPTION_BACKCOMPAT_MAP_SEMANTIC_NAMES | VKD3D_SHADER_COMPILE_OPTION_DOUBLE_AS_FLOAT_ALIAS : 0u}};
        vkd3d_shader_compile_info info{};
        info.type = VKD3D_SHADER_STRUCTURE_TYPE_COMPILE_INFO; info.next = &hlsl;
        info.source = {source, length}; info.source_type = VKD3D_SHADER_SOURCE_HLSL;
        info.target_type = VKD3D_SHADER_TARGET_D3D_BYTECODE;
        info.options = options; info.option_count = 2; info.log_level = VKD3D_SHADER_LOG_INFO;
        Compilation compiled;
        const int status = vkd3d_shader_compile(&info, &compiled.code, &compiled.messages);
        if (diagnostics && compiled.messages && *compiled.messages) {
            HRESULT hr = copy(compiled.messages, std::strlen(compiled.messages) + 1, diagnostics);
            if (hr < 0) return hr;
        }
        if (includes.failure < 0) {
            if (!diagnostics || *diagnostics) return includes.failure;
            return diagnostic("error: native shader include callback failed\n", includes.failure, diagnostics);
        }
        if (status < 0) {
            if (status == VKD3D_ERROR_OUT_OF_MEMORY) return E_OUTOFMEMORY;
            if (status == VKD3D_ERROR_NOT_IMPLEMENTED) return E_NOTIMPL;
            return invalidData;
        }
        if (!compiled.code.code || compiled.code.size < 8 || compiled.code.size % sizeof(DWORD))
            return diagnostic("error: compiler returned invalid shader bytecode\n", invalidData, diagnostics);
        ID3DXConstantTable *table = nullptr;
        if (constants) {
            HRESULT hr = bo1::android::ShaderConstantTable(static_cast<const DWORD *>(compiled.code.code),
                compiled.code.size / sizeof(DWORD), &table);
            if (hr < 0) return hr;
        }
        ID3DXBuffer *buffer = nullptr;
        if (shader) {
            HRESULT hr = copy(compiled.code.code, compiled.code.size, &buffer);
            if (hr < 0) { if (table) table->Release(); return hr; }
        }
        if (shader) *shader = buffer;
        if (constants) *constants = table;
        return S_OK;
    } catch (const std::bad_alloc &) { return E_OUTOFMEMORY; }
}
