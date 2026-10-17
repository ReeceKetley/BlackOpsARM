#include "gfx_vulkan/forward_opaque.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace bo1::vulkan::forward_opaque;
int main() {
    for(unsigned flags=0;flags<16;++flags)assert(SafeCustomSamplers(flags));
    for(unsigned flags:{16u,17u,31u,32u,128u,255u,0xffffffffu})assert(!SafeCustomSamplers(flags));
    constexpr unsigned color=0x18004800u;
    for(unsigned mode:{8u,12u,13u}) {
        assert(EligibleWords(color,mode));
        unsigned words[]{color,0xabcd0000u|mode};
        assert(ConvertWords(words)&&words[0]==color&&words[1]==0xabcd000du);
        assert(ConvertWords(words)&&words[0]==color&&words[1]==0xabcd000du);
        for(unsigned forbidden:{2u,16u,32u,64u,128u})assert(!EligibleWords(color,mode|forbidden));
    }
    assert(EligibleWords(0x18128812u,13u)); // SDK673 observed opaque world state.
    for(unsigned bit:{0x100u,0x200u,0x400u,0x1000000u,0x2000000u,0x4000000u,0x80000000u})
        assert(!EligibleWords(color|bit,8));
    assert(!EligibleWords(color&~0x800u,8));
    assert(!EligibleWords(color&~0x8000000u,8));
    assert(!EligibleWords(color&~0xc000u,8));
    for(unsigned depth:{0u,1u,4u,5u,9u,24u,40u,72u,136u}) {
        assert(!EligibleWords(color,depth));
        unsigned words[]{color,depth};
        assert(!ConvertWords(words)&&words[0]==color&&words[1]==depth);
    }
    const std::vector<std::uint32_t> header{0x07230203u,0x10000u,0,32,0};
    auto safe=[&](std::initializer_list<std::uint32_t> instructions){
        auto code=header;code.insert(code.end(),instructions);return SafeFragment(code.data(),code.size());
    };
    assert(!SafeFragment(nullptr,0));
    assert(!SafeFragment(header.data(),4));
    assert(safe({0x00010000u})); // OpNop
    assert(!safe({0}));
    assert(!safe({0x00020000u})); // truncated instruction
    for(unsigned opcode:{252u,4416u,5380u})assert(!safe({0x10000u|opcode}));
    assert(!safe({0x00030010u,1,12})); // DepthReplacing
    assert(safe({0x00030010u,1,7})); // OriginUpperLeft
    assert(!safe({0x00040047u,2,11,22})); // BuiltIn FragDepth
    assert(safe({0x00040047u,2,11,15})); // BuiltIn FragCoord
    std::puts("forward opaque state and fragment proof PASS");
}
