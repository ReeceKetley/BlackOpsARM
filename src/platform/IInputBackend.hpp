#pragma once

// Input host surface. Windows backend forwards to existing IN_* / XInput paths.
// Godot will later feed the same usercmd pipeline through this interface.

struct IInputBackend
{
    virtual ~IInputBackend() = default;

    virtual const char *Name() const = 0;
    virtual void Init() = 0;
    virtual void Shutdown() = 0;
    virtual void Frame() = 0;
    virtual void Activate(bool active) = 0;
};
