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
#include <cstdio>
#if defined(__ANDROID__)
#include "platform/android/engine_paths.h"
#include <android/log.h>
#endif
namespace bo1::vulkan::shader_assets {
namespace {
std::mutex mutex;
std::atomic<bool> active{};
std::uint64_t serial{};
std::unordered_map<const MaterialVertexShader*,std::shared_ptr<const Record>,bo1::vulkan::PointerHash> vertices;
std::unordered_map<const MaterialPixelShader*,std::shared_ptr<const Record>,bo1::vulkan::PointerHash> pixels;
std::uint64_t BytecodeHash(const Record& record) {
    std::uint64_t hash=14695981039346656037ull;
    for(auto word:record.bytecode)for(unsigned byte=0;byte<4;++byte){hash^=(word>>(byte*8))&255;hash*=1099511628211ull;}
    return hash;
}
void SaveWatched(const Record& record,const char* kind,std::uint64_t hash) {
#if defined(__ANDROID__)
    const auto root=Sys_EngineUserRoot();if(!root||!*root)return;
    char filename[128];std::snprintf(filename,sizeof(filename),"/native-watch-%s-%08x-%016llx.sm3",kind,record.loadHeader,(unsigned long long)hash);
    const auto path=std::string(root)+filename;
    // Exclusive creation preserves the first evidence across boundaries/runs.
    if(auto file=std::fopen(path.c_str(),"wx")) {
        const bool written=std::fwrite(record.bytecode.data(),sizeof(std::uint32_t),record.bytecode.size(),file)==record.bytecode.size();
        const bool closed=std::fclose(file)==0;
        __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan","native_shader_watch_file path=%s saved=%u words=%zu",path.c_str(),unsigned(written&&closed),record.bytecode.size());
    }
#endif
}
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
    result->integrityWatched=result->name=="pimp_shader_flamethrowerfx_97dd740c.hlsl";
    if(result->integrityWatched){result->loadWords=result->bytecode.size();result->loadHeader=result->bytecode[0];result->loadHash=BytecodeHash(*result);result->diagnosticOwner=owner;SaveWatched(*result,"original",result->loadHash);}
    if(!TranslateShader(result->bytecode.data(),result->bytecode.size()*4,result->translation)||result->translation.stage!=stage) {
        SaveFailure(*result);
        error=result->translation.diagnostics.empty()?"Shader stage/translation mismatch":result->translation.diagnostics;return false;
    }
    // Registration validates the shader and keeps its reflection. Programs
    // translate their linked vertex/pixel pair themselves, so this record's
    // SPIR-V words are not kept (thousands of shaders: hundreds of MB).
    std::vector<std::uint32_t>().swap(result->translation.spirv);
    {
        std::lock_guard<std::mutex> lock(mutex);
        if(!active.load()) {error="Native shader backend not selected";return false;}
        result->serial=++serial;table[owner]=result;
    }
    TraceIntegrity(result,"publication");return true;
}
template<class Owner,class Table>
std::shared_ptr<const Record> Find(const Owner* owner,const Table& table) {
    std::lock_guard<std::mutex> lock(mutex);const auto it=table.find(owner);return it==table.end()?nullptr:it->second;
}
}
void TraceIntegrity(const std::shared_ptr<const Record>& record,const char* boundary) {
    if(!record||!record->integrityWatched)return;
    const auto hash=BytecodeHash(*record);
    const auto header=record->bytecode.empty()?0u:record->bytecode[0];
    const bool mismatch=hash!=record->loadHash||header!=record->loadHeader||record->bytecode.size()!=record->loadWords;
    // Bound healthy logs separately per boundary/stage; mismatches always log.
    static const char* boundaries[]={"publication","select_vs","select_ps","capture_vs","capture_ps","before_key_vs","before_key_ps","before_pair_vs","before_pair_ps","after_pair_vs","after_pair_ps"};
    static std::atomic<unsigned> validCounts[sizeof(boundaries)/sizeof(*boundaries)][2]{};
    unsigned slot=0;for(unsigned i=0;i<sizeof(boundaries)/sizeof(*boundaries);++i)if(!std::strcmp(boundary,boundaries[i])){slot=i;break;}
    const unsigned stage=record->loadHeader==0xffff0300u;
    if(!mismatch&&std::strcmp(boundary,"publication")&&validCounts[slot][stage].fetch_add(1,std::memory_order_relaxed)>=3)return;
    char line[1024];std::snprintf(line,sizeof(line),
        "native_shader_integrity boundary=%s mismatch=%u serial=%llu owner=%p record=%p payload=%p words=%zu load_words=%zu header=%08x load_header=%08x hash=%016llx load_hash=%016llx use_count=%ld name=%s",
        boundary,unsigned(mismatch),(unsigned long long)record->serial,record->diagnosticOwner,static_cast<const void*>(record.get()),static_cast<const void*>(record->bytecode.data()),record->bytecode.size(),record->loadWords,header,record->loadHeader,(unsigned long long)hash,(unsigned long long)record->loadHash,record.use_count(),record->name.c_str());
#if defined(__ANDROID__)
    __android_log_print(ANDROID_LOG_ERROR,"BO1NativeVulkan","%s",line);
#else
    std::fprintf(stderr,"%s\n",line);
#endif
    if(mismatch)SaveWatched(*record,"mismatch",hash);
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
void ForEach(const std::function<void(const std::shared_ptr<const Record>&)>& visit) {
    std::vector<std::shared_ptr<const Record>> records;
    {std::lock_guard<std::mutex> lock(mutex);records.reserve(vertices.size()+pixels.size());
     for(const auto& entry:vertices)records.push_back(entry.second);for(const auto& entry:pixels)records.push_back(entry.second);}
    for(const auto& record:records)visit(record);
}
std::uint64_t Generation(){std::lock_guard<std::mutex> lock(mutex);return serial;}
}
