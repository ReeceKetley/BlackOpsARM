# Ledger Agent Instructions

This repository uses Ledger for persistent project memory.

## Scope and parent relationship

Run Ledger from `BlackOpsARM/` for Android-specific knowledge. `../.ledger` is
the parent project's authoritative shared T5 history and Windows evidence.
Consult both; do not copy its entries/database or maintain competing Windows
status here. Root and Android entry IDs are separate namespaces: include the
Ledger path when citing an entry from the parent.

The current user request authorizes Phase 1: a small ARM64 compile-only source
subset, static dependency/ABI audit and justified compatibility foundation.
Full engine porting and game/backend integration remain deferred. Historical
root statements do not override this bounded compilation authorization;
historical Godot/SDL3 or device-GPU assumptions do not override this request's
DXVK Native 1.9.2, tested SDL2/Vulkan integration and named target devices.
Record Android progress here and link shared evidence back to the parent.

The user's follow-up also authorizes auditing every BO1_PORT_X64 use and enabling
shared native-width behavior on Android ARM64. Use BO1_NATIVE_64BIT, retain
Windows-specific guards and fixed four-byte/disk assertions, and consult
docs/port/evidence/arm64_phase1/flag_guard_audit.json. The connected HONOR is
authorized for bounded ARM64 probes through ADB; keep their artifacts in a
unique /data/local/tmp/blackopsarm-phase1-* directory. This is not game or APK
installation authorization for future milestones.

## At the beginning of significant work

The user subsequently authorized the next bounded actual-header milestone.
See DEC-0008/DEC-0009 and docs/port/arm64_script_headers.md for the completed gate.
Real native profiler services are implemented, with stable enable/thread setup
required before workers start. Android counters use nanoseconds; Windows keeps
TSC units. The original RNG calculation is preserved but its libc-dependent
range/stream is a recorded blocker (WARN-0004), not a completed RNG port.
Keep earlier Phase 1 evidence immutable; store this gate's evidence separately.

The user then authorized continuation to the RNG/script lookup milestone.
DEC-0010/0011 and FACT-0011/0012 record the completed Windows-reference CRT
service and unchanged cscr_readwrite.cpp gate. WARN-0004 is superseded: the
first-party RNG range/stream is now verified; vendor interleaving and wider
allocator/runtime boundaries remain unvalidated (WARN-0005). Preserve earlier
evidence under arm64_phase1 and arm64_script_headers; use arm64_rng_script for
this milestone. TASK-0005 was the next bounded parse-tree/allocator gate.

The user then authorized TASK-0005 through continuation. DEC-0012/0013 and
FACT-0014/0015 record the completed normal parse-tree/default HunkUser gate.
WARN-0006 supersedes WARN-0005; debugger pointers/encoded-address arithmetic,
Z heap/nondefault allocation/tracking/evaluator and actual 16 KB Android device
behavior remain unvalidated. HunkUser is owner-thread confined; do not retain
handles after arena release. All earlier evidence is immutable; this gate uses
arm64_parsetree. TASK-0006 is the next bounded debugger allocation boundary.

The user then authorized TASK-0006 through continuation. DEC-0014/0015 and
FACT-0017/0018 record the completed debugger allocation/selected evaluation gate.
Ten actual T5 units compile and thirteen HONOR probes pass. Real native Z heap,
first-fit synchronization and explicit main-thread registration are available.
WARN-0007 supersedes WARN-0006: expression compilation/string/callable/VM paths
remain unvalidated. Host debugger probes use the Android heap; Windows engine
Z heap alignment is not certified. Test-owned cleanup state and fifty aborting
endpoints must never be shipped. Default hunks/debugger lists are owner-thread
confined; first-fit reset/destroy require quiescence. Freeze arm64_debugger
evidence at handoff. TASK-0007 is the next bounded expression compilation gate.

At agent rotation the user requests a local checkpoint of completed Android
work and a fresh-agent handoff; no push or further porting is authorized. Read
docs/port/agent_handoff.md for exact commands, current limits and TASK-0007
acceptance criteria. The checkpoint containing that document captures durable
sources/Ledger/evidence; ignored local build/SQLite/scratch files remain intact.
New porting work and later commits/pushes require fresh user authorization.

Current user continuation authorized TASK-0010 and two read-only audit agents.
Bounded full wrapped decode is complete (FACT-0029/0030, DEC-0019, FAIL-0005);
WARN-0010 supersedes WARN-0009. Twelve actual T5 units plus six unchanged matching
custom-ABI zlib C units, sixteen HONOR probes and MSVC x86/x64 pass. Genuine
code_pre_gfx and zombie_pentagon decode completely; known retail root is
../BlackOps64/build/Release, read-only. Root wrote/built/tested only BlackOpsARM.
Read docs/port/arm64_inflate.md for current commands, failures, padding/limits
and TASK-0011 checked genuine-header-to-zone acceptance. Engine async/asset
pipeline remains deferred. Never mix system libz with engine custom streams;
keep uint64 budgets independent of native zlib ulong and fixed disk widths.
Root shared history/all older evidence stay immutable. Historical no-assets
statements are superseded. TASK-0011 has not started. The user now authorizes
one local checkpoint of accumulated completed work (DEC-0020); no push or later
commit is authorized. Read docs/port/android_checkpoint.md. Follow user direction
and update all current pointers at the next completed gate.

Current user TASK-0011 supersedes the historical bootstrap plan above: checked
allocation stops before DB_InitStreams, zone initialization, assets/fixups or
rendering. Completed gate: DEC-0021/0022, FACT-0032/0033/0034, FAIL-0006 and
WARN-0011 (supersedes WARN-0010). Read docs/port/arm64_checked_zone.md for current
commands, measurements and failure semantics. Native64 plans exactly match
production Windows x64; Win32 sizes/layouts stay intact. Use genuine validated
36-byte XFile metadata, never payload-derived descriptors. Actual expansion and
alignment/tail capacity are checked in uint64 before backend allocation.
Begin failures must leave no owner; End/Release must not throw. Main-thread native
owner registration and allocation-free rollback are explicit. Windows retained
PMem commit/working set differ from Android protection/touch/RSS. Separate decoder
and allocation processes; preserve original raw samples, failed diagnostics,
independent oracles and old archives. Two read-only agents audited policy and
adversarial/methodology; lead owns edits/tests/Ledger. All tests completed.
TASK-0012 recommendation is genuine DB_InitStreams cursor/reset only, not started.
Current request forbids commit/push; HEAD6f0787ab and protected checkouts remain.

Run `ledger context "<your current task>"`. Use current ACTIVE knowledge. Do not assume historical entries remain valid.

## Before major architectural decisions

Run `ledger search "<topic>"` and check decisions, failures, warnings, and experiments.

## During work

Record useful discoveries with `ledger add fact|decision|experiment|failure|hypothesis|warning|task|note "..."`. Avoid trivial details.

When evidence disproves a belief, use `ledger invalidate <ID> --because <ID>` or `ledger supersede <OLD_ID> <NEW_ID>`. Preserve history.

## At the end of significant work

Record important discoveries, decisions, outcomes, failures, unresolved questions, and changes in project state. Run `ledger status` and update STATE.md when needed.

## Current TASK-0012 continuation â€” complete

The user's continuation authorized genuine checked blocks to DB_InitStreams.
DEC-0023, FACT-0035/0036 and WARN-0012 supersede the historical not-started pointers
above. Read docs/port/arm64_stream_init.md and stream_init_handoff.md. All tests
completed; no shared engine edits or asset reads. Preserve847 prior archive/
handoff files and protected repositories. Persistent detached descriptors before
DB_FreeXBlocks/named release; initializer/fatal OOM recovery is not certified.
TASK-0013 is recommended16-byte XAssetList envelope only, before strings/assets/
fixups/zone init; no implementation started. Current HEAD6f0787ab; TASK-0011/0012
remain uncommitted; no commit/push is authorized. Lead owns this gate; no new
subagent delegation requested. Exact reproduction and inventory are in report.

## Current TASK-0013 continuation â€” complete

User authorized the next bounded envelope task and optional delegation.
DEC-0024/0025, FACT-0037/0038, FAIL-0007, WARN-0013 supersede historical pointers
above. Read docs/port/arm64_assetlist.md/assetlist_handoff.md. Both genuine files/
3 ABIs pass checked16-byte envelope read through actual native expander/Win32
branch; no strings/assets/fixups. Finite reader is explicit standalone fixture;
normal async/refill loader unvalidated. Persistent final aliases detach before
owner/output reuse. Normal full Windows expander and actual registry pass;
BO1_ASSETLIST_GATE is bounded Android subset, guards/layouts unchanged.
Preserve997 prior evidence/report/handoff files and protected repositories.
All tests completed; two read-only agents finished; lead owns edits/Ledger.
TASK-0014 pointer table only is recommended, not started. No commit/push;
HEAD6f0787ab, all TASK-0011/0012/0013 work uncommitted. See exact inventory/report.

## Current approved overnight goal

DEC-0026 and docs/port/overnight_frontend.md now authorize the structural front end
through pointer tables, raw string bytes and asset directory, before dispatch;
production interning is conditional stretch. Local checkpoint commits after
verified milestones are explicitly authorized; no push. Earlier no-commit
statements are historical. Lead owns code/tests/Ledger; three read-only audits.
Preserve all prior evidence/protected checkouts and record each boundary/decision.

DEC-0028 records active user-controlled follow-on goal: after core, continue
validated Android port boundaries toward real game-code/assets/graphics; game
root D:/Games/Call of Duty - Black Ops, separate DXVK dependency. Aspirations are
loose, never force/fake success. Checkpoint ffc4241 verified (FACT-0039), no push.

Current TASK-0014 completed per DEC-0029/FACT-0040/0041/WARN-0014; failures8/9
retained and corrected.26 fixtures/12cycles eachABI, actual pointertable source,
114 AArch64 artifacts/20exes. Next raw bytes -> directory -> real SL/MT, then
critical engine/app/game-owned graphics dependencies, no deadline/fake progress.
Updated objective file read; preserve1280 prior archives/protected state.


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
