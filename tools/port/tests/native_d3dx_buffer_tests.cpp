#if defined(__ANDROID__)
#include <platform/win32_compat/native_d3dx9_api.h>
#else
#include <d3dx9.h>
#endif
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>

static void require(bool value, const char *reason) {
    if (!value) { std::fprintf(stderr, "FAIL %s\n", reason); std::abort(); }
}
int main() {
    require(D3DXCreateBuffer(10, nullptr) == D3DERR_INVALIDCALL, "null output");
    ID3DXBuffer *buffer = nullptr;
    require(D3DXCreateBuffer(0, &buffer) == S_OK && buffer->GetBufferSize() == 0, "zero size");
    require(buffer->Release() == 0, "zero-size final release");
    require(D3DXCreateBuffer(70001, &buffer) == S_OK && buffer->GetBufferSize() == 70001, "size retained");
    auto *bytes = static_cast<unsigned char *>(buffer->GetBufferPointer());
    require(bytes != nullptr, "backing storage");
    for (unsigned i = 0; i != 70001; ++i) bytes[i] = static_cast<unsigned char>(i % 251);
    void *identity = nullptr;
    require(buffer->QueryInterface(IID_ID3DXBuffer, &identity) == S_OK && identity == buffer, "buffer identity");
    require(static_cast<ID3DXBuffer *>(identity)->Release() == 1, "query retains reference");
    require(buffer->QueryInterface(IID_IUnknown, &identity) == S_OK && identity == buffer, "unknown identity");
    require(static_cast<IUnknown *>(identity)->Release() == 1, "unknown release");
    GUID unsupported = {0x12345678,0,0,{0}};
    identity = buffer;
    require(buffer->QueryInterface(unsupported, &identity) == E_NOINTERFACE && !identity, "unsupported interface");
#if defined(__ANDROID__)
    std::vector<std::thread> workers;
    for (int i = 0; i != 4; ++i) workers.emplace_back([&] {
        for (int j = 0; j != 10000; ++j) { buffer->AddRef(); buffer->Release(); }
    });
    for (auto &worker : workers) worker.join();
#else
    // Windows buffer lifetime checks use serial ownership. The separate native
    // concurrency gate verifies the stronger atomic ownership we implement.
    for (int j = 0; j != 40000; ++j) { buffer->AddRef(); buffer->Release(); }
#endif
    require(buffer->AddRef() == 2 && buffer->Release() == 1, "concurrent reference ownership");
    for (unsigned i = 0; i != 70001; ++i) require(bytes[i] == i % 251, "contents preserved");
    require(buffer->Release() == 0, "final release");
    std::puts("PASS D3DX buffer: size, contents, COM identity, 40000 reference cycles and final release");
}
