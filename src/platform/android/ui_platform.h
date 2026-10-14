#pragma once
namespace bo1::android {
// True only after actual UI action/positive user response. Missing/destroyed
// Activity, dismissal, rejection and wrong waiting thread return false.
bool ConfirmConfigurationChange(const char *title, const char *message);
bool LaunchAndroidURL(const char *url);
// Bounded real engine output; never invokes JNI/log/error callbacks under locks.
void AppendAndroidConsoleOutput(const char *message);
// Install after actual rendered input/event-queue initialization on its SDL
// event owner; pump on that owner and retire before its SDL lifecycle retires.
void InstallAndroidConsoleCommandOwner();
bool PumpAndroidConsoleCommandsIfOwner();
void RetireAndroidConsoleCommandOwner();
}
