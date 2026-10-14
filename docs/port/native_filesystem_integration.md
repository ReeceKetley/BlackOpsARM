# Native filesystem and original IWD reader integration

Full Com_Init remains the active objective. This continuation selects the actual
original IWD reader and wavelet decoder in the expanded database library, alongside
the previously added full filesystem/image-loader sources. It adds real directory
services and verifies the pending user-root, file attributes and text IO on HONOR.
BlackOps64 is the user's validated Windows x64 behavior reference.
Latest continuation adds actual directory-list/hunk ownership, virtual allocation,
filesystem commands, checksum initialization and canonical reference helpers.
Later key/image evidence and the current localization source/link status are in
[native_localization_source_closure.md](native_localization_source_closure.md).

## Changes

- Original unzip.cpp uses explicit two/four-byte little-endian disk reads on
  Android, with short-read errors. Native unsigned long is eight bytes; disk
  fields remain two/four bytes. Windows implementations remain unchanged.
  The reader's existing engine seek and size adapters were inspected and retained:
  literal origin 2 means absolute seek, origin 0 means relative seek. Its central
  directory search intentionally obtains the entire file size, not current tell.
- Full original r_image_wavelet.cpp compiles without changing decoder tables or
  bodies. Compilation alone does not establish actual image/wavelet loading.
- Existing tested native CRT RNG is linked for the full math/resource sources.
- Actual canonical thread-name table/getter is selected for native use. Stream
  identity queries use the registered native TLS role, including retirement.
- Original Com_Memcpy, I_islower and I_strlwr bodies are selected in startup
  services. Their behavior is unchanged; no duplicate substitute was added.
- Native Sys_Mkdir resolves engine path spelling, creates actual directories and
  accepts existing directories; other failures abort explicitly. Directory-content
  queries preserve the original dot-entry and CVS-directory exclusions.
- New writable user-root installation validates an actual directory separately
  from the staged data root. FS_GetOsFolderPath rejects a root exceeding its
  canonical 256-byte output capacity instead of silently truncating it.
- Actual chmod maps the supported read-only attribute to owner-write permission;
  other permission bits are preserved. Unsupported attributes return error 50.
- Engine text IO resolves actual paths, writes LF as CRLF and reports logical
  caller byte counts. Binary writes retain their bytes. Text streams support
  append and update with the original engine seek/read wrappers. Complete Windows
  ACL/file-attribute and CRT text-position-cookie equivalence is not claimed.

## Verification and evidence

Latest filesystem-integration-run5.log passes 42 HONOR UBSan checks. New coverage
uses actual FS_AllocMem/FS_FreeMem with zero-filled mmap backing, captured ownership
and original null-release behavior. A separate child process with RLIMIT_AS proves
Z_TryVirtualAlloc returns null on a real mapping failure without adding an owner.
Actual native Sys_ListFiles uses original Hunk_UserCreate/Alloc/CopyString and
FS_FreeFileList destruction; list names, null terminator, extension/directory/filter
policies and complete mapping release are verified. The native pointer array
reserves both its owner prefix and trailing null and uses native alignment.

The original Windows filtered list is flat, not recursive. Its star-substring
matching also has a case quirk in Com_FilterPath/Com_Filter. That same behavior is
preserved and tested, including the backslash-prefixed returned name. The first
new fixture wrongly assumed a lowercase star pattern matched Mixed.TXT; run3's
failure is preserved and corrected in run4/5. Engine filter bodies were not changed.

Full original qcommon/files.cpp, md4.cpp, mem_userhunk.cpp, mem_firstfit.cpp and
mem_fixed.cpp are now selected. FILE replaces the identical Windows _iobuf type;
the map basename buffer is renamed to avoid Android libc's basename function.
Native CRC table initialization uses call_once; eight concurrent callers match
the known original CRC32 result. Existing native virtual reservation/commit/free
services now also provide actual try/full committed allocation; no duplicate Z
heap or Windows VirtualAlloc bridge was introduced. Reference memory-tracker
commit/decommit bodies are disabled, so no new telemetry success was invented.

Original Hunk_UserStartup replaces its prior unavailable endpoint for the full
build only. Its 48 MiB debug arena has not run through full Com_Init yet. The
canonical resource bit counter and original no-CD-search function are selected
and verified without replacing their behavior. The no-CD function remains the
actual Windows reference body; it is not a new placeholder.

Seven whole normal Windows TUs pass both ABIs in
takeover_directory_memory_windows1; two additional helper-owner TUs pass both ABIs
in takeover_reference_helpers_windows1. Logs full-files-compile1 and
filesystem-integration-build3/4 retain corrected FILE/basename and test pointer
cast/missing wildcard selection failures. full-engine-link23 preserves the
duplicate inherited Hunk_UserStartup endpoint; its full-build suppression selects
the original implementation. No duplicate implementation or assertion bypass.

bo1_native_filesystem_integration_tests links the actual production filesystem
wrappers, native file/path/thread services, original unzip reader and matching
modified engine zlib. It uses inherited explicit unavailable diagnostic endpoints
for unselected services; none were newly added or invoked. Only tracing callbacks
are test-owned no-ops. The generated archive contains two synthetic entries and
no retail game content.

HONOR UBSan filesystem-integration-run2.log passes 31 checks: actual main/stream
TLS names; separate roots; directory creation/case lookup/CVS exclusion; engine
text update, append and physical CRLF bytes; attributes; original central directory
metadata/positions; 70000-byte deflate output in small reads, CRC and EOF; stored
entry; independent reopen/close; and cleanup of test-owned files/directories.
The archive is reused on the second run; no retail assets were transferred.

Normal whole-source Windows x86/x64 builds of six affected engine files pass in
takeover_filesystem_windows2. The first build omitted WIN32, which the original
unzip reader requires for its endian macros; that failure is retained in
takeover_filesystem_windows1. The corrected build defines WIN32 as the Windows
project does. This is compilation evidence, not Windows runtime execution.

Native logs are under build-android-audit/native_registry_closure:
closure-next-0-compile1.log preserves the original missing native endian helpers;
closure-next-1-compile1.log records full wavelet compilation;
filesystem-integration-build1/2.log and filesystem-integration-run1/2.log record
the focused integration; full-engine-link20/21.log preserve remaining failures.

## Reproduction and remaining work

Configure the existing native-engine-entry project with BO1_NATIVE_FULL_DATABASE=ON.
Build bo1_native_filesystem_integration_tests; generate its small input with:

```
python tools/port/tests/make_filesystem_fixture.py <local-fixture-directory>
```

Push Fixture.IWD to a separate device data directory and push the test executable;
chmod 700 after the push. Supply absolute data and writable-user directories as
its two arguments. Use the entry's existing libc++_shared.so via LD_LIBRARY_PATH.
The test executable removes its own contents but leaves the root directories.

At the directory/memory gate, engine cache remains FULL_DATABASE=ON. Full engine link26
still fails on 61 symbols, including renderer/cinematic resource synchronization,
remaining memory/asset enumeration, key/localization and post-load dependencies.
Original resource events are created by Sys_SpawnRenderThread; their wrappers
must retain that real owner/initialization contract, not fabricate readiness.
Next resume genuine dependency integration and native VM ownership, then run the
actual full entry. No full FS initialization, DB_Thread zone loading, Com_Init
return, Com_Frame, APK storage or gameplay completion is claimed.
