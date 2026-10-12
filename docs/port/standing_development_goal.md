# BlackOpsARM standing development goal — 12 October 2026

Continue successive bounded milestones autonomously from verified TASK31/f523687
toward a genuine native Android ARM64 BO1/T5 Zombies engine. Milestone completion
is not a reason to wait for another prompt. Correctness and preservation beat speed.
Use real production engine code, genuine assets and increasingly representative
integration; never fabricate initialization, graphics, menus or gameplay.

Current strategic path: asset deserialization/fixups/ownership; game asset and core
engine initialization; real script/server runtime; Zombies map; existing DXVK-Native
D3D9-to-Vulkan integration; native input/Xbox controller; audio/lifecycle/save/resume;
performance and memory. Dependencies may justify another order. Prefer integrating
a ready production boundary over endlessly expanding isolated fixtures.

All engine work belongs in BlackOpsARM/android-arm64. BlackOps64 remains read-only,
including outputs. Root Ledger/unrelated projects stay read-only. Original retail
D:/Games/Call of Duty - Black Ops files are read-only and never committed. Ordinary
needed asset transfers to an ADB-discovered authorized HONOR are allowed: only
required files in designated application/test storage with provenance/checksums.
Protect HONOR4GB RAM from uncontrolled workloads. SDK D:/Android/Sdk and existing
NDK/CMake/Gradle/ADB configurations remain authoritative in current Ledger.

Use DXVK-Native when graphics is ready; inspect/build/integrate it rather than
rewriting the renderer. Independent repository changes require a justified tested
reason, isolation and documentation. No such change is needed at the present gate.

Preserve Windows x86/x64 behavior, disk widths/four-byte handles and explicit native
alignment/lifetimes. No production fixtures or silent-success shims, disabled
checks, unsafe casts just for tests, speculative broad changes or original-asset
modification. Preserve passing/failing evidence. No destructive Git/history rewrite,
merge or push. Prior explicit user permission for local verified ARM64 checkpoints
continues (DEC45/47); the standing no-commit-without-authorization condition is met.

For each gate inspect latest relevant code/Ledger, define acceptance, delegate
independent reviews where useful, implement minimally under lead ownership,
validate Windows and HONOR, record exact stage/evidence/reproduction, update Ledger
and roadmap, then move to the next dependency. Standalone/production-leaf/integrated
application/playable gameplay are distinct evidence stages.

Stop only for fundamental behavior-changing architecture choices, destructive or
irreversible action, unavailable resources/authorization, uncontained serious
safety/correctness risk, unstable environment, or frequent compaction/context
loss making a fresh-agent handoff preferable. Investigate blockers and pursue an
independent useful task when safe. Before intentional handoff record exact HEAD,
capabilities, pending changes/test ownership, blockers/results and next three gates.
No deadline or fake stretch-goal result: headless startup/assets/loading screen/menu/
engine-owned graphics/simulation are aspirations. Record smaller verified progress.


User scope clarification (DEC55): Dead Ops Arcade is a known limitation and is
out of scope. Pentagon TechniqueSet comma stub matches x64; ignore its empty
authored payload, preserve actual existing/default reference resolution.
