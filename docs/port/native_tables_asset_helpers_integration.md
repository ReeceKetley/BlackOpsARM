# Native tables and asset helper batch

Seven canonical owners are selected as one production batch: `bg_weapons_attachment.cpp`,
`bg_misctables.cpp`, `bg_vehicles_mp.cpp`, `com_encode.cpp`, `CurveManager.cpp`,
`g_weapon_load_obj.cpp`, and `g_actor_prone.cpp`. The latter vehicle-slot and curve-manager
owners require no algorithm changes. Production/Windows build evidence is maintained
by the integrating root agent.

Android-only corrections retain the original algorithms and Windows branches:

- Vehicle configuration offsets after the first runtime pointer map to native
  `vehicle_info_t` member anchors. Pointer-width growth and graph-pointer alignment
  are handled explicitly. Static assertions verify the unaffected prefix and tail
  group relationships. The traction callback uses the supplied native field offset.
- Attachment acquisition uses an actual `XAssetHeader` local and its string-table
  member, rather than aliasing a pointer local as a union.
- Weapon accuracy graphs address actual two-float rows; reject empty/out-of-capacity
  counts, oversize formatted paths, and short real file reads. All failure branches
  close the opened file. The original rejection of a sixteenth pair remains.
- Actor prone initialization stores the three actual bool members individually.
- yEnc rejects unusable/negative destination lengths before writing; original
  byte transformation and escaping remain unchanged.

## Focused HONOR execution

`tools/port/tests/native_tables_asset_helpers_tests.cpp` directly links the canonical
vehicle callback/table, weapon graph parser, yEnc codec, `q_parse.cpp` tokenizer, and
matching Android legacy CRT. The isolated filesystem provider uses real `FILE`,
`fread`, and `ftruncate` calls. Actual file truncation after length measurement
exercises both short-header and short-body failures. Thread selection compares the
current pthread with the fixture's actual owner. Unrelated vehicle callback services
abort if reached; they are not successful replacements.

Build from BlackOpsARM with:

```powershell
./tools/port/tests/build-native-tables-asset-helpers.ps1
```

NDK 30.0.16248370, Android API 28, ARM64, C++20, signed char, native MSVC spelling
adapter, UBSan with no recovery, static libc++, section garbage collection.
`BO1_STARTUP_SERVICES=1` selects the production tokenizer/CRT declaration path.

HONOR `AKWJ9X5116G04618` completed the executable with exit 0 and
`TABLES_ASSET_HELPERS PASS checks=49`. Evidence:

- `build-android-audit/native_tables_asset_helpers/build4.log`
- `build-android-audit/native_tables_asset_helpers/honor-run1.log`

Checks cover native button/graph/scalar parameter offsets; actual traction and
scaled-mass callback writes with whole-object byte comparisons and canaries;
three-pair parsing with the real tokenizer; zero/partial/capacity failures and
balanced parse sessions; real valid/truncated graph files with handle retirement;
oversize path rejection before open; complete 256-byte yEnc round-trip and
zero/one/two/three-byte output boundaries plus decode capacity failures.

Scope excludes initialized production filesystem/hunk consumers, real retail
graphs/string tables, attachment publication, graph-pointer callback loading,
curve lifecycle/concurrency, actor consumers, Com_Init, and gameplay. The fixture
does not establish those milestones. Only the new executable was transferred;
existing assets and protected checkouts were untouched.
