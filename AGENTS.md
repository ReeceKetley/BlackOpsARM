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


CurrentTASK16structuralcorecomplete: arm64_asset_directory.md, FACT46/WARN16.
RealpoolprerequisitesverifiedFACT48/FAIL11(correctedliveworker), actualSLMTnext.
Next3 TASK17intern/user4release, TASK18firstStringTableheader, boundedcells/index
beforefixups/registry. ContinueautonomouslyDEC29, localverifiedcommitsDEC26/no push.


TASK-0017 complete: actual pool and genuine859 interned IDs match HONOR/Windows,
including optimized ARM assertions/UBSan. Read arm64_string_pool.md, FACT-0051/
0052 and WARN-0017; failures preserved. Only serial pool lifecycle certified.
Next TASK-0018 first genuine StringTable header; TASK19 name/cell array, then
real encoded cell-string fixups before registry. Block0 Pop rewind/transient
alias lifetime must be preserved. Continue autonomously DEC-0029, local verified
checkpoints DEC-0026; no push. Protected checkouts/assets/evidence unchanged.


TASK-0018 is complete: actual genuine first StringTable headers on HONOR and
Windows x86/x64, 73 adversarial cases and12 cycles each. Read
docs/port/arm64_stringtable_header.md, FACT-0054/0055 and WARN-0018. TEMP block0
Pop rewinds native workspace while retaining shadow/maps; aliases are transient,
not registered asset ownership or a general duplicate-key ordering guarantee.
Next TASK-0019 actual name34/58 bytes and cell arrays400/1594, then initialized
cell-string range checks for2/49 genuine encoded offsets, then signed16 indices
and actual lookup before registration. Reference-only payload oracle does not
certify execution. Continue autonomously DEC-0029, verified local checkpoints
DEC-0026; no push. Preserve prior evidence, original assets and protected repos.


TASK-0019 is complete: actual genuine StringTable raw names and unresolved cell
records on HONOR/Windows x86/x64,98 cases/12 cycles each. Read
docs/port/arm64_stringtable_cells.md, FACT-0057/0058 and WARN-0019. Parent4/header0/
child4 scope keeps TEMP header live; per-cell strings/index/fixups remain unread.
Next TASK-0020 real cell strings and checked2/49 offsets to earlier initialized
character starts/NUL/ranges, then actual signed16 indices and hash/lookup before
registration. Original tag, exact mapping and native owned address must agree;
never infer initialized strings from zero pages or capacity/drift fallback.
Com_HashString overflow/ctype and CellCompare subtraction require bounded review
before UBSan lookup. Continue DEC-0029, verified local commits DEC-0026; no push.
Preserve protected repos/assets/frozen evidence. No game-owned graphics yet.


TASK-0020 is complete: actual cell strings and guarded2/49 pointer conversions
on HONOR/Windows x86/x64,129 cases/12 cycles each and optimized ARM pass. Read
docs/port/arm64_stringtable_strings.md, FACT-0060/0061/0062 and WARN-0020/FAIL16.
Proof comes only from earlier successful loads in the live owner, never capacity/
zero pages or drift fallback. Clear records before reset/release. Exact-earlier
block4 starts only; caller liveness/proof creation remains a contract.
Revised next3: original signed16 indices, isolated Windows-equivalent modulo32
hash, actual lookup parity/investigation before registration. Genuine index arrays
are permutations but not ordinarysigned/unsigned hash-sorted. Preserve bytes/order;
do not reject/resort original data to satisfy the earlier plan. Modelmisses are
not yet native execution. Continue DEC-0029, verified local commits DEC-0026;
no push. Protected repos/assets/frozen evidence remain unchanged, graphics absent.


TASK-0021 complete: original signed16 indices preserve every byte/order and
stride2 on HONOR/Windows,162 cases/12 cycles each plus optimized ARM UBSan.
Read arm64_stringtable_indices.md, FACT64/65, WARN21/FAIL17. Actual Load_shortArray
body/defaultWindowsglobalorder remain unchanged; varshort invalid before reset/
owner release. No ordinary hash sorting prerequisite. Next3: real modulo32 hash,
actual lookup/reference investigation on original arrays, then smallest real
registration/next-asset boundary. Finite reader/full async distinction persists.
Continue autonomously DEC29; local verified commits DEC26/no push. Preserve all
prior evidence/protected checkouts/original assets. No game-owned graphics yet.


TASK-0022 complete: actual canonical Com_HashString and unchanged StringTable
wrapper match originalWindows and genuine1994hashes;12857vectors7974overflow,
162cases12cycles perABI plusoptimizedARMUBSan. Read arm64_stringtable_hash.md,
FACT67/68, WARN22/FAIL18, DEC37. Onlyuint32 accumulation/int memcpy changes;
originaltolowerargument/length/null semantics retained, highbytes/locale deferred.
Unique canonical source/declaration/callers and normalWindows3TUs verified.
Next3: actualhash-index/row/column andrealI_stricmp onoriginalindices; provenance-
backed sharedordering investigation; smallest actualregistration/ownership gate.
No resort/sortingreject/fallback/fakequery success. ContinueautonomousDEC29,
localverifiedcheckpointsDEC26/no push. Protectedrepos/assets/olderproofs intact.


TASK-0023 actualqueryparity audit complete: sixoriginal bodies HONOR/Windows,
262180comparison/22controlledlookup +162loadercases12cycles/12857hash pass.
Actualsharedrowmisses24/400 and796/1594 FAIL20 retained, allcolumnqueriesagree;
notoriginalretailinstruction proof. Read arm64_stringtable_lookup.md, FACT70/71,
WARN23/FAIL19/20/21. Preserveoriginalindices/Windowsbehavior, noresort/fallback.
Next3: originalSHA/addressinstruction+loadingpath audit; smallestverifiedbehavior
correction ifdifferent; actualfirstassetownership/registration. Unsafe negative
row/nullcompare/negativeprefix paths excludedpendingseparatefix. ContinueDEC29,
localverifiedcheckpointsDEC26/no push, protectedrepos/assets/oldproofs intact.


TASK24 original registration/sort audit complete. Read arm64_original_query.md,
FACT73/74 and WARN24. Original signed lower bound agrees; missing post-registration
sort of RETURNED header is next TASK25. Preserve LoadObj wrapping comparator and
equalhash/column equality zero. HONOR native CRT differs6/14 equal-key positions;
bounded original-policy sorting and actual query parity required, no arbitrary
row/index tie-break. Read-only original instruction oracle is analysis only,
never Android implementation; registry/roworiginal not executed. Next persistent
registry/TEMP lifetime, then actual next asset/workspace reuse. Continue DEC29,
localverifiedcommitsDEC26/no push; protect all existing evidence/repos/assets.


TASK25 complete: arm64_stringtable_normalization.md, FACT76/77/WARN25/DEC40.
Actualnormalizer/originalqueries exactoriginal400/1594 arrays/all1994results
onHONOR/Windows/optimized. ExistingLoadObj/comparison/hash/load bodies unchanged;
onlymissing post-DB_AddXAsset sort restored onreturnedheader. Checkedborrowed
spans/permutationbeforemutation, specificoriginalCRT16policy/noheap/globalcontext/
tiebreak. Actualfullregistry notcertified; no fabricated registration success.
Next TASK26 realpoolheader/cloneownership survivesTEMPoverwrite, then original
shadow/map/alias reuseaudit andsmallestrealregistryboundary. ContinueDEC29,
localverifiedcommitsDEC26/no push. Preserve allprior evidence/repos/assets.


TASK26 complete: arm64_stringtable_owner.md, FACT79/80/WARN26/DEC41/NOTE6.
Actualpool/init/alloc/clone/size/free leaves surviveTEMPoverwrite/genuine1994queries
on3ABIs/optimized; preserve576/768 policy/native32-Win3220 header/zonechildren.
Normalizeexactlyonce afterpoolclone; freepoolheaderbeforezone, noretargetedTEMP
aliases/fullregistryclaim. Genericfree4qualified pointer-copy correction retains
historicalWindows semantics; defaultsource/BOM/order/canonicaltypes proved.
Next TASK27 originalTEMP physical/shadow/maps/aliases reuse beforeasset2, then real
DBHASH/asset-entry ownership/init andregistryboundary. No fakeDB_Addsuccess or
arbitrarycaprelaxation. ContinueDEC29/localverifiedcommitsDEC26/no push; preserve
all prior evidence/protectedrepos/assets. Invalidcaller guards arefixturecontracts,
notuncheckedrawleaf recovery/concurrency certification.


Ledger metadata correction: FACT82/83 supersede formatting of FACT76/77;
FACT84/85 supersede FACT79/80; WARN27 supersedes WARN26. Evidence/results unchanged,
old entry history retained; current TEMP reuse/real registry limitations unchanged.


TASK27 cursor/alias prerequisite complete; see arm64_temp_cursor.md/handoff,
DEC42 and current Ledger evidence. Native block0 shadow must rewind with physical
cursor to TARGET entry; nonzero streams keep advances. Keep maps on Pop until
actual overwrite; latest equal-key alias ordering is stable. General TEMP reuse
is NOT certified: retainedsegments may be unordered and native expansion may
overwrite logically preserved Win32tails. Never hide this through driftfallback
or wholemap rollback. NextTASK28 write-aware mappings/explicit suffix handling,
then genuine codeRawFile numericheader12->24 atTEMP0; zombie nextTechniqueSet7
deferredwithoutskipping. No nextasset/registry/render successclaim. Alltestsended;
strictcurrentrunners expect observed0postPop while frozenold20evidence preserved.
ContinueDEC29, localverifiedmilestone commitsDEC26 authorized; no push.


TASK28 complete: read arm64_temp_write.md/temp_write_handoff.md and currentLedger.
TEMP aliases retain current VALUES when logicallyuntouched/nativeoverwritten;
never expose their overwrittenaddresses. Newstagedfields must becompleteactual
logical4/native slots. Missing/invalid/external TEMP converterroutes failexplicitly;
no stale driftfallback. Single-read stagedmetadata andfinite C++throw adapter
verified; realCom_Errorlongjmp/async recovery NOTcertified. OriginalRawFilebody/
order andcanonical12/24 layout preserved; codeheaderends10419, zombie nexttype7
deferredat65678withoutskip. NextTASK29 actualname/presentbuffer141bytes, preserve
allfirsttablequeries/block4/ownership, stopbeforeDBAdd. NoRawchildren/registry/
render claimyet. UserresumedfullgoalDEC29/localverifiedcommitsDEC26, no push.


TASK29 complete: arm64_rawfile_payload.md/rawfile_payload_handoff.md, current Ledger.
Canonical loader bodies unchanged; genuine name17/buffer141 now read to10577.
Buffer any nonzero is presence, exact len+1 binary copy; no added NUL policy.
Name only absent/inline certified here. Preflight all children before owned writes.
Current and retained aliases rechecked after children; all first-table queries
and bytes survive. Partial wrapper failure uses fixture-only unwind, not atomic
or longjmp/async recovery. Original RawFile header remains TEMP-owned, no DB_Add
success yet. Next actual RawFile pool1024/init/alloc/clone/free and zone child
lifetime, registry-entry/DBHASH init, then smallest real registration. Preserve
all evidence and protected checkouts; full goal active/local commitsDEC26/no push.


TASK30 complete: read arm64_rawfile_owner.md, DEC45/FACT95/96/WARN31/FAIL29.
Latest user handoff resumes work after NOTE9 and explicitly authorizes local
verified ARM64 checkpoints; no push. Actual RawFile1024/2048 pool/clone/free and
complete genuine child lifetime survive TEMP overwrite on3ABIs/debug/O3; source
proof/defaultWindows compile and protected16875 paths pass. No alias retargeting
or DB_Add success. Unchecked raw leaf rejection remains fixture caller contracts.
Next actual registry-entry/DBHASH init services, then real registration/lookup.
Preserve zombie's actual TechniqueSet7 boundary and all previous evidence/repos.


TASK31 complete: read arm64_asset_entry_init.md, DEC46/FACT97/98/WARN32/FAIL30.
Actual canonical entry-list helper/real DBHASH74 recursion verified on3ABIs/O0/O3;
head16/free32752-or65520 and native24/Win3216/fixed16 links preserved. Helper has
startup/quiescence contract and no hash/counter resets/lock. Existing pool locks
0/1 unchanged; internal Database lock2 means invalid-lock fixture now uses3.
Normal Windows/source/script-service/SLMT/protected17014 checks pass. No fullDB_Init
or entry alloc/free/publication claim. Next actual header dispatch/entry alloc/free
and failure/partial-progress semantics, then lookup and smallest real registration.
Local verified checkpoints allowed/no push; all evidence/protected repos preserved.


Standing autonomous goal DEC47 applies: do not stop on gate completion. Preserve
protected repos/assets/evidence, local verified commits remain allowed/no push;
read standing_development_goal.md for resources and intentional stop conditions.
TASK32 actual entry/header dispatch/free/FF diagnostics complete (arm64_entry_allocation,
DEC48/FACT99/100/WARN33/FAIL31). Genericfree now exactvoid*, typed printcallback,
namegetter representation memcpy; source/policy/Windows/genuine owner parity pass.
Next genuine entry-owned headers/zone children, hash lookup, smallest publication.
Logging/error/dvar controls are fixtures; LoadObj explicitly unsupported; no full
engine/registry or rollback claim. Continue toward actual production integration.


TASK33 genuine entry-owned headers complete: arm64_entry_owned_genuine.md,
DEC49/FACT101/102/WARN34/FAIL32. Actual alloc/clone/free owns2 code/1 zombie headers,
allchildren/queries/TEMP integrity and entry/pool release baseline verified3ABIs/
O0/O3. Production trees unchanged. Serial/private/nohash/noDBAdd boundary only;
finite reader/error inputs not fullengine/async/recovery. Next actualname/hash
lookup, smallest publication/returned-header, nextgenuineasset. Continue autonomous
DEC47/localverifiedcommits/no push; preserve TechniqueSet7 and all old evidence.

TASK34 current: actual genuine registry lookup verified on3ABIs/O0/O3; fixture
membership restored before frees, not DB_Link/DB_Add. Read arm64_registry_lookup.md
and DEC50/FACT103/104/WARN35/FAIL33; next real insertion/returned-header integration
then next genuine asset. DEC47 standing autonomous goal/localcommits/no push active.

TASK35 production insertion/lookup passes3ABIs/O0/O3. FullAdd/Link unverified; next actual returned-header registration on liveTEMP before normalization/Pop then next genuine asset. DEC51/FACT105/106/WARN36; standing DEC47 active.

TASK36 current: actual Add/fullLink new-name no-mod path on liveTEMP returns
persistent headers before normalization/Pop;3ABIs/O0/O3 pass. Cleanup fixture-owned.
Read arm64_registry_registration.md, DEC52/FACT107/108/WARN37/FAIL34. Next genuine
asset deserialization/wider zone integration/coreinit. No fullstartup claim. DEC47 active.

TASK37 current: all37 code_pre_gfx assets deserialize/register,3ABIs/O0/O3 pass.
Finite reader/fixtureunlink, not coreboot. Read arm64_code_zone.md and DEC53/FACT109/110/
WARN38/FAIL35. Next firstzombieTechniqueSet7 all130null through66226/type7registration
then genuineXModel5; no scanning/skipping. DEC47 autonomous/localcommits/no push active.


# Current state - TASK38 diagnostic verified; genuine bootstrap next

TASK37 remains the successful asset checkpoint: all37 genuine code_pre_gfx assets
register, survive TEMP reuse and release on x86/x64/HONOR O0/O3. TASK38 executes
actual first Pentagon TechniqueSet header/name/130-null loop and Add/Link until
required default resolution, not successful registration. Exact asset is
`,mc_l_sm_r0c0n0s0x0`: x64 source confirms comma reference semantics. Empty payload
needs no authored-content repair; referenced pointer must resolve through an
existing asset or genuine default. No silent skip, fake provider or shader graph.

Root-final host/O0/O3 builds, prerequisites and strict diagnostic collectors pass
all3ABIs; observed exact exit3 Windows/134 HONOR at unavailable fixture endpoint.
Root Pop, persistent type7 header and cleanup have not executed. Source bodies
and normal Windows policy preserved; normal8TUs perABI pass. All18483 protected
paths and exact Windows/DXVK Git unchanged. Archive1027 inventory rows plus
inventory file; no game bytes/binaries committed. See arm64_techset.md,
DEC54/55, FACT111-116, WARN39/FAIL36. All tests ended; Git supplies exact HEAD.

Next3: genuine first code_post_gfx_mp/default CPU graph under actual headless
resource policy; live SL/MT ownership plus real default clone/reference resolution;
real bootstrap sequence (code_pre_gfx_mp/postgfx/common_zombie) then Pentagon
XModel/script-index integration. Exact named provider before Pentagon is not
proven; don't scan unknown payloads. Dead Ops Arcade is out of scope per user.
No core engine boot/async/APK/game-owned graphics/playable claim. DEC47 remains
active: autonomous milestones, local verified commits/no push. Root owns all
returned implementation edits. Stable BlackOps64/original assets remain read-only.


User reference policy: the working Windows x64 build is verified by extensive
testing and gameplay. Intentional stubs matching that reference, including future
discoveries, are accepted content and require no separate repair/implementation
milestone. Preserve x64 reference handling; focus on actual Android startup and
Zombies dependencies. Dead Ops Arcade remains excluded. See DEC56.
