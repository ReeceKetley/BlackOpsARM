#pragma once
#include "sun_bmodel_cache.h"
#include <unordered_map>
namespace bo1::vulkan::near_shadow_cache {
using sun_bmodel_cache::Values;
using sun_bmodel_cache::Snapshot;
// Per-preparation only. Owner identity references exact copied routing data;
// nothing is retained from this dictionary across frames.
struct Declarations {
    bool enabled;std::size_t inlineWords{};
    std::unordered_map<const void*,sun_bmodel_cache::MaterialSnapshot> entries;
    explicit Declarations(bool deduplicate):enabled(deduplicate){}
    template<class Build> bool Append(Values& out,const void* owner,Build build){
        if(!enabled){const auto start=out.size();if(!build(out))return false;
            inlineWords+=out.size()-start;entries.emplace(owner,sun_bmodel_cache::MaterialSnapshot{});return true;}
        const auto& entry=sun_bmodel_cache::FindOrBuildMaterialSnapshot(entries,owner,[&](sun_bmodel_cache::MaterialSnapshot& snapshot){return build(snapshot.values);});
        return !entry.values.empty();
    }
    unsigned UniqueCount()const{unsigned count=0;for(const auto& entry:entries)if(!enabled||!entry.second.values.empty())++count;return count;}
    std::size_t Finish(Values& base)const{
        if(!enabled)return inlineWords*sizeof(Values::value_type);
        const auto start=base.size();sun_bmodel_cache::AppendMaterialDefinitions(base,entries);
        return (base.size()-start)*sizeof(Values::value_type);
    }
};
// Native material_state.cpp: bit0 writes depth, bit1 disables testing,
// bits2..3 select ALWAYS/LESS/EQUAL/LEQUAL. Cache only monotonic depth writes.
inline bool DepthPassSafe(unsigned word){const unsigned compare=(word>>2)&3;
    return !(word&0xc2u)&&(word&1u)&&(compare==1||compare==3);}

inline bool AppendIndices(Values& out,const unsigned short* indices,std::size_t size,std::uint64_t first,std::uint64_t count){
    if(!indices||first>size||count>size-first)return false;
    out.push_back(count);
    // Four exact 16-bit indices per word; no hash or uninitialized padding.
    for(std::uint64_t i=0;i<count;){std::uint64_t packed=0;
        for(unsigned shift=0;shift<64&&i<count;shift+=16,++i)packed|=std::uint64_t(indices[first+i])<<shift;
        out.push_back(packed);}
    return true;
}
// Canonical non-pretess BSP / rigid / cached-smodel word streams. Padding is
// not semantic. The callback resolves stable surface/instance identities.
template<class Emit> bool Decode(const unsigned* words,std::size_t size,std::size_t start,unsigned type,Values& out,Emit emit) {
    if(!words||start>=size||(type!=0&&type!=2&&type!=4))return false;
    std::size_t pos=start;bool any=false;
    while(pos<size){
        const unsigned header=words[pos++];const unsigned count=type==2?(header&0x0fffffff):header;
        if(!count)return any;
        unsigned surface=0;
        if(type!=0){if(pos>=size)return false;surface=words[pos++];if(!surface)return false;}
        const std::size_t packedWords=(std::size_t(count)+1)/2;
        if(packedWords>size-pos)return false;
        out.push_back(header);out.push_back(count);
        for(unsigned i=0;i<count;++i){const unsigned id=(words[pos+i/2]>>((i&1)*16))&0xffff;
            if(!emit(type,surface,id,out))return false;}
        pos+=packedWords;any=true;
    }
    return false; // Missing terminator: leave the canonical draw path untouched.
}
struct Decision {std::uint64_t key{};bool ready{},changed{};};
class State {
    Values previous_;std::uint64_t key_{};bool havePrevious_{};
public:
    Decision Update(Values base,Snapshot records){
        // Preserve the canonical prefix order; the dynamic suffix is never moved.
        base.push_back(records.size());
        for(const auto& record:records){base.push_back(record.size());base.insert(base.end(),record.begin(),record.end());}
        const bool changed=!havePrevious_||base!=previous_;
        if(changed){++key_;if(!key_)++key_;}
        const bool ready=havePrevious_&&!changed&&!records.empty();
        previous_=std::move(base);havePrevious_=true;return {key_,ready,changed};
    }
    void Reset(){previous_.clear();havePrevious_=false;} // retain key namespace
};
}
