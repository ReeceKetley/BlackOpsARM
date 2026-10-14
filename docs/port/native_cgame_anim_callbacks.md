# Native CGame animation allocation and retirement callbacks

Frozen 2026-10-13. Only existing `src/cgame_mp/cg_main_mp.cpp` and
`src/cgame_mp/cg_animtree_mp.cpp` changed. No headers, source selection, feature
flags, protected files or assets were edited. The canonical Windows bodies and
CP1252 encoding are preserved.

## Actual callback contracts

`Scr_PrecacheAnimTrees` accepts `void *(*)(int)`. Four CGame callers previously
cast `Hunk_AllocXAnimCreate`, whose real declaration and implementation are
`unsigned char *(*)(unsigned int)`. Both argument signedness and return pointer
type therefore differed at the actual indirect invocation. Three callers are
in `cg_main_mp.cpp` (server scripts, SP client scripts, MP client scripts); the
fourth is `CGScr_LoadAnimTrees` in `cg_animtree_mp.cpp`.

Each native translation unit now contains an exact `void *(*)(int)` thin wrapper.
It rejects negative byte counts through the real fatal error and aborts if that
callback returns. Nonnegative values convert explicitly to unsigned and invoke
the original `Hunk_AllocXAnimCreate`; its returned byte pointer converts normally
to `void *`. The actual low hunk allocation, its name/type and its 32-byte
alignment remain in the original owner. No TLS/global callback capture or
replacement allocator was introduced. All four Windows casts remain in the
Windows branches. Exact source readback asserts the four native call sites and
retained Windows counterparts.

`CG_SafeDObjFree` also previously cast `MT_Free(unsigned char *,int,scriptInstance_t)`
to the `XAnimFreeTree` callback type `void (*)(void *,int,scriptInstance_t)`.
Its native thin wrapper now converts the ordinary data pointer and invokes the
actual `MT_Free` owner with the size and script instance unchanged. Tree clearing
and pointer retirement remain in the original engine algorithm. Windows retains
its original callback expression. This matches the already implemented genuine
common small-tree owner adapter.

## Coordinated audit findings

The shared XAnim/memory owner retains the existing signed APIs and adds native
unsigned overloads for genuine unsigned providers. Script animation loading
supplies a signed callback to `XAnimCreateAnimsWithValues`; it must not be
globally converted to unsigned without adapting its caller chain.

The separate `BG_LoadAnim` cast of dynamic `bgs_t::AllocXAnim` is a distinct
contract mismatch. That provider can be the actual server or client allocator.
`Scr_PrecacheAnimTrees` synchronously forwards it through animation tree loading,
creation with values and recursive node creation. It requires typed propagation
of the actual provider; substituting one fixed hunk owner or storing a new shared
or TLS callback would change its contract. This finding was handed to root and
the shared callback owner; no BG source was modified by this batch.

Reading the actual hunk registration bodies also confirms that `Hunk_AddData`
and `Hunk_SetDataForFile` invoke their allocator immediately and retain allocated
records/data, not allocator callbacks. A preliminary audit message claiming
callback persistence was corrected before implementation. `TreeNameMap` is an
int plus 80 character bytes, so its existing 84-byte script allocation stride
has no native pointer-width defect. The current server/client hunk providers
already allocate with 32-byte alignment. The real debug arena uses FirstFit,
which raises effective alignment to its native node alignment; no guaranteed
debug pointer-array alignment defect was found. This audit did not certify
all XAnim gameplay or optional LoadObj paths.

## Verification

- `build-android-audit/native_registry_closure/cg_main_mp-anim-callbacks-audit1`
  and `cg_animtree_mp-anim-callbacks-audit1`: actual FULL instrumented ARM64
  whole-TU compilation passes. Exact argv JSON, logs and defined/undefined
  symbol manifests are retained.
- Both compile after the shared owner's final `xanim.h` / `com_memory.h` freeze.
  `cgame-anim-callbacks-compile-hashes1.json` records source/header hashes, verified
  unchanged throughout compilation.
- `build-android-audit/cgame-anim-callbacks-windows1/{x86,x64}`: normal Windows
  whole-TU comparisons pass once per ABI, two translation units each, exit0,
  without native selectors. Commands, source manifests and result JSON remain.
- `cgame-anim-callbacks-frozen-hashes1.json` records the final source/header ABI
  evidence. Owned source hashes are
  `DBCF5A807FD0257EE3DC2A0AB96A2D810FAF7750F29EEB8D80F54CBF0409E986` and
  `646DF5F1AF3AA7EEC06D26A38D455E97776F038ECB8363154A8C96381AD8B90D`.

No synthetic fixture, tablet execution or production build was performed for
these thin adapters. Actual map-start use and initialized retirement are for
the next coordinated production runtime; compiler checks alone do not establish
animation, client or map readiness.
