# Native runtime render-command alignment

2026-10-14. Production source scope: `src/gfx_d3d/r_rendercmds.cpp` only. Shared render-command headers and rb_backend remain unchanged; protected Windows/DXVK/SDL sources are read-only. Durable focused fixture: `tools/port/render_command_stream_fixture.cpp`. Root owns full SDK builds/device execution.

## Actual fault and coherent stream contract

Actual SDK434 loaded ui_mp then UBSan stopped `AddBaseDrawTextCmd` at r_rendercmds.cpp1217: `GfxCmdDrawText2D` needs8-byte alignment but its actual command address0x7074a74024 is4mod8. Root's matching stage6 engine BuildId is d934146ffe257d28dd82c1334f0a296489155ea0; actual log is `native_registry_closure/sdk-native-entry434-launch1-only.log`.

`R_GetCommandBuffer` is the only stream writer. It previously advanced usedTotal/usedCritical and published header.byteCount using requested4-byte spans. Scalar clear records and variable text spans can be4mod8; every4-byte end marker also makes the next sublist4mod8. Base storage itself comes from `R_AllocGlobalVariable -> Z_VirtualAlloc`, an actual page-aligned native mmap allocation. `R_ClearCmdList` resets usedTotal0; client/shared/composite list starts capture the current cursor.

All27 backend command consumers and the skipped/UI3D walker advance by the header's unsigned-short byteCount. They do not independently round/calculate the next record. No command batching or lastCmd growth exists: lastCmd is only reset, assigned or cleared inside r_rendercmds.cpp. Pointer-bearing native command records require natural8 alignment. The shared four-byte header and native runtime pointer widths are retained; this runtime stream is not a disk/FastFile/wire format.

## Repair

Under `__ANDROID__ && BO1_NATIVE_64BIT`, the actual allocator rounds every accepted record span to8 before existing buffer-size/ushort assertions, warning thresholds, critical reserve calculations, counters and header publication. The end marker requested4 therefore consumes/publishes8, making subsequent sublists aligned without inter-record gaps. Native requests below actual header size or above maximum aligned16-bit span65528 fail explicitly before mutation. Original4-byte requested-size assertions remain. Base/cursor alignment and nonnegative/in-bounds usedTotal/usedCritical accounting are checked; no readiness or assertion bypass is introduced.

Native text producers now retain strlen as size_t and size the actual payload from `offsetof(GfxCmdDrawText2D,text)+characters+1`, at least sizeof the typed command. This owns the entire NUL-terminated tail, checks the16-bit bound before addition and passes a4-byte requested span to the allocator, which owns the final8-byte stride. Both regular and console text use this exact native helper. Text content, style/color/font/effect fields and real ring-buffer copying remain canonical. Fixed-size/pointer command payloads remain unchanged. Static assertions verify that all16 actual producer record types fit the stream's8-byte alignment and header sizeof4.

No consumer/header edit is required because original walkers consume the now-correct byteCount. No struct packing, asset edit, guessed alignment, copy-to-hide-misalignment or widened header is used. Windows branches preserve the original allocator/text sizing and records. Removing native additions reconstructs exact captured prechange bytes; inserted lines are ASCII-compatible CRLF and original CP1252 bytes are retained.

## Final source verification

Current production whole native TU compile passes with UBSan/no recovery and instrumentation retained. Normal Windows x86 and x64 whole TU compiles each pass once using the established `/permissive /DWIN32=1` contract. Source hash remains fixed through all checks. Root's integrated435 build includes the frozen source.

The CPU fixture links that actual native allocator/producer object plus actual UI3D stack/count, r_utils color packing and fx_convert objects. One aligned byte buffer exercises scalar4/EOL records, all text-length residues0..23, pointer-bearing successors, real text and console ring-copy/NUL ownership, captured sublist starts and original header-byteCount walking. It checks65528 maximum stride, exact capacity/end marker, critical reserve rejection/cursor preservation and canaries. Four child cases exercise explicit oversize/base/cursor/text-bound rejection through a fixture terminal diagnostic (exit77); this is not a claim of canonical production Sys_Error termination. The isolated fixture constructs a valid producer frame only and has no graphics provider, engine startup or readiness claim.

Root's final actual HONOR fixture3 runs under UBSan/no recovery, returns0 in under1second and prints PASS785; `honor-fixture3.log` retains positive checks plus the four expected explicit negative guard diagnostics. The children exit77 through the fixture terminal diagnostic. This proves actual CPU producer/allocator/walker contracts, not canonical Com_Error/Sys_Error termination or graphics execution.

Two fixture setup failures are retained, distinct from the production alignment failure. Fixture1 returned139 immediately because fixture tracing hook definitions inherited instrumentation and recursively called themselves (`honor-fixture1.log`/`honor-fixture1-crash.txt`). Only these two harness hook definitions gained no_instrument_function; production source/flags/instrumentation stayed unchanged. Fixture2 returned134 after stream tests because the isolated frontEndDataOut had no viewInfo binding when calling actual R_BeginClientCmdList2D (`honor-fixture2.log`). Fixture3 binds the existing actual g_viewInfo[0] array before that real call. No engine or provider change was needed for either harness repair. Original executables/source snapshots/commands/manifests preserve both failed setups; each repair rebuilt only the harness TU and relinked existing final production/provider objects.

Evidence directory `build-android-audit/native_render_command_alignment1` retains before bytes/manifest, production/native/Windows commands/logs/results, actual objects, fixture compile/link commands and executable. Final fixture3 ELF SHA256 `EE6EC2E9A705B9DD0EABB9435C67B6E4329A7EC3BA2480F33A46544B3CBCA2DD`.

Before production-source SHA256: `BA7B0319D8CDFBBB6092A14810D231581B4006EC49892E4C0D7B0BFF280FD3A0`

Final production-source SHA256: `1390CF3B79CFA7E582525A0A8CF14856B15B7820156332DB9C630AFD2515B8E4`

No engine assets, protected provider/source, config/runtime/header/walker or ledger state was changed by this owner. Actual SDK435 rendering, later backend consumers and lifecycle remain root's production verification work; a CPU stream fixture alone proves no GPU draw.
