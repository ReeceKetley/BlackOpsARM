#include "shader_translate.h"
#include <unordered_map>
#include <atomic>
#include <vkd3d_shader.h>
#include <cstring>
#include <utility>
#include <cctype>

namespace bo1::vulkan {
namespace {
std::atomic<bool> relaxedPrecision{};
struct CompilerMemory {
    vkd3d_shader_scan_descriptor_info descriptors{};
    vkd3d_shader_scan_signature_info signatures{};
    vkd3d_shader_code code{};
    char* messages{};
    CompilerMemory() {
        descriptors.type=VKD3D_SHADER_STRUCTURE_TYPE_SCAN_DESCRIPTOR_INFO;
        signatures.type=VKD3D_SHADER_STRUCTURE_TYPE_SCAN_SIGNATURE_INFO;
        descriptors.next=&signatures;
    }
    ~CompilerMemory() {
        vkd3d_shader_free_messages(messages);
        vkd3d_shader_free_shader_code(&code);
        vkd3d_shader_free_scan_descriptor_info(&descriptors);
        vkd3d_shader_free_scan_signature_info(&signatures);
    }
    void Messages(std::string& diagnostics) {
        if(messages) { diagnostics+=messages; vkd3d_shader_free_messages(messages); messages=nullptr; }
    }
};
void Signature(const vkd3d_shader_signature& source, std::vector<ShaderVarying>& target) {
    for(unsigned i=0;i<source.element_count;++i) {
        const auto& e=source.elements[i];
        target.push_back({e.semantic_name?e.semantic_name:"",e.semantic_index,
            e.register_index,e.mask,e.used_mask,unsigned(e.sysval_semantic),unsigned(e.component_type)});
    }
}
}
namespace {
// Conservative SPIR-V scan of the float-constant uniform block: returns its
// declared vec4 array length only when every access chain into it uses
// constant indices (no relative addressing). Anything else reads all 256.
unsigned ScanConstantRows(const std::vector<std::uint32_t>& code,unsigned set,unsigned binding) {
    constexpr unsigned Full=256;
    if(code.size()<5)return Full;
    std::unordered_map<std::uint32_t,std::uint32_t> setOf,bindingOf,strideOf,constants,pointee;
    std::unordered_map<std::uint32_t,std::vector<std::uint32_t>> structs;
    std::unordered_map<std::uint32_t,std::pair<std::uint32_t,std::uint32_t>> arrays;
    std::vector<std::pair<std::uint32_t,std::uint32_t>> uniformVariables;
    for(std::size_t i=5;i<code.size();) {
        const std::uint32_t count=code[i]>>16,op=code[i]&0xffffu;
        if(!count||i+count>code.size())return Full;
        const std::uint32_t* w=&code[i];
        if(op==71&&count>=4) { // OpDecorate
            if(w[2]==34)setOf[w[1]]=w[3];else if(w[2]==33)bindingOf[w[1]]=w[3];else if(w[2]==6)strideOf[w[1]]=w[3];
        } else if(op==43&&count==4)constants[w[2]]=w[3]; // OpConstant (32-bit)
        else if(op==32&&count==4)pointee[w[1]]=w[3]; // OpTypePointer
        else if(op==30)structs[w[1]].assign(w+2,w+count); // OpTypeStruct
        else if(op==28&&count==4)arrays[w[1]]={w[2],w[3]}; // OpTypeArray
        else if(op==59&&count>=4&&w[3]==2)uniformVariables.push_back({w[2],w[1]}); // OpVariable Uniform
        i+=count;
    }
    std::uint32_t variable=0,pointerType=0;
    for(const auto& v:uniformVariables) {
        const auto s=setOf.find(v.first),b=bindingOf.find(v.first);
        if(s!=setOf.end()&&b!=bindingOf.end()&&s->second==set&&b->second==binding){if(variable)return Full;variable=v.first;pointerType=v.second;}
    }
    if(!variable)return Full;
    const auto block=pointee.find(pointerType);if(block==pointee.end())return Full;
    const auto members=structs.find(block->second);if(members==structs.end()||members->second.size()!=1)return Full;
    const auto array=arrays.find(members->second[0]);if(array==arrays.end())return Full;
    const auto stride=strideOf.find(array->first);const auto length=constants.find(array->second.second);
    if(stride==strideOf.end()||stride->second!=16||length==constants.end()||!length->second||length->second>Full)return Full;
    for(std::size_t i=5;i<code.size();) {
        const std::uint32_t count=code[i]>>16,op=code[i]&0xffffu;const std::uint32_t* w=&code[i];
        if(op==65||op==66) { // OpAccessChain / OpInBoundsAccessChain
            bool base=count>=4&&w[3]==variable;
            for(std::uint32_t k=4;k<count;++k)if(w[k]==variable)return Full;
            if(base) {
                if(count<6)return Full; // must select the member and a row
                for(std::uint32_t k=4;k<count;++k)if(!constants.count(w[k]))return Full;
            }
        } else if(op!=71&&op!=72&&op!=5&&op!=6&&op!=59) {
            for(std::uint32_t k=1;k<count;++k)if(w[k]==variable)return Full;
        }
        i+=count;
    }
    return length->second;
}
}
bool TranslateShader(const void* bytecode, std::size_t bytes, ShaderTranslation& output,
                     const std::vector<ShaderVarying>* nextStageInputs,const ShaderAlphaTest* alphaTest,unsigned depthSamplerMask,unsigned vertexUbyteMask) {
    output={};
    if(!bytecode||bytes<8||bytes%4) { output.diagnostics="Invalid SM3 bytecode length"; return false; }
    // Asset load addresses need not be uint32 aligned on ARM64.
    std::vector<std::uint32_t> input(bytes/4);
    std::memcpy(input.data(),bytecode,bytes);
    const auto version=input[0];
    if(version!=0xfffe0300u&&version!=0xffff0300u) {
        output.diagnostics="Native backend currently requires vs_3_0 or ps_3_0"; return false;
    }
    ShaderTranslation result;
    result.stage=version==0xfffe0300u?ShaderStage::Vertex:ShaderStage::Fragment;
    if((depthSamplerMask&~0xffffu)||(depthSamplerMask&&result.stage!=ShaderStage::Fragment)) {
        output.diagnostics="Depth comparison mask requires SM3 pixel slots 0..15";return false;
    }
    if((vertexUbyteMask&~0xffffu)||(vertexUbyteMask&&result.stage!=ShaderStage::Vertex)) {
        output.diagnostics="UBYTE conversion mask requires VS3 input slots 0..15";return false;
    }
    const unsigned set=result.stage==ShaderStage::Vertex?0:1;
    const auto visibility=result.stage==ShaderStage::Vertex?VKD3D_SHADER_VISIBILITY_VERTEX:VKD3D_SHADER_VISIBILITY_PIXEL;
    CompilerMemory memory;
    vkd3d_shader_compile_info info{};
    info.type=VKD3D_SHADER_STRUCTURE_TYPE_COMPILE_INFO;
    info.next=&memory.descriptors;
    info.source={input.data(),bytes};
    info.source_type=VKD3D_SHADER_SOURCE_D3D_BYTECODE;
    info.target_type=VKD3D_SHADER_TARGET_SPIRV_BINARY;
    info.log_level=VKD3D_SHADER_LOG_WARNING;
    vkd3d_shader_compile_option options[3];
    if(depthSamplerMask)options[info.option_count++]={VKD3D_SHADER_COMPILE_OPTION_NATIVE_D3DBC_DEPTH_SAMPLERS,depthSamplerMask};
    if(vertexUbyteMask)options[info.option_count++]={VKD3D_SHADER_COMPILE_OPTION_NATIVE_D3DBC_UBYTE_INPUTS,vertexUbyteMask};
    if(relaxedPrecision.load(std::memory_order_relaxed))options[info.option_count++]={VKD3D_SHADER_COMPILE_OPTION_NATIVE_D3DBC_RELAXED_PRECISION,1};
    if(info.option_count)info.options=options;
    int status=vkd3d_shader_scan(&info,&memory.messages);
    memory.Messages(result.diagnostics);
    if(status<0) { output.diagnostics=std::move(result.diagnostics); return false; }
    std::vector<vkd3d_shader_resource_binding> bindings;
    for(unsigned i=0;i<memory.descriptors.descriptor_count;++i) {
        const auto& d=memory.descriptors.descriptors[i];
        if(d.type==VKD3D_SHADER_DESCRIPTOR_TYPE_SRV&&d.register_index<16
            &&(depthSamplerMask&(1u<<d.register_index))&&d.resource_type!=VKD3D_SHADER_RESOURCE_TEXTURE_2D) {
            output.diagnostics="Native comparison sampling requires a routed 2D depth image";return false;
        }
        ResourceKind kind;unsigned slot,flags;
        if(d.register_space||d.count!=1) { output.diagnostics="Unsupported SM3 descriptor space/array"; return false; }
        if(d.type==VKD3D_SHADER_DESCRIPTOR_TYPE_CBV&&d.register_index<=2) {
            kind=ResourceKind::Constants;slot=d.register_index;flags=VKD3D_SHADER_BINDING_FLAG_BUFFER;
        } else if(d.type==VKD3D_SHADER_DESCRIPTOR_TYPE_SRV&&d.register_index<16) {
            kind=ResourceKind::Texture;slot=16+d.register_index;flags=VKD3D_SHADER_BINDING_FLAG_IMAGE;
        } else if(d.type==VKD3D_SHADER_DESCRIPTOR_TYPE_SAMPLER&&d.register_index<16) {
            kind=ResourceKind::Sampler;slot=32+d.register_index;flags=VKD3D_SHADER_BINDING_FLAG_IMAGE;
        } else { output.diagnostics="Unsupported SM3 descriptor kind/index"; return false; }
        bindings.push_back({d.type,0,d.register_index,visibility,flags,{set,slot,1}});
        result.bindings.push_back({kind,d.register_index,set,slot,unsigned(d.resource_type)});
    }
    Signature(memory.signatures.input,result.inputs);
    Signature(memory.signatures.output,result.outputs);
    std::vector<vkd3d_shader_varying_map> varyingMaps;
    if(nextStageInputs) {
        if(result.stage!=ShaderStage::Vertex) { output.diagnostics="Only vertex shaders have a following fragment stage";return false; }
        for(const auto& input:*nextStageInputs) {
            if(input.systemValue||!input.usedMask)continue;
            bool found=false;
            for(unsigned i=0;i<result.outputs.size();++i) {
                const auto& candidate=result.outputs[i];
                bool same=candidate.semantic.size()==input.semantic.size();
                for(unsigned j=0;same&&j<input.semantic.size();++j)
                    same=std::toupper(static_cast<unsigned char>(candidate.semantic[j]))
                        ==std::toupper(static_cast<unsigned char>(input.semantic[j]));
                if(same&&candidate.semanticIndex==input.semanticIndex&&!candidate.systemValue
                    &&(candidate.usedMask&input.usedMask)==input.usedMask) {
                    varyingMaps.push_back({i,input.registerIndex,input.usedMask});found=true;break;
                }
            }
            if(!found) { output.diagnostics="Missing vertex output for fragment semantic "+input.semantic;return false; }
        }
    }
    vkd3d_shader_spirv_target_info target{};
    target.type=VKD3D_SHADER_STRUCTURE_TYPE_SPIRV_TARGET_INFO;
    target.environment=VKD3D_SHADER_SPIRV_ENVIRONMENT_VULKAN_1_1;
    vkd3d_shader_parameter1 parameters[2]{};
    vkd3d_shader_parameter_info parameterInfo{};
    if(alphaTest) {
        if(result.stage!=ShaderStage::Fragment||alphaTest->comparison<1||alphaTest->comparison>8||!(alphaTest->reference>=0&&alphaTest->reference<=1)) {
            output.diagnostics="Invalid native fragment alpha test";return false;
        }
        parameters[0].name=VKD3D_SHADER_PARAMETER_NAME_ALPHA_TEST_FUNC;
        parameters[0].type=VKD3D_SHADER_PARAMETER_TYPE_IMMEDIATE_CONSTANT;parameters[0].data_type=VKD3D_SHADER_PARAMETER_DATA_TYPE_UINT32;
        parameters[0].u.immediate_constant.u.u32=alphaTest->comparison;
        parameters[1].name=VKD3D_SHADER_PARAMETER_NAME_ALPHA_TEST_REF;
        parameters[1].type=VKD3D_SHADER_PARAMETER_TYPE_IMMEDIATE_CONSTANT;parameters[1].data_type=VKD3D_SHADER_PARAMETER_DATA_TYPE_FLOAT32;
        parameters[1].u.immediate_constant.u.f32=alphaTest->reference;
        parameterInfo.type=VKD3D_SHADER_STRUCTURE_TYPE_PARAMETER_INFO;parameterInfo.parameters=parameters;parameterInfo.parameter_count=2;
        target.next=&parameterInfo;
    }
    vkd3d_shader_interface_info interface{};
    interface.type=VKD3D_SHADER_STRUCTURE_TYPE_INTERFACE_INFO;
    interface.next=&target;
    interface.bindings=bindings.data();interface.binding_count=unsigned(bindings.size());
    info.next=&interface;
    vkd3d_shader_varying_map_info varying{};
    if(nextStageInputs) {
        varying.type=VKD3D_SHADER_STRUCTURE_TYPE_VARYING_MAP_INFO;
        varying.next=&interface;
        varying.varying_map=varyingMaps.data();varying.varying_count=unsigned(varyingMaps.size());
        info.next=&varying;
    }
    status=vkd3d_shader_compile(&info,&memory.code,&memory.messages);
    memory.Messages(result.diagnostics);
    if(status<0||!memory.code.code||memory.code.size<20||memory.code.size%4) {
        output.diagnostics=std::move(result.diagnostics); return false;
    }
    result.spirv.resize(memory.code.size/4);
    std::memcpy(result.spirv.data(),memory.code.code,memory.code.size);
    if(result.spirv[0]!=0x07230203u||result.spirv[1]>0x00010300u) {
        output.diagnostics="Compiler returned invalid or newer-than-Vulkan-1.1 SPIR-V";return false;
    }
    for(const auto& binding:result.bindings)if(binding.kind==ResourceKind::Constants&&binding.registerIndex==0)
        result.constantRows=ScanConstantRows(result.spirv,binding.set,binding.binding);
    output=std::move(result);
    return true;
}
bool TranslateShaderPair(const void* vertex,std::size_t vertexBytes,
                         const void* fragment,std::size_t fragmentBytes,
                         ShaderTranslation& vs,ShaderTranslation& ps,const ShaderAlphaTest* alphaTest,unsigned pixelDepthSamplerMask,unsigned vertexUbyteMask) {
    vs={};ps={};ShaderTranslation v,p;
    if(!TranslateShader(fragment,fragmentBytes,p,nullptr,alphaTest,pixelDepthSamplerMask)||p.stage!=ShaderStage::Fragment) {
        ps.diagnostics=p.diagnostics.empty()?"Expected fragment shader":std::move(p.diagnostics);return false;
    }
    if(!TranslateShader(vertex,vertexBytes,v,&p.inputs,nullptr,0,vertexUbyteMask)) { vs.diagnostics=std::move(v.diagnostics);return false; }
    vs=std::move(v);ps=std::move(p);return true;
}
void SetRelaxedPrecision(bool enabled){relaxedPrecision.store(enabled,std::memory_order_relaxed);}
}
