#pragma once
namespace bo1::android {
// Main/native SDL event owner only. Requires actual SDLActivity JNI adoption.
bool InitializeRenderedInput();
void ActivateRenderedInput(bool active);
void FrameRenderedInput();
void ShutdownRenderedInput();
// Android-private scalar events extend only this provider's event protocol;
// the canonical enum/layout remain unchanged. No evPtr allocation is owned.
enum : int { RenderedTouchHoverEvent=4,RenderedTouchDownEvent=5,RenderedTouchUpEvent=6 };
// Called by the native-guarded Com_EventLoop on the installed input owner.
// Returns true for a consumed private event, including rejected stale epochs.
bool DispatchRenderedTouchPointerEvent(int type,unsigned time,int x,int y,int epoch);
// Input shutdown may precede renderer retirement. Retire events only after the
// actual last renderer window has detached; call from its SDL owner.
void RetireRenderedInputEventsIfStopped();
}
