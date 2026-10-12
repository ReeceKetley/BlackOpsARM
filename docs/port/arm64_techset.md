# TASK38: genuine TechniqueSet deserialization and missing default dependency

TASK38 is **blocked before registration**. The actual first zombie TechniqueSet
is a reference stub: its genuine 20-byte name begins with a comma. The unchanged
production `DB_LinkXAssetEntry` strips that marker and, because the referenced
TechniqueSet is absent from this bounded registry, calls `DB_CreateDefaultEntry`.
The fixture explicitly aborts there. It does not supply a default, rename the
asset, skip it, or claim successful registration.

The complete retail streaming oracle independently verifies the raw and decoded
SHA/count/Adler provenance and the exact 66226-byte prefix. Header 65678..66206 is
528 bytes, SHA `533086489d129a1222717dac632c2bda2c68ffce33faefbe4f90c7866f44f614`;
all 130 technique tags and scalar fields are zero. Name 66206..66226 is 20 bytes,
SHA `4b5cba3bbecffc1cb6788a25f63ef57c76679644fdd88064bf9aec4670ee4ad9`,
Adler 1243743876. Next directory asset is XModel type5 at 66226. No asset bytes are
stored in Git; native decoded prefixes remain ignored scratch outputs.

The fixture executes the actual `Load_MaterialTechniqueSetPtr(false)` against
the widened directory slot1, actual header expansion, `Load_XString`, all130
actual null pointer-loop iterations, and actual asset wrapper/Add/fullLink up
to the explicit missing default endpoint. Immediately before abort the test-only
observer verifies source66226, TEMP extent528/1056, null130, comma marker, scopes
0/2, exactly the preceding StringTable registered, and all1594 zombie table
queries with zero misses/disagreements. Expected Windows exit3 and HONOR exit134
are checked exactly. This is diagnostic evidence, not rollback/recovery or a
successful full wrapper: root Pop, returned persistent TechniqueSet, TEMP
overwrite survival and type7 cleanup have not executed.

Canonical `MaterialTechniqueSet` moved once to `db_techset_types.h` without
changing its fields; size528/1056 and offsets0/4-or8/6-or10/8-or16 are asserted.
Canonical pool policy2048/mod3072 moved once to `db_asset_pool.h`. New standalone
gate selects real slot7 alloc/free/name/size dispatch; old selections retain
their prior contents. Actual loader, expander, registry, clone/free/size bodies
remain byte-identical after newline normalization. Normal Windows builds retain
their policy and pragma packing. No shader/declaration path is reached.

Implementation-agent evidence (preserved; root-final checks below supersede run labels):

- `techset_host_final`: Windows x86/x64 compile; `techset_O0_final` and
  `techset_O3_final`: HONOR ABI with assertions and UBSan. Original legacy host
  compiler setting remains `/O2` with assertions, matching prior gates.
- `techset_normal_pass1`: eight normal gates-disabled Windows translation units
  per ABI passed. No production source changed after that normal compile.
- `techset_prereq_O0_final` and `techset_prereq_O3_final`: all TASK36 prerequisites
  (215 cases/12 cycles/all1994 aggregate queries) and complete TASK37 code zone
  (4 cycles,37 assets,148 actual Add calls, full49593 bytes, reverse cleanup and
 32752 entry baseline per ABI) pass on Windows x86/x64 and HONOR.
- `techset_blocker_O0_final` and `techset_blocker_O3_final`: strict all3ABI
  expected-refusal collector; `genuine_registration_passed` remains false.
- `techset_full_request_final_failure2`: original requested success collector
  fails at Windows x86 actual `DB_CreateDefaultEntry` exit3, after retained
  prerequisite and whole-zone success. This failure is intentional evidence of
  the unfulfilled registration boundary.
- `techset_source_final.json`: exact actual function-body/declaration/policy
  preservation proof; `techset_protected_final.json`: all18483 protected paths
  unchanged. Root separately verifies protected Git/checkpoint state.

Failed and superseded records are retained. Hostpass1 used the wrong output root
and stopped at reference generation; hostpass2 had the nested-main macro collision.
The first scratch oracle test supplied a directory reference where the original
inflate schema was needed (`KeyError xFileFields`, no source mutation). A direct
pass4 execution incorrectly supplied prefix length as full decoded size and
failed validated envelope metadata; full size108009041 corrected it. `techset_run1`
started before x64 compilation completed; `techset_run2` exposed the old fixture's
two-type size expectation. `techset_prereq_O3_run1` exposed the observer being
called by the historical RawFile default refusal control; the callback is now
type7-only. `techset_blocker_O0_run1` reported the prior aggregate query count1994;
final observer asserts and reports the zombie's actual1594. Late
`techset_full_request_final_failure` correctly rejected an old build manifest
after builder manifest coverage was expanded. Passing superseded builds/runs
remain as historical records; final labels above are authoritative.

Reproduce from this ARM worktree in PowerShell, with fresh outputs:

```powershell
tools/port/build_techset_host.ps1 -OutputDirectory build-android-audit/techset_host_new
tools/port/build_techset_arm64.ps1 -BuildDirectory build-android-audit/techset_O0_new -Optimization O0
tools/port/build_techset_arm64.ps1 -BuildDirectory build-android-audit/techset_O3_new -Optimization O3
tools/port/build_stringtable_owner_windows_compile.ps1 -OutputDirectory build-android-audit/techset_normal_new
```

Use the bundled Python recorded in the handoff with
`run_techset_prerequisites.py --host-build ... --android-build ... --output ...`
for each optimization, then `collect_techset_blocker.py` with the same build
arguments, `--prior-results <matching prerequisite output>/results.json` and a
fresh `--output`. `run_techset_gate.py` with those build arguments is expected
to fail at the missing genuine default. Rediscover HONOR with `adb devices -l`.

Next dependency is genuine bootstrap load order. Independent read-only research
found a real nonstub type7 `default` as asset0 in retail code_post_gfx and
code_post_gfx_mp. Its own technique dependencies must be validated and loaded
before certifying the actual default clone/interning path or referenced-name
resolution. That predecessor graph was not integrated in TASK38. Core startup,
async/refill/error recovery, actual graphics, device16KB/S25 and gameplay remain
outside this proof. HEAD at handoff is5ae358a; implementation agent made no commit
or Ledger edit. Root owns continuity and checkpointing.


## Root-final checkpoint and x64 reference comparison

The exact asset is `,mc_l_sm_r0c0n0s0x0` in `zombie_pentagon.ff`. Read-only stable
BlackOps64 `db_registry.cpp` lines2648-2651 strips the comma; lines2741-2768 return
an existing matching header or call `DB_CreateDefaultEntry`. Lines2031-2061 require
a real default provider, clone it and intern the requested name with user4 ownership.
The same genuine name is decoded by the standalone x64 probe. This is source
comparison and isolated native execution, not a fresh full Windows engine launch
or proof of its live registry contents. No earlier exact-name provider has yet
been proven without sequentially loading unknown predecessor bodies. The canonical
first postgfx `default` is independently proven. User excludes Dead Ops Arcade
(DEC55); this Pentagon reference remains a normal Zombies dependency.

Root-final authoritative builds are `techset_host_root_final`,
`techset_O0_root_final`, `techset_O3_root_final`; matching
`techset_prereq_{O0,O3}_root_final` and `techset_blocker_{O0,O3}_root_final` pass
all three ABIs. Prior complete code zone registration remains successful; TechniqueSet
registration remains false. Root checks exact prefix size before allocating/reading,
ASCII comma marker and complete name Adler, and removes unreachable hypothetical
success/TEMP/cleanup branches. Unexpected return before a real dependency is available
fails explicitly. Collector matches executable SHA to prerequisite and pushed device
binary, accepts one exact typed diagnostic record, rejects six malformed variants,
and verifies the terminal unavailable endpoint with no assertion/UBSan diagnostic.

`techset_source_root_final.json` verifies unchanged actual production bodies and
canonical declaration/pool policy. `techset_protected_root_final.json` verifies all
18483 protected paths and exact reference Git state. Root changes after normal
Windows compilation affect fixtures/tools/docs only. Original801 inventory rows
are retained with exact hashes;226 root records added (1027 rows, plus inventory).
Decoded prefixes, retail binaries and compiled probes remain ignored scratch only.

Next work loads the genuine bootstrap default graph under the actual renderer-off
resource branch, retains its owner, and integrates real SL/MT/default clone naming.
Do not repair the stub into an authored empty asset or silently drop its pointer.
TASK37 SP zone proof is not certification of actual Com_InitCodeXAssets, which loads
code_pre_gfx_mp before graphics bootstrap. Core engine startup remains ahead.
