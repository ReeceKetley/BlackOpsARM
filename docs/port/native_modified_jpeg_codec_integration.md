# Original modified JPEG codec with native ABI and memory-source handling

2026-10-13. Full engine unlinked; D3DX file-save adapter and real Com_Init unfinished.

The full target includes a static library of46 canonical src/jpeg C modules,
using the engine's modified version62 codec and existing jmemansi backend. The
mutually exclusive jmemname/jmemnobs alternative memory backends are not linked.
No system JPEG substitution or independent encoder/decoder algorithm.

Native adaptations:

- Default allocator hooks become thread-local. New codec contexts capture their
  actual allocator/free hooks as the reference already does, retaining context
  lifetime even after this thread's next-context defaults are cleared.
- JPEG INT32 is actual int32_t on Android, preserving Windows32 coefficient and
  bit-buffer semantics rather than Android LP64 long64.
- Memory-source allocation uses actual sizeof(my_source_mgr) rather than fixed32;
  restart callback uses its canonical boolean-returning declaration without an
  incompatible uint8 function-pointer cast.
- Memory input rejects invalid lengths/pointers, out-of-range skips and exhausted
  input through real JPEG errors. It never calls stdio on a memory source, invokes
  a null refill callback or inserts fake end-of-image bytes.
- Native integer DCT/IDCT and tested Huffman-buffer shifts preserve original
  32-bit bit patterns through unsigned shifting and memcpy representation transfer,
  avoiding signed-left-shift UB. Windows macro expands to its original operations.
  No sanitizer disabling or successful error substitution.

Evidence under build-android-audit:

- native_registry_closure/jpeg-codec-build1.log records incompatible callback;
  build2 compiles all46 original modules after source ABI repair.
- jpeg-codec-tests-build1/run1.log: first real tablet run catches negative signed
  DCT shift with UBSan. Failure evidence retained. tests-build2/run2.log passes
  after actual width/shift repairs; no relaxed assertions.
- takeover_jpeg_memory_source_allocator_windows1 and final
  takeover_jpeg_native_integer_abi_windows1: final scope six changed/affected
  normal C TUs plus normal R_jpeg.cpp header consumer, Windows x86/x64 exit0,
  source manifests retained. Header consumers preserve Windows behavior.
- jpeg-codec-tests-push2.log/run2.log: HONOR AKWJ9X5116G04618 ADB-shell UID exit0.
  Actual baseline RGB16x16 encoding and decoding through the modified library,
  native sizeof codec contexts, encoded SOI/pixel tolerance/scanline canaries,
  retained context allocators after hook clear, concurrent thread-local hook
  isolation and allocator use on two threads pass. Strict truncated12-byte input
  prints real premature-input error and child aborts. Error test forks from main;
  worker thread runs positive codec work only. Codec destination is an isolated
  actual output byte buffer, not an image-file save provider.
- full-engine-link370.log is latest production attempt with actual codec library;
  genuine remaining dependencies still prevent linking.

No D3DX surface-save file encoding/I/O, actual GPU capture or native R_SaveJpg/
R_ReadJpg wrapper lifecycle implemented by this codec integration. Those wrappers
still need native context sizes, typed destination ownership/callbacks, capacity,
global read/hunk state and failure handling audits. Other JPEG modes/DCT paths,
extreme/malicious images, all integer shift/arithmetic sites, allocation failure
and arbitrary concurrent engine consumers remain unverified. No retail asset-file
test loop repeated; only changed fixture binaries transferred. Native image-save,
audio/cinematic/renderer/input/Steam/debug/Monkey and other dependencies remain.
Actual Com_Init has not returned and Com_Frame remains unexecuted.
