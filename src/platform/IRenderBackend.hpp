#pragma once

// Render host surface. Windows backend identifies the live D3D9 path in gfx_d3d.
// Godot RenderingServer will replace the API; game/render state stays authoritative.

struct IRenderBackend
{
    virtual ~IRenderBackend() = default;

    virtual const char *Name() const = 0;
    virtual const char *ApiName() const = 0;
};
