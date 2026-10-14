# Original global hunk source and native allocation ownership

2026-10-13. Full-engine-link146 compiles and resolves original global hunk bodies,
including Com_InitHunkMemory. Full engine still fails linking; actual hunk init,
Com_Init and Com_Frame remain unexecuted.

Selected complete com_memory in the full database target and removed its former
startup-services partial selection. Canonical hunk state/allocation/temp/mark/
tracking/file-data/enumeration/string bodies now have one owner. FULL
Com_InitHunkMemory diagnostic is excluded only because original body is present.
OFF diagnostic/partial build remains as before. No replacement hunk algorithm,
new unsupported endpoint, readiness or weakened assertion was introduced.

Windows-only Z virtual-query/heap implementation bodies are excluded on Android
where actual existing native virtual_memory/zone_heap services own those APIs.
Original hunk calls real Z_VirtualReserve/Commit/Decommit/Free, not PMem's limited
VirtualAlloc bridge. Native page-floor uses actual OS page size; Windows retains
its4KiB policy. Other original page arithmetic/alignment and16KiB-device coverage
require runtime audit before broader portability claims. Typed native pointers
and original integer hunk usage state remain; disk/VM fields are unchanged.

Native zone owner now implements actual try-zeroed, try-garbage and fatal-garbage
variants with calloc/malloc, one metadata registry and type-correct release.
Try allocation returns null on OS or metadata allocation failure, cleaning backing
memory if registry insertion throws. Fatal variants keep explicit fatal OOM
behavior. Garbage memory is not falsely claimed zero. No Windows164-byte tracking
prefix is used as a second incompatible native heap owner.

HONOR UBSan zone-variants-build1/run1 passes actual production native owner:
1009/37/73-byte zeroed/garbage allocations, max alignment, zeroed byte contract,
full writable ranges, shared allocation/byte accounting1119 bytes, correct-type
cleanup and zero-size owned allocation/free. Forced OS/registry OOM and invalid
type paths were not injected. Only this test binary was transferred; assets reused.
These are heap variants, not a full hunk-initialization test.

Normal whole com_memory TU passes Windows x86/x64 in takeover_full_hunk_windows1.
Logs145/146 preserve duplicate hunk-init diagnostic, correction and remaining link
failure. Final native new-header include does not alter behavior. Original hunk
init still calls real filesystem load-stack policy, reflection Dvar registration,
memory tracking, clear and command registration; none bypassed for a fixture.

Next execute real hunk startup once its retained prerequisites link, audit bounds,
alignment/page boundaries/decommit/recovery and ownership at authentic callers.
Remaining UI/client/script/screen/event/input/native audio/cinematic/game/renderer/
network services are still required. Source or heap-probe closure is not evidence
that original initialization or complete startup has returned successfully.
