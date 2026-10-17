#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <vector>
namespace bo1::vulkan::sun_bmodel_cache {
using Values=std::vector<std::uint64_t>;
using Snapshot=std::vector<Values>;
struct MaterialSnapshot {Values values;bool usesDestructible{};};
template<class Lookup,class Key,class Build> const MaterialSnapshot& FindOrBuildMaterialSnapshot(Lookup& materials,Key key,Build build) {
    auto entry=materials.find(key);
    if(entry==materials.end()){
        MaterialSnapshot snapshot;if(!build(snapshot))snapshot.values.clear();
        entry=materials.emplace(key,std::move(snapshot)).first;
    }
    return entry->second;
}
// The lookup exists for one build only. Definitions (including all shader and
// asset serials) are compared exactly once; brush records retain the identity.
// Length framing prevents differently partitioned definitions from aliasing.
template<class Lookup> void AppendMaterialDefinitions(Values& base,const Lookup& materials) {
    struct Entry {std::uintptr_t identity;const Values* values;};
    std::vector<Entry> sorted;sorted.reserve(materials.size());
    for(const auto& entry:materials)if(!entry.second.values.empty())
        sorted.push_back({reinterpret_cast<std::uintptr_t>(entry.first),&entry.second.values});
    std::sort(sorted.begin(),sorted.end(),[](const Entry& a,const Entry& b){return a.identity<b.identity;});
    base.push_back(sorted.size());
    for(const auto& entry:sorted){base.push_back(entry.identity);base.push_back(entry.values->size());
        base.insert(base.end(),entry.values->begin(),entry.values->end());}
}
// Append semantic values, never native structs/padding or borrowed frame pointers.
inline bool Finite(const float* values,unsigned count){for(unsigned i=0;i<count;++i)if(!std::isfinite(values[i]))return false;return true;}
inline void Floats(Values& out,const float* values,unsigned count) {
    for(unsigned i=0;i<count;++i){std::uint32_t bits;std::memcpy(&bits,values+i,sizeof(bits));out.push_back(bits);}
}
template<class Brush> Values BrushValues(Values values,std::uint64_t packed,const Brush& brush,bool usesDestructible=true) {
    if(!Finite(brush.placement->base.quat,4)||!Finite(brush.placement->base.origin,3)||!Finite(&brush.placement->scale,1)
        ||(usesDestructible&&(!Finite(&brush.bmodelBurnAmt,1)||!Finite(&brush.bmodelFadeAmt,1))))return {};
    if(brush.shaderConstSet)for(unsigned j=0;j<7;++j)if((brush.shaderConstSet->used&(1u<<j))&&!Finite(brush.shaderConstSet->value[j],4))return {};
    const auto semanticStart=values.size();
    values.push_back(packed&~std::uint64_t(0xffff));
    values.push_back(reinterpret_cast<std::uintptr_t>(brush.surf));
    const auto& tris=brush.surf->tris;
    values.push_back(tris.firstVertex);values.push_back(tris.vertexCount);values.push_back(tris.baseIndex);values.push_back(tris.triCount);
    values.push_back(tris.vertexLayerData);values.push_back(tris.stream2ByteOffset);
    Floats(values,brush.placement->base.quat,4);Floats(values,brush.placement->base.origin,3);Floats(values,&brush.placement->scale,1);
    if(usesDestructible){Floats(values,&brush.bmodelBurnAmt,1);Floats(values,&brush.bmodelFadeAmt,1);}
    else {values.push_back(0);values.push_back(0);}
    // Inactive slots are not initialized by R_InitShaderConstantSet and are
    // never consumed by RB_ApplyShaderConstantSet. Do not read those bytes.
    values.push_back(brush.shaderConstSet!=nullptr);
    if(brush.shaderConstSet){const auto& set=*brush.shaderConstSet;values.push_back(set.used);
        for(unsigned j=0;j<7;++j){if(set.used&(1u<<j)){values.push_back(set.constantSource[j]);Floats(values,set.value[j],4);}
            else for(unsigned k=0;k<5;++k)values.push_back(0);}}
    values.push_back(semanticStart); // diagnostic field map, no transient identity
    return values;
}
struct Difference {
    bool changed{};std::size_t index{};std::uint64_t before{},after{};
};
inline Difference Compare(const Values& before,const Values& after){
    const auto n=std::min(before.size(),after.size());std::size_t i=0;
    while(i<n&&before[i]==after[i])++i;
    return {i<n||before.size()!=after.size(),i,i<before.size()?before[i]:0,i<after.size()?after[i]:0};
}
struct Decision {
    std::uint64_t key{};bool includeBrushes{},invalidated{};
    Difference baseDifference,brushDifference;
    std::size_t brushIndex{},beforeCount{},afterCount{},semanticStart{};
    std::uint64_t beforeSurface{},afterSurface{};
};
// Key namespaces must never share a stored depth image across a mode change.
// Off means the original frontend key, with no snapshot work at all.
class Mode {
    bool enabled_{};
public:
    bool Change(bool enabled){const bool changed=enabled!=enabled_;enabled_=enabled;return changed;}
    bool Enabled()const{return enabled_;}
    std::uint64_t Key(std::uint64_t original,std::uint64_t extended)const{return enabled_?extended:original;}
};
class State {
    Values base_;
    Snapshot previous_,stored_;
    std::uint64_t serial_{};
    bool havePrevious_{},include_{};
public:
    Decision Update(Values base,Snapshot snapshot,bool enabled) {
        std::sort(snapshot.begin(),snapshot.end());
        Decision result;
        result.baseDifference=Compare(base_,base);
        result.beforeCount=previous_.size();result.afterCount=snapshot.size();
        const auto n=std::min(previous_.size(),snapshot.size());std::size_t i=0;
        while(i<n&&previous_[i]==snapshot[i])++i;
        result.brushIndex=i;
        if(i<n){result.brushDifference=Compare(previous_[i],snapshot[i]);
            result.semanticStart=snapshot[i].empty()?0:snapshot[i].back();
            const auto oldStart=previous_[i].empty()?0:previous_[i].back();
            if(oldStart+1<previous_[i].size())result.beforeSurface=previous_[i][oldStart+1];
            if(result.semanticStart+1<snapshot[i].size())result.afterSurface=snapshot[i][result.semanticStart+1];
        }else result.brushDifference.changed=previous_.size()!=snapshot.size();
        const bool baseChanged=result.baseDifference.changed;
        const bool stable=enabled&&!snapshot.empty()&&!baseChanged&&havePrevious_&&snapshot==previous_;
        const bool invalidate=baseChanged||stable!=include_||(stable&&snapshot!=stored_);
        if(invalidate||!serial_){++serial_;if(!serial_)++serial_;}
        base_=std::move(base);previous_=std::move(snapshot);havePrevious_=enabled;
        include_=stable;
        if(stable)stored_=previous_;else stored_.clear();
        result.key=serial_;result.includeBrushes=stable;result.invalidated=invalidate;return result;
    }
};
}
