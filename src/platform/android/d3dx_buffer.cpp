#include <platform/win32_compat/native_d3dx9_api.h>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <new>

namespace {
class ShaderBuffer final : public ID3DXBuffer {
    std::atomic<ULONG> references{1};
    void *storage;
    DWORD bytes;
public:
    ShaderBuffer(void *memory, DWORD size) : storage(memory), bytes(size) {}
    ~ShaderBuffer() { std::free(storage); }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **result) override {
        if (!result) return E_POINTER;
        *result = nullptr;
        if (std::memcmp(&iid, &IID_IUnknown, sizeof(GUID)) &&
            std::memcmp(&iid, &IID_ID3DXBuffer, sizeof(GUID))) return E_NOINTERFACE;
        *result = static_cast<ID3DXBuffer *>(this);
        AddRef();
        return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override {
        return references.fetch_add(1, std::memory_order_relaxed) + 1;
    }
    ULONG STDMETHODCALLTYPE Release() override {
        const ULONG remaining = references.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (!remaining) delete this;
        return remaining;
    }
    void *STDMETHODCALLTYPE GetBufferPointer() override { return storage; }
    DWORD STDMETHODCALLTYPE GetBufferSize() override { return bytes; }
};
}
extern "C" HRESULT WINAPI D3DXCreateBuffer(DWORD bytes, ID3DXBuffer **result) {
    if (!result) return D3DERR_INVALIDCALL;
    *result = nullptr;
    void *memory = std::malloc(bytes ? bytes : 1);
    if (!memory) return E_OUTOFMEMORY;
    auto *buffer = new (std::nothrow) ShaderBuffer(memory, bytes);
    if (!buffer) { std::free(memory); return E_OUTOFMEMORY; }
    *result = buffer;
    return S_OK;
}
