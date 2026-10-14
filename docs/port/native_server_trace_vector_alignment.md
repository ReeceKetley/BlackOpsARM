# Native server trace vector copies

This bounded batch repairs a nominal alignment contract in four copies in canonical `src/server/sv_world.cpp`. Actual full422 stopped at the analogous `CM_BoxLeafnums` bounds load, but no runtime misalignment at these four server sites has been observed. The next genuine production trace remains authoritative integration evidence; this report does not predict the next failure or claim trace/gameplay execution.

`SV_SightTracePointToEntity` copies `sightpointtrace_t::start[3]` and `end[3]` into a local `TraceExtents`. These are actual float arrays declared in `qcommon/cm_world.h`, initialized component-by-component by `SV_SightTracePoint`. The synchronous `CM_PointSightTraceToEntities` recursion borrows that live caller-owned clip and calls the entity trace; no source pointer is captured or retained. `SV_TracePoint` copies its public `const float *start/end` three-component inputs into its local `pointtrace_t::extents`, after the original vector assertions and initial collision/static-model checks. Neither interface promises eight-byte source alignment. Sources and destinations are distinct live storage.

Both destinations contain `hybrid_vector` with a four-float `float4::v[4]` array and explicit 16-byte alignment. The real `TraceExtents` constructor initializes all four lanes of start/end/invDelta. Four native `std::memcpy` calls copy exactly `3 * sizeof(float)` (12 bytes) into lanes0..2 and preserve lane3 exactly as the original 8-byte plus 4-byte operations do. The guard is `defined(__ANDROID__) && BO1_NATIVE_64BIT`; a guarded `<cstring>` include supplies the declaration. The original four Windows `_QWORD` loads and third-component expressions remain verbatim in their `#else` branches. No header, structure, alignment, capacity, assertion, callback, matrix transform, collision algorithm or lifetime changes are part of this batch.

A same-file inventory of `_QWORD`, `__int64`, `uint64_t`, `int64_t` and memcpy sites finds only these four wide vector copies. Other same-file start/end producers already use actual scalar component accesses and are unchanged. `CM_CalcTraceExtents`, bounds intersection and subsequent matrix/DObj/collision calls are unchanged. No synthetic trace fixture was added for four pure copies; no engine/device run or transfer was performed by this source owner.

Verification, all under `build-android-audit/native_server_trace_copies`:

- `native-command.json` and `native-tu.log`: exact current production ARM64 whole-TU command with UBSan and no recovery, redirected only to this evidence object; exit0 once.
- `x86` and `x64`: normal Windows whole-TU builds, native selector absent, both exit0 once. Saved commands/logs/results retain the existing compiler warnings.
- `scope.json`, `sv_world.cpp.before`, `source.diff`: exact CP1252/CRLF source readback, four 12-byte copies, Windows branch projection byte-for-byte identical to both the frozen before snapshot and read-only `BlackOps64/src/server/sv_world.cpp` oracle. The before/oracle SHA256 is `21E68E32EA4A7EBBD901E03CCD507348F4F398D9B2F7EF9C869C025CDE19636C`.
- `compile-results.json`: source hashes unchanged throughout all three compiles. No unrelated source was edited, no checks disabled and no protected checkout/assets/DXVK content modified.

Frozen source SHA256: `FBFD55532569B51E16337B05E740EC5282E13CE77F8B540BE7EEECD865A94D83` for `src/server/sv_world.cpp`.

Reproduction helpers: `build-android-audit/native_registry_closure/repair_server_trace_copies.py` preserves the original bytes and verifies the bounded branch delta; `build_server_trace_copies.py` runs the three whole-TU checks. The repair helper is intentionally single-use and refuses an existing evidence directory. Reports/evidence JSON use UTF-8; canonical source remains CP1252/CRLF. Root owns production integration, Ledger and current STATE updates. No commit or push occurred.
