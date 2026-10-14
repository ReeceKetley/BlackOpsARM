#pragma once
struct IDirect3DDevice9;
struct Material;
struct GfxViewInfo;
namespace bo1::android::post_fusion_trial {
void RegisterControls();
bool TryBloomColor(const GfxViewInfo* view);
// The caller must skip ONLY an owned fused draw when this returns false.
// Original draws always return true, including unsupported validation cases.
bool ObservePass(IDirect3DDevice9* device, const Material* material, unsigned passIndex);
void EndOriginalColor(const GfxViewInfo* view);
void Release();
}
