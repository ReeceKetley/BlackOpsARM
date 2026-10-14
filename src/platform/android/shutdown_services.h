#pragma once
namespace bo1::android {
// Published only immediately after the actual Com_Init call returns in main.
void EngineInitializationReturned();
// Dedicated startup is a running checkpoint, not terminal cleanup or readiness.
void DedicatedInitializationCheckpoint();
// Guard Com_Quit_f entry before any renderer/recorder/thread prelude.
void RequireInitializedQuitEntry();
// Published only after actual recorder/thread sync and server terminal park.
void QuitPreludeCompleted();
// Called by canonical Sys_Quit; direct initialized quit routes to Com_Quit_f.
void BeginFinalSystemCleanup();
[[noreturn]] void FinishSystemQuit();
}
void Sys_Quit();
void Sys_NormalExit();
void Sys_SpawnQuitProcess();
void Sys_DestroyConsole();
void Sys_DestroySplashWindow();
void Sys_HideSplashWindow();

void Sys_QuitAndStartProcess(const char *exeName);
extern char sys_exitCmdLine[1024];
