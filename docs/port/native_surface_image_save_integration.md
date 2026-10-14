# Native surface JPEG/TGA file saving

2026-10-13. Production engine still has unresolved links; real Com_Init return,
initialized screenshot capture and GPU readback have not executed.

New src/platform/android/surface_image_save.cpp owns the actual SDK-compatible
D3DXSaveSurfaceToFileA entry declared in native_d3dx9_api.h. It queries the supplied
surface description, validates a supplied or complete rectangle, obtains a real
readonly full-surface LockRect and computes the ROI from the actual returned pitch.
Only A8R8G8B8/X8R8G8B8 and JPG/TGA are supported; other formats/palettes fail
explicitly. Empty/negative/out-of-bounds rectangles, invalid storage/pitch and
native address-span overflow fail before pixel access. GetDesc/Lock/Unlock errors
propagate. Every successful lock receives exactly one unlock.

JPEG uses the matching original modified codec through its normal compressor API.
A custom 8192-byte buffered file destination replaces the disabled stdio
destination. Actual BGRA data is converted row-by-row to RGB; quality is90.
The codec/context/error/destination/jump state is heap POD. A custom codec error
callback jumps to the local cleanup target without crossing any nontrivial C++
destructor; actual codec and row storage are released. Allocator hooks are restored
immediately after the modified codec captures them; initialization failure restores
them in cleanup. TGA is uncompressed32-bit BGRA with top-left origin and eight
alpha bits; A8 alpha is retained and X8 alpha is made opaque. Rows respect pitch.

BO1_SaveSurfacePixels is a shared real encoding helper for readable CPU storage.
Its borrowed pointer must address the top row, have positive byte pitch and remain
readable for the entire call. As with LockRect, the API does not receive the actual
allocation length, so the owner must supply sufficient storage.

Both encoders stage into an actual mkstemp file in the target directory. All
fwrite/flush/fsync/close results are checked; surface unlock succeeds before rename
publishes the file. Encoder, storage, allocator, write or unlock failures remove
the staging file and preserve an existing target. Rename errors also fail and
retire the staging file. This does not promise directory-fsync crash durability.

Evidence in build-android-audit/native_registry_closure:

- surface-image-save-direct-build3.log: final NDK30/API28 ARM64 UBSan/no-recover
  build using original codec, GC sections and static C++ runtime; exit0. Final
  source/test-only wrapper linkage uses
  -Wl,--wrap=malloc,--wrap=calloc,--wrap=free.
- surface-image-save-push3.log/run3.log: changed fixture binary only transferred
  to HONOR AKWJ9X5116G04618; ADB-shell UID exit0. Real JPEG file decoded by the
  original codec has expected2x2 RGB pixels. Exact TGA header/pixels, retained/
  opaque alpha, padded pitch and >8192-byte buffered JPEG write pass.
- CPU COM fixture exercises actual adapter ROI addressing and description/lock/
  pitch/unlock errors. Unlock failure preserves the old target. This is interface
  contract evidence, not DXVK/Vulkan/GPU proof.
- Test-only linker wrappers observe actual writer/codec malloc/calloc/free
  retirement on success/error and inject the first four allocation failures.
  Previous hooks restore and existing target/temp-file cleanup pass. Real
  RLIMIT_FSIZE failures in JPEG/TGA writes likewise preserve the old file and
  leave no staging files. Invalid format/storage/span and missing parent path fail.
- Initial unwrapped writer run1 passed. Wrapped fixture run2 failed from recursive
  Android emulated-TLS allocation inside test bookkeeping; crash log/address
  evidence retained in surface-image-save-crash2.log. Fixture-only counters use
  ordinary globals in final single-thread tests; production source was unchanged.

Root owns production and fixture CMake selection. These new files do not alter
renderer JPEG save/read code, Windows sources, original assets or DXVK sources.
No actual GPU/device-backed surface capture, initialized renderer filesystem,
all format/quality modes, parallel surface saves or production startup is proven.
