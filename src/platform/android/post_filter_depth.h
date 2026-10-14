#pragma once

struct IDirect3DDevice9;
struct IDirect3DSurface9;

namespace bo1::android::post_filter_depth {
// Experimental filter-only scope; pixel-cost callers must not enter DrawScope.
class FilterScope final {
public:
    FilterScope();
    ~FilterScope();
    FilterScope(const FilterScope&) = delete;
    FilterScope& operator=(const FilterScope&) = delete;
};

// Create after all material/pass state updates, immediately around the draw.
// Restores the exact device attachment before the engine target cache is reused.
class DrawScope final {
public:
    explicit DrawScope(IDirect3DDevice9* device);
    ~DrawScope();
    DrawScope(const DrawScope&) = delete;
    DrawScope& operator=(const DrawScope&) = delete;
private:
    [[maybe_unused]] IDirect3DDevice9* device_{};
    [[maybe_unused]] IDirect3DSurface9* surface_{};
};
}
