#pragma once
#include <cstdio>
namespace bo1::android {
FILE *OpenStdio(const char *path, const char *mode);
unsigned int ReadStdio(void *destination, unsigned int bytes, FILE *stream);
unsigned int WriteStdio(const void *source,unsigned int bytes,FILE *stream);
void CloseStdio(FILE *stream);
}
