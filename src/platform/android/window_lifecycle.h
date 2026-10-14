#pragma once
#include <SDL.h>
namespace bo1::android {
using SDLEventConsumer = void (*)(const SDL_Event &,void *);
struct WindowLifecycleState {
    unsigned windowId{},flags{};int width{},height{};
    bool foreground{},focused{},closing{};
};
// Call from SDL's video/event owner. Consumer must handle remaining engine input.
void InstallSDLEventOwner(SDLEventConsumer consumer,void *context);
void AttachSDLWindow(SDL_Window *window);
void DetachSDLWindow(SDL_Window *window);
void PumpSDLEvents();
bool PumpSDLEventsIfOwner();
// Read-only identity check; it does not establish SDL subsystem readiness.
bool IsSDLEventOwner();
// Actual attached-resource presence, not a video/device readiness claim.
bool HasAttachedSDLWindow();
bool PushSDLFocus(SDL_Window *window,bool focused);
void RetireSDLEventOwner();
bool ReadSDLWindow(SDL_Window *window,WindowLifecycleState *state);
void RequestSDLCursor(bool visible);
bool PushSDLQuit();
}
