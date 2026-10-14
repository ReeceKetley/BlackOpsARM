#pragma once
// Native SDK omissions needed by original declarations. Opaque types describe
// handles only; this header implements no Windows services or error recovery.
#if defined(__ANDROID__)
#include <Windows.h>
#include <cstdint>
struct HWND__;
struct HINSTANCE__;
struct HMONITOR__;
struct HDC__;
using tagRECT = RECT;
using tagPOINT = POINT;
static_assert(sizeof(tagPOINT) == 8 && sizeof(tagRECT) == 16);
using LPARAM = std::intptr_t;
struct _EXCEPTION_POINTERS;
struct _SYSTEMTIME;
#endif
