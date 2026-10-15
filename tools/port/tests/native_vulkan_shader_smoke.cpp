#include "shader_translate.h"
#include "texture_bindings.h"
#include "pipeline_key.h"
#include "shader_assets.h"
#include <limits>
#include "platform/android/post_fusion_trial_bytecode.h"
#include <vulkan/vulkan.h>
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>
bool NativeVulkanDrawStorage(VkPhysicalDevice,VkDevice,unsigned);
bool NativeVulkanConstantUpload(VkPhysicalDevice,VkDevice,unsigned);
bool NativeVulkanImageCopy(VkPhysicalDevice,VkDevice,unsigned);
bool NativeVulkanUploadFlights(VkPhysicalDevice,VkDevice,unsigned);
bool NativeVulkanDepthCopy(VkPhysicalDevice,VkDevice,unsigned);
bool NativeVulkanUbyteDraw(VkPhysicalDevice,VkDevice,unsigned,const char*);
bool NativeVulkanDepthBiasDraw(VkPhysicalDevice,VkDevice,unsigned);
bool NativeVulkanDepthCompareDraw(VkPhysicalDevice,VkDevice,unsigned,const char*);
bool NativeVulkanLrpDraw(VkPhysicalDevice,VkDevice,unsigned);
bool NativeVulkanColourDraw(VkPhysicalDevice,VkDevice,unsigned,const char*,const char*);

namespace {
bool Check(VkResult result,const char* action) {
    if(result==VK_SUCCESS)return true;
    std::fprintf(stderr,"FAIL %s VkResult=%d\n",action,int(result));return false;
}
bool Program(VkDevice device,const char* name,const void* data,std::size_t bytes,const char* directory) {
    bo1::vulkan::ShaderTranslation shader;
    if(!bo1::vulkan::TranslateShader(data,bytes,shader)) {
        std::fprintf(stderr,"FAIL shader=%s diagnostics=%s\n",name,shader.diagnostics.c_str());return false;
    }
    VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    info.codeSize=shader.spirv.size()*4;info.pCode=shader.spirv.data();
    VkShaderModule module{};
    if(!Check(vkCreateShaderModule(device,&info,nullptr,&module),"vkCreateShaderModule"))return false;
    vkDestroyShaderModule(device,module,nullptr);
    std::printf("PASS shader=%s stage=%s bytes=%zu bindings=%zu inputs=%zu outputs=%zu diagnostics=%s\n",
        name,shader.stage==bo1::vulkan::ShaderStage::Vertex?"vertex":"fragment",info.codeSize,
        shader.bindings.size(),shader.inputs.size(),shader.outputs.size(),shader.diagnostics.c_str());
    for(const auto& b:shader.bindings)std::printf("binding shader=%s kind=%u register=%u set=%u binding=%u dimension=%u\n",
        name,unsigned(b.kind),b.registerIndex,b.set,b.binding,b.dimension);
    for(const auto& v:shader.inputs)std::printf("input shader=%s semantic=%s index=%u register=%u mask=%u used=%u system=%u\n",
        name,v.semantic.c_str(),v.semanticIndex,v.registerIndex,v.mask,v.usedMask,v.systemValue);
    for(const auto& v:shader.outputs)std::printf("output shader=%s semantic=%s index=%u register=%u mask=%u used=%u system=%u\n",
        name,v.semantic.c_str(),v.semanticIndex,v.registerIndex,v.mask,v.usedMask,v.systemValue);
    if(directory) {
        std::string path=std::string(directory)+"/"+name+".spv";
        FILE* file=std::fopen(path.c_str(),"wb");
        if(!file)return false;
        const bool written=std::fwrite(shader.spirv.data(),1,info.codeSize,file)==info.codeSize;
        const bool closed=std::fclose(file)==0;if(!written||!closed)return false;
    }
    return true;
}
}
int main(int argc,char** argv) {
    std::setvbuf(stdout,nullptr,_IONBF,0);
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName="boARM native shader foundation";app.apiVersion=VK_API_VERSION_1_1;
    VkInstanceCreateInfo create{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};create.pApplicationInfo=&app;
    VkInstance instance{};if(!Check(vkCreateInstance(&create,nullptr,&instance),"vkCreateInstance"))return 1;
    unsigned count{};bool ok=Check(vkEnumeratePhysicalDevices(instance,&count,nullptr),"enumerate count")&&count;
    std::vector<VkPhysicalDevice> devices(count);
    if(ok)ok=Check(vkEnumeratePhysicalDevices(instance,&count,devices.data()),"enumerate devices");
    VkPhysicalDevice physical{};unsigned family{};bool found=false;
    for(auto candidate:devices) {
        VkPhysicalDeviceProperties props{};vkGetPhysicalDeviceProperties(candidate,&props);
        if(props.apiVersion<VK_API_VERSION_1_1)continue;
        unsigned n{};vkGetPhysicalDeviceQueueFamilyProperties(candidate,&n,nullptr);
        std::vector<VkQueueFamilyProperties> queues(n);vkGetPhysicalDeviceQueueFamilyProperties(candidate,&n,queues.data());
        for(unsigned i=0;i<n;++i)if(queues[i].queueCount&&(queues[i].queueFlags&VK_QUEUE_GRAPHICS_BIT)) {
            physical=candidate;family=i;found=true;
            std::printf("device=%s vulkan=%u.%u.%u graphics_family=%u\n",props.deviceName,
                VK_VERSION_MAJOR(props.apiVersion),VK_VERSION_MINOR(props.apiVersion),VK_VERSION_PATCH(props.apiVersion),i);break;
        }
        if(found)break;
    }
    ok=ok&&found;VkDevice device{};
    float priority=1;VkDeviceQueueCreateInfo queue{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queue.queueFamilyIndex=family;queue.queueCount=1;queue.pQueuePriorities=&priority;
    VkDeviceCreateInfo deviceInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};deviceInfo.queueCreateInfoCount=1;deviceInfo.pQueueCreateInfos=&queue;
    if(ok)ok=Check(vkCreateDevice(physical,&deviceInfo,nullptr,&device),"vkCreateDevice");
    const bool keysOnly=argc>1&&!std::strcmp(argv[1],"--owned-keys-only");
    if(ok&&keysOnly) {
        using namespace bo1::vulkan;std::string error;
        auto a=std::make_shared<shader_assets::Record>();a->bytecode={1,2,3,4};
        auto b=std::make_shared<shader_assets::Record>();b->bytecode=a->bytecode;
        ProgramDescription d;d.vertexOwner=a;d.vertexBytecode=a->bytecode.data();d.vertexBytes=16;
        PipelineKey first,same,other;ok=PipelineKey::Create(d,first,error)&&PipelineKey::Create(d,same,error)&&first==same&&first.Hash()==same.Hash();
        d.vertexOwner=b;d.vertexBytecode=b->bytecode.data();ok=ok&&PipelineKey::Create(d,other,error)&&other!=first;
        d.vertexBytecode=a->bytecode.data();ok=ok&&!PipelineKey::Create(d,other,error);
        std::weak_ptr<const shader_assets::Record> weak=a;d={};a.reset();ok=ok&&!weak.expired();
        first={};same={};ok=ok&&weak.expired();
        // Raw callers still own copied contents, including unaligned inputs.
        unsigned char bytes[]={1,2,3,4,5};d.vertexBytecode=bytes+1;d.vertexBytes=4;
        ok=ok&&PipelineKey::Create(d,first,error)&&PipelineKey::Create(d,same,error)&&first==same;
        bytes[4]=9;ok=ok&&PipelineKey::Create(d,other,error)&&other!=first;
        std::printf("native_owned_pipeline_keys=%s owner_identity=1 mismatch_rejected=1 retained_lifetime=1 raw_value_fallback=1 no_gpu_draw=1\n",ok?"PASS":"FAIL");
    }
    const bool samplersOnly=argc>1&&!std::strcmp(argv[1],"--sampler-cache-only");
    if(ok&&samplersOnly) {
        using bo1::vulkan::texture_bindings::Sampler;std::string error;
        const unsigned decoded=(3u<<20)|(3u<<22)|(3u<<24)|(2u<<8)|(2u<<12)|(2u<<16);
        auto a=Sampler::Acquire(device,decoded,4,0,error);auto b=Sampler::Acquire(device,decoded,4,0,error);
        auto mip=Sampler::Acquire(device,decoded,3,0,error),bias=Sampler::Acquire(device,decoded,4,1,error);
        auto compare=Sampler::Acquire(device,decoded,4,0,error,true);
        ok=a&&a==b&&mip&&mip!=a&&bias&&bias!=a&&compare&&compare!=a&&compare->Comparison()&&!a->Comparison();
        ok=ok&&!Sampler::Acquire(device,decoded,4,std::numeric_limits<float>::infinity(),error)
            &&!Sampler::Acquire(device,decoded,0,0,error)&&!Sampler::Acquire(device,0,4,0,error);
        // Hold more than the metadata bound: eviction must not destroy users.
        std::vector<std::shared_ptr<Sampler>> held;
        for(unsigned i=0;i<300&&ok;++i){auto v=Sampler::Acquire(device,decoded,4,float(i)/300,error);ok=bool(v);held.push_back(v);}
        ok=ok&&a&&a->Handle()&&b->Handle()==a->Handle();
        held.clear();std::weak_ptr<Sampler> weak=a;a.reset();b.reset();
        ok=ok&&weak.expired();auto recreated=Sampler::Acquire(device,decoded,4,0,error);ok=ok&&bool(recreated);
        std::printf("native_sampler_cache=%s reuse=1 distinct_mips_bias_compare=1 invalid_rejected=3 eviction_retains_users=300 weak_lifetime=1\n",ok?"PASS":"FAIL");
    }
    const bool arenaOnly=argc>1&&!std::strcmp(argv[1],"--arena-only");
    if(ok&&arenaOnly)ok=NativeVulkanDrawStorage(physical,device,family);
    if(ok&&arenaOnly)ok=NativeVulkanConstantUpload(physical,device,family);
    const bool flightsOnly=argc>1&&!std::strcmp(argv[1],"--upload-flights-only");
    if(ok&&flightsOnly)ok=NativeVulkanUploadFlights(physical,device,family);
    const bool depthCopyOnly=argc>1&&!std::strcmp(argv[1],"--depth-copy-only");
    if(ok&&depthCopyOnly)ok=NativeVulkanDepthCopy(physical,device,family);
    const bool copyOnly=argc>1&&!std::strcmp(argv[1],"--copy-only");
    if(ok&&copyOnly)ok=NativeVulkanImageCopy(physical,device,family);
    const bool ubyteOnly=argc>1&&!std::strcmp(argv[1],"--ubyte-only");
    if(ok&&ubyteOnly)ok=NativeVulkanUbyteDraw(physical,device,family,argc>2?argv[2]:nullptr);
    const bool biasOnly=argc>1&&!std::strcmp(argv[1],"--bias-only");
    if(ok&&biasOnly)ok=NativeVulkanDepthBiasDraw(physical,device,family);
    const bool depthOnly=argc>1&&!std::strcmp(argv[1],"--depth-only");
    if(ok&&depthOnly)ok=NativeVulkanDepthCompareDraw(physical,device,family,argc>2?argv[2]:nullptr);
    if(ok&&!keysOnly&&!samplersOnly&&!flightsOnly&&!depthCopyOnly&&!depthOnly&&!biasOnly&&!ubyteOnly&&!copyOnly&&!arenaOnly) {
        using namespace bo1::android::post_fusion_trial::bytecode;
        const char* directory=argc>1?argv[1]:nullptr;
        ok=Program(device,"t5-fullscreen-vs",SharedVS,sizeof(SharedVS),directory)
            &&Program(device,"t5-bloom-ps",BloomPS,sizeof(BloomPS),directory)
            &&Program(device,"t5-colour-ps",ColorPS,sizeof(ColorPS),directory);
        // ARM asset addresses can be unaligned; no typed source dereference.
        std::vector<unsigned char> unaligned(sizeof(ColorPS)+1);
        std::memcpy(unaligned.data()+1,ColorPS,sizeof(ColorPS));
        bo1::vulkan::ShaderTranslation aligned,odd;
        ok=ok&&bo1::vulkan::TranslateShader(ColorPS,sizeof(ColorPS),aligned)
            &&bo1::vulkan::TranslateShader(unaligned.data()+1,sizeof(ColorPS),odd)&&aligned.spirv==odd.spirv;
        std::printf("unaligned_source=%s\n",ok?"PASS":"FAIL");
        bo1::vulkan::ShaderTranslation invalid=aligned;
        const bool rejected=!bo1::vulkan::TranslateShader(ColorPS,5,invalid)&&invalid.spirv.empty()&&invalid.bindings.empty();
        std::printf("invalid_length_clears_output=%s\n",rejected?"PASS":"FAIL");ok=ok&&rejected;
        bo1::vulkan::ShaderTranslation linkedVS,linkedPS;
        const bool linked=bo1::vulkan::TranslateShaderPair(SharedVS,sizeof(SharedVS),ColorPS,sizeof(ColorPS),linkedVS,linkedPS);
        std::printf("real_colour_stage_pair=%s\n",linked?"PASS":"FAIL");ok=ok&&linked;
        if(linked&&directory) {
            std::string path=std::string(directory)+"/t5-colour-linked-vs.spv";FILE* f=std::fopen(path.c_str(),"wb");
            if(f) { const bool written=std::fwrite(linkedVS.spirv.data(),4,linkedVS.spirv.size(),f)==linkedVS.spirv.size();
                const bool closed=std::fclose(f)==0;ok=ok&&written&&closed; } else ok=false;
        }
        if(ok)ok=NativeVulkanLrpDraw(physical,device,family);
        if(ok)ok=NativeVulkanColourDraw(physical,device,family,directory,argc>2?argv[2]:nullptr);
    }
    if(device)vkDestroyDevice(device,nullptr);
    vkDestroyInstance(instance,nullptr);
    std::printf("native_shader_foundation=%s real_shader_material_draw_tested=%u full_game_frame_tested=0\n",ok?"PASS":"FAIL",(keysOnly||samplersOnly||flightsOnly||copyOnly||depthCopyOnly)?0u:1u);return ok?0:1;
}
