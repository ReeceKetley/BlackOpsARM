# Native glass renderer (batch46)

## Repairs

- `Allocator::Init` aligns full-width arena addresses. The tail leaves room for
  the actual native `Memory` header, not a20-byte Win32 header.
- Shard payloads start at `offsetof(Memory, prevFree)`:16bytes on Win32,
  32bytes on x64. The last free-list link doubles as payload while allocated.
  Header recovery, free-list access, split thresholds and block accounting now
  use typed members and native sizes. `GetMemorySize` retains its original
  contract of returning the full block span including the prefix.
- Zero-byte requests reserve enough space for the full header. Negative and
  overflowing request sizes return null.
- Client pane storage and pre-shattered outline records use native `sizeof`.
  Clearing the remaining nine split-result pointers uses their actual width.
- Renderer destruction destroys the standard-library lists before freeing the
  shared small-node pool.
- Glass texture streaming reads named `scene.glassBrush` members. The previous
  reads indexed backward from the visibility array with Win32 offsets and a
  40-byte stride; enabling glass on unrestricted x64 crashed there.
- The x64 opt-in guard is removed. Client glass uses the regular allocation
  path on both architectures. The fallback snapshot reader remains for an
  absent client renderer, including zero-glass maps.

## Focused allocator verification

Run `tools/port/check_glass_allocator.ps1 -NegativeControl`. The generator
copies the current production allocator header and function bodies into the
standalone test; it does not replace the allocation algorithm with a model.

Win32 and native x64 each pass6000 allocation/free operations inside a guarded
arena, retaining payloads and checking non-overlap, both linked lists, address
alignment, accounting, coalescing, exhaustion/recovery and reset. Real
`std::list` nodes and the production fixed-object allocator are constructed,
exhausted, reused and destroyed. Native arena address is `0x180010000`.
The original committed x64 implementation crashes with access violation
`0xC0000005` at that address as the negative control. No crash dialog is shown.

## Runtime evidence so far

- `x64hi_dxvk46_glass_smoke`: glass initializes but the unfixed streaming reads
  crash in `R_StreamUpdateForBModel`. Failed attempt retained; launcher also
  hit a process-exit/Kill race. The launcher now handles that race.
- `x64hi_dxvk46_glass_stream`: repaired streaming, glass enabled via the old
  opt-in.46client panes initialize, grenade smoke3PASS/0FAIL/exit0 and no Vulkan
  validation errors through teardown.
- `x64hi_dxvk46_glass_break`: pistol-only test produces a cracked pane, but
  insufficient damage to shatter within its20second interval.1PASS/1FAIL,
  normal exit0, clean Vulkan validation. This is not accepted as a glass gate.
- `x64hi_dxvk46_glass_shotgun`:3PASS/0FAIL/exit0. Pane2 moves from pristine to
  shattered on server and client; five actual shotgun shots. Vulkan captures
  show the broken pane and live shard statistics. Zero validation errors
  through teardown, soundON/god automation,20seconds after completion.
- `w32_glass46_client`: final source without the opt-in guard, real Win32
  private-desktop client,3PASS/0FAIL/exit0 after79seconds. Pane2 shatters on both
  sides; genuine back-buffer captures show broken glass. No WER/Application
  Error event. Muted audio/god automation.
- `x64guard_glass46_client`: the existing restricted-address guard executable
  cannot allocate a map texture before client glass initializes. System D3D9
  returns out-of-memory; the fatal cleanup also asserts on screen-update state.
  This failed attempt is retained. It does not verify the glass renderer.

The separate `build-x64-hi-guard` directory is configured with
`BO1_PORT_X64=ON`, `BO1_X64_LOW_VA=OFF` and `BO1_X64_GUARDHEAP=ON` so real
renderer testing can use unrestricted address space together with guard pages.
The existing low-address guard directory is preserved for its headless gate.

## Current final-source gates

The four original headless routes complete through actual Pack-a-Punch:
Win32/high/low/guard26/26/26/31PASS, zero failures, normal exit0. Counts include
automatic recovery steps. All four SaveGame checks pass in each run. These
headless routes do not instantiate the client renderer; the real-client tests
above provide separate rendering evidence.

The unrestricted full rebuild completes231seconds/zero errors,175unique
truncation sites (173src/tl plus2DemonWare), down from178. No glass truncation
sites remain in that warning category; silent casts elsewhere still need audit.
The new unrestricted guard build compiles241seconds/zero errors and175sites.

`x64hi_dxvk46_final_route` uses the full rebuilt unrestricted executable with
glass enabled by default (`BO1_X64_GLASS` is absent), soundON/god automation.
The original route plus an added real glass step completes25PASS/0FAIL/exit0,
including recovery. It shatters pane2, receives `rottweil72_upgraded_zm`
through the real Pack-a-Punch machine, continues30seconds after completion
and exits with zero Vulkan validation errors through teardown. All four
SaveGame checks pass. Presented frame captures show the active glass renderer.

The unrestricted guard configuration also passes:

- `x64higuard_glass46_gate`:24PASS/0FAIL/complete/exit0 through the original
  headless route and actual upgraded shotgun, all four SaveGame checks.
- `x64higuard_dxvk46_client`:3PASS/0FAIL/complete/exit0, default-enabled glass,
  soundON/god automation. Pane2 shatters on both server and client. Genuine
  presented frames show the broken pane and49visible shards; Vulkan validation
  has zero errors through teardown. All four SaveGame checks pass.

All13runs, including failed attempts, are recorded in
`build/run/glass46_results.json`. The first crash has no recovered process exit
code because of the launcher's old race; it is recorded as failed, not clean.
All test processes have ended. These focused tests do not prove every glass
pane or every map's collision and physics behavior. Script debugger and the
wider native cold-path audit remain open; LOW_VA still defaultsON in CMake.
