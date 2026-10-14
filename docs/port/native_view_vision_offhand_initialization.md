# Original view, vision, offhand, weapon options and turret initialization

2026-10-13. Full-engine-link205 compiles selected sources and fails remaining
genuine dependencies. Actual Com_Init/Com_Frame unexecuted.

Selected whole cg_spikeacoustic, offhandweapons, cg_weapon_options, cg_visionsets,
turret_placement and cg_view_mp. Actual client Dvar registration, gameplay/view,
offhand/spike, weapon-option, vision and placement logic now retained. No dummy
registration, initialized state or fabricated rendered/gameplay output supplied.

Native adaptations: vision underscore formatting uses existing native CRT;
optional view sway file uses actual native stdio under Sys_EngineUserRoot;
x86 prefetch replaced by actual compiler prefetch on the same address/locality.
Windows original formatter/file/intrinsic branches remain unchanged.

Canonical shared PITCH/YAW/ROLL indexes moved outside unrelated WIN32 definitions;
native BigShort/BigLong use original ShortSwap/LongSwap routines. No global WIN32
or BO1_PORT_X64 enabled, and no layout assertions or fixed32 metadata changed.
Existing flag audit consulted; this is neutral math/endian exposure, not a Windows
hardware/platform identity substitution. Native little-endian ARM64 convention
matches existing original disk/network usage.

Evidence:

-202 preserves initial missing formatter/stdio/angle symbols.203 corrects those
  and exposes prefetch;204 adapts prefetch and reaches real linker dependencies.
-205 compiles final native user-root sway path and reaches genuine link failures.
- takeover_view_vision_offhand_windows1 passes seven whole normal sources
  (six selected plus q_shared) on x86/x64 exit0. Source manifests/commands/results
  retained. Final native-only logging path leaves Windows branches unchanged.

No new runtime probe or transfer. Actual registrations, sway logging, view/vision/
offhand/weapon/turret state and native pointer/stride paths remain unexecuted.
Compilation does not certify initialized game/graphics state or whole engine ABI.
Continue real remaining allocator/contact-manifold/glass, server/game/client,
audio/cinematic, renderer/input/network/recovery owners from205, then run authentic
startup once the entry links. Prior scoped proofs retain only their recorded scope.
