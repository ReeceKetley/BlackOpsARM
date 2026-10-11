# Approved milestone — Native FastFile structural front end

User approved this milestone and local checkpoint commits after verified gates;
no push. DEC-0026 supersedes the earlier no-commit restriction for these local
milestone checkpoints. Lead owns integration and device tests; three read-only
audits cover format/provenance, adversarial cursor/ownership behavior and the
production string-pool stretch. Windows/DXVK/root Ledger stay unchanged.

## Required finish line

A reproducible HONOR ARM64 probe and Windows x86/x64 reference pass for both
genuine code_pre_gfx.ff and zombie_pentagon.ff through:

1. Actual block4 script-string pointer-table reservation and native expansion,
   including absent/null/zero and encoded-tag cases (TASK-0014).
2. Bounded table-directed string-byte loading, with terminators, byte lengths,
   content/provenance and disk/native cursor/alias checks. Raw strings must be
   explicitly identified as raw, rather than substituted production string IDs.
3. Actual asset-directory expansion:37 and1045 typed records respectively,
   retaining disk tags without dispatching assets or loading their payloads.

Each boundary uses genuine complete-decode provenance and original layouts,
checked arithmetic/source/allocation bounds, repeated detach/free/named release,
Windows behavior parity and adversarial fixtures. Freeze machine-readable results,
reference evidence and failed diagnostics. Retain original assets outside Git/APK,
avoid duplicate full decoded buffers and keep the HONOR4GB memory bound practical.
Use the existing finite validated reader boundary unless real async/refill work
is separately justified; don't claim it is the full engine loader.

## Conditional stretch

Exercise real SL_Init/SL_GetString(user4,server)/release with production MT memory
tree, actual synchronization and fixed32 ARM64 refcount atomics. This is additional
platform work: current code reaches critical sections/debug pools and Windows
atomics. Attempt it only through minimal isolated changes preserving Windows and
four-byte records. If not feasible safely, record the exact dependency/blocker
and finish the required structural milestone without fabricated IDs or success
stubs. The stretch is not required to claim the structural milestone complete.

## Boundaries and checkpoints

Stop before asset dispatch, asset payload deserialization/fixups, zone/game init,
rendering/audio/input/backend integration or broad allocator/engine rewrites.
Necessary string-offset resolution must validate mapping and bounds; don't turn
unresolved tags into null or silently clamp/drop inconsistent counts.

Checkpoint the already verified TASK-0011/0012/0013 work first. Then implement,
verify and record one bounded successor at a time, making local commits only
after that gate passes and protected/evidence hashes are verified. Keep ignored
scratch/SQLite/binaries and all previous diagnostics. Never push. If an actual
blocker prevents required progress, preserve a clear reproducible handoff and
report it honestly. Stop when required work and any safe stretch attempt finish;
do not invent unrelated overnight work.

Keep PC awake/Codex running and HONOR USB/debugging authorised for local work.
An active goal tracks continuation; no standalone recurring job was requested.
Current sources/Ledger/handoff identify the next bounded step after every gate.
