# Native packed animation and debugger operands

Frozen 2026-10-13. Only `src/clientscript/cscr_animtree.cpp` and
`src/clientscript/cscr_debugger.cpp` changed. The frozen shared
`platform/bo1_bytecode_io.h` is consumed without modification. Prior native
unsigned animation allocation templates, headers, script layouts, Windows
branches, feature gates and source selection are preserved.

## Six actual byte-stream accesses

| Function | Operand | Native operation |
| --- | --- | --- |
| `Scr_EmitAnimationInternal` existing-name branch | Previous animation link | Store uint32 |
| `Scr_EmitAnimationInternal` new-name branch | Zero link terminator | Store uint32 |
| `ConnectScriptToAnim` | Next link | Load uint32, then existing Narrow32 decode |
| `ConnectScriptToAnim` | Resolved `scr_anim_s` | Copy exact four local bytes into uint32, store uint32 |
| `Scr_ScriptWindow::AddBreakpointAtSourcePos` | Builtin index at opcode + 1 | Load uint16 |
| `Scr_ScriptWindow::EnterCallInternal` | Call target at opcode + 1 | Load uint32, then Narrow32 decode |

The animation compiler writes opcode `0x13` followed by a four-byte operand, so
these link/backpatch addresses have no aligned-object contract. The original
link chain remains a fixed four-byte handle stream, including zero termination.
`scr_anim_s` remains exactly four bytes, checked statically; its native union is
never treated as a wider pointer or padded native object. The same functions
contain no additional identical-width packed access beyond these four sites.

The debugger previously read a two-byte scalar through an aligned `_WORD *` at
an arbitrary bytecode address. It also read a native `const char *` through an
unaligned bytecode position while advancing by only four bytes. The latter was
both an alignment violation and an eight-byte native read of a four-byte handle.
The native repair increments past the opcode, loads/decode the actual four-byte
handle, and preserves the original subsequent cursor advance by four. It does
not widen instructions or insert alignment padding. Windows retains each
original expression in its platform branch. Exact source readback verifies all
six native sites and the corresponding retained Windows operations.

## Audit boundaries

`cscr_readwrite.cpp` contains variable-table graph helpers rather than packed
bytecode/archive scalar access. `Scr_GetNextCodepos` lives in `cscr_vm.cpp` and
belongs to the VM owner's separate repair; no standalone disassembly source
was found in this checkout.

Animation `ParseEntry[20]` accesses use real aligned twelve-byte rows containing
three four-byte fields. Vector refcount words belong to an actual MT-aligned
allocation header. These were not reclassified as bytecode. The compressed
rawfile header's two fixed four-byte words are read from the genuine loader's
native-aligned buffer; no guaranteed current alignment fault was identified
there. Separate header-bounds/decompression concerns were reported without
changing parser behavior.

Debugger watch sorting uses truncated runtime pointers and legacy object
offsets, a distinct native-layout problem. Treating those as packed four-byte
bytecode operands would preserve the defect. It remains outside this six-site
batch and was reported separately; this batch does not certify an initialized
debugger or unrelated watch/UI functionality.

## Scoped verification

Evidence under `build-android-audit/native_registry_closure`:

- `cscr_animtree-packed-consumers-audit1` and
  `cscr_debugger-packed-consumers-audit1`: actual FULL instrumented ARM64 whole
  translation units compile. Exact command JSON, logs, defined/undefined
  symbol manifests and source/shared-header hash snapshots remain.
- `build-android-audit/script-packed-consumers-windows1/{x86,x64}`: normal
  Windows whole-TU comparison passes once per ABI, two TUs each, exit0 without
  native selectors. Source manifests and commands are retained.
- `script-packed-consumers-fixture1.cpp`: generated from the actual frozen
  emitter/backpatch function bodies and the exact two debugger read statements.
  It links the genuine Android Narrow32 implementation. Its variable-store
  callbacks bind an isolated typed slot; they do not initialize a VM or provide
  fake debugger/UI success.
- `script-packed-consumers-fixture1-command.json`, build log and run log: one
  actual HONOR ARM64 UBSan run passes. It exercises both emitter branches,
  chain encoding/decoding and backpatching at all four scalar alignment residues,
  exact animation bytes, zero head retirement and adjacent opcode/gap canaries.
  Debugger read statements are checked at eight address residues with an actual
  high native pointer represented by a four-byte handle. An actual inaccessible
  next page proves the call read uses four bytes and the builtin read uses two;
  native-pointer overreads cannot pass that boundary.
- `script-packed-consumers-frozen-hashes1.json`: final source/helper/fixture
  hashes and sizes. Owned source hashes are
  `EBEF3FC087655BD6D6BCF1F795E9CEDC6E120598F4B0031E7A566735A7404B93` and
  `8A433E3909B716495A46E361223D98E28087B8857EBA7B11375146676C7B66FE`.

Only the new 1,270,176-byte fixture was transferred; no assets or shared libraries
were recopied and no production runtime was launched. Rebuild from the saved
fixture command JSON in the repo root. Actual tablet invocation:

```sh
/data/local/tmp/blackopsarm-script-packed-consumers-fixture1
```

The proof concerns byte-stream width, alignment, handle identity and boundary
access. Full compiler/VM execution, initialized debugger behavior and actual
map startup remain the coordinated production runtime's responsibility.
