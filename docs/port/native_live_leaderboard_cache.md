# Native leaderboard cache access

Frozen 2026-10-13 after full408 reached actual `Live_InitPlatform` / `LB_Init`
and UBSan rejected the first negative index into `g_lbGlob.feederText[32]`.
Only existing `src/live/live_leaderboard.cpp` changed. The header, fixed stat
columns, serialization bodies, task policy, online feature gates, and Windows
reference checkout were not modified. No new production source or CMake
definition is required.

## Actual field mapping

The legacy feeder-relative expressions use a 15,520-byte 32-bit runtime cache
stride and walk backwards out of a 32-byte text member. They are not serialized
slots. Native ARM64 objects are `sizeof(Leaderboard)==16336` and
`sizeof(LbGlob)==264912`, so both the stride and enclosing offsets are invalid.

| Old feeder-relative offset for cache zero | Actual cache field | Initialization |
| --- | --- | --- |
| -233008 | `type` | row index 0 through 15 |
| -233004 | `offset` | 0 |
| -233000 | `curIdx` | 0 |
| -232996 | `forceRefresh` | true |
| -232988 | `lastUpdate` | 0 |
| -232984 | `filter` | 3 |
| -232980 | `lbTypeByResetPeriod` | `LB_RESETPERIOD_COUNT` (3) |

The native initialization helper operates on the actual `leaderboards[16]`
members and writes exactly these seven fields. It preserves existing cached
rows, result counts, updating/filter-change flags, vtables, other globals and
the genuine 32-byte feeder text buffer. Canonical `LB_Init` still performs
actual task clearing, structure registration, command registration, player
stat clearing and dvar registration in the original order.

Native refresh/update/count/selection/player-info/cursor/item-text consumers
now use checked typed cache access. Remaining legacy reads map `-232995` to
`isUpdating`, `-232968` to `totalResults`, and the old cache base `-248320` to the
actual cache object / its `userStats` member. Windows macros retain the original
raw expressions and original seven-write loop; both normal Windows whole-TU
compilation paths pass. No packing or assertion was weakened.

Row selection checks that the result object is the actual cache's `userStats`,
counts fit the 100-entry array, the offset is nonnegative, and the selected
absolute index lies in the populated cached interval and reported result range.
Malformed input calls the real fatal error and aborts if that callback returns.
Native rank sorting uses the actual 64-bit `m_rank` field and the correct qsort
callback signature, preserves the original zero-rank-last ordering and trimming,
and rejects an oversized/negative count. The old `_QWORD[4]` rank assumption
remains only in the original comparator used by Windows.

The equivalent DWORD task access at word 3878/3868 maps to
`numMatchesPlayed = playerLbRow.m_columns[0]`. The initial multiline replacement
missed that optional body; it was explicitly reported pending while the active
cache initialization remained verified. After full411 integration, the exact
function span was replaced and read back with an assertion that the typed
assignment occurs once and the legacy assignment remains only in the Windows
branch. The canonical request body publishes `ov->data = lb` after requesting
`lb->playerLbRow`, establishing the actual target type. Both source and target
fields have explicit four-byte static checks; the borrowed task data pointer
retains native width. No `BO1_LIVE` enabling or other online code was changed.
The current production runtime did not consume this disabled path.

The feeder's default-stat branch also previously crossed out of
`GlobalLbStat[3]` using a flattened index. Native display columns now index the
actual `LbStatPerLbGameMode[mode][column-5]` table and reject columns beyond that
mode's registered count. The fixture compares this mapping against the actual
canonical registration output for all eight game modes. Windows retains its
original expression. No independent leaderboard algorithm or substitute online
provider was introduced.

## Scoped verification

Evidence under `build-android-audit/native_registry_closure`:

- `live_leaderboard-cache-audit1-command.json` / log: actual FULL instrumented
  ARM64 whole-TU compilation passes, with defined/undefined symbol manifests.
- `leaderboard-cache-frozen-hashes.json`: source/header/fixture sizes and hashes;
  the header is recorded for ABI evidence and remains unchanged.
- `leaderboard-cache-fixture-build1-command.json` / log: standalone ARM64 UBSan
  fixture includes the actual canonical TU and calls the production helper and
  actual canonical stat registration/sorting bodies.
- `leaderboard-cache-fixture-run1.log`: real HONOR execution exit0. It verifies
  every byte outside the seven allowed fields remains unchanged across all
  sixteen initialized rows, enclosing canaries, actual eight mode/four global
  registries, cache-owner and populated-window bounds, 64-bit rank sorting and
  eight fatal-return subprocess guards. No backend success is manufactured.
- `build-android-audit/leaderboard-cache-windows1/{x86,x64}`: one normal Windows
  whole-TU comparison per ABI, no native selectors, both exit0. Exact commands,
  source manifests and result JSON are retained.

Rebuild the focused fixture by executing its saved JSON argv from the repo root.
Only its new 525,928-byte executable was transferred; no libraries or assets
were recopied. Actual tablet command:

```sh
/data/local/tmp/blackopsarm-leaderboard-cache-fixture
```

The production repair is ready for the next root engine startup. The fixture
proves local cache/ABI behavior, not completion of `Com_Init`, online service
readiness, DemonWare callbacks, or initialized recovery.

## Follow-up guarded matches-count compilation

The final follow-up source SHA256 is
`969857cf210f6888d714e8b0eae651b548b1882658203c8ea2d68d1810fb311e`, recorded in
`leaderboard-matches-count-frozen-hashes2.json` with unchanged header/declaration
ABI evidence. `live_leaderboard-matches-count-audit2` is a passing actual FULL
instrumented ARM64 whole-TU compilation. Normal Windows whole-TU comparison
`build-android-audit/leaderboard-matches-count-windows1/{x86,x64}` passes both
ABIs once, without native selectors.

The optional body is also compiled with `BO1_LIVE=1` in the isolated generated
`leaderboard-matches-count-guarded2.cpp`: it contains the actual function and
fatal helper extracted verbatim from this frozen source, actual engine type
headers, and the exact existing `dwStats.h` prototype. Guarded2 passes ARM64
compilation, including both four-byte field assertions. Its undefined-symbol
manifest retains genuine `dwReadStatsComplete` and `TaskManager_ClearTask`
dependencies, so no backend provider or fake completion implementation was
supplied. Exact argv, log, defined/undefined manifests and disassembly are saved.
The first guarded attempt (`guarded1`) additionally enabled broad `BO1_DW`
headers and failed on their existing incomplete declaration dependencies; that
failed log is retained. The successful isolated compile certifies this guarded
body, not an enabled whole online subsystem. No additional tablet execution or
production runtime was performed for this disabled-path follow-up.
