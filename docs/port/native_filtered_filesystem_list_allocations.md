# Native filtered filesystem list allocations

SDK435 reaches real foreground presentation and finishes the UI FastFiles, then
fails while `CL_CreateDevGui` builds the map-directory slider. UBSan identifies
`FS_ListFilteredFilesInLocation` accessing `searchpath_s` at `0x00704d10507c`,
which is four bytes short of its required eight-byte alignment. Matching shared
library BuildId is `cd75596a4e068bb0691f77c8ccf00142611b6e26`. The isolated
`sdk-native-entry435-launch1-only.log` and `sdk-native-launch435-crash1.txt` retain
the current launch; cumulative launch logs contain historical failures as well.

Both temporary search-path clone allocations still use the original Win32
28-byte size and four-byte alignment. The actual native structure contains three
eight-byte pointers and four four-byte integers: size 40, alignment 8. A second
28-byte allocation begins at 4mod8, while its language field at offset 36 also
lies outside the old extent. Persistent search paths already allocate
`sizeof(searchpath_s)`. The read-only BlackOps64 reference retains the same two
temporary Win32 literals; this is a native allocation contract repair.

Android now allocates both temporary nodes using their actual `sizeof` and
`alignof`. The same narrow typed-hunk allocation inventory reveals the adjacent
result-pointer table still allocating 65540 bytes/alignment 4. Its canonical
capacity is an owner prefix, up to 16383 filename pointers, and a null terminator:
16385 native pointer slots. Android allocates 131080 bytes/alignment 8 and raises
its hunk reserve by the table-width increase, then rounds the requested capacity
up to 200704 bytes (`0x31000`). `Hunk_UserCreate` requires a 4096-byte multiple.
Compile-time checks enforce that contract and capacity for the native table plus
the aligned allocator header. Original Windows allocation expressions stay in
their original branches.

The first SDK436 attempt exposed a missed caller requirement: the initial widened
reserve was 196612 bytes, which fit the table but violated that existing 4096-byte
constructor assertion. The real launch failed at `mem_userhunk.cpp:383` before
application initialization returned; its raw log and fatal stacks are preserved
in `sdk-native-entry436-launch1-only.log`. The correction rounds the caller's
request upward and leaves the constructor assertion and allocator unchanged.

The lifetime remains canonical. Temporary nodes borrow persistent directory/IWD
owners solely while filtering. `FS_ListFilteredFiles` creates its separate
result hunk, stores that actual owner in the preceding native pointer slot, and
copies filename strings into that result owner. The temporary path hunk is
destroyed after filtering; `FS_FreeFileList` later reads the prefix and destroys
the result owner. Filtering, localization, duplicate suppression, filename
spelling, max count and null termination are unchanged. No packing, misaligned
member-access workaround, copied filesystem implementation or fake allocator
was introduced.

Only `src/universal/com_files.cpp` changed, preserving its exact source encoding
and all pre-existing changes. Frozen SHA256 is
`9399E7F77B4D1B9DD910D5B176DF94D3464C382C2CE636CE170DCC61F9CD6F1F`.
Evidence under `build-android-audit/native_registry_closure/fs-native-list-alloc2`
retains the prior source/report bytes, source manifest and final actual
production-command native whole-TU compile result. The original batch and failed
SDK436 evidence remain preserved. NDK30/API28/PIC/debug/function instrumentation and
UBSan/no recovery are retained; compilation passes. Normal Windows x86/x64
whole-TU comparisons also pass with `/permissive /DWIN32=1`.

Root owns the next genuine SDK437 startup, which exercises these owners through
the original dev-GUI/file-list callers. No complex standalone filesystem or hunk
fixture was added for this bounded allocation repair, and no assets were copied
or changed. Compilation alone does not prove application startup or rendering.

## Actual corrected SDK startup

SDK437/APK9 with the corrected page-aligned reserve passes the previous
Hunk_UserCreate page assertion and native searchpath failure. It loads startup
and UI assets, presents loading frames to the real foreground window and passes
the map-list construction into CG_InitVisionSetsMenu. A distinct asset-enumerator
callback signature mismatch then aborts. Matching engine
BuildId312114383d5adc11e196eb72eaf9705d74e3652f; actual new-launch log and
crash are native_registry_closure/sdk-native-entry437-launch1-only.log and
sdk-native-launch437-crash1.txt. This is production startup evidence, not a
standalone filesystem fixture or complete client/gameplay claim.
