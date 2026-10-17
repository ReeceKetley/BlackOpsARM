#include "program_cache.h"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <vector>
#include "shader_assets.h"
#if defined(__ANDROID__)
#include <android/log.h>
#endif
namespace bo1::vulkan {
namespace {
constexpr std::uint32_t RecipeMagic=0x52504f42u,RecipeVersion=1; // "BOPR"
std::uint64_t Fnv(const void* data,std::size_t bytes,std::uint64_t h=1469598103934665603ull) {
    const auto* p=static_cast<const unsigned char*>(data);for(std::size_t i=0;i<bytes;++i){h^=p[i];h*=1099511628211ull;}return h;
}
struct RecipeWriter {
    std::vector<unsigned char> out;
    void Raw(const void* p,std::size_t n){const auto* b=static_cast<const unsigned char*>(p);out.insert(out.end(),b,b+n);}
    void U32(std::uint32_t v){Raw(&v,4);}void U64(std::uint64_t v){Raw(&v,8);}void F32(float v){Raw(&v,4);}
    void Str(const std::string& s){U32(std::uint32_t(s.size()));Raw(s.data(),s.size());}
};
struct RecipeReader {
    const unsigned char* p;std::size_t n,at{};bool ok=true;
    bool Raw(void* d,std::size_t k){if(!ok||k>n-at){ok=false;return false;}std::memcpy(d,p+at,k);at+=k;return true;}
    std::uint32_t U32(){std::uint32_t v{};Raw(&v,4);return v;}std::uint64_t U64(){std::uint64_t v{};Raw(&v,8);return v;}
    float F32(){float v{};Raw(&v,4);return v;}
    std::string Str(){const auto k=U32();if(!ok||k>4096||k>n-at){ok=false;return {};}std::string s(reinterpret_cast<const char*>(p+at),k);at+=k;return s;}
};
void WriteStencil(RecipeWriter& w,const VkStencilOpState& s){w.U32(s.failOp);w.U32(s.passOp);w.U32(s.depthFailOp);w.U32(s.compareOp);w.U32(s.compareMask);w.U32(s.writeMask);w.U32(s.reference);}
VkStencilOpState ReadStencil(RecipeReader& r){VkStencilOpState s{};s.failOp=VkStencilOp(r.U32());s.passOp=VkStencilOp(r.U32());s.depthFailOp=VkStencilOp(r.U32());
    s.compareOp=VkCompareOp(r.U32());s.compareMask=r.U32();s.writeMask=r.U32();s.reference=r.U32();return s;}
std::uint64_t ShaderHash(const shader_assets::Record& record){return Fnv(record.bytecode.data(),record.bytecode.size()*4);}
}
ProgramCache::~ProgramCache(){Clear();if(driver_)vkDestroyPipelineCache(device_,driver_,nullptr);}
void ProgramCache::EraseRenderPass(VkRenderPass pass) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    for(auto it=programs_.begin();it!=programs_.end();)if(it->first.RenderPass()==pass){++evictions_;it=programs_.erase(it);}else ++it;
}
void ProgramCache::RecordRecipe(const ProgramDescription& d) {
    if(!d.vertexOwner||!d.pixelOwner||!d.passKeyed||d.vertexBytes!=d.vertexOwner->bytecode.size()*4||d.pixelBytes!=d.pixelOwner->bytecode.size()*4)return;
    auto hash=[&](const std::shared_ptr<const shader_assets::Record>& owner) {
        auto found=shaderHashes_.find(owner.get());if(found!=shaderHashes_.end())return found->second;
        return shaderHashes_.emplace(owner.get(),ShaderHash(*owner)).first->second;
    };
    RecipeWriter w;
    w.Str(d.vertexOwner->name);w.U64(hash(d.vertexOwner));w.Str(d.pixelOwner->name);w.U64(hash(d.pixelOwner));
    for(auto v:d.passKey)w.U32(v);
    w.U32(d.subpass);w.U32(d.samples);w.U32(d.topology);w.U32(d.cull);w.U32(d.frontFace);
    w.U32(d.alphaTest.comparison);w.F32(d.alphaTest.reference);w.U32(d.pixelDepthSamplerMask);w.U32(d.vertexUbyteMask);
    w.U32(d.depthBias.enabled);w.U32(d.depthBias.resolved);w.F32(d.depthBias.constantFactor);w.F32(d.depthBias.slopeFactor);
    w.U32(d.stencilTest);WriteStencil(w,d.frontStencil);WriteStencil(w,d.backStencil);
    w.U32(d.colorAttachment);w.U32(d.depthOnly);w.U32(d.depthTest);w.U32(d.depthWrite);w.U32(d.depthCompare);
    w.U32(d.color.blendEnable);w.U32(d.color.srcColorBlendFactor);w.U32(d.color.dstColorBlendFactor);w.U32(d.color.colorBlendOp);
    w.U32(d.color.srcAlphaBlendFactor);w.U32(d.color.dstAlphaBlendFactor);w.U32(d.color.alphaBlendOp);w.U32(d.color.colorWriteMask);
    w.U32(d.vertexBindingCount);for(unsigned i=0;i<d.vertexBindingCount;++i){const auto& b=d.vertexBindings[i];w.U32(b.binding);w.U32(b.stride);w.U32(b.inputRate);}
    w.U32(d.vertexAttributeCount);for(unsigned i=0;i<d.vertexAttributeCount;++i){const auto& a=d.vertexAttributes[i];w.U32(a.location);w.U32(a.binding);w.U32(a.format);w.U32(a.offset);}
    if(!recipeHashes_.insert(Fnv(w.out.data(),w.out.size())).second)return;
    recipes_.push_back(std::move(w.out));recipeBuilt_.push_back(true);
}
ProgramCache::PrewarmResult ProgramCache::Prewarm(const std::function<VkRenderPass(const std::array<unsigned,8>&)>& resolvePass) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    PrewarmResult result;const auto start=std::chrono::steady_clock::now();
    if(!driver_)return result;
    // name+hash -> record for each stage, from the shaders registered now.
    std::unordered_map<std::string,std::shared_ptr<const shader_assets::Record>> stages[2];
    shader_assets::ForEach([&](const std::shared_ptr<const shader_assets::Record>& record) {
        if(!record||record->bytecode.empty())return;
        const unsigned stage=record->translation.stage==ShaderStage::Fragment?1:0;
        stages[stage].emplace(record->name+"#"+std::to_string(ShaderHash(*record)),record);
    });
    prewarming_=true;
    for(std::size_t i=0;i<recipes_.size();++i) {
        if(recipeBuilt_[i])continue;
        if(programs_.size()+16>=capacity_){++result.pending;continue;}
        RecipeReader r{recipes_[i].data(),recipes_[i].size()};
        const auto vsName=r.Str();const auto vsHash=r.U64();const auto psName=r.Str();const auto psHash=r.U64();
        if(!r.ok){recipeBuilt_[i]=true;++result.failed;continue;}
        auto vs=stages[0].find(vsName+"#"+std::to_string(vsHash));auto ps=stages[1].find(psName+"#"+std::to_string(psHash));
        if(vs==stages[0].end()||ps==stages[1].end()){++result.pending;continue;} // shaders of another map
        ProgramDescription d;d.vertexOwner=vs->second;d.pixelOwner=ps->second;
        d.vertexBytecode=d.vertexOwner->bytecode.data();d.vertexBytes=d.vertexOwner->bytecode.size()*4;
        d.pixelBytecode=d.pixelOwner->bytecode.data();d.pixelBytes=d.pixelOwner->bytecode.size()*4;
        for(auto& v:d.passKey)v=r.U32();d.passKeyed=true;
        d.subpass=r.U32();d.samples=VkSampleCountFlagBits(r.U32());d.topology=VkPrimitiveTopology(r.U32());d.cull=r.U32();d.frontFace=VkFrontFace(r.U32());
        d.alphaTest.comparison=r.U32();d.alphaTest.reference=r.F32();d.pixelDepthSamplerMask=r.U32();d.vertexUbyteMask=r.U32();
        d.depthBias.enabled=r.U32()!=0;d.depthBias.resolved=r.U32()!=0;d.depthBias.constantFactor=r.F32();d.depthBias.slopeFactor=r.F32();
        d.stencilTest=r.U32()!=0;d.frontStencil=ReadStencil(r);d.backStencil=ReadStencil(r);
        d.colorAttachment=r.U32()!=0;d.depthOnly=r.U32()!=0;d.depthTest=r.U32()!=0;d.depthWrite=r.U32()!=0;d.depthCompare=VkCompareOp(r.U32());
        d.color.blendEnable=r.U32();d.color.srcColorBlendFactor=VkBlendFactor(r.U32());d.color.dstColorBlendFactor=VkBlendFactor(r.U32());d.color.colorBlendOp=VkBlendOp(r.U32());
        d.color.srcAlphaBlendFactor=VkBlendFactor(r.U32());d.color.dstAlphaBlendFactor=VkBlendFactor(r.U32());d.color.alphaBlendOp=VkBlendOp(r.U32());d.color.colorWriteMask=r.U32();
        std::vector<VkVertexInputBindingDescription> bindings;
        const auto bindingCount=r.U32();if(bindingCount>16)r.ok=false;
        for(unsigned b=0;r.ok&&b<bindingCount;++b){VkVertexInputBindingDescription v{};v.binding=r.U32();v.stride=r.U32();v.inputRate=VkVertexInputRate(r.U32());bindings.push_back(v);}
        std::vector<VkVertexInputAttributeDescription> attributes;
        const auto attributeCount=r.U32();if(attributeCount>32)r.ok=false;
        for(unsigned a=0;r.ok&&a<attributeCount;++a){VkVertexInputAttributeDescription v{};v.location=r.U32();v.binding=r.U32();v.format=VkFormat(r.U32());v.offset=r.U32();attributes.push_back(v);}
        recipeBuilt_[i]=true;
        if(!r.ok||r.at!=r.n){++result.failed;continue;}
        d.vertexBindings=bindings.data();d.vertexBindingCount=unsigned(bindings.size());
        d.vertexAttributes=attributes.data();d.vertexAttributeCount=unsigned(attributes.size());
        d.renderPass=resolvePass(d.passKey);
        if(!d.renderPass){++result.failed;continue;}
        std::string error;
        if(Acquire(d,error))++result.built;else ++result.failed;
    }
    prewarming_=false;
    result.ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
    return result;
}
bool ProgramCache::SaveRecipes(std::string& error)const {
    if(path_.empty()||recipes_.size()==savedRecipes_)return true;
    RecipeWriter w;w.U32(RecipeMagic);w.U32(RecipeVersion);w.U32(std::uint32_t(recipes_.size()));
    for(const auto& recipe:recipes_){w.U32(std::uint32_t(recipe.size()));w.Raw(recipe.data(),recipe.size());}
    const std::string path=path_+".recipes",temporary=path+".tmp";FILE* file=std::fopen(temporary.c_str(),"wb");
    if(!file){error="Native program recipe temp file unavailable";return false;}
    const bool written=std::fwrite(w.out.data(),1,w.out.size(),file)==w.out.size();
    if(std::fclose(file)!=0||!written||std::rename(temporary.c_str(),path.c_str())!=0){std::remove(temporary.c_str());error="Native program recipe write failed";return false;}
    return true;
}
void ProgramCache::LoadRecipes() {
    if(path_.empty())return;
    FILE* file=std::fopen((path_+".recipes").c_str(),"rb");if(!file)return;
    std::vector<unsigned char> data;
    if(!std::fseek(file,0,SEEK_END)){const long size=std::ftell(file);if(size>0&&size<=(16l<<20)&&!std::fseek(file,0,SEEK_SET)){data.resize(std::size_t(size));if(std::fread(data.data(),1,data.size(),file)!=data.size())data.clear();}}
    std::fclose(file);
    RecipeReader r{data.data(),data.size()};
    if(data.size()<12||r.U32()!=RecipeMagic||r.U32()!=RecipeVersion)return;
    const auto count=r.U32();if(count>65536)return;
    for(std::uint32_t i=0;i<count&&r.ok;++i) {
        const auto size=r.U32();if(!r.ok||size>65536||size>r.n-r.at)break;
        std::vector<unsigned char> recipe(r.p+r.at,r.p+r.at+size);r.at+=size;
        if(recipeHashes_.insert(Fnv(recipe.data(),recipe.size())).second){recipes_.push_back(std::move(recipe));recipeBuilt_.push_back(false);}
    }
    savedRecipes_=recipes_.size();
}
bool ProgramCache::Initialize(std::string& error,std::string path,const VkPhysicalDeviceProperties* properties) {
    if(!device_||!capacity_||driver_) {error="Invalid native program cache initialization";return false;}
    VkPipelineCacheCreateInfo info{VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
    std::vector<unsigned char> stored;
    if(!path.empty()&&properties) {
        path_=std::move(path);
        if(FILE* file=std::fopen(path_.c_str(),"rb")) {
            if(!std::fseek(file,0,SEEK_END)) {
                const long size=std::ftell(file);
                if(size>=32&&size<=(64l<<20)&&!std::fseek(file,0,SEEK_SET)) {
                    stored.resize(std::size_t(size));if(std::fread(stored.data(),1,stored.size(),file)!=stored.size())stored.clear();
                }
            }
            std::fclose(file);
        }
        // VkPipelineCacheHeaderVersionOne: length, version, vendor, device, UUID.
        std::uint32_t header[4]{};if(stored.size()>=32)std::memcpy(header,stored.data(),sizeof(header));
        if(stored.size()<32||header[0]<32||header[0]>stored.size()||header[1]!=VK_PIPELINE_CACHE_HEADER_VERSION_ONE
            ||header[2]!=properties->vendorID||header[3]!=properties->deviceID
            ||std::memcmp(stored.data()+16,properties->pipelineCacheUUID,VK_UUID_SIZE))stored.clear();
        info.initialDataSize=stored.size();info.pInitialData=stored.empty()?nullptr:stored.data();
    }
    auto result=vkCreatePipelineCache(device_,&info,nullptr,&driver_);
    if(result!=VK_SUCCESS&&!stored.empty()) {info.initialDataSize=0;info.pInitialData=nullptr;result=vkCreatePipelineCache(device_,&info,nullptr,&driver_);}
    if(result!=VK_SUCCESS) {error="Native driver pipeline cache: "+std::to_string(int(result));return false;}
    LoadRecipes();return true;
}
bool ProgramCache::Save(std::string& error) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if(!driver_||path_.empty())return true;
    if(!SaveRecipes(error))return false;
    std::size_t size=0;if(vkGetPipelineCacheData(device_,driver_,&size,nullptr)!=VK_SUCCESS) {error="Native pipeline cache size query failed";return false;}
    if(!size)return true;
    std::vector<unsigned char> data(size);
    if(vkGetPipelineCacheData(device_,driver_,&size,data.data())!=VK_SUCCESS) {error="Native pipeline cache read failed";return false;}
    const std::string temporary=path_+".tmp";FILE* file=std::fopen(temporary.c_str(),"wb");
    if(!file) {error="Native pipeline cache temp file unavailable";return false;}
    const bool written=std::fwrite(data.data(),1,size,file)==size;
    if(std::fclose(file)!=0||!written||std::rename(temporary.c_str(),path_.c_str())!=0) {std::remove(temporary.c_str());error="Native pipeline cache write failed";return false;}
    return true;
}
std::shared_ptr<GraphicsProgram> ProgramCache::Acquire(const ProgramDescription& description,std::string& error) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if(!driver_) {error="Native program cache is not initialized";return {};}
    const auto keyStart=timing_?std::chrono::steady_clock::now():std::chrono::steady_clock::time_point{};
    shader_assets::TraceIntegrity(description.vertexOwner,"before_key_vs");shader_assets::TraceIntegrity(description.pixelOwner,"before_key_ps");
    PipelineKey key;const bool keyed=PipelineKey::Create(description,key,error);
    if(timing_)keyNs_+=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-keyStart).count();
    if(!keyed)return {};
    auto found=programs_.find(key);if(found!=programs_.end()) {++hits_;return found->second;}
    if(programs_.size()>=capacity_) {
        // Only cache-owned programs can be evicted. In-flight frame borrowers
        // prevent destruction; a fully pinned cache fails explicitly.
        auto victim=programs_.begin();for(;victim!=programs_.end();++victim)if(victim->second.use_count()==1)break;
        if(victim==programs_.end()) {error="Native program cache capacity pinned by frame borrowers";return {};}
        programs_.erase(victim);++evictions_;
    }
    auto program=std::make_shared<GraphicsProgram>(device_);
    const auto createStart=std::chrono::steady_clock::now();
    const std::array<ShaderTranslation,2>* translated=nullptr;
    bool pairCached=false;
    // Pipeline raster/blend/depth/vertex-layout variants do not change linked
    // SPIR-V. Raw preview/test bytecode keeps the ordinary translation path.
    if(description.vertexOwner&&description.pixelOwner) {
        unsigned reference{};static_assert(sizeof(reference)==sizeof(description.alphaTest.reference));
        std::memcpy(&reference,&description.alphaTest.reference,sizeof(reference));
        const bool relaxed=RelaxedPrecisionEnabled();
        for(const auto& pair:shaderPairs_)if(pair.vertex==description.vertexOwner&&pair.pixel==description.pixelOwner
            &&pair.alphaComparison==description.alphaTest.comparison&&pair.alphaReference==reference
            &&pair.depthMask==description.pixelDepthSamplerMask&&pair.ubyteMask==description.vertexUbyteMask&&pair.relaxed==relaxed) {
            translated=&pair.shaders;pairCached=true;++shaderPairHits_;break;
        }
        if(!translated) {
            ShaderPair pair;pair.vertex=description.vertexOwner;pair.pixel=description.pixelOwner;
            pair.alphaComparison=description.alphaTest.comparison;pair.alphaReference=reference;
            pair.depthMask=description.pixelDepthSamplerMask;pair.ubyteMask=description.vertexUbyteMask;pair.relaxed=relaxed;
            shader_assets::TraceIntegrity(description.vertexOwner,"before_pair_vs");shader_assets::TraceIntegrity(description.pixelOwner,"before_pair_ps");
            const bool pairTranslated=TranslateShaderPair(description.vertexBytecode,description.vertexBytes,description.pixelBytecode,description.pixelBytes,
                pair.shaders[0],pair.shaders[1],&description.alphaTest,description.pixelDepthSamplerMask,description.vertexUbyteMask);
            shader_assets::TraceIntegrity(description.vertexOwner,"after_pair_vs");shader_assets::TraceIntegrity(description.pixelOwner,"after_pair_ps");
            if(!pairTranslated) {
                error=pair.shaders[0].diagnostics+pair.shaders[1].diagnostics;
                auto token=[](const void* bytes,std::size_t size){
                    std::uint32_t value{};if(bytes&&size>=sizeof(value))std::memcpy(&value,bytes,sizeof(value));return value;
                };
                error+=" vs="+description.vertexOwner->name+" words="+std::to_string(description.vertexBytes/4)
                    +" token="+std::to_string(token(description.vertexBytecode,description.vertexBytes))
                    +" ps="+description.pixelOwner->name+" words="+std::to_string(description.pixelBytes/4)
                    +" token="+std::to_string(token(description.pixelBytecode,description.pixelBytes));return {};
            }
            // Bound CPU-side metadata independently of in-flight GPU programs;
            // programs own copies of the translated data they need.
            if(shaderPairs_.size()>=capacity_)shaderPairs_.erase(shaderPairs_.begin());
            shaderPairs_.push_back(std::move(pair));translated=&shaderPairs_.back().shaders;
        }
    }
    const bool created=program->Create(description,driver_,error,translated);
    const auto createNs=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-createStart).count();
    createNs_+=createNs;
#if defined(__ANDROID__)
    // Misses are rare and may land between the every-tenth-frame summaries.
    __android_log_print(ANDROID_LOG_INFO,"BO1NativeVulkan",
        "native_pipeline_create frame=%u serial=%u total_ms=%.3f success=%u cache_size=%zu vs=%s ps=%s depth_mask=%u ubyte_mask=%u pair_cached=%u pair_count=%zu",
        frame_,misses_+1,double(createNs)/1e6,unsigned(created),programs_.size(),
        description.vertexOwner?description.vertexOwner->name.c_str():"<raw>",
        description.pixelOwner?description.pixelOwner->name.c_str():"<raw>",description.pixelDepthSamplerMask,description.vertexUbyteMask,unsigned(pairCached),shaderPairs_.size());
#endif
    if(!created)return {};
    programs_.emplace(std::move(key),program);++misses_;
    if(!prewarming_)RecordRecipe(description);
    return program;
}
}
