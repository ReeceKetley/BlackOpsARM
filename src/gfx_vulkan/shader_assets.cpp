#include "shader_assets.h"
#include "pointer_hash.h"
#include "vertex_assets.h"
#include "image_assets.h"
#include "buffer_assets.h"
#include "database/db_material_graph_types.h"
#include <mutex>
#include <unordered_map>
#include <atomic>
#include <cstring>
#if defined(__ANDROID__)
#include "platform/android/engine_paths.h"
#include <cstdio>
#endif
namespace bo1::vulkan::shader_assets {
namespace {
std::mutex mutex;
std::atomic<bool> active{};
std::uint64_t serial{};
std::unordered_map<const MaterialVertexShader*,std::shared_ptr<const Record>,bo1::vulkan::PointerHash> vertices;
std::unordered_map<const MaterialPixelShader*,std::shared_ptr<const Record>,bo1::vulkan::PointerHash> pixels;
void SaveFailure(const Record& record) {
#if defined(__ANDROID__)
    const auto root=Sys_EngineUserRoot();if(!root||!*root)return;
    std::uint64_t hash=14695981039346656037ull;
    for(auto word:record.bytecode)for(unsigned byte=0;byte<4;++byte) {hash^=(word>>(byte*8))&255;hash*=1099511628211ull;}
    char filename[80];std::snprintf(filename,sizeof(filename),"/native-failed-shader-%016llx.sm3",(unsigned long long)hash);
    const auto path=std::string(root)+filename;
    // Unique immutable diagnostic; never overwrite earlier failure evidence.
    if(auto file=std::fopen(path.c_str(),"wx")) {
        const bool written=std::fwrite(record.bytecode.data(),sizeof(std::uint32_t),record.bytecode.size(),file)==record.bytecode.size();
        const bool closed=std::fclose(file)==0;
        std::fprintf(stderr,"NATIVE_SHADER_FAILURE bytecode=%s words=%zu saved=%u name=%s\n",path.c_str(),record.bytecode.size(),unsigned(written&&closed),record.name.c_str());
    }
#endif
}
template<class Owner,class Table>
bool Build(Owner* owner,Table& table,ShaderStage stage,std::string& error) {
    error.clear();
    if(!active.load()) {error="Native shader backend not selected";return false;}
    if(!owner||!owner->prog.loadDef.program||!owner->prog.loadDef.programSize) {error="Missing native shader load definition";return false;}
    auto result=std::make_shared<Record>();result->name=owner->name?owner->name:"<unnamed>";
    // Canonical material loader and DB stream both define programSize in DWORDs.
    result->bytecode.resize(owner->prog.loadDef.programSize);
    std::memcpy(result->bytecode.data(),owner->prog.loadDef.program,result->bytecode.size()*4);
    if(!TranslateShader(result->bytecode.data(),result->bytecode.size()*4,result->translation)||result->translation.stage!=stage) {
        SaveFailure(*result);
        error=result->translation.diagnostics.empty()?"Shader stage/translation mismatch":result->translation.diagnostics;return false;
    }
    std::lock_guard<std::mutex> lock(mutex);
    if(!active.load()) {error="Native shader backend not selected";return false;}
    result->serial=++serial;table[owner]=std::move(result);return true;
}
template<class Owner,class Table>
std::shared_ptr<const Record> Find(const Owner* owner,const Table& table) {
    std::lock_guard<std::mutex> lock(mutex);const auto it=table.find(owner);return it==table.end()?nullptr:it->second;
}
}
bool SelectNative(bool enabled) {
    std::lock_guard<std::mutex> lock(mutex);
    if(active.load()==enabled)return true;
    if(!vertices.empty()||!pixels.empty()||!vertex_assets::Empty()||!image_assets::Empty()||!buffer_assets::Empty())return false;active.store(enabled);return true;
}
bool Active(){return active.load();}
bool Create(MaterialVertexShader* owner,std::string& error) {
    if(owner&&owner->prog.vs) {error="Cannot replace a live D3D9 vertex shader";return false;}
    return Build(owner,vertices,ShaderStage::Vertex,error);
}
bool Create(MaterialPixelShader* owner,std::string& error) {
    if(owner&&owner->prog.ps) {error="Cannot replace a live D3D9 pixel shader";return false;}
    return Build(owner,pixels,ShaderStage::Fragment,error);
}
std::shared_ptr<const Record> Acquire(const MaterialVertexShader* owner){return Find(owner,vertices);}
std::shared_ptr<const Record> Acquire(const MaterialPixelShader* owner){return Find(owner,pixels);}
void Release(const MaterialVertexShader* owner){std::lock_guard<std::mutex> lock(mutex);vertices.erase(owner);}
void Release(const MaterialPixelShader* owner){std::lock_guard<std::mutex> lock(mutex);pixels.erase(owner);}
}
