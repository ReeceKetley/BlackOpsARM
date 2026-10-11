#pragma once
namespace bo1::android {
// Bind explicitly on the application/main thread before debug-hunk startup.
void RegisterHunkMainThread();
bool IsHunkMainThread();
void LockFirstFit();
void UnlockFirstFit();
}
