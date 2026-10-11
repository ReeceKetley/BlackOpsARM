# Windows x64 Vulkan bring-up

## Locally built DXVK lifecycle fix

The official 3.1.1 package reproduces one leaked `VkPipeline` at `vkDestroyDevice`
on both Pentagon and Moon, even with implicit layers, screenshot capture and the
DXVK HUD disabled. `DxvkMetaCopyObjects` creates input-attachment image-copy
pipelines but does not destroy that cache in its destructor. The tracked
`dxvk-patches/free-input-copy-pipelines.patch` adds the missing destroy loop.

`tools/port/build_dxvk.ps1` checks out exact upstream release commit
`b1a1c99ab52b687cf950d62c88bc2fa316b41663` and its pinned submodules, applies
only the recorded patch, and builds D3D9 with MSVC x64. It requires MSVC and
Vulkan SDK/glslang available on this host.
The fix must be reassessed against the selected Android DXVK revision. Meson 1.12.1 is isolated in an ignored
Python environment; the compiler and actual Ninja are recorded. No Android-only
conditional is introduced: the lifecycle fix can also apply to DXVK Native.

The build writes `build/deps/dxvk-win64-patched/package.json` with commit,
submodules, patch/DLL hashes, compiler versions and build time. Pass that manifest
explicitly to the launcher; the default manifest still selects the official
package for comparison:

```powershell
.\tools\port\build_dxvk.ps1
.\tools\port\dxvk.ps1 -Name dxvk_patched_fresh -PackageFile build\deps\dxvk-win64-patched\package.json -Validation -Wait
```

Initial patched Pentagon grenade test `x64hi_dxvk44_patched_frag` completes
3 PASS / 0 FAIL / exit 0 and no Vulkan validation errors, including teardown. Patched Moon `x64hi_dxvk44_patched_moon` also completes5/5, exit 0 with no
Vulkan validation errors through teardown. Only the expected debug-utils warning
appears. Final `x64hi_dxvk44_patched_route` completes 26 PASS / 0 FAIL
(including recovery), receives `ak74u_upgraded_zm` through the real machine after weapon recovery,
continues 30 seconds after completion and exits 0. All three patched runs pass
all four SaveGame checks and have no Vulkan errors through shutdown.
The aggregate `build/run/native_brush42_dxvk44_results.json` retains earlier
failed launch and validation attempts. No game remains running. This is a locally modified
upstream build, not an unmodified official binary.

## Reproducible setup

Batch46 restores default client glass on native x64. Native arena/header sizes,
client outline allocation and a silent Win32 stride in glass texture streaming
are repaired. The final default-enabled Vulkan route adds a real pane-shattering
step and completes25PASS/0FAIL/exit0 through actual upgraded shotgun and clean
Vulkan teardown. An additional unrestricted guard build passes the glass
client check3/0 and its original headless route24/0. See
[native glass evidence](glass_native.md). The older low-address guard real
window runs out of texture address space before glass; that failed attempt is
retained and does not replace the unrestricted guard result.

`tools/port/dxvk.ps1` launches a fresh snapshot of the unrestricted native x64
build (`build-x64-hi`, `BO1_X64_LOW_VA=OFF`) with the existing D3D9 renderer
translated to Vulkan. No x86 executable, Wine or CPU emulation is involved.

The official DXVK 3.1.1 Windows package is pinned in
`tools/port/dxvk-package.json`. Its SHA256 is checked before extraction; the
launcher verifies both EXE and D3D9 DLL are x64 PE files. Only `d3d9.dll` is
installed beside this disposable run's executable. System libraries and the
normal game installation are unchanged.

Each run preserves `launch.json` (binary hashes, source HEAD/dirty flag, arguments
and environment), EXE/PDB, `modules.json` / `all-modules.json`, DXVK and engine logs, shader cache,
PID and, with `-Wait`, the actual exit code. Environment changes are scoped to
the launching process and restored after the child starts. Empty `dxvk.conf`
uses upstream defaults. The disposable profile seeds the hardware state before
the engine's recommended-settings check; command-line variables alone are
overwritten by startup config and do not prevent its dialog.

From the repository in PowerShell:

```powershell
# Asus window, sound ON, automated god mode, two frags and 20 seconds after completion.
.\tools\port\dxvk.ps1 -Name dxvk_frag_fresh -Wait

# Original Pentagon route through the Pack-a-Punch machine.
.\tools\port\dxvk.ps1 -Name dxvk_pap_fresh -Plan 'qr wall_rottweil fire3 ammo_rottweil door_conf debris_elev2 floor2 debris_stair debris_elev1 battery floor3 power jugg speedcola doubletap pap' -AutoquitMs 900000 -Wait

# Matched system-D3D9 control (same EXE and launch settings, no local D3D9 DLL).
.\tools\port\dxvk.ps1 -Name d3d9_frag_control -NativeD3D9 -Wait

# Manual play uses ordinary mouse input, no bot and no god mode.
.\tools\port\dxvk.ps1 -Name dxvk_manual_fresh -PackageFile build\deps\dxvk-win64-patched\package.json -ManualPlay -AutoquitMs 1800000 -Wait
```

Fresh names are required. Automated tests use god mode and an explicit test
score. Sound is enabled by omitting `nosnd`, not by passing `nosnd 0`.
The default window is 1024x576 at (60,60) on the authorized Asus screen.

## Presented-frame capture and validation

On this host, Vulkan SDK 1.4.357.0 supplies the registered
`VK_LAYER_LUNARG_screenshot` and `VK_LAYER_KHRONOS_validation` layers.
`-CaptureFrames '1000,3000,6000'` enables the screenshot layer only for the child
and saves actual presented Vulkan images to `frames/*.ppm`. Frame numbers are
presentation counts, not milliseconds. Convert PPM to PNG with Pillow to view
in Codex. These captures do not copy the monitor; BO1's normal front-buffer
screenshot path previously captured the desktop and is unsuitable evidence.
Capture requires the layer installed on the host. `-Validation` enables the
Khronos layer and DXVK's validation callback; inspect the DXVK log for `err:`,
`error:` and `warn:` messages. The validation debug-utils performance warning is
expected, but Vulkan VUID errors must be investigated. With `-Validation -Wait`, the launcher
records `validation-errors.txt` and throws on `err:`/`error:` messages even when
the engine exits0.

By default the launcher disables implicit Vulkan layers for this child using
`VK_LOADER_LAYERS_DISABLE=~implicit~`. This isolates tests from system-wide
capture/overlay hooks while preserving explicitly requested screenshot and
validation layers. No registry or other app settings are changed.
`-AllowImplicitLayers` restores the host's usual implicit-layer behavior for
comparison. This filter requires a Vulkan loader with headers1.3.234 or newer.

`-Weapons 'name,name' -Map zombie_moon` reuses the existing runtime weapon
sweep with a rendered client and sound. It does not imply all weapons or maps
are tested on Vulkan.

## Verified evidence (2026-10-11)

- `x64hi_dxvk43_smoke3`: official DXVK 3.1.1, NVIDIA RTX 3070 / driver596.36;
  actual Vulkan swapchain1024x576. Loaded modules confirm run-local DXVK D3D9,
  system Vulkan loader, NVIDIA ICD and screenshot layer. Captured images show
  Round1 pistol/HUD/map geometry and the Quick Revive machine.
- Three planned steps pass, including two actual `grenade_fire` events. The game
  continues20seconds after completion and exits0. Audio initializes on Realtek;
  this run has no new user confirmation of audibility.
- `x64hi_dxvk43_smoke` and `smoke2` were deliberately stopped at the optimal
  settings dialog before renderer initialization. They are failed launch
  attempts, not Vulkan runtime failures or accepted tests.
- `x64hi_dxvk43_route`:24 PASS / 0 FAIL / complete / exit 0, real Pack-a-Punch upgrades
  the shotgun to `rottweil72_upgraded_zm`. All four SaveGame checks pass. Sound
  ON/god mode,30seconds after completion, no DXVK warning/error messages.
  Presented frames show shotgun/lower-floor lighting and the Pack-a-Punch machine.
- `x64hi_dxvk43_moon_validation`:5/5 weapon cases pass and exit 0, but the run is
  **not a clean Vulkan validation gate**. Overwolf's layer lacks an entry point;
  null-renderPass framebuffer errors occur with OBS/Overwolf/Twitch implicit
  layers loaded. An otherwise matching repeat disables those implicit layers.
  Isolated repeat5/5, exit 0 removes those framebuffer/loader errors but reports
  a distinct one-pipeline teardown leak. Validation-only Moon and HUD-off
  Pentagon reproduce that leak, so screenshot capture/HUD are not its cause.
- Moon's missing `p_zom_moon_cassimir_plate` precache script error also appears
  in prior Win32 and native D3D9 runs. Weapon passes do not validate the affected
  sidequest or every map feature. Captured FPS values during compilation or
  validation/concurrent regressions are not a benchmark.
- Moon's checker texture on part of the weapon HUD also appears in the native
  system-D3D9 control and its true back-buffer screenshots. This is an existing
  engine/content issue, not established as a Vulkan regression. The ordinary
  native sound-on control passes the same 5 weapon cases/exit 0. **Batch45 resolves
  the checker square:** Win32 also reproduced it, and `CG_Init` assigned HUD
  materials to the wrong `cgMedia` members. Corrected Win32 and unrestricted
  x64 Vulkan each pass5/5/exit0 with clean captured HUDs. Vulkan validation is
  clean through teardown. See [HUD material registration](hud_materials.md).
  Windows.Graphics.Capture
  failed twice; native image evidence instead uses the existing private-desktop
  headless-client COPY swapchain/back-buffer capture (1920x1080, AA1), so it is
  not a pixel-for-pixel comparison of the windowed AA4 Vulkan run.

## Boundary for the later Android port

Windows success checks BO1's D3D9 calls, shaders and resources against DXVK. The
engine still has Windows windowing, XAudio2, D3DX and vendor stubs. A Windows DLL
does not provide a native Android implementation.

Keep gameplay and the D3D9 renderer as the compatibility baseline. Later native
ARM64 work must move D3D9 creation/device and window/surface ownership behind a
platform boundary, use DXVK Native with a native WSI backend, and port the
remaining Windows/D3DX/audio dependencies. Upstream DXVK Native supports SDL3,
SDL2 and GLFW backends; native WSI selection is separate from this Windows DLL
experiment. Do not add Android-specific workarounds to gameplay or pretend a
desktop capability set is available on mobile.

User targets: Samsung S25 EU and Honor X8a with Mali, deferred while Windows
Vulkan is the active focus. Native Android DXVK version/features and any mobile
patches must be selected from actual device capability data. RTX3070 validation
does not establish compatibility or performance for either phone.

Primary references:

- [DXVK 3.1.1 release](https://github.com/doitsujin/dxvk/releases/tag/v3.1.1)
- [Upstream DXVK usage and Native WSI](https://github.com/doitsujin/dxvk)
- [LunarG screenshot layer](https://vulkan.lunarg.com/doc/sdk/latest/windows/screenshot_layer.html)
- [Khronos loader layer filtering](https://github.com/KhronosGroup/Vulkan-Loader/blob/main/docs/LoaderLayerInterface.md#layer-filtering)


## Latest native renderer and planner regressions — batches47/48

Native water surface/COM fields and vertex/mesh pointers are full width; the
worker command now copies its native152bytes and all16mesh pointers. Packed
swizzle indices initialize integer lanes. Focused production tests pass on
Win32 and above4GB on x64. Full high rebuild206s/0errors/144warning sites,
down175. Full water simulation/vertex math is not certified by these tests;
see [native water audit](water_native.md) and WARN0009.

Vulkan47 reaches actual PaP and exits with clean validation but has a portal
recovery failure after a thief/Bonfire reset; it remains a failed route. Batch48
leaves deferred portals unarmed until their switch prerequisites finish, so the
readiness check runs again. Current-production fixture tests reproduce the
reset and pass on Win32/x64; the pinned old code fails.

Final original Win32/high/low/guard routes26/26/25/25PASS, all0FAIL/exit0/all four
SaveGame checks. Final `x64hi_dxvk48_pane2_route` explicitly selects glass pane2,
shatters it, completes32PASS/0FAIL/exit0 through actual upgraded M14, continues
30seconds after completion and has zero Vulkan validation errors through
teardown. SoundON/god automation, real presented-frame captures and confirmed
Vulkan modules. Earlier pane0 glass observation fails its20second budget and is
intentionally stopped; that is retained as failed, not a crash or a pass. See
[portal recovery and evidence](pack_rearm.md), `water47_results.json` and
`pack48_results.json` under the ignored build/run folder. All test games have
ended. Source follow-ups remain uncommitted; Android remains deferred.
