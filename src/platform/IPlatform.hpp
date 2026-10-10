#pragma once

// Narrow host/lifecycle surface. Windows backend forwards to existing Sys_* code.
// Godot (and later Android) will replace the implementation, not the callers.

struct IPlatform
{
    virtual ~IPlatform() = default;

    virtual const char *Name() const = 0;
    virtual unsigned int Milliseconds() = 0;
    virtual unsigned int MillisecondsRaw() = 0;
    virtual void RequestQuit() = 0;
    virtual bool IsQuitRequested() = 0;
    virtual bool IsHeadless() = 0;
    virtual void Print(const char *msg) = 0;
};
