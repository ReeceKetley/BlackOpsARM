#include "shader_dump.h"
#include <vkd3d_shader.h>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <set>
#include <string>
#include <sys/stat.h>
char *Sys_EngineUserRoot();
namespace bo1::vulkan::shader_dump {
namespace {
// Diagnostic only: inactive unless <user root>/dump-shaders exists at first use.
std::once_flag checked;std::atomic<bool> enabled{};std::string directory;
std::mutex mutex;std::set<std::string> written;
std::string Safe(const char* text) {
    std::string result=text?text:"none";
    for(auto& c:result)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'))c='_';
    return result;
}
void Disassemble(FILE* file,const std::vector<std::uint32_t>& code) {
    vkd3d_shader_compile_info info{};info.type=VKD3D_SHADER_STRUCTURE_TYPE_COMPILE_INFO;
    info.source={code.data(),code.size()*4};info.source_type=VKD3D_SHADER_SOURCE_D3D_BYTECODE;
    info.target_type=VKD3D_SHADER_TARGET_D3D_ASM;info.log_level=VKD3D_SHADER_LOG_NONE;
    vkd3d_shader_code out{};char* messages{};
    if(vkd3d_shader_compile(&info,&out,&messages)>=0&&out.code)std::fwrite(out.code,1,out.size,file);
    else std::fprintf(file,"<disassembly failed>\n");
    vkd3d_shader_free_shader_code(&out);vkd3d_shader_free_messages(messages);
}
// SM1-3 CTAB comment block: names and registers of the shader's constants.
void ConstantTable(FILE* file,const std::vector<std::uint32_t>& code) {
    for(std::size_t i=1;i<code.size();) {
        const std::uint32_t token=code[i];
        if((token&0xffffu)!=0xfffeu)break; // comments precede instructions
        const std::uint32_t words=token>>16;
        if(i+1+words>code.size())break;
        if(words>=8&&code[i+1]==0x42415443u) { // 'CTAB'
            const auto* base=reinterpret_cast<const unsigned char*>(&code[i+2]);const std::size_t bytes=(words-1)*4;
            auto u32=[&](std::size_t o)->std::uint32_t{std::uint32_t v=0;if(o+4<=bytes)std::memcpy(&v,base+o,4);return v;};
            auto str=[&](std::uint32_t o)->std::string{std::string s;while(o<bytes&&base[o])s+=char(base[o++]);return s;};
            const std::uint32_t count=u32(12),info=u32(16);
            for(std::uint32_t c=0;c<count&&info+c*20+20<=bytes;++c) {
                const std::size_t e=info+c*20;std::uint16_t set,index,size;
                std::memcpy(&set,base+e+4,2);std::memcpy(&index,base+e+6,2);std::memcpy(&size,base+e+8,2);
                static const char* sets[]={"b","i","c","s"};
                std::fprintf(file,"; %s%u..+%u %s\n",set<4?sets[set]:"?",unsigned(index),unsigned(size),str(u32(e)).c_str());
            }
        }
        i+=1+words;
    }
}
}
bool Enabled() {
    std::call_once(checked,[]{
        const char* root=Sys_EngineUserRoot();if(!root||!*root)return;
        struct stat info{};if(stat((std::string(root)+"/dump-shaders").c_str(),&info)!=0)return;
        directory=std::string(root)+"/shader-dump";mkdir(directory.c_str(),0700);enabled=true;
    });
    return enabled.load();
}
void Write(const char* material,const char* technique,const std::vector<std::uint32_t>& vertex,const std::vector<std::uint32_t>& pixel,
    const float (*vertexConstants)[4],const float (*pixelConstants)[4]) {
    if(!Enabled())return;
    const std::string name=Safe(material)+"__"+Safe(technique);
    {std::lock_guard<std::mutex> lock(mutex);if(!written.insert(name).second)return;}
    FILE* file=std::fopen((directory+"/"+name+".txt").c_str(),"w");if(!file)return;
    std::fprintf(file,"material %s\ntechnique %s\n\n== vertex constants (CTAB)\n",material?material:"",technique?technique:"");
    ConstantTable(file,vertex);
    std::fprintf(file,"\n== pixel constants (CTAB)\n");ConstantTable(file,pixel);
    for(unsigned stage=0;stage<2;++stage) {
        const auto* constants=stage?pixelConstants:vertexConstants;if(!constants)continue;
        std::fprintf(file,"\n== %s constant values (non-zero rows)\n",stage?"pixel":"vertex");
        for(unsigned r=0;r<256;++r)if(constants[r][0]||constants[r][1]||constants[r][2]||constants[r][3])
            std::fprintf(file,"c%u = %g %g %g %g\n",r,constants[r][0],constants[r][1],constants[r][2],constants[r][3]);
    }
    std::fprintf(file,"\n== vertex shader\n");Disassemble(file,vertex);
    std::fprintf(file,"\n== pixel shader\n");Disassemble(file,pixel);
    std::fclose(file);
}
}
