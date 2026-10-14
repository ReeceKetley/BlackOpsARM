#pragma once
#include <platform/win32_compat/native_d3dx9_api.h>
#include <cstddef>
namespace bo1::android {
// Compiler adapters have an authoritative length, unlike the SDK pointer API.
HRESULT ShaderConstantTable(const DWORD *shader, std::size_t words, ID3DXConstantTable **result);
}
