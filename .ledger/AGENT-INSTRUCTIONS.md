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

Run `ledger context "<your current task>"`. Use current ACTIVE knowledge. Do not assume historical entries remain valid.

## Before major architectural decisions

Run `ledger search "<topic>"` and check decisions, failures, warnings, and experiments.

## During work

Record useful discoveries with `ledger add fact|decision|experiment|failure|hypothesis|warning|task|note "..."`. Avoid trivial details.

When evidence disproves a belief, use `ledger invalidate <ID> --because <ID>` or `ledger supersede <OLD_ID> <NEW_ID>`. Preserve history.

## At the end of significant work

Record important discoveries, decisions, outcomes, failures, unresolved questions, and changes in project state. Run `ledger status` and update STATE.md when needed.
