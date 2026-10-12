# Roadmap to a native Android ARM64 headless T5 engine

## Current integration direction (12 October 2026)

TASK46 is verified: actual main TLS/parse/Dvar/SL/CbufCmd and genuine registered
settings now precede all71 MP startup-zone assets in one process on Windows and
HONOR O0/O3. Ownership cleanup and focused ABI/CRT checks pass. Read
`arm64_native_startup_services.md`. DEC64 prioritizes the exact actual Com_Init
executable next, executing to its first unsupported dependency and implementing
the required behavior. Database events/workers/reads/allocator ownership remain
real dependencies as reached, not an isolated fixture program to expand first.
Actual Com_Frame follows successful initialization. Full Com_Init has not run.

TASK45 verifies the original FastCriticalSection writer/release protocol on
native fixed32 atomic counters, preserving normal Windows behavior. See
`arm64_fast_critical_section.md`; TSan runtime failure remains nonpassing
coverage with independent no-engine reproduction. Next real Dvar/Cmd/Cbuf
startup closure and SL-owned settings, then database orchestration/headless boot.

TASK44 executes original thread-data initialization/reset with native roles/TLS,
then all71 genuine MP assets in the same process on Windows and HONOR O0/O3.
See `arm64_native_thread_data.md`. Next actual fixed32 FastCriticalSection
protocol (`native_dvar_lock_plan.md`), real Dvar/Cmd/Cbuf services, then genuine
DB workers/file publication and headless Com_Init/frames. Caller Dvars remain
test-controlled; full startup/gameplay has not run.

TASK43 completes all75 native recursive enum critical sections while preserving
actual SL/MT/DBHASH lock identity. Windows/HONOR O0/O3 mutex and genuine asset/
string-owner regressions pass; see `arm64_native_critical_sections.md`. Next:
native role/TLS bindings executing original `Com_InitThreadData`, then real
Dvar/Cmd/Cbuf with fixed32 synchronization, before actual DB orchestration.

TASK42 removes the native Windows two-owner PMem validation limit after
reproducing and repairing its native record stride; both primitive stacks and
the genuine reader/MP71 regression pass. See `arm64_pmem_owner_stack.md`.
`native_startup_services_plan.md` adds concrete Dvar/command ordering, fixed32
synchronization and TLS requirements. Next bounded service: general recursive
critical sections retaining existing SL/MT/DBHASH lock identity, then authentic
thread/TLS and real Dvar/Cmd/Cbuf integration. Android DB-worker ownership remains
unresolved; serial loading is not worker execution.

TASK41 complete: all71 genuine MP startup assets now execute through the reusable
production library on Windows x86/x64 and HONOR O0/O3, including357 earlier-string
references, real lookups and ownership cleanup. See `arm64_native_startup_zone.md`
and `native_startup_zone_handoff.md`. Controlled test caller inputs do not establish
full Dvar/DB initialization. The next milestone is genuine startup platform/service
closure from the completed dependency map, then actual database orchestration and
`Com_Init` / `Com_Frame`; headless gameplay remains ahead.

The paragraphs below preserve the milestone's earlier planning checkpoint.

Latest verified checkpoint: TASK40, `db6c0e4`. Genuine asset registration and
default resolution pass Windows x86/x64 and HONOR O0/O3; complete engine startup
and headless gameplay remain unverified. Historical milestones below are retained.

TASK41 is building a reusable production decoded-zone reader and owner for the
genuine first MP startup zone (71 assets: three StringTables and 68 RawFiles).
An independent full-zone oracle reaches exact source/block boundaries and rejects
13 malformed controls. The reader/decoder prerequisite now passes Windows x86/x64
and HONOR O0/O3; see `arm64_startup_reader.md`. Full71 dispatch remains active.
Enforce allocation budgets during decompression, preserve real initialization and
ownership contracts, and do not ship fixture Dvars or test callbacks as services.

Alongside this work, the user requests a read-only dependency map of the actual
`Com_Init` / `Com_Frame` path. Identify call order, required services, ARM64
compile-versus-execution evidence, first platform/database/ABI blockers, and the
smallest authentic headless integration target. Use those findings to prioritize
increasing amounts of production code running together in one Android process.
Do not invent a replacement boot sequence and call it T5 startup.
The completed audit is `production_startup_dependency_map.md`: genuine first DB
loading precedes filesystem initialization. Native TLS/recovery, general critical
sections, Dvar/Cmd and database workers are the first hard startup dependencies.

Next: complete the startup-zone reader/dispatch integration; implement the real
startup services exposed by that investigation; execute genuine Zombies world,
script and server simulation headlessly on HONOR. DXVK-Native rendering follows
headless validation. Accepted x64 stubs and Dead Ops Arcade exclusion still apply.

## Historical roadmap

Living plan. Each milestone is bounded, keeps Windows compatible, preserves
32-bit disk layouts, freezes its own evidence, and never widens into
gameplay/backend work without authorization.

## Where we are

Twelve actual T5 units compile to AArch64. TASK-0007 normal-to-debugger
expression compilation, TASK-0008 raw FastFile reader and TASK-0009 actual
zone allocation/stream bootstrap are complete. Fifteen HONOR probes and all
host x86/x64/Narrow32 regressions pass. TASK-0010 now adds six unchanged bundled
codec C units and the sixteenth HONOR probe; complete code_pre_gfx and
zombie_pentagon decode on all three targets. See arm64_inflate.md for current
evidence/limits/reproduction. Actual zone bootstrap remains fixture-only; no game runs.

## Next three bounded milestones

### TASK-0008 â€” FastFile preamble reader (complete 2026-10-11)

Shared parser + synchronous file-I/O boundary land in
`src/platform/fastfile_reader.h`: 12-byte raw head (production-accurate,
verified pre-inflate) plus the logical 48-byte stream head; 14/14 HONOR
probes and all host checks pass with no engine changes
(docs/port/arm64_fastfile.md, FACT-0022). The device runner's stderr-stop
abort is fixed in the same gate (FACT-0023). Retail files at `BO1_GAME_DIR`
verified genuine: all 158 zone FastFiles are `IWffu100` / 473, and
`code_pre_gfx.ff` reads back identically on HONOR. Decompressed `XFile`
content still needs the inflate stage. No zone loading, no async/APC
pipeline.

### TASK-0009 â€” Zone block allocation and stream init (complete 2026-10-11)

Actual complete DB source paths pass on HONOR and Windows x86/x64 using
real native named HunkUser/first-fit ownership. Seven blocks, fixed disk sizes,
cursor/alias/stride reset and release pass. Native pending-stride reset fixes
a retained x64 control failure. This is a logical XFile fixture, not an inflated
retail zone or the legacy PMem pool/tracker. See arm64_zone.md; no asset parsing.

### TASK-0010 â€” Deflate stage isolation (complete 2026-10-11)

Complete synchronous bounded wrapped decoding and genuine XFile validation
pass on HONOR/MSVC x86/x64, with checksum/end/exact output, custom bundled codec,
explicit errors/padding and independent fixtures/oracles. No assets parsed;
engine async pipeline remains deferred. See arm64_inflate.md.

### TASK-0011 â€” Checked genuine FastFile allocation (complete 2026-10-11)

Genuine code_pre_gfx/zombie_pentagon decoded headers drive checked unchanged
production allocation policies on HONOR and Windows x86/x64. uint64 preflight,
actual native capacities, controlled rollback, warm/cold latency and distinct
decoder/allocator memory peaks are frozen. Native64 plans match; Windows retained
pool and Android touched mappings are different contracts. No new stream init,
asset parsing or fixups. See arm64_checked_zone.md, FACT-0032/0033, WARN-0011.

### TASK-0012 â€” Genuine checked blocks to DB_InitStreams (complete 2026-10-11)

Actual stream initialization on both genuine files/HONOR/Windows x86/x64 passes
seven cursors, zero slots, independent pending size/stride/alias/drift reset and
repeated detach/free/named release.12 cycles and61 Init calls per ABI; invalid
plans/partial failures never publish streams. No shared engine edits or payload
reads. See arm64_stream_init.md, FACT-0035/0036, WARN-0012 and exact inventory.

### TASK-0013 â€” Genuine16-byte asset-list envelope (complete 2026-10-11)

Actual native expander/Win32 branch, checked count/footprint metadata, persistent
aliases and exact16-byte side-buffer read pass both files/3 ABIs.72 fixtures and
12 repeated ownership cycles each; cursors unchanged, reset/release baseline.
Independent full decoded SHA/Adler/prefix provenance; actual body/declaration
bytes unchanged. Finite reader does not certify async/refill loader. Full normal
Windows expander passes; bounded Android subset only. See arm64_assetlist.md,
FACT-0037/0038, FAIL-0007, WARN-0013 and exact inventory.

### TASK-0014 â€” Actual script-string pointer table (complete)

Both genuine files/HONOR/x86/x64,26 fixtures/12cycles, exact3440 disk/6880 native
zombie860slot tags, allaliases/elements/cursors and cleanup verified. No strings.
See arm64_string_table.md/FACT-0040/0041/WARN-0014; failures retained.

### Next3 under active autonomous objective

- TASK-0015 actual Load_XStringCustom raw bytes, bounded terminators/offsets/
  capacity,859 genuine inline strings, empty versus null, exact file/memory cursors.
- Actual37/1045 asset directory at proven next file offset, before dispatch.
- Real SL/MT intern/user4release with recursive locks/fixed32 ARM64 atomics,
  then integration. No fabricated IDs/VM callbacks. Game-owned graphics follow
  real engine/app/backend dependencies; no synthetic-frame success claims.

## Beyond

GSC string-intern/callable services, asset-stream expansion on ARM64 (the
Win32â†’native offset remap is the known x64 blocker, see fastfile_32_to_64.md),
server init, headless `platform: ready`. Each gets its own bounded task when
its predecessor lands.


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
