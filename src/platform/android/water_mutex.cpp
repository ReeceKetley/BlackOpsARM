#include "water_mutex.h"
#include <pthread.h>
#include <mutex>
#include <cstdio>
#include <cstdlib>

namespace {
pthread_mutex_t updateMutex;
std::once_flag initialized;
[[noreturn]] void fail(int error) {
    std::fprintf(stderr, "Water update mutex initialization failed: %d\n", error);
    std::abort();
}
}
void *bo1::android::InitializeWaterUpdateMutex() {
    std::call_once(initialized, [] {
        pthread_mutexattr_t attributes;
        int error = pthread_mutexattr_init(&attributes);
        if (error) fail(error);
        error = pthread_mutexattr_settype(&attributes, PTHREAD_MUTEX_RECURSIVE);
        if (!error) error = pthread_mutex_init(&updateMutex, &attributes);
        const int destroyError = pthread_mutexattr_destroy(&attributes);
        if (error) fail(error);
        if (destroyError) fail(destroyError);
    });
    return &updateMutex;
}
unsigned int bo1::android::LockWaterUpdateMutex(void *handle) {
    if (handle != &updateMutex) return 0xFFFFFFFFu;
    return pthread_mutex_lock(&updateMutex) == 0 ? 0u : 0xFFFFFFFFu;
}
bool bo1::android::UnlockWaterUpdateMutex(void *handle) {
    return handle == &updateMutex && pthread_mutex_unlock(&updateMutex) == 0;
}
