# Native material shader-cache services

2026-10-13. Full Com_Init remains incomplete.

The original full material loader now compiles on ARM64. Native cache IO and
cleanup live in platform/android/shader_cache.cpp, selected at the original
engine call sites; Windows keeps its existing implementations. Cache names and
byte format are unchanged. Legacy pc/shadercache1/2 paths map to the installed
engine user root, rather than the project/data root. Directory creation and
file IO use descriptor-relative POSIX operations; path components and file opens
reject symlinks. Regular-file/single-link checks precede truncation. Other cache
versions and traversal leaf names are rejected.

Cleanup preserves the reference policy: only writable regular files, last access
before the cutoff; read-only files and directories remain. UTC age calculation
preserves Material_SubtractDays' original 28-day rollover behavior. Version1
cleanup removes eligible files then attempts to remove the directory, preserving
reference nonrecursive behavior. Native once initialization waits for real cleanup
completion with call_once. No synthetic successful cleanup or shader data.

Original cached-payload reading now checks real bytes remaining before allocating
the advertised shader size and rejects short payload reads with proper cleanup
on Android. The original Windows read behavior is unchanged. These original
material read/write wrappers themselves have compiled but have not executed;
the focused runtime gate executes the production native service beneath them.

## Validation

- Actual service passes HONOR UBSan IO, remaining-byte/EOF bounds, readonly/age/
  nested directory behavior, traversal/unknown-version rejection, symlink write
  rejection, outside-byte preservation and obsolete-version cleanup. Test uses
  a unique synthetic root, never staged retail assets. Evidence:
  native_registry_closure/shader-cache-build2.log and shader-cache-run2.log.
- The device prohibits creating hardlinks. That fixture explicitly skips; its
  write-rejection code is implemented but runtime verification remains missing.
  Initial run1 fixture failure is preserved. Its exact synthetic directory was
  inspected and individually cleaned; absence verified afterward.
- Whole original material TU compiles on normal Windows x86/x64 in
  takeover_material_cache_windows2. Windows1 and link56/57 preserve earlier
  type-correction iterations. Link58/59/60 compile and reach real linker failures.
- Only the small test binary was transferred. DXVK/Vulkan baseline unchanged.

Latest full-engine-link60 still fails on actual material handle, primary-light,
debug/client/pose, cinematic/sound, collision/physics, input/network and recovery
owners. The D3DX reflection/compiler declarations remain unimplemented, but
those names are not current link60 errors: their original loader paths are not
retained in this entry's present dependency graph. Do not infer implementation
from linker garbage collection or make them an immediate startup gate without
an actual retained call. Continue genuine Com_Init dependency closure from60.
