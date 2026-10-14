#include "d3dx_ctab.h"
#include <atomic>
#include <cstring>
#include <new>
#include <vector>
#include <algorithm>

namespace {
constexpr HRESULT invalidData = static_cast<HRESULT>(0x88760b59u);
struct TypeInfo { WORD kind, type, rows, columns, elements, members; DWORD memberInfo; };
struct MemberInfo { DWORD name, type; };
static_assert(sizeof(TypeInfo) == 16 && sizeof(MemberInfo) == 8);
struct View {
    const unsigned char *bytes;
    std::size_t size;
    bool range(std::size_t offset, std::size_t count) const { return offset <= size && count <= size - offset; }
    bool string(DWORD offset) const { return offset < size && std::memchr(bytes + offset, 0, size - offset); }
    template<class T> bool read(DWORD offset, T &value) const {
        if (!range(offset, sizeof(T))) return false;
        std::memcpy(&value, bytes + offset, sizeof(T)); return true;
    }
};
bool typeValid(const View &view, DWORD offset, std::vector<DWORD> &active, std::vector<DWORD> &done) {
    if (std::find(done.begin(), done.end(), offset) != done.end()) return true;
    if (active.size() >= 64 || std::find(active.begin(), active.end(), offset) != active.end()) return false;
    TypeInfo type;
    if (!view.read(offset, type) || type.kind > 5 || type.rows > 4 || type.columns > 4
        || !view.range(type.memberInfo, static_cast<std::size_t>(type.members) * sizeof(MemberInfo))) return false;
    active.push_back(offset);
    for (unsigned i = 0; i < type.members; ++i) {
        MemberInfo member;
        if (!view.read(type.memberInfo + i * sizeof(MemberInfo), member) || !view.string(member.name)
            || !typeValid(view, member.type, active, done)) return false;
    }
    active.pop_back(); done.push_back(offset); return true;
}
bool valid(const View &view, DWORD version) {
    _D3DXSHADER_CONSTANTTABLE header;
    if (!view.read(0, header) || header.Size != sizeof(header) || header.Version != version
        || (header.Creator && !view.string(header.Creator)) || (header.Target && !view.string(header.Target))
        || header.Constants > view.size / sizeof(_D3DXSHADER_CONSTANTINFO)
        || !view.range(header.ConstantInfo, static_cast<std::size_t>(header.Constants) * sizeof(_D3DXSHADER_CONSTANTINFO))) return false;
    std::vector<DWORD> active, done;
    for (DWORD i = 0; i < header.Constants; ++i) {
        _D3DXSHADER_CONSTANTINFO constant;
        if (!view.read(header.ConstantInfo + i * sizeof(constant), constant)
            || !view.string(constant.Name) || constant.RegisterSet > 3
            || !typeValid(view, constant.TypeInfo, active, done)) return false;
        if (constant.DefaultValue) {
            const std::size_t stride = constant.RegisterSet == 0 ? 4 : 16;
            if (!view.range(constant.DefaultValue, static_cast<std::size_t>(constant.RegisterCount) * stride)) return false;
        }
    }
    return true;
}
class ConstantTable final : public ID3DXConstantTable {
    std::atomic<ULONG> references{1};
    ID3DXBuffer *contents;
public:
    explicit ConstantTable(ID3DXBuffer *buffer) : contents(buffer) {}
    ~ConstantTable() { contents->Release(); }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **out) override {
        if (!out) return E_POINTER;
        *out = nullptr;
        // The native header exposes the consumed buffer surface only. Do not
        // advertise SDK constant-setting/reflection slots not declared here.
        if (std::memcmp(&iid, &IID_IUnknown, sizeof(GUID)) && std::memcmp(&iid, &IID_ID3DXBuffer, sizeof(GUID)))
            return E_NOINTERFACE;
        *out = static_cast<ID3DXBuffer *>(this); AddRef(); return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return references.fetch_add(1, std::memory_order_relaxed) + 1; }
    ULONG STDMETHODCALLTYPE Release() override {
        ULONG left = references.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (!left) delete this;
        return left;
    }
    void *STDMETHODCALLTYPE GetBufferPointer() override { return contents->GetBufferPointer(); }
    DWORD STDMETHODCALLTYPE GetBufferSize() override { return contents->GetBufferSize(); }
};
}
HRESULT bo1::android::ShaderConstantTable(const DWORD *shader, std::size_t words, ID3DXConstantTable **out) {
    if (!out) return D3DERR_INVALIDCALL;
    *out = nullptr;
    if (!shader) return D3DERR_INVALIDCALL;
    if (words < 2 || ((shader[0] & 0xffff0000u) != 0xfffe0000u && (shader[0] & 0xffff0000u) != 0xffff0000u)) return invalidData;
    const unsigned major = (shader[0] >> 8) & 0xff;
    if (major < 2 || major > 3) return E_NOTIMPL;
    View table{};
    bool ended = false;
    for (std::size_t at = 1; at < words;) {
        if (shader[at] == D3DSIO_END) { ended = true; break; }
        const unsigned opcode = shader[at] & D3DSI_OPCODE_MASK;
        const std::size_t length = opcode == D3DSIO_COMMENT
            ? (shader[at] & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT
            : (shader[at] & D3DSI_INSTLENGTH_MASK) >> D3DSI_INSTLENGTH_SHIFT;
        if (length >= words - at) return invalidData;
        if (!table.bytes && opcode == D3DSIO_COMMENT && length && shader[at + 1] == 0x42415443u)
            table = {reinterpret_cast<const unsigned char *>(shader + at + 2), (length - 1) * sizeof(DWORD)};
        at += length + 1;
    }
    if (!ended) return invalidData;
    if (!table.bytes) return invalidData;
    try { if (!valid(table, shader[0])) return invalidData; }
    catch (const std::bad_alloc &) { return E_OUTOFMEMORY; }
    ID3DXBuffer *buffer;
    HRESULT hr = D3DXCreateBuffer(static_cast<DWORD>(table.size), &buffer);
    if (hr < 0) return hr;
    std::memcpy(buffer->GetBufferPointer(), table.bytes, table.size);
    auto *owner = new (std::nothrow) ConstantTable(buffer);
    if (!owner) { buffer->Release(); return E_OUTOFMEMORY; }
    *out = owner; return S_OK;
}
extern "C" HRESULT WINAPI D3DXGetShaderConstantTable(const DWORD *shader, ID3DXConstantTable **out) {
    if (!out) return D3DERR_INVALIDCALL;
    *out = nullptr;
    if (!shader) return D3DERR_INVALIDCALL;
    // SDK precondition: readable END-terminated bytecode. Compiler adapters use
    // the bounded entry above. This scan cannot establish a pointer's extent.
    constexpr std::size_t limit = 1u << 20;
    if ((shader[0] & 0xffff0000u) != 0xfffe0000u && (shader[0] & 0xffff0000u) != 0xffff0000u) return invalidData;
    const unsigned major = (shader[0] >> 8) & 0xff;
    if (major < 2 || major > 3) return E_NOTIMPL;
    std::size_t at = 1;
    while (at < limit && shader[at] != D3DSIO_END) {
        const DWORD token = shader[at];
        const std::size_t length = (token & D3DSI_OPCODE_MASK) == D3DSIO_COMMENT
            ? (token & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT
            : (token & D3DSI_INSTLENGTH_MASK) >> D3DSI_INSTLENGTH_SHIFT;
        if (length >= limit - at) return invalidData;
        at += length + 1;
    }
    return at < limit ? bo1::android::ShaderConstantTable(shader, at + 1, out) : invalidData;
}
