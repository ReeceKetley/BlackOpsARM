# Native snapshot score order

2026-10-14. Frozen/released source: `src/cgame_mp/cg_snapshot_mp.cpp`, `src/cgame_mp/cg_scoreboard_mp.cpp`, and the single authorized `GetClientInPlace` consumer in `src/universal/com_expressions_eval.cpp`. Root owns SDK450 integration and observation. No runtime/gameplay success claim follows from these compiler checks.

## Actual failure and intended storage

SDK449/stage21 engine BuildId `798e8ed200c05a1cbbdb951ac44fabce5affb11b` passes the preceding FX camera repair and reaches `CG_SetNextSnap`. Nonrecovering UBSan stops at `cg_snapshot_mp.cpp:888:13`, `index 4 out of bounds for type 'int[4]'`. The old expression writes `cgameGlob->teamPlayers[ci->score.place + 3]`. Index four means the actual client's place is one, matching the canonical producer contract.

Evidence: [sdk-native-entry449-follow1-only.log](../../build-android-audit/native_registry_closure/sdk-native-entry449-follow1-only.log) and the matching BuildId segment in [sdk-native-launch449-follow1-crash.txt](../../build-android-audit/native_registry_closure/sdk-native-launch449-follow1-crash.txt). Older crash segments do not establish this run's behavior.

`cg_local_mp.h` declares `int teamPlayers[4]` followed immediately by `int scoreOrder[32]`. The old indexing expresses a physical write into the next member through an out-of-bounds access to the team-count member. Rank one belongs to `scoreOrder[0]`; rank 32 belongs to `scoreOrder[31]`. The scoreboard reader and expression reader used the same cross-array alias.

## Canonical rank and snapshot contract

[producer-consumer-contract.txt](../../build-android-audit/native_snapshot_score_order1/producer-consumer-contract.txt) records the actual source excerpts and line numbers.

- `com_maxclients` is registered with bounds 1 through 32; game initialization assigns it to `level.maxclients`. `level.sortedClients` has 32 elements. `CalculateRanks` gathers every connected session, including spectators, sorts them, and assigns `sess.cs.score.place = ia + 1` for each. `ClientConnect` and `ClientBegin` both call `CalculateRanks`; score/team updates also invoke the producer. The only assignment to the server `score.place` in the source audit is this one-based assignment.
- `G_GetClientState` returns the canonical session `cs`; `SV_BuildClientSnapshot` copies that `clientState_s` into snapshot storage. The archived path likewise copies `G_GetClientStateLocal` into the canonical client-state record and calls `MSG_WriteDeltaClient`. The unchanged wire field declaration carries `score.place` as six bits, sufficient for 32. No serialization alteration is needed.
- `CL_DeltaClient` reads into the actual parse-client ring or copies the previous record for an unchanged client; `CL_GetSnapshot` copies those records to `snapshot->clients`. `CG_SetNextSnap` selects `clientState = &snap->clients[num]`, selects the matching `ci`, and earlier sets `ci->clientNum = clientState->clientIndex`.
- `CG_CopyScoreFromClientState` copies the producer's place when outside killcam. During killcam it deliberately preserves the existing `ci->score`, including the previously assigned rank; the repair continues to use `ci->score.place`, not a different snapshot rank. `CG_TransitionKillcam` remains before the score-copy loop. No copy/getter/callback/refcount order changes.
- `UI_GetScoreAtRank` already returns null outside ranks 1 through 32, then retains its client-number and `infoValid` checks. `GetClientInPlace` already asserts that `place - 1` indexes 32 clients and retains its default -1/result publication semantics.

## Repair and bounds

Each source explicitly includes `platform/bo1_native_width.h`. Under `BO1_NATIVE_64BIT && !defined(_WIN32)`, the writer uses `scoreOrder[ci->score.place - 1]`, the scoreboard reader uses `scoreOrder[rank - 1]`, and the expression reader uses `scoreOrder[place - 1]`. Each legacy branch retains its exact original expression. No global X64 selection was added.

The native writer first checks `ci->score.place < 1 || ci->score.place > 32`, reporting the actual invalid place through `Assert_MyHandler` and the existing `__debugbreak` convention. This checks the one-based contract before subtraction/indexing, without clamping, fabricating rank zero, skipping score publication, or changing killcam score retention. An invalid producer/retained rank remains an explicit failure.

`scoreOrder` still initializes every slot to client sentinel 32 before the client loop. `CG_UpdateScoreData` still consumes the rank table, counts valid clients, then updates the four team counters; `CG_UpdateMatchScoreboard` remains after it. Scores, team counters, client sentinels, arrays, structure declarations and serialized snapshot data remain unchanged.

## Actual compiler layout proof

The compile-only [layout.cpp](../../build-android-audit/native_snapshot_score_order1/layout.cpp) includes the actual project header and proves `sizeof(int) == 4`, the exact 4/32 array extents and 16/128 byte sizes, and `offsetof(scoreOrder) == offsetof(teamPlayers) + sizeof(teamPlayers)`. A constexpr assertion checks all 32 ranks: the intended physical address of the old flattened offset exactly equals the new canonical member offset. Compilation passes on Android ARM64 and Windows x86/x64.

| ABI | teamPlayers offset | scoreOrder offset |
| --- | ---: | ---: |
| Android ARM64 | 394332 | 394348 |
| Windows x86 | 387656 | 387672 |
| Windows x64 | 394332 | 394348 |

Compiler record-layout logs retain these measured offsets. Every native array index is 0 through 31; the highest accessed scalar begins at byte 124 and ends at byte 128 of the actual 128-byte member. The repair gives the compiler the correct legal object extent without resizing either member or changing ABI.

## Bounded family audit

[teamPlayers-all-src-audit.txt](../../build-android-audit/native_snapshot_score_order1/teamPlayers-all-src-audit.txt) preserves the all-source search. It found exactly the three active rank-based cross-member consumers now repaired. The expression consumer was discovered during this audit and added only after root explicitly extended ownership.

Remaining active `teamPlayers` accesses are team counters: literal indexes 0 through 3, snapshot counting by `clientinfo.team`, and the scoreboard banner's team argument. The canonical enum defines FREE/AXIS/ALLIES/SPECTATOR as 0/1/2/3. The scoreboard dispatch normalizes its first team to 1 or 2 and passes the other literals 0/1/2/3; the banner receives the same argument. SP HUD reads AXIS/ALLIES/FREE and explicitly leaves its unported fifth-team case as a comment. The three legacy expressions remain in Windows branches only; no other active native rank-based cross-array expression remains.

## Validation and freeze

[check-results.json](../../build-android-audit/native_snapshot_score_order1/check-results.json), [source-manifest.json](../../build-android-audit/native_snapshot_score_order1/source-manifest.json) and [native_snapshot_score_order1](../../build-android-audit/native_snapshot_score_order1) retain all commands, compiler logs, before snapshots, diffs, preprocessing outputs and layout proof.

- Nine whole translation units pass: three actual Android production commands from `native_engine_entry1/compile_commands.json` and six Windows x86/x64 comparisons. Android commands only redirect output objects to this dedicated evidence directory; NDK30/API28/O0/debug/PIC/instrumentation/UBSan/nonrecovery remain intact.
- Six Android preprocessing runs pass. Complete selected bodies compare exactly after only the three stated member/index substitutions and removal of the newly added writer bounds diagnostic. `CG_CopyScoreFromClientState`, `CG_CopyUIVisibilityBitsFomClientState`, and `CG_UpdateScoreData` are unchanged. This verifies operation/callback order and score/UI retention through the actual preprocessed native code.
- Eighteen Windows preprocessing runs pass: before/after/read-only BlackOps64 for all three files and both architectures. All six selected full function bodies match both before and oracle byte-for-byte, without any diagnostic metadata normalization; [windows-raw-body-comparisons.json](../../build-android-audit/native_snapshot_score_order1/windows-raw-body-comparisons.json) records this stricter result.
- Three actual-header layout proof compilations pass. CP1252 high bytes and every preexisting bare-LF ending are preserved; inserted source lines use CRLF. Sources remained unchanged during the checks and audit.
- No device/GPU fixture, shared-root build, Gradle/asset/config change, or runtime-provider edit was made by this owner. The first checker run passes; there are no discarded failed validation results.

| Frozen source | SHA256 | Bytes |
| --- | --- | ---: |
| cg_snapshot_mp.cpp | `9F41519ABF92462797DB8932B05CEC34AFCFE3F71A437CD727B30541A5B104C6` | 78505 |
| cg_scoreboard_mp.cpp | `BD6090A716A6D514458217F6B63FEA5981693A5CAC45A013DE927A09469DDECF` | 80041 |
| com_expressions_eval.cpp | `D143AF91A7A9200F863D711EA9048D03054105623F9290AC93F2B80F4D8B3480` | 528005 |

The real SDK450 execution must confirm progress beyond this snapshot boundary. Stable rendered gameplay and meaningful gameplay performance remain root's unfinished goal.
