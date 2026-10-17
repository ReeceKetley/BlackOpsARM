#include "gfx_vulkan/sun_shadow_quality.h"
#include "gfx_vulkan/sun_bmodel_cache.h"
#include <cstdlib>
#include <cstdio>
#include <vector>
using namespace bo1::vulkan::sun_shadow_quality;
void Check(bool ok){if(!ok)std::abort();}
int main() {
    // Canonical SF_BMODEL is 6; test all 4-bit draw-surface type values.
    constexpr unsigned brush=6;
    for(unsigned partition=0;partition<2;++partition)for(unsigned enabled=0;enabled<2;++enabled) {
        std::vector<unsigned> full,split;
        for(unsigned type=0;type<16;++type) {
            const bool expected=partition==0||enabled||type!=brush;
            Check(KeepCaster(partition,type==brush,enabled)==expected);
            if(SelectCaster(partition,type,brush,enabled,-1,false))full.push_back(type);
            unsigned selected=0;
            for(int kind=0;kind<2;++kind)selected+=SelectCaster(partition,type,brush,enabled,kind,false);
            Check(selected==unsigned(expected));
            if(selected)split.push_back(type);
            // Previously cache-eligible brush surfaces must also be omitted.
            Check(SelectCaster(partition,type,brush,enabled,-1,type==brush)==expected);
        }
        Check(full==split);
        Check(full.size()==(partition==1&&!enabled?15u:16u));
        for(std::size_t i=1;i<full.size();++i)Check(full[i-1]<full[i]);
    }
    Policy policy;bo1::vulkan::sun_bmodel_cache::Mode mode;
    bool depthValid=true;unsigned releases=0;
    auto update=[&](bool far,bool cache) {
        const bool modeChanged=mode.Change(far&&cache);
        if(policy.Update(far,modeChanged)){++releases;depthValid=false;}
    };
    update(true,true);Check(releases==1&&!depthValid);depthValid=true;
    update(true,true);Check(releases==1&&depthValid);
    update(false,false);Check(releases==2&&!depthValid&&!mode.Enabled()); // both flip, one release
    depthValid=true;update(false,true);Check(releases==2&&depthValid&&!mode.Enabled()); // quality suppresses brush cache
    update(true,true);Check(releases==3&&!depthValid&&mode.Enabled()); // discard reduced depth before restore
    depthValid=true;update(true,false);Check(releases==4&&!depthValid);
    depthValid=true;update(false,false);Check(releases==5&&!depthValid); // retire even with cache mode already off
    depthValid=true;update(true,false);Check(releases==6&&!depthValid); // uncached/full fallback restored
    std::puts("native sun shadow quality passed: all types/partitions, cached+uncached filters, one release per transition");
}
