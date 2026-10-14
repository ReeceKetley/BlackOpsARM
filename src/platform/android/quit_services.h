#pragma once
void Sys_SetQuitRequested();
bool Sys_IsQuitRequested();
[[noreturn]] void Sys_QuitWithLostDevice(int lostMilliseconds);
