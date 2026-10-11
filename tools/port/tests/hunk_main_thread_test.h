#pragma once
#if defined(__ANDROID__)
#include <platform/android/hunk_services.h>
inline void RegisterHunkTestMainThread() { bo1::android::RegisterHunkMainThread(); }
inline bool IsHunkTestMainThread() { return bo1::android::IsHunkMainThread(); }
#else
void RegisterHunkTestMainThread();
bool IsHunkTestMainThread();
#endif
