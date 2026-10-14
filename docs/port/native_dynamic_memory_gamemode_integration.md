# Native dynamic memory and game-mode owners

2026-10-13. Genuine Com_Init/Com_Frame remains incomplete. No tablet probe,
asset copy, initialized subsystem or GPU execution occurred in this change.

Canonical common.cpp now selects its original Com_InitDynamicRender,
Com_InitDynamicMemorySystems and Com_ShutdownDynamicMemorySystems for native
boot. Bodies and ordering are unchanged: resource callback/flush, water and
extra-camera/UI3D initialization, glass/demo allocations, and actual release
owners remain responsible for their services. No synthetic readiness or
allocation success is provided. Original accepted nullsub comments remain.

FULL_DATABASE=ON now compiles the entire existing com_gamemodes.cpp rather than
only onlinegame. Original Dvar policies and ValidateGameModes assertions are
retained. OFF diagnostic builds retain the existing globals-only selection.

Including actual renderer declarations exposed a global LARGE_INTEGER collision
between the native timing API and DXVK's Windows declarations. common_boot_api.h
no longer exports that alias globally; its QPC declaration is fully qualified.
Native common.cpp uses the explicit bo1::win32 timer type and global forwarding
function; profilemapload keeps its required alias locally. Both forwarding and
native clock implementations already exist. Windows branches remain unchanged.

Evidence in build-android-audit:

- native_registry_closure/full-engine-link224.log and link225 retain the header
  collision and dependent alias failures; link226 compiles the corrected lifecycle.
- full-engine-link227.log compiles the full game-mode source and reaches genuine
  unresolved links (968 reported symbols; order/count is not a progress estimate).
- native_registry_closure/dynamic-memory-gamemode-definitions1.log verifies actual
  function definitions from the startup-services and entry-state archives.
- takeover_dynamic_memory_gamemode_windows1: all three whole normal TUs (common,
  profilemapload, gamemodes) pass x86/x64 exit0 after the final changes.

Real runtime requires prior Dvar, world, render-resource and memory initialization.
The full executable still lacks actual audio/cinematic/live/profile/game/client/
renderer/network/recovery owners. These libraries have not performed dynamic
initialization, shutdown or validation in a running engine. Preserve existing
focused runtime evidence at its scope; no need to repeat unchanged probes.
