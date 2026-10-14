# Native weapon shared-zero pointer array

The full429 diagnostic run proves that the bad reference predates map restart.
During initial `common_zombie` loading, `default_vehicle_weapon`, `syrette_sp`
and `zombie_knuckle_crack` resolve gun/world model arrays through the same baked
offset `800C23DF`. The block4 Win32 target is `C23DE`; the recorded native segment
is `w=C23DE+97 n=CD880+97`, an unexpanded region. The native array begins at
offset 841856; slot 8 contains `5F6D735F6C5F636D`, exactly the ASCII value in the
full428 restart crash. The adjacent words decode as model-name text.

This is a representation mismatch at shared blob reuse. A sixteen-element model
array occupies 64 bytes of original four-byte pointer slots but 128 bytes of native
slots. Generic offset mapping preserves the original unexpanded scalar/tag blob;
using that address as sixteen native pointers reads beyond its disk-sized zero
extent into the following name. The exact original scalar owner is not identified
by these diagnostics. The target is 2mod4, consistent with sharing a prior tag
array rather than an independently aligned model array. No unload or name-getter
change is warranted by the evidence.

The actual Android gun/world non-inline loader now uses a typed pointer-array
offset conversion. The native stream owner implements two defined cases:

- A recorded pointer array already expanded from four-byte to native slots
  resolves directly to its existing native extent, preserving nonzero values.
- An exact unexpanded span with all 64 bytes of the requested sixteen disk
  slots equal to zero is materialized as sixteen native null slots. The runtime
  checks every source byte before taking this branch. Nonzero, short, ambiguous,
  TEMP, invalid-block or insufficient-capacity requests fail through `ERR_DROP`.

The new array is allocated in the same zone's existing native XBlock. Native
alignment and 128 native bytes advance the real native cursor; the shadow Win32
cursor consumes zero additional disk bytes. Original scalar/tag mappings and
adjacent bytes remain unchanged. Repeated typed aliases share the cached native
array. Every original pointer-slot position is registered to its corresponding
native slot, including the last four-byte disk slot beside the following string.
Metadata is cleared by genuine `DB_InitStreams` for each loading zone. Native
storage remains owned and released by the ordinary XBlock/zone owner, with no
independent global heap lifetime or fabricated models.

The change is limited to Android. Generic pointer/scalar mapping and normal
Windows source bodies remain unchanged. No asset bytes, disk layouts, model
provider readiness, assertions or sanitizer settings were changed.

Evidence is under
`build-android-audit/native_registry_closure/asset-recovery-zero-alias1`:

- Actual production-command whole-TU compiles for `db_stream.cpp`,
  `db_stream_load.cpp` and `db_load.cpp` pass with NDK30/API28, PIC, debug,
  function instrumentation and UBSan/no recovery retained.
- Normal Windows x86/x64 comparisons pass for all three translation units with
  the established `/permissive /DWIN32=1` configuration.
- The HONOR `pointer-array-tests2` fixture uses the actual final stream and
  conversion objects and exits0 with `PASS ... checks=113 native_pointer_bytes=8`.
  It covers 2mod4 source alignment, 64 disk/128 native extents, unchanged adjacent
  ASCII and scalar mapping, cached gun/world identity, all 16 slot aliases,
  later native/shadow mapping, existing nonzero expanded arrays, rejection and
  capacity boundaries, and loading-zone metadata reset with old backing retained.
  Assertion/error/file-input/diagnostic providers are explicitly fixture-only;
  `ERR_DROP` rejection uses the fixture's nonlocal error handler.
- The first fixture failed before main with instrumentation-hook recursion.
  Its matching PC14C18 resolves to the fixture-only `__cyg_profile_func_enter`.
  The initial ELF, source snapshot and crash log are preserved. Only those two
  fixture hooks received `no_instrument_function`, as instrumentation hooks
  require; the production objects still retain their original instrumentation.

This fixture verifies representation, cursor and alias contracts. It does not
prove actual zone release, successful second game initialization, APK rendering
or gameplay. Root owns the next full430 production restart run and must verify
the real shared-zero materialization record, absence of bad model-array records,
second initialization, continued frames and canonical mapped shutdown. The staged
game root is now `/sdcard/boARM` following the user's relocation request.

Full430 subsequently verifies the actual repair on unchanged retail assets.
Its first `common_zombie` load records the guarded real 64-byte zero span at
block4/Win32 offset `C23DE` materialized into 128 native bytes; no bad model-array
diagnostic appears. The first map completes 300 frames, real `map_restart`
unloads/replaces the level assets, and the second genuine game initialization and
server spawn return. Another 300 completed frames bring the process serving count
to 600 and level time 30000 before canonical mapped quit exits 0 at 309.8142776s.
The main common zone and its native array backing are finally released by the
ordinary zone shutdown. Evidence is `full-engine430-honor-restart-baseline1.log`
and `.log.json`; root owns the separate clean process-guard check. This proves
the real headless restart cycle; rendered client and APK lifecycle remain separate.
