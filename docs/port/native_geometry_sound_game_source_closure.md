# Geometry and sound-game source integration

2026-10-13. Full Com_Init remains incomplete.

Selected original bg_slidemove, phys_effects, cg_sound, glass_renderer and
snd_local sources. Canonical geometry virtual methods/vtable and physics-effect
Dvar storage now come from their actual owners, alongside client sound entity/
pose bridging, glass renderer and sound-game reset/control code.

The full build excludes inherited unavailable CG_SndGameReset because the real
function is now selected. OFF diagnostic remains unchanged. No independent
implementation, fake geometry, sound reset or initialized state added. Actual
Dvar registration and resource initialization still require their real callers.

## Evidence and next work

All five sources compile ARM64. Whole Windows x86/x64 checks pass for four in
takeover_geometry_sound_glass_windows1 and snd_local in takeover_sound_game_windows1.
full-engine-link100 preserves duplicate CG_SndGameReset;101/102 compile and reach
remaining actual dependencies. Latest102 still lacks native device capability,
cinematic, reliable-command/auto-simulation reset, game/client/script state,
ragdoll/destructible, input/network and recovery owners.

No new tablet runtime gate or transfer. Geometry collision correctness, physical
effect setup, glass rendering, sound-game reset and whole initialization remain
unverified. Prior isolated allocator/event/IO probes retain their original scope.
DXVK/Vulkan and staged retail assets unchanged.

Continue actual source/platform closure from102; preserved original implementations
are the reference. Do not manufacture ready state or weaken invariants to link.
