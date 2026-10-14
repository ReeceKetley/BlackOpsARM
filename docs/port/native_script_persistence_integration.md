# Native script persistence and lookup owners

Canonical `clientscript/cscr_save.cpp` and `cscr_readwrite.cpp` supply the omitted
save/load/object-reference and variable hash/sibling owners. The latter retains
its original typed list traversal unchanged. Production integration and normal
Windows whole-TU comparisons belong to the root agent's integration evidence.

Existing native VM representations remain intact: `VariableUnion` is four bytes,
archived stack header payload begins at byte thirteen, entries are one type byte
plus four payload bytes, and actual pointers use the existing region-tagged
Narrow32 service. Disk code positions remain signed 32-bit bytecode offsets;
save maps remain unsigned 16-bit IDs. No independent serializer is introduced.

Android-only guards in the original persistence source:

- All three archived-stack traversal/write paths validate `13 + 5 * size`
  against the stack's owned `bufLen` before reading payload entries.
- Code-position saving validates actual native address range against
  `programBuffer` and canonical `gScrCompilePub.programLen` before narrowing
  the bytecode-relative offset. It rejects offsets exceeding signed32 capacity.
- Code-position loading rejects negative values other than the canonical null
  sentinel `-1`, and offsets outside retained bytecode.
- Stack restoration rejects allocations exceeding fixed16 length before `MT_Alloc`.
- Save-map publication rejects IDs/counts unrepresentable in the retained fixed16
  format, including grown server object pools; restoration checks allocated IDs
  before writing the reverse map. Existing assertions remain enabled.

Both actual TUs compile ARM64/API28 with UBSan and without the retired startup or
string-pool test gates. No special source flags or associated header edits are
required. The native path additionally includes `cscr_compiler.h`; production
already selects its canonical global owner. All source bytes remain ASCII, which
preserves CP1252 interpretation and Windows preprocessor branches.

## Bounded HONOR fixture

`tools/port/tests/native_script_persistence_tests.cpp` includes the actual source
translation unit to access internal codec boundary functions; it does not copy
their logic. It links actual `memfile.cpp`, `minilzo.cpp`, native Narrow32 and
`mem_largelocal.cpp`. Typed fixture script-public/compiler storage provides an
isolated retained bytecode allocation. Main-thread selection compares real pthread
identity; expected invalid-input errors exit the child with code 73. Each child
is joined. This is a boundary fixture, not a replacement VM implementation.

Reproduce from BlackOpsARM:

```powershell
./tools/port/tests/build-native-script-persistence.ps1
```

NDK 30.0.16248370 / API28 / ARM64 / C++20 / signed char / UBSan no recovery /
static libc++ / native MSVC spelling adapter / section garbage collection.
No diagnostic engine-slicing definitions are used.

HONOR `AKWJ9X5116G04618` passed with exit 0 and `SCRIPT_PERSISTENCE PASS checks=20`.
Evidence is under `build-android-audit/native_script_persistence`:
`cscr_save-build2.log`, `cscr_readwrite-build1.log`, `fixture-build2.log`,
and `honor-run2.log` (final fixture; run1 is preserved).

Checks establish a genuine native bytecode address above 4GiB; actual Narrow32
round-trip; tagged fixed32 offset bytes; actual MemFile decode to the native
pointer; null `-1` bytes; before-buffer/end-buffer save rejection; one-past and
noncanonical-negative load rejection; fixed stack layout; and malformed owned
stack-size rejection. Unaligned tagged MemFile integer writing was exercised
without a UBSan failure in this scope; no unobserved failure is asserted.

Full object-graph round-trip, hash collision traversal, forward-reference/array-key
reference ownership, restore failure cleanup, save-map exhaustion, nested stacks,
initialized VM lifetimes, retail save compatibility, and actual Com_Init consumers
remain unverified. This result does not certify them. Only the new binary was
transferred; original assets and protected checkouts were untouched.
