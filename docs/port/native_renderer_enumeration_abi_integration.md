# Native renderer asset enumeration callback ABI

Four canonical enumeration paths in `gfx_d3d/r_material.cpp` and `r_model.cpp` cast one-argument functions to the database's two-argument `(XAssetHeader, void *)` callback type. Matching register placement does not make that an exact C++ callable ABI; native function sanitization can reject it.

Android now supplies four exact static adapters. Each accepts the header and unused enumeration context, then calls the actual existing owner:

- `Material_FreeAllTechniqueSets`: `Material_FreeTechniqueSet`.
- `Material_UpdatePicmipAll`: `Material_UpdatePicmipSingle`.
- `R_ReleaseAllModels`: `R_ReleaseModel`.
- `R_OptimizeAllModels`: `R_OptimizeModel`.

The asset type, context value, includeOverride setting, owner algorithms and iteration conditions remain unchanged. Windows keeps its original calls. No sanitizer suppression, graphics behavior replacement, public header, CMake, source selection or flag was added. CP1252 and original line endings retained.

Verification:

- Actual NDK whole translation units compile, exit0: `build-android-audit/native_registry_closure/r_material-enum-native.log` and `r_model-enum-native.log`. No source body exclusion was introduced.
- Normal Windows x86/x64 both translation units pass once: `build-android-audit/takeover_renderer_enumeration_windows1/{x86,x64}/build-result.json`.
- These callback adapters need no synthetic GPU fixture. Actual rendered enumeration/shutdown and on-device GPU behavior remain unexecuted; whole-TU compilation does not certify them.

Reproduction for normal Windows comparison, with a fresh output directory:

```powershell
& tools/port/build_startup_services_windows_compile.ps1 -OutputDirectory build-android-audit/takeover_renderer_enumeration_windows1 -TranslationUnits @('src/gfx_d3d/r_material.cpp','src/gfx_d3d/r_model.cpp')
```

Frozen SHA256:

- r_material.cpp: CAB15CB3F9E1DAF606B426FF4A97DC6E2B8F39D81170DD5D51D5FD52D2D3CC8B
- r_model.cpp: CFB7C8A5BD1B15B6D4F8A8ED914FBFF81A8E90786C667B2DABF6891E6A1F9307

Associated read-only DB retirement audit: `DB_ShutdownXAssets` waits database/external consumers, unloads asset resources and entries, frees default entries/cache resources, then frees zone memory in reverse order. Native runtime XBlock pointers and the typed PMem allocation-record repair already exist. `SND_Shutdown` retires voices/streaming while separate bank registries retain real entries for typed DB removal. No additional mandatory headless teardown failure was established beyond the assigned FreeHeader signature adapters.

A separate concrete conditional ownership defect remains in `ui/ui_shared.cpp:Menus_RemoveMenu`: menu pointer table movement uses four-byte counts despite `menuDef_t *Menus[600]`. It needs a narrow sizeof-element repair when assigned; this batch does not own UI. The earlier audit's tentative `1 << 31` warning was withdrawn: signed shifts of that form are not demonstrated UB under this project's C++20 rules. No r_stream change is warranted by that claim.
