# Native actor transition member accesses

SDK466/stage38 actual SERVER execution fails at actor_state.cpp:378 while the simplifier reads transition commands through eSubState. Six native-only typed member accesses now preserve the declared transition and state-stack owners. actor_state.cpp is frozen and released for SDK467/stage39 integration; runtime success after this repair is not claimed here.

## Actual failure

The launch1-only log reports `index 9 out of bounds for type 'ai_substate_t[6]'` at actor_state.cpp:378:40. Matching crash1 records PID32211/tid32387 SERVER and every engine frame has BuildId8092ab8eae41ad27afa232d6fcb62dd80ec163d2. The stack is Actor_SimplifyStateTransitions → Actor_PushState → G_ScriptedAnim_Start_SP → GScr_AnimScripted_SP → script VM/server frame processing. Older appended crash records have different PIDs and BuildIds and are excluded from the saved matched excerpt.

## Declared owners and exact mapping

actor.h declares eState[6], eSubState[6], stateLevel, iStateTime, preThinkTime, then StateTransitions[13]. ai_transition_cmd_t is exactly eight bytes: four-byte eTransition followed by four-byte eState. No header or owner layout changes are made.

For n=transitionCount, the original simplifier addresses map as follows:

| Original expression | Native typed owner |
| --- | --- |
| eSubState[2*n+5] command read/write | StateTransitions[n-2].eTransition |
| *(&iStateTime+2*n) command read | StateTransitions[n-1].eTransition |
| eSubState[2*n+6] state write | StateTransitions[n-2].eState |
| *(&preThinkTime+2*n) state read | StateTransitions[n-1].eState |

Two immediate POP-family expressions anchored at legsAnimDuration map to eState[stateLevel-1]: the next-state argument passed by Actor_ThinkStateTransitions and the return from Actor_GetNextPopedState. Their existing positive-stateLevel gates remain. Four guarded substitutions implement these six accesses under BO1_NATIVE_64BIT && !defined(_WIN32); original expressions remain in Windows branches.

The simplifier still reads the penultimate/latest commands in order, checks the same command range, warns and keeps one transition for dead actors, consults the same 4x4 canonical rule table, returns on CANONICAL, replaces penultimate command and state with the same newest state for nonzero reductions, decrements count once, and subtracts two for NONE. SET/PUSH/POP producers, simulated stack updates, superseding-state logic, callbacks, callback ordering and assertions remain exact. No clamping, enlarged array, dummy transition, assertion bypass, count or rule changes. The earlier direct bool pfnStart dispatch and its Windows cast branch remain exact.

## Proportional verification

- Three whole actor_state.cpp TU compiles pass with actual ARM64 engine flags and Windows x86/x64 comparison flags. Eight preprocess runs pass: native before/after plus each Windows before/after/read-only BlackOps64 oracle.
- All19 actor_state bodies are compared. The three native changed bodies differ only by the six exact member substitutions;16 remain exact including the prior bool callback fix. All76 Windows before/oracle body comparisons pass. Only generated source/line diagnostic metadata and whitespace are normalized.
- Three additional owner-layout TUs compile against the real actor.h declarations. constexpr offset equalities prove all four simplifier addresses at every count2..13 and both POP addresses at every level1..5, along with enum widths, command field offsets and declared array sizes.
- A Windows x64 host fixture executes nine source-extracted producer/simplifier/POP bodies using the exact native member clauses and real actor_s declaration.91 focused checks pass: SET+SET collapse, SET+PUSH canonical retention, PUSH+POP cancellation, POP+PUSH replacement, superseding-state pop/push behavior, disallowed pushes, previous state at levels1..5/default, POP consumer next/previous state arguments, dead-actor warning/retained transition, adjacent substates/timestamps/legs-duration and outer canaries. The fixture supplies recording callback dependencies, a canonical source-extracted visitor constructor, and unused host dependencies that abort if called. It does not run the Android engine or claim a device sanitizer execution.
- Reversing only the four guarded substitutions restores all saved-before bytes. CP1252 high bytes, BOM status and CRLF endings are preserved. Frozen source hash matches final verification.

Initial fixture link, dependency declaration and array-size harness failures are preserved in separate logs/results. They arose from header static NetField table relocations and actor_s visitor construction. Only the harness was corrected; production source remained unchanged. No broad AI simulation tests were added.

## Frozen evidence and runtime limit

`src/game/actor_state.cpp` SHA256: `8D6EA955F3CEB4CCE952FA0A3DE2C5E68714014D341A10D89ADE075EBD5AE7C6`.

Evidence: build-android-audit/native_actor_transition_member_access1 contains saved-before bytes, exact substitutions/diff, source inverse manifest, three production objects/eight PP outputs, per-body comparisons, three real-header owner-layout compiles, source-extracted fixture/logs, matched actual failure excerpts and final-audit-results.json. This report was written to a UTF8 temporary file and verified before publication.

SDK466 proves the original failure during real server scripted-actor transition processing. Root must integrate SDK467/stage39 and perform the next actual launch. Conditional POP/dead/superseding paths are statically and fixture verified but not separately claimed on the tablet. No master engine build, packaging, ADB/device action, provider/assets/config, Ledger/STATE edits, commit or push occurred in this source-owned task.
