# Genuine MP startup zone through the production reader

TASK41 runtime checkpoint, 12 October 2026, following reader prerequisite
`bf199b7`. The complete genuine MP startup zone now executes through a reusable
production library on HONOR. This is production asset integration with controlled
test caller inputs, not completed `Com_Init`, database-worker startup or gameplay.

The library connects canonical `DB_LoadXFileData` / null-terminated reads to the
owned finite reader. It expands the original envelope/directory, invokes original
`Load_XAsset`, RawFile/StringTable pointer loaders and registration wrappers, and
normalizes the returned registered table header. Original normal Windows read
and dispatch bodies remain unchanged through explicit compilation selection.
Unsupported content is refused rather than skipped.

All 71 assets register: three StringTables and 68 RawFiles. The tables have
531/290/75 cells; the latter two reuse 357 earlier strings. Runtime fixups require
successful live-owner string records and matching exact stream mappings. Registered
headers and all children survive TEMP overwrite. Actual entry lookup, table row
and column queries agree with the independent complete-source oracle.

`NativeZoneLoad` retains a child lease on its file owner through scoped removal;
detached streams cannot permit premature release. Detached descriptor storage has
process lifetime. Removal validates membership and refuses referenced entries
before mutation, then uses actual DBHASH locking and actual entry/header frees.
It is explicitly a primary-only RawFile/StringTable operation, not a replacement
for general `DB_UnloadXZoneInternal`.

Read failures unwind canonical loader C++ buffers before entering `Com_Error`;
the handler has ended before that recovery transfer. Persistent context/owner
state must live outside the engine's longjmp extent. Full native engine recovery
is not certified by these aborting test endpoints.

## Final verification

- Windows x86/x64: `native_startup_zone_host_final1`; four complete cycles each.
- HONOR: `native_startup_zone_arm_O0_final2` and O3 equivalent; assertions/UBSan,
  exact pushed binary/retail SHA, independent collectors, four cycles each.
- Per run: 284 asset records, 1792 real table queries per cycle, full150108 source
  bytes, zero residue, high-water71 and free-entry count32752 after removal.
- All576 StringTable and1024 RawFile free header slots are restored without
  reinitializing pools between cycles. Actual Windows PMem/Android named owners
  restore their counts/cursors. Referenced removal and premature owner close fail
  without freeing children. Detached globals outlive the context.
- Eight complete normal Windows TUs per ABI compile, including the async file
  loader and stream converter. Source proof verifies20 canonical/normal bodies
  and the explicit reader child-lease delta against `bf199b7`.
- Independent full-source oracle plus13 malformed oracle controls pass. These
  are oracle controls; malformed production recovery is not an acceptance claim.

The reusable static library has no `tools/port` source or test failure-injection
dependency. Controlled Dvar inputs and aborting unavailable endpoints are linked
only into its driver. Actual Dvar/Cmd initialization remains a required production
caller contract. Native Windows third-plus same-primitive PMem records remain
contained by the reader's explicit refusal; no allocator policy was rewritten.

Failed/superseded builds are preserved. Initial probe1 lacked the selected original
column getter; probe3 rejected an unrelated default-true lazy-model policy. The
getter was exposed unchanged, and source investigation showed that policy only
affects XModels. Engine policy remains unchanged. Root also corrected child-owner
and detached-descriptor lifetime gaps before final validation.

## Reproduction and next integration

Build with `build_native_startup_zone_host.ps1` or
`build_native_startup_zone_arm64.ps1` using fresh worktree outputs. Run host binaries
against the original MP FastFile and validate logs with
`check_startup_zone_runtime.py --pointer-bytes 4|8 --retail <original> --output
<fresh>`. On HONOR use `run_native_startup_zone_gate.py` with the build, fresh
output, detected serial and matching optimization. Commands, source manifests,
provenance and result JSON are preserved. No retail data or binaries enter Git.

The next priority is actual native startup service closure from
`production_startup_dependency_map.md`: main-thread/TLS/recovery and general
synchronization, actual Dvar/Cmd/Cbuf, then database workers/file completion and
genuine `Com_Init` / bounded `Com_Frame`. Keep the original pre-gfx-before-filesystem
ordering. Headless Zombies simulation precedes DXVK rendering.
