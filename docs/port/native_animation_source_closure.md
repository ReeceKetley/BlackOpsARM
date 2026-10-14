# Native animation and effect source integration

2026-10-13. Full `Com_Init` remains incomplete; this is source/link integration,
not a successful runtime startup or effect-system execution claim.

## Changes

- Full database build selects original `xanim`, `xmodel`, model/DObj utilities,
  `r_rendercmds`, angle-vector math, and effect update/system/archive/sort/random/
  wind/update-util/dvar/unique-handle translation units. XAnim has one owner;
  its former unavailable endpoint is excluded only from the full build.
- Former unavailable remote-screen begin/end endpoints are excluded from the
  full build now that actual renderer bodies are selected. Their original
  nesting, notification and wait behavior remains intact.
- Original `Sys_FrontEndSleep` and `Sys_NotifyRenderer` bodies are moved outside
  the CPU-query selection guard and use the existing backend event owner.
- Android model prefetch uses the native compiler primitive. Render-warning
  counters retain Windows LONG's four-byte width on Android LP64, with a
  canonical declaration/definition and a width assertion. Windows retains long.
- Android effect initialization/shutdown clears the actual `FxSystem` size
  rather than the Win32 0x360 literal. Effect handle validation uses the native
  container stride with compile-time divisibility and uint16 capacity checks;
  the original offset/4 handle encoding remains unchanged. Windows branches
  retain their existing behavior. These two effect changes await runtime checks.

## Evidence and remaining work

`build-android-audit/native_registry_closure/full-engine-link36.log` preserves
the duplicate remote-screen diagnostics. Link37 exposes the real dependency
graph after their removal. Link38 adds the canonical model/math utilities;
link39 adds original effect sources. Latest link40 compiles those sources and
fails to link on actual renderer, client/event/network, sound/cinematic,
physics, effect-draw/marks and recovery dependencies. No new placeholders were
added to satisfy them.

Normal whole-source Windows x86 and x64 compilation passes for nine affected
animation/renderer/profile/thread TUs in
`build-android-audit/takeover_animation_services_windows1/`, and eight effect
TUs in `build-android-audit/takeover_effect_services_windows1/`.
Both directories contain source manifests, commands, logs and build results.

No tablet execution or asset transfer occurred in this source-integration step.
Pending filesystem-startup, enum/localization and render-service runtime gates
still require dependency closure. Preserve their earlier evidence and avoid
claiming that compilation proves resource loading or frame execution.

Next: continue from link40, especially original FX draw/archive helpers, sound/
cinematic and client state dependencies; audit hard-coded object sizes and
fixed-width counters as their owners are selected. `g_scsFrameStamp` and its
shader-constant owner still need their native counter-width integration.
