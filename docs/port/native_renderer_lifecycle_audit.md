# Full renderer lifecycle audit

2026-10-13. This is an unfinished implementation audit, not a working renderer.

The full build still selects r_init.cpp with BO1_NATIVE_RENDER_GLOBALS and
BO1_NATIVE_RENDER_CONFIG, which include globals/configuration but exclude the
original renderer lifecycle, device/window creation and registration bodies.
Those bodies exist in the validated Windows reference and need porting.

Root compiled the whole canonical translation unit with the globals-only selector
removed, retaining the exact production toolchain and other flags. The first
audit fails on genuine Windows monitor/DPI/window APIs. The dxerr include is now
Windows-only; the existing native R_ErrorDescription still formats the actual
HRESULT. Normal Windows threads/r_init whole translation units pass both ABIs.
No lifecycle selector has been retired yet and no renderer readiness is published.

Evidence: build-android-audit/native_registry_closure/r_init_full_audit1.log and
its saved command; takeover_server_threads_renderer_windows1 results/manifests.

The protected DXVK1.9.2 source establishes the actual SDL2 contract:
include/native/wsi/native_sdl2.h converts HWND to a real SDL_Window pointer;
HMONITOR represents display index plus one. wsi/sdl2/wsi_window_sdl2.cpp and
wsi_presenter_sdl2.cpp use actual SDL window sizes/modes/Vulkan surface creation.
Do not substitute a synthetic window identity or upgrade Vulkan/DXVK to evade
the platform dependency.

Window/display/DPI operations need actual SDL video/Java/event ownership and
native dispatch to that owner. Original R_BeginRegistrationInternal executes on
the renderer backend while the controlling main caller waits, so directly using
the main-owned SDL lifecycle from that backend would violate ownership. Requests
must be serviced during initialization waits as well as during later input
frames. Actual input translation and the APK SDL Java lifecycle remain unfinished.
Neither the present event fixture nor a retained window pointer proves those
prerequisites.

Original registration, D3D capability/resource initialization, publication and
shutdown algorithms must remain the owners. Device and monitor information must
come from the actual SDL/DXVK backend. Vendor-specific NVAPI behavior needs a
truthful platform capability policy; an initialized Windows NVIDIA service must
not be fabricated on Android. Positive graphics/device/recovery and complete
Com_Init remain unverified.
