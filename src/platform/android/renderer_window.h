#pragma once
#include <SDL.h>
#include <cstdint>
namespace bo1::android {
using RendererOwnerTask = bool (*)(void *);
// Calls execute only on the installed SDL event owner. A pending timeout cancels
// before the caller returns; once started, the caller waits until completion.
bool RunRendererOwnerTask(RendererOwnerTask task, void *context, unsigned timeoutMs = 5000);
bool PumpRendererOwnerRequestsIfOwner();
struct RendererDisplay { int index{}, x{}, y{}, width{}, height{}, refreshRate{}; float dpi{}; };
struct RendererWindowSpec { const char *title{}; int x{}, y{}, width{}, height{}, refreshRate{}; bool fullscreen{}, borderless{}, hidden{}; };
struct RendererWindowInfo { int x{}, y{}, width{}, height{}, drawableWidth{}, drawableHeight{}, displayIndex{}; unsigned flags{}; };
bool PrepareRendererVideo();
bool GetRendererDisplay(int index, RendererDisplay *out);
bool ChooseRendererDisplay(int preferredOneBased, int x, int y, RendererDisplay *out);
bool CreateRendererWindow(const RendererWindowSpec &spec, SDL_Window **out);
bool ConfigureRendererWindow(SDL_Window *window, const RendererWindowSpec &spec);
bool InspectRendererWindow(SDL_Window *window, RendererWindowInfo *out);
bool ShowRendererWindow(SDL_Window *window);
bool DestroyRendererWindow(SDL_Window *window);
// SDL-owner-only resource-presence query, not capability readiness.
bool RendererVideoResourcesOwned();
bool RetireRendererVideo();
}
