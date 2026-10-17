#pragma once
#include <cstdint>
#include <cstddef>
struct GfxCmdBufState;struct GfxDrawSurfListInfo;struct GfxViewInfo;
namespace bo1::vulkan::forward_opaque {
// Canonical slots15,12,13,14: reflection probe and three lightmap textures.
inline bool SafeCustomSamplers(unsigned flags){return (flags&~15u)==0;}
inline bool EligibleWords(unsigned color,unsigned depth) {
    const unsigned mode=depth&0xffu;
    return (color&0x08000000u)&&(color&0x800u)&&!(color&0x87000700u)
        &&(color&0xc000u)&&(mode==8u||mode==12u||mode==13u);
}
inline bool ConvertWords(unsigned words[2]) {
    if(!EligibleWords(words[0],words[1]))return false;
    // True means accepted by the forward policy, including existing LEQUAL/write.
    words[1]=(words[1]&~15u)|13u;return true;
}
inline bool SafeFragment(const std::uint32_t* code,std::size_t words) {
    if(!code||words<5||code[0]!=0x07230203u)return false;
    for(std::size_t i=5;i<words;) {
        const unsigned count=code[i]>>16,op=code[i]&0xffffu;
        if(!count||count>words-i)return false;
        if(op==252||op==4416||op==5380)return false; // kill/terminate/demote
        if(op==16&&count>=3&&code[i+2]==12)return false; // DepthReplacing
        if(op==71&&count>=4&&code[i+2]==11&&code[i+3]==22)return false; // FragDepth
        i+=count;
    }
    return true;
}
void SetEnabled(bool);bool Enabled();
bool ViewEligible(const GfxViewInfo*);
bool Apply(const GfxCmdBufState*,unsigned words[2]);
std::uint64_t OmittedGroups();
class MainLitScope {
    const GfxCmdBufState* previous_{};
public:
    MainLitScope(const GfxCmdBufState*,bool);~MainLitScope();
};
class PrepassScope {
    const GfxDrawSurfListInfo* previous_{};
public:
    PrepassScope(const GfxDrawSurfListInfo*,bool);~PrepassScope();
};
}
