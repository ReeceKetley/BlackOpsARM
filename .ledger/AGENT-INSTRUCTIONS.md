# Ledger agent instructions

## Orientation

Run Ledger from BlackOpsARM/. Read GOAL.md and STATE.md, then run
ledger context "<current task>" at the start of significant work. Context search
can return older ACTIVE milestones: compare their dates, scope and evidence
against current STATE and explicit user instructions.

This ledger owns Android progress. ../.ledger owns shared T5/Windows history;
consult relevant parent evidence read-only. IDs are independent; qualify parent
references with their ledger path. Never copy or rewrite the parent database.

## Recording work

Use ledger search/show before major architecture decisions. Record significant
decisions, findings, tests, failures and unresolved work with ledger add.
Use supersede/invalidate only when evidence warrants it; preserve history.
Do not mark historical verification false merely because development advanced.

Keep STATE as one concise current handoff: verified behavior, unfinished edits,
actual blockers, next actions, reproduction/evidence and asset/device locations.
Update GOAL only when the objective changes. Put detailed results in docs/port
reports; preserve failed logs and distinguish fixtures from production execution.
At handoff run ledger status and ledger doctor.

## Historical archive

The verbatim pre-cleanup instructions, state, goal and worktree AGENTS.md are in
[archive/handoff-cleanup-20261012](archive/handoff-cleanup-20261012/README.md).
Its manifest records SHA256 and sizes. Historical scope restrictions, commit
permissions, rotation stops and next-task recommendations describe their original
sessions; they do not override current user direction. Keep all database entries,
JSONL history, evidence and older archives.
