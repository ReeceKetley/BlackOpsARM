# BlackOpsARM agent instructions

Work only in this Android worktree unless the user explicitly authorizes another
location. The branch is `android-arm64`; `../BlackOps64` is the Windows reference
checkout on `main`. Never edit its files, index, or branch as part of Android work.
Do not commit or push without explicit user authorization.

## Source and platform boundaries

- This is a Git worktree of BlackOps64, not an independent engine fork. Never copy
  or independently duplicate engine sources. Use this worktree's `src/`, `tl/`,
  `DemonWare/`, existing CMake source lists, and platform interfaces.
- Prefer minimal, isolated changes. Avoid broad cosmetic refactors of decompiled
  code. Keep platform implementations separate from shared gameplay logic.
- Put native Android implementations in `src/platform/android/`; small Win32 OS
  service compatibility shims belong in `src/platform/win32_compat/`.
- Use native Android implementations for graphics, audio, application lifecycle
  and controller input. Never add dummy Windows functions that silently succeed.
  Unsupported operations must produce an explicit error or documented failure.
- Preserve 32-bit serialization, original FastFile layouts, packaged shader
  bytecode and original game assets. Native runtime pointers must use native
  width; disk offsets/handles are not pointers. Do not apply blanket packing or
  disable layout checks to make ARM64 compile.
- Use correct ARM64 atomics and memory ordering. Do not assume x86 ordering,
  pointer-sized operations using 32-bit integers, or low-address allocation.
- Android ARM64 selects shared native-width layouts through
  `platform/bo1_native_width.h`; never enable `BO1_PORT_X64` globally on Android.
  That legacy option still controls Windows comparison builds and diagnostics.
  Consult the per-site flag audit before changing guards. Four-byte VM/parse
  slots use region-tagged Narrow32 handles by default; retain fixed disk/slot
  assertions. Test with the existing ARM64 narrow-slot and ABI probes, generated
  production declaration probes, and Win32/x64 host probes in this worktree.
- Shared engine changes must remain compatible with Windows. Record which
  Windows checks are needed; ask for authorization before writing or building in
  the Windows checkout. Read-only reference inspection is allowed.
- `../DXVK-Native` is a separate repository. Do not change it as part of this
  setup or routine engine work without explicit authorization. Initial renderer
  target: DXVK Native 1.9.2 with the tested SDL2/Vulkan integration.

## Ledger and reproducibility

Read `.ledger/AGENT-INSTRUCTIONS.md` and run `ledger context "<task>"` here at the
start of significant work. Also consult `../.ledger` for shared T5 history and
Windows evidence. Root Ledger owns shared project facts; this Ledger owns Android
setup, decisions, blockers and progress. Qualify root entry references by path;
IDs in the two stores are independent. Do not copy the root database or rewrite
its history. Current explicit user instructions supersede historical plans.

Use `ledger add` for significant decisions, findings, failures, blockers and
verification. Preserve history via `ledger supersede`/`invalidate` when needed.
Update `.ledger/STATE.md`, run `ledger status` and `ledger doctor` at handoff.
Keep `docs/port/android_arm64.md` build/test steps reproducible and distinguish
toolchain/APK checks from engine, renderer and on-device gameplay evidence.

Current milestone: TASK-0013 genuine16-byte asset-list envelope complete.
Read docs/port/arm64_assetlist.md and assetlist_handoff.md, .ledger/STATE.md:
DEC-0024/0025, FACT-0037/0038, FAIL-0007, WARN-0013. Actual native envelope/helper
bodies and moved3 declarations remain unchanged; lightweight types assert actual
pointer width independently of legacy Windows selector. BO1_ASSETLIST_GATE is
standalone compile selection, not full Android expander support. Normal Windows
expander/full registry x86/x64 pass; light legacy-off header passes while old
heavy legacy-off graphics guards remain. Never disable layout assertions.
Use genuine validated complete-decode prefix/header provenance. PlanAssetList
checks signed counts and present-array uint64 disk/native minimum footprints
against genuine remaining/block4 capacities; no arbitrary limits or tag-address
dereference. Root tags mean presence; null-positive counts retain legacy meaning.
Finite checked reader adapter is fixture-only, not engine async/refill integration.
Keep envelope alias destination persistent; detach streams before reuse/owner
release. Side-buffer alias registration is verified, later fixup conversion is
not. No post-read bad_alloc or initializer/fatal OOM recovery certification.
TASK-0011 checked production policies/main-thread ownership and atomic Begin/
nonthrowing End/Release remain required. DB_FreeXBlocks alone does not free owner.
Windows retained PMem/Android touched mappings and historical timing/RSS baseline
stay distinct. Native multi-owner PMem stride and diagnostic freeTot remain gaps.
Next recommended TASK-0014: block4 script-string pointer table reservation and
packed4-byte tag expansion only, before string loop/bytes/interning, assets/
fixups/registry/zone init/async/rendering. Not started; await next continuation.
Prior regressions pass; actual16KB/S25/full engine/backend remain deferred.
Two user-authorized read-only agents audited source/policy and adversarial cases;
lead owns all edits/tests/Ledger. No commit/push; HEAD6f0787ab. Preserve protected
checkouts/root and997 prior archive/report/handoff files. Exact current inventory
is docs/port/arm64_assetlist_changes.md. Historical source/evidence stays intact.

## Current overnight milestone authorization

User approved docs/port/overnight_frontend.md (DEC-0026): pointer tables, bounded
raw string data and asset directory for both genuine files/3 ABIs, before asset
dispatch. Real production interning is conditional stretch with genuine memory
pool/synchronization/ARM64 atomics; never fabricate IDs. Lead owns integration;
three read-only reviews run independently. Local checkpoint commits after verified
milestones are now explicitly authorized; no push. This overrides historical
no-commit statements above. Preserve protected checkouts and all prior evidence.
TASK-0014 is the first implementation boundary. Stop at the approved finish line.

DEC-0028 broadens follow-on work after core through further validated Android
execution/asset/graphics milestones, with D:/Games/Call of Duty - Black Ops game
root and separate DXVK dependency. Aspirations are optional; never fake success.
Continue one production boundary at a time; current checkpoint ffc4241 verified.

Current gate TASK-0014 complete: read arm64_string_table.md, FACT-0040/0041,
FAIL-0008/0009, WARN-0014.26fixtures/12genuine cycles, original-game full hash
provenance, actual pointertable all slot/alias/stride and release checks pass.
Next actual Load_XStringCustom raw-only boundary; whole Load_TempStringCustom
also interns and must not masquerade as raw validation. Full objective file
read per DEC-0029 supersedes previous loose planning; no fake gameplay/frames.
Record next3 milestones and distinguish actual production leaves/fixture readers.
Local verified checkpoint approval DEC-0026 persists; no push. Preserve1280 prior
archive/report/handoff files for this gate. Goal continues autonomously after gates.


TASK-0015 completed: see docs/port/arm64_raw_strings.md, FACT-0043/WARN-0015.
Actual bounded raw strings pass HONOR/x86/x64; raw pointers are not interned IDs.
Next TASK-0016 actual asset directory before dispatch, TASK17 real SL/MT pool,
then first genuine StringTable header before payload/fixups. Continue bounded
autonomously per DEC-0029; local verified checkpoints DEC-0026, no push.
