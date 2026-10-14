# Native actor state callback types

SDK458 actual frontend actor execution reaches an incompatible state-start callback dispatch. Three native-only typed-call corrections preserve the canonical state machine and complete the bounded AIFuncTable dispatch family. Sources are frozen and released for root's SDK459/stage31 integration; runtime success after repair is not claimed here.

## Actual failure and canonical callback

`native_registry_closure/sdk-native-entry458-launch1-only.log` reports actor_state.cpp:196 calling through `int (*)(actor_s *, ai_state_t)` at library offset `0x1238d44`. The same launch also reports the separate rb_tess particle bound failure. The matching crash buffer has actual PID7177/render tid7292; older appended records are not this run. Immutable stage30 ELF matches manifest SHA256 `3c78123ad802243e2688cf17b2e7daa3041cd8ca71c911bb488e8fe4206d1c00` and BuildId `f44c8fbf95434368f8f96f087ef8cc847ba93d55`. `llvm-addr2line` against that saved ELF identifies `Actor_ScriptedAnim_Start_SP`, actor_scripted.cpp:46. Its header and definition return bool; the SP path sets ent->s.lerp.eFlags2 bit2 and returns true, and its existing non-SP gate calls the loud canonical provider. This callback, its flags, its gate, and every registration remain unchanged.

## Exact native changes

`ai_funcs_t::pfnStart` and `pfnResume` are bool fastcall callbacks; pfnThink returns actor_think_result_t. actor_function_table.cpp registers four species tables of AIS_COUNT12 rows, with canonical functions already typed. Native compilation of the entire registration TU succeeds without function-pointer casts or declaration edits.

1. Actor_StartState calls the existing bool pfnStart directly. Its public int wrapper remains; Clang's production AST proves a bool CallExpr with precisely one IntegralCast to int at return. False becomes0 and true becomes1. stateLevel0 timestamp, pfnStart assertion, eState publication before the callback, and both exact callback arguments are unchanged.
2. Actor_Generic_Resume calls the same canonical pfnStart directly and returns bool directly. Its stateLevel/state-range/callback assertions and ePrevState argument are unchanged. It remains registered as pfnResume in the existing badplace, death, pain, negotiation, scripted/custom, dog and zombie-dog rows.
3. Actor_CallThink calls the declared enum pfnThink directly and assigns its enum result to the existing enum variable. No int conversion is required by its variable or public return type. The exact gjkcc prolog, m_gjkcc_input publication, callback point, epilog, pointer clearing, result assertion and final return are unchanged.

Each change is guarded by `BO1_NATIVE_64BIT && !defined(_WIN32)`, with one native-width header include per TU. The original cast remains in the Windows branch. No table, header, callback implementation, state logic, locks, events, math, pointer ownership, resource layout, global flag or sanitizer setting changes.

## Bounded closure and immediate consumers

The full src AIFuncTable search finds only these three incompatible active dispatches; the Android-preprocessed repaired TUs have zero int function-pointer casts at those table calls. Actor initialization already calls pfnStart directly; finish and suspend dispatch void callbacks directly, and Actor_ResumeState calls bool pfnResume directly. Table rows and all providers remain canonical and unchanged.

Actor_ThinkStateTransitions preserves SET finish/start/default-on-false, PUSH suspend/increment/start/resume-on-false, POP finish/pop-resume, transition count clearing and simulated-state copy. Actor_StartDefaultState retains the startSuccess assertion. Actor_ResumePopedState retains decrement-before-resume, success return and base-state fallback; Actor_ResumeState retains finish-on-false. Actor_Think retains its enum repeat loop and maximum-repeat assertion, all physics/state assertions and subsequent processing. These immediate consumers are included in unchanged-body proofs. The existing preserved legacy accesses inside those bodies were not expanded into unrelated work.

## Verification

- Nine whole-TU compiles pass: each of three changed sources on actual native ARM64 flags and Windows x86/x64 comparison flags. The unchanged registration TU adds one successful native compile.
- 24 preprocess runs pass: native before/after for each source plus Windows before/after/read-only BlackOps64 oracle for each source and architecture.
- Three native changed bodies are proven to differ only by the exact typed dispatch;117 other extracted actor bodies remain exact.240 Windows bodies match both saved-before and read-only oracle, yielding480 comparisons. Only generated source file/line diagnostic metadata and profile identifiers derived from source lines are normalized; predicates, calls, arguments, math, constants, assertions and ordering remain exact.
- Three production Clang AST checks pass: start bool→int wrapper promotion, resume bool→bool, and think enum→enum. Each dispatched callee expression has its exact canonical function-pointer type and no pointer cast.
- Reversing only the scoped include and guarded dispatch restores every original source byte. CP1252 high bytes, BOM status and CRLF are preserved, as are all preexisting native guards.

The early audit harness failures are preserved as check-results-initial-harness-failure.json, check-results-source-location-harness-failure.json and check-results-profile-name-harness-failure.json. They arose from expecting Clang preprocessing to erase __fastcall, and from unnormalized __FILE__/__LINE__ assertion/profile metadata after inserting guards. Source corrections did not change during those audit repairs. Final check-results.json, type-proof-results.json and final-audit-results.json all pass.

## Frozen hashes and evidence

- `src/game/actor_state.cpp`: `502F30AC401005800361A3F47017EEC177CECF68A97CECE9CD3FBE415DEBBA55`
- `src/game/actor_generic.cpp`: `F76B540B589AB59D710090FACC1DBECC7A2EC543F13F1ED481D48C33D34191F0`
- `src/game_mp/actor_mp.cpp`: `9F11E9B06CDF4049C3DF678406A194208B5410C821F846878E4F0F2D5DA84CE9`

Evidence: `build-android-audit/native_actor_state_callbacks1/` contains saved-before source bytes, byte manifests, exact diffs, native/Windows objects and full PP outputs, compile commands/logs, compiler ASTs, matching staged symbolization, full-src bounded consumer search and final results. This report is written and verified as UTF8 before publication.

## Runtime limits

SDK458 proves real frontend actors and the streaming adapter reached the 3D backend, then encounters the simultaneous actor-start/particle failures. Resume and think mismatches are the same statically established callback family; their execution in this launch is not separately claimed. Native repair still requires root's combined SDK459/stage31 actual launch. Complete visible3D and playable Zombies remain unproved. No master engine build, packaging, ADB, assets, Ledger/STATE edits, commit or push occurred in this source-owned task.
