#pragma once
#include <cstdio>
#include <ctime>
namespace bo1::android {
FILE *OpenShaderCacheFile(const char *legacyPath, const char *mode);
bool EnsureShaderCacheDirectory(const char *legacyDirectory);
void CleanShaderCacheDirectory(const char *legacyDirectory, unsigned short daysOld, bool removeDirectory);
bool ShaderCacheHasBytes(FILE *stream, unsigned int bytes);
time_t ShaderCacheCutoff(time_t now, unsigned short daysOld);
}
