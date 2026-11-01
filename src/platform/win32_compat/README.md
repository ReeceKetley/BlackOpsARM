# Small Win32 OS service compatibility layer

Reserved for minimal, explicitly specified compatibility shims for small OS
services, such as timing, files and synchronization, where keeping the engine
call site is less invasive than rewriting it.

Phase 1 implements scoped `bo1::win32::QueryPerformanceFrequency`, `Sleep`,
`GetLastError` and `SetLastError`. Fixed-width BOOL/DWORD and the QuadPart-only
LARGE_INTEGER are not global replacements for DXVK's Windows types. The timer
uses a fixed nanosecond timebase; a future QueryPerformanceCounter must use that
same monotonic timebase. It is not implemented yet. Sleep resumes EINTR, yields
for zero, waits indefinitely for INFINITE, and aborts on unrepresentable POSIX
failure. Last-error state is thread-local. Historical audit reports are retained locally.

`msvc_compat.h` accepts only Android ARM64 Clang: the audit uses MSVC spelling
extensions, signed char, native ARM calling convention for __cdecl, and a real
debug trap. It never impersonates Windows or defines BO1_PORT_X64. Memory,
event, thread, file and graphics APIs remain unimplemented; do not add stubs.

Each shim must document supported semantics, observable errors, ownership and
threading. Never silently return success for an unsupported operation. Use
native-width pointers and correct ARM64 atomics/memory ordering. Keep on-disk
32-bit records separate from runtime structures.

Graphics, audio, lifecycle and controller input use native Android backends in
`../android/`, rather than a broad Windows emulation layer. Keep shared changes
compatible with Windows and use the existing platform interfaces where possible.
