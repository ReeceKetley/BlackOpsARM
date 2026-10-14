#include "post_filter_depth.h"

#if defined(BO1_NATIVE_FILTER_DEPTH_UNBIND) && BO1_NATIVE_FILTER_DEPTH_UNBIND
#include <d3d9.h>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>

namespace bo1::android::post_filter_depth {
namespace {
thread_local unsigned filterDepth{};
struct Counters {
    std::uint64_t outside{}, nullDevice{}, checked{}, skipped{}, eligible{}, noSurface{}, unbound{}, restored{};
    std::chrono::steady_clock::time_point lastReport{};
};
thread_local Counters counters;

[[noreturn]] void Fail(const char* operation, HRESULT result) {
    std::fprintf(stderr, "BO1_NATIVE_FILTER_DEPTH failure: %s HRESULT=0x%08x\n",
        operation, static_cast<unsigned>(result));
    std::fflush(stderr);
    std::abort();
}

void Report() {
    const auto now = std::chrono::steady_clock::now();
    if (now - counters.lastReport < std::chrono::seconds(1)) return;
    counters.lastReport = now;
    std::fprintf(stderr,
        "BO1_NATIVE_FILTER_DEPTH outside=%llu null_device=%llu checked=%llu skipped=%llu eligible=%llu no_surface=%llu unbound=%llu restored=%llu\n",
        static_cast<unsigned long long>(counters.outside),
        static_cast<unsigned long long>(counters.nullDevice),
        static_cast<unsigned long long>(counters.checked),
        static_cast<unsigned long long>(counters.skipped),
        static_cast<unsigned long long>(counters.eligible),
        static_cast<unsigned long long>(counters.noSurface),
        static_cast<unsigned long long>(counters.unbound),
        static_cast<unsigned long long>(counters.restored));
}
}

FilterScope::FilterScope() {
    if (filterDepth == std::numeric_limits<unsigned>::max())
        Fail("filter scope overflow", E_FAIL);
    ++filterDepth;
}

FilterScope::~FilterScope() {
    if (!filterDepth) Fail("filter scope underflow", E_FAIL);
    --filterDepth;
}

DrawScope::DrawScope(IDirect3DDevice9* device) {
    if (!filterDepth) { ++counters.outside; return; }
    if (!device) { ++counters.nullDevice; Report(); return; }
    ++counters.checked;
    DWORD depthTest{}, depthWrite{}, stencil{};
    HRESULT hr = device->GetRenderState(D3DRS_ZENABLE, &depthTest);
    if (hr != S_OK) Fail("GetRenderState(ZENABLE)", hr);
    hr = device->GetRenderState(D3DRS_ZWRITEENABLE, &depthWrite);
    if (hr != S_OK) Fail("GetRenderState(ZWRITEENABLE)", hr);
    hr = device->GetRenderState(D3DRS_STENCILENABLE, &stencil);
    if (hr != S_OK) Fail("GetRenderState(STENCILENABLE)", hr);
    if (depthTest != FALSE || depthWrite != FALSE || stencil != FALSE) {
        ++counters.skipped;
        Report();
        return;
    }
    ++counters.eligible;
    IDirect3DSurface9* surface{};
    hr = device->GetDepthStencilSurface(&surface);
    if (hr == D3DERR_NOTFOUND && !surface) {
        ++counters.noSurface;
        Report();
        return;
    }
    if (hr != S_OK || !surface) {
        if (surface) surface->Release();
        Fail("GetDepthStencilSurface", hr != S_OK ? hr : E_POINTER);
    }
    hr = device->SetDepthStencilSurface(nullptr);
    if (hr != S_OK) {
        // An error cannot be assumed to preserve binding; attempt exact recovery.
        const HRESULT restore = device->SetDepthStencilSurface(surface);
        surface->Release();
        if (restore != S_OK) Fail("restore after unbind failure", restore);
        Fail("SetDepthStencilSurface(null)", hr);
    }
    device_ = device;
    surface_ = surface; // GetDepthStencilSurface owns this AddRef until restoration.
    ++counters.unbound;
}

DrawScope::~DrawScope() {
    if (!surface_) return;
    const HRESULT hr = device_->SetDepthStencilSurface(surface_);
    surface_->Release();
    surface_ = nullptr;
    device_ = nullptr;
    if (hr != S_OK) Fail("SetDepthStencilSurface(restore)", hr);
    ++counters.restored;
    Report();
}
}
#else
namespace bo1::android::post_filter_depth {
FilterScope::FilterScope() = default;
FilterScope::~FilterScope() = default;
DrawScope::DrawScope(IDirect3DDevice9*) {}
DrawScope::~DrawScope() = default;
}
#endif
