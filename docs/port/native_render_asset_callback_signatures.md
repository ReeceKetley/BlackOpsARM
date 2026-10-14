# Native rendered asset callback signatures

Updated 2026-10-14. This batch repairs six incompatible asset-enumerator
registrations. It does not establish a successful `Com_Init` return or gameplay.
The root agent owns SDK438 integration and the actual tablet startup result.

## Actual failure and canonical contract

SDK437's genuine foreground startup passes the first three loading-context
Present calls, then fails while `CL_StartHunkUsers` constructs the developer GUI.
`CG_InitVisionSetsMenu` passes `CG_AddVisionSetMenuItem(XAssetHeader)` through a
cast to `void (*)(XAssetHeader, void *)`. The real indirect call in
`DB_EnumXAssets_FastFile` has the latter type. The SDK437 crash target
`0x2c2aa70` resolves to the former function in `cg_visionsets.cpp`.

Evidence is retained in
`build-android-audit/native_registry_closure/sdk-native-entry437-launch1-only.log`
and `sdk-native-launch437-crash1.txt`. UBSan and its failure policy remain enabled.

## Scoped implementation

Android registers exact `void __cdecl (XAssetHeader, void *)` adapters. Each
adapter calls the existing canonical owner using the received header. The two
typed-context owners receive `static_cast<ImageList *>(data)` or
`static_cast<AssetList *>(data)`. Single-argument owners ignore only the unused
context argument; they perform their original work. Every canonical return is
void. There are no function-pointer casts in the native registrations.

| Producer | Canonical owner | Data | Type / override filter |
| --- | --- | --- | --- |
| `CG_InitVisionSetsMenu` | `CG_AddVisionSetMenuItem` | unused | RAWFILE / false |
| `R_GetImageList` | `R_AddImageToList` | actual ImageList pointer | IMAGE / true |
| `R_ReleaseLostImages` | `R_FreeLostImage` | unused | IMAGE / true |
| `R_ReloadLostImages` | `R_RebuildLostImage` | unused | IMAGE / true |
| `DB_GetAllXAssetOfType_LoadObj` | `Hunk_AddAsset` | actual AssetList pointer | caller type / false |
| `UIViewer::SetupStreamer` | `ForceLoadWeapon` | unused | WEAPON / false |

The UI viewer path remains gated by its existing initialization, database and
zone conditions. No gate, asset header, asset format, registry traversal,
critical section, list capacity or resource ownership was changed.
`BlackOps64` was inspected read-only as the Windows oracle. The original Windows
registrations and all canonical callback bodies are retained verbatim.

## Registration closure and validation

The source audit found 17 active `DB_EnumXAssets` call sites across eight TUs.
Actual Android preprocessing confirms all 17 registrations have no
function-pointer cast. This includes the correctly typed existing material and
model adapters, image delay-load, material/model list, Mapkit callbacks and the
typed forwarding parameter in `DB_PrintXAssetsForType`; those remain untouched.
The existing LoadObj enumeration adapters also remain untouched.

All four changed whole TUs compile successfully with the actual native shared
engine compile commands: NDK30, Android API28, O0/debug/PIC, function
instrumentation and UBSan with no recovery. `com_memory.cpp` is compiled with its
full canonical body, without `BO1_STARTUP_SERVICES`. All four also compile under
MSVC Windows x86 and x64 comparison configurations in the Android worktree.
These are 12 successful compile checks, not a linked Windows gameplay run.

Byte-level verification reverses only the scoped adapter additions and recovers
each pre-edit SHA256. CP1252 source bytes are preserved; inserted lines use CRLF,
and existing bare LF counts remain unchanged. All four source hashes remain
stable during compilation and audit. No fixture involving fabricated assets was
added. Actual SDK438 startup is the integration authority.

Evidence is in `build-android-audit/native_render_asset_callbacks1/`:
`source-manifest.json`, `compile-results.json`, `audit-results.json`, per-TU
compile/preprocessing commands and logs, native object and preprocessed files.
The source editor and audit scripts record exact reproducible changes/checks.

## Frozen sources

| Source | SHA256 |
| --- | --- |
| `src/cgame/cg_visionsets.cpp` | `BA4109364A0F8A7DF912CA5B9813F3C10558C8123A2F3E09C710DD6BAE8D788D` |
| `src/gfx_d3d/r_image.cpp` | `DAF87DBFBAA0B066DF67BC79F3D89E4773C8AFA95B0F41F6521520D761E04C81` |
| `src/universal/com_memory.cpp` | `14EA1556120450FA30E07E0117050F73156A3AC1C53FBCDCA1CE22125BE99D38` |
| `src/ui/ui_viewer.cpp` | `5922BA66FA9FBFC0E12B3C60841A37B7CAAC54C4E67479180DABA209F9CA8D3A` |

This agent has released these four source files. No commit, push, reset, clean,
asset modification, transfer, device operation or ledger mutation was performed.
