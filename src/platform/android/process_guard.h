#pragma once
namespace bo1::android {
// Main acquires under the actual user root before engine workers start.
// false means another live instance owns this same inode; failures are fatal.
bool AcquireProcessGuard(const char *userRoot);
void ReleaseProcessGuardClean();
bool ProcessGuardHeld();
}
