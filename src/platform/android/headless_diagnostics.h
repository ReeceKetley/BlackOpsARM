#pragma once
namespace bo1::android {
// Arguments come from the canonical command-line parser, before engine init.
void ConfigureDiagnostics(int framePerfMs, int hitchWatchMs, int threadDumpMs);
// Genuine Sys_Init Dvar registration, before startup commands execute.
void RegisterDiagnosticsDvars();
// Call immediately after the actual Com_Init return, on its main owner.
void DiagnosticsInitializationReturned();
// Call after each genuine Com_Frame, before the next frame starts.
void DiagnosticsFrame();
// Stop and join this observer before engine logger/allocator retirement.
void StopDiagnostics();
}
unsigned int Sys_FramePerfHitchMs();
unsigned int Sys_HitchWatchBegin();
void Sys_HitchWatchEnd();
void Sys_HeadlessNoteHud(int pmType, bool hudDrawn);
void Sys_HeadlessTimeline(const char *what);
void Sys_HeadlessLevelNotify(const char *name);
