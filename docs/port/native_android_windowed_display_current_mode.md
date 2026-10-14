# Android windowed current display selection

2026-10-14. Scope: only `src/gfx_d3d/r_init.cpp::R_EnumDisplayModes`. Root owns full linking, packaging and actual device runs. Protected BlackOps64, DXVK Native 1.9.2 and SDL sources/binaries were inspected read-only and preserved.

## Actual failure and source contract

Root's actual SDK431 execution completed genuine SDLActivity adoption and entered shared SDL_main/canonical Com_Init. `sdk-native-entry431-launch1.log` records actual Adreno610/Vulkan1.1.128 discovery and shader3.0 capability, then R_EnumDisplayModes fatal `No valid resolutions of 800 x 600 or above found`. `sdk-native-launch-crash1.txt` resolves the fatal stack to matching APK shared engine BuildId586258f5196c0d0007a9a05a02554a8e4ef94128. These observations prove that discovery/capability code ran, not successful window/device/render initialization. The failed log has no mode counts or individual mode observations.

Read-only pinned provider sources establish a concrete possible mismatch:

- SDL `third_party/SDL/src/video/android/SDL_androidvideo.c::Android_VideoInit` publishes the real Android device dimensions/rate with `Android_ScreenFormat`. That format initially defaults to RGB565; Android_SetFormat is called only from SDL_egl.c. Android_CreateWindow skips EGL surface creation for a Vulkan window, so Vulkan creation itself does not update format through that path. Android_SendResize updates current/display dimensions/rate. Source behavior permits a 16-bit SDL display throughout this Vulkan-only startup; the exact431 runtime format remains unobserved. See native_working_dxvk_spike_comparison.md for the working spike audit and exact provider identity.
- DXVK `src/wsi/sdl2/wsi_mode_sdl2.cpp::convertMode` derives WSI bitsPerPixel from SDL format. `src/d3d9/d3d9_adapter.cpp::CacheModes` filters enumerated modes by exact requested monitor bpp. Thus X8R8G8B8 enumeration can legitimately be empty while SDL has a real display. DXVK `GetAdapterDisplayModeEx` independently reads the real SDL current width/height/refresh and returns its actual D3D current-display record with X8R8G8B8 format.
- Working BlackOps64 requires valid enumerated ≥800x600 resolutions and retains its Windows zero-refresh→60 convention. Android windowed operation does not require proof that the current display is an available exclusive 32-bit mode. Exclusive operation does require actual enumeration and exact SDL mode application.

## Bounded change

Android enumerates as before and additionally queries genuine `GetAdapterDisplayMode`. Diagnostics record D3D adapter/mode count/current HRESULT/dimensions/rate/format/fullscreen request; actual selected SDL driver/display, current and desktop query results/dimensions/rates/formats/bpp and mode count; first eight SDL mode records and first eight successful D3D records. SDL observations execute only through existing real SDL owner dispatch. Diagnostics do not mutate provider state or initialize a replacement provider.

If no valid enumerated resolution exists, the Android path may populate windowed resolution/refresh Dvar tables from the genuine current D3D mode only when `r_fullscreen` is false, its query succeeded, width≥800 and height≥600 fit signed engine fields, observed refresh is positive and bounded, and D3D format is X8R8G8B8. Invalid, missing or undersized current mode continues to the canonical fatal path. No fixed dimensions, guessed refresh, fabricated enumeration or GPU capability is supplied.

The actual `dx.displayModes` exclusive table and `dx.displayModeCount` are unchanged by this fallback. Consequently later fullscreen refresh selection still requires an actual enumerated matching mode and fails if it is absent. SDL exact-exclusive-mode application guards are unchanged. Android stops replacing a provider's zero enumerated refresh with guessed60; zero is preserved. The Windows original substitution remains under its original non-Android selection. No engine assertions or ready-publication paths changed.

Checked canonical defaults are `r_fullscreen=0` and `r_borderless=0` (`r_dvars.cpp`); archived settings may change actual values, which the diagnostic records. Native `R_BorderlessSetWndParms` uses real SDL display bounds/current refresh, and renderer-window desktop-fullscreen application validates actual flags/drawable. Root may explicitly select `r_fullscreen=0` plus `r_borderless=1` for Android composited full-display presentation without claiming exclusive-mode support; dedicated CLI configuration stays unchanged.

## Final verification and limits

Current production whole Android r_init TU compile passes with unchanged flags (including actual `-UBO1_NATIVE_RENDER_GLOBALS`) and an isolated output. Normal Windows x86 and x64 whole r_init TU compiles each pass once with established `/permissive /DWIN32=1` options. Source hash stayed fixed during all three compiles. `build-android-audit/native_renderer_current_mode1` preserves exact commands/logs/results, captured prechange bytes and source manifest.

Only the diagnostic/current query insertion, Android refresh preservation branch, and windowed current-mode fallback were changed. Reversing those replacements restores exact prechange source bytes. New lines use ASCII-compatible CRLF; all original CP1252 bytes and Windows bodies remain intact. Existing prior work is preserved.

Before SHA256: `2E43463328B9F867BB519F57CA5BBA5B23C9CDB5926FFC02B0ED716DF707FC9E`

Final SHA256: `8EEECBE6024F4FE39DA5C2D8BADB55421F469D8F5FA0F0953CEC6BFE70082A91`

No full CMake build, device command, broad fixture suite, asset transfer, ledger mutation or readiness simulation was performed by this owner. Root's next real APK execution must establish the actual enumerated/current mode values, whether this fallback runs, successful real window/device/resource initialization, rendering and teardown. The source hypothesis about431's format is deliberately distinct from those pending provider observations.
