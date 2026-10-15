#pragma once
#include "shader_translate.h"
#include <memory>
struct MaterialVertexShader;struct MaterialPixelShader;
namespace bo1::vulkan::shader_assets {
struct Record {
    std::uint64_t serial{};std::string name;
    std::vector<std::uint32_t> bytecode;
    ShaderTranslation translation;
};
// Select before native resource loading; reject switching away while database
// shader owners remain registered. D3D9 remains the default during migration.
bool SelectNative(bool enabled);
bool Active();
bool Create(MaterialVertexShader*,std::string& error);
bool Create(MaterialPixelShader*,std::string& error);
std::shared_ptr<const Record> Acquire(const MaterialVertexShader*);
std::shared_ptr<const Record> Acquire(const MaterialPixelShader*);
void Release(const MaterialVertexShader*);
void Release(const MaterialPixelShader*);
}
