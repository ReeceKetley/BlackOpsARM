#pragma once
#include "db_rawfile_types.h"
extern RawFile*varRawFile;
extern const char**varXString;
extern const char*varConstChar;
unsigned char* __cdecl AllocLoad_raw_byte();
void __cdecl Load_XString(bool atStreamStart);
void __cdecl Load_ConstCharArray(bool atStreamStart,int count);
void __cdecl Load_RawFile(bool atStreamStart);
