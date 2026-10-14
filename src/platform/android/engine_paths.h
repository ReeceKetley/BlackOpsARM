#pragma once
namespace bo1::android {
// Install the actual imported/staged data directory before engine workers start.
void InstallEngineDataRoot(const char *directory);
void InstallEngineUserRoot(const char *directory);
}
char *Sys_DefaultInstallPath();
char *Sys_Cwd();
char *Sys_EngineUserRoot();
