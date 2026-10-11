# Android checkpoint through bounded deflate

The user authorized this local checkpoint after TASK-0010 on 2026-10-11.
Branch: android-arm64. Parent: 64c5d3c5f8126a2e3a7b09dffee3a0f1f957d939.
Use `git log -1 --format=fuller` to obtain this checkpoint's commit ID.

Includes the user's completed expression compilation/FastFile reader work and
the subsequent actual zone bootstrap and bounded full deflate gates: source,
native services, tests, reproducible instructions, Ledger and archived evidence.
Sixteen HONOR probes and MSVC x86/x64 regressions pass. code_pre_gfx.ff and
zombie_pentagon.ff decode fully on all three targets with independent oracle
counts/fields/checksums. Latest results and exact commands: arm64_inflate.md.
Existing compiled sources/headers match their archived hashes; no new build or
device test is needed for this documentation/commit step. Ledger doctor is healthy.

`evidence/android_checkpoint/pre_commit_inventory.json` records every staged
path, working-file hash, all earlier evidence hashes and protected checkout
snapshots. Evidence Git attributes disable text normalization and whitespace
checks for immutable diagnostic archives (including original CRLF). Source and
documentation whitespace checks remain active. Commit validation
checks staged/committed evidence bytes against working hashes, then verifies
protected snapshots and clean durable Android status. Post-commit results stay
in ignored `build-android-audit/checkpoint_post_commit.json` to avoid a second
commit merely to record the first ID.

Windows main76edfd5, its files/index/scratch, independent DXVK master0e271c6 and
root Ledger remain unchanged. Ignored build outputs, binaries, SQLite, SDK local
properties, original game assets and scratch remain local and retained. Original
game bytes are not committed or packaged. No push is authorized.

Next: TASK-0011 checked genuine XFile-to-zone allocation and stream lifecycle;
stop before assets/strings/async integration. Acceptance and remaining WARN-0010
limits are documented in arm64_inflate.md. This checkpoint request starts no new
porting work. Later commits/pushes require fresh user authorization.
