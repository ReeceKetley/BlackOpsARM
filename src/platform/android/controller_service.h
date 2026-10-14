#pragma once
#include <cstdint>
#if defined(__ANDROID__)
#include <jni.h>
#endif
namespace bo1::android {
// Canonical client/global arrays support one controller port.
constexpr int ControllerSlots = 1;
// Transport sample in the canonical engine's original button/stick units.
struct ControllerSample {
    std::uint16_t wButtons{};
    std::uint8_t bLeftTrigger{}, bRightTrigger{};
    std::int16_t sThumbLX{}, sThumbLY{}, sThumbRX{}, sThumbRY{};
};
static_assert(sizeof(ControllerSample) == 12);
#if defined(__ANDROID__)
// Call once from the real Java JNI startup before SDL.setupJNI runs elsewhere.
// Performs the matching SDL Java/native setup; no synthetic readiness setter.
bool SetupSDLControllerJava(JNIEnv *environment);
// Adopt completed matching SDLActivity.onCreate without repeating SDK setup.
bool AdoptSDLActivityJava(JNIEnv *environment, jobject activity);
// Shared SDL audio/video prerequisites read the actual successful Java setup publication.
bool SDLJavaSetupComplete();
// True only after validated matching SDLActivity adoption, never manual setup.
bool SDLActivityAdopted();
#endif
bool InitControllers();
bool RefreshControllers();
bool ControllersReady();
// 1 attached sample, 0 enumerated empty slot, -1 invalid/uninitialized/wrong owner.
int ReadController(int slot, ControllerSample *sample);
std::uint64_t ControllerGeneration(int slot);
bool RumbleController(int slot, std::uint16_t low, std::uint16_t high, std::uint32_t milliseconds);
bool ShutdownControllers();
}
