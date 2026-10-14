#pragma once

namespace bo1::android {
// Process-owned recursive mutex used by the original water update lifecycle.
void *InitializeWaterUpdateMutex();
unsigned int LockWaterUpdateMutex(void *handle);
bool UnlockWaterUpdateMutex(void *handle);
}
