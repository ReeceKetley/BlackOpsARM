#pragma once
#include <cstdio>
void Sys_InitializeHeadlessPolicy(const char *commandLine);
bool Sys_IsHeadless();
bool Sys_IsHeadlessClient();
