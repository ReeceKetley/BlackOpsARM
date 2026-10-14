#pragma once
struct IDirect3DDevice9;
struct Material;
namespace bo1::android::fullscreen_triangle {
void RegisterControls();
class Scope { public: Scope(); ~Scope(); Scope(const Scope&)=delete; Scope& operator=(const Scope&)=delete; };
bool TryDraw(IDirect3DDevice9*,const Material*,unsigned passIndex);
void Release();
}
