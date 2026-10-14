# Original LargeLocal arena integration

2026-10-13. Full-engine-link144 compiles and resolves original LargeLocal
constructor/GetBuf/destructor. Remaining engine services still prevent linking;
actual Com_Init/Com_Frame remain incomplete.

Selected full original mem_largelocal source; no separate allocator replacement.
The original512KiB two-ended arena, main/server role assertions, class fields,
rollback/reset interface and Windows behavior remain. Android aligns static
storage and rounds requests to max_align_t so native pointer/long-double objects
remain aligned. This runtime arena policy does not change serialized/VM slots.

Native mutex covers cursor-pair reads/updates in Begin/BeginRight/End/EndRight,
GetBuf and Reset. Bounds/size validation occurs before advancing native cursors;
invalid requests/overlap fail explicitly. Original right-side strict gap and
main-side inclusive boundary are preserved. Assertions were not weakened.
Reset still requires callers to quiesce owners; the mutex alone cannot retire
handed-out buffers. Actual recovery/shutdown/thread lifecycle remains unverified.

The existing script-pool bridge already supplies real Sys_IsServerThread from
immutable registered pthread identity. No false-returning fixture or new thread
role query was added. Native thread-data service's full server-role startup is
still absent; that broader owner is not supplied by this allocator integration.

HONOR UBSan actual production-source probe passes: real Sys_InitMainThread/TLS,
nested odd-sized aligned main allocations, preserved parent bytes, destructor
rollback, full512KiB boundary/reclaim and zero-size reference behavior. Expanded
run2 creates an actual OS thread, registers it through existing server-identity
API, checks right/nested alignment/nonoverlap, holds server buffer during64 main
allocation cycles, verifies contents and both-side reclamation. No server queries
occur after join, respecting existing registration's process-lifetime constraint.
This validates coexisting owners; it is not exhaustive simultaneous cursor racing,
TSan, invalid-size assertion injection or full server frame execution.

Build1 omitted the real script server-query provider and failed linking. Build2
adds existing script_pool_bridge/script_string_services and passes main run1;
build3 expanded owner test passes run2. Existing fail-on-use assertion/error
endpoints would fail the fixture; no successful substitute service is supplied.
Only changed test binary was transferred; staged assets were reused.

Normal whole Windows mem_largelocal TU passes x86/x64 in
takeover_large_local_windows1. Final native range-test ordering leaves tested
Windows branch unchanged. Logs143/144 record full integration link failures.

Next genuine global hunk reserve/commit/temp/recovery ownership must be integrated
without duplicating existing native Z/zone services or using PMem's limited
VirtualAlloc bridge as generic reserve. Screen/event/input, native audio/cinematic
and remaining script/client/UI/game/renderer/network owners also remain. Preserve
the full startup objective; this arena probe is not its completion proof.
