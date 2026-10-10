#pragma once

unsigned int __cdecl Sys_Milliseconds();
unsigned int __cdecl Sys_MillisecondsRaw();
// Raw Win32 timer used by the platform Windows backend (avoids recursion through IPlatform).
unsigned int __cdecl Sys_Milliseconds_Win32();
unsigned int __cdecl Sys_MillisecondsRaw_Win32();
void __cdecl Sys_SnapVector(float *v);
