#pragma once
#include <functional>
#include "shader_translate.h"
#include <memory>
struct MaterialVertexShader;struct MaterialPixelShader;
namespace bo1::vulkan::shader_assets {
struct Record {
    std::uint64_t serial{};std::string name;
    std::vector<std::uint32_t> bytecode;
    ShaderTranslation translation;
    // Narrow diagnostic for the observed late flamethrower shader corruption.
    bool integrityWatched{};std::uint64_t loadHash{};std::uint32_t loadHeader{};
    std::size_t loadWords{};const void* diagnosticOwner{};
};
// Select before native resource loading; reject switching away while database
// shader owners remain registered. D3D9 remains the default during migration.
void TraceIntegrity(const std::shared_ptr<const Record>&,const char* boundary);
bool SelectNative(bool enabled);
bool Active();
bool Create(MaterialVertexShader*,std::string& error);
bool Create(MaterialPixelShader*,std::string& error);
std::shared_ptr<const Record> Acquire(const MaterialVertexShader*);
std::shared_ptr<const Record> Acquire(const MaterialPixelShader*);
void Release(const MaterialVertexShader*);
void Release(const MaterialPixelShader*);
// Every registered record (both stages), and a counter that changes whenever
// a record is registered. Used to rebuild recorded programs at map load.
void ForEach(const std::function<void(const std::shared_ptr<const Record>&)>&);
std::uint64_t Generation();
}
