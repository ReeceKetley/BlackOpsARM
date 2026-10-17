#pragma once
namespace bo1::vulkan::sun_shadow_quality {
// This changes caster quality only. The far/overlap receivers may lose brush
// shadows; it does not extend the near cascade or alter shader selection.
inline bool KeepCaster(unsigned partition,bool isBrush,bool farBrushes) {
    return partition!=1||!isBrush||farBrushes;
}
inline bool SelectCaster(unsigned partition,unsigned type,unsigned brushType,bool farBrushes,int kind,bool cachedBrush) {
    if(!KeepCaster(partition,type==brushType,farBrushes))return false;
    const bool dynamic=type>=brushType&&!cachedBrush;
    return kind<0||dynamic==bool(kind);
}
struct Policy {
    bool farBrushes=true;
    // Combine both policy changes into one depth-cache retirement, before
    // restore. Returning to full quality also discards reduced cached depth.
    bool Update(bool requested,bool brushCacheModeChanged) {
        const bool changed=requested!=farBrushes||brushCacheModeChanged;
        farBrushes=requested;return changed;
    }
};
}
