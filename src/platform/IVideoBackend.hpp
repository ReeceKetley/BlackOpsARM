#pragma once

// Cinematics / Bink surface. Windows backend reports Bink is present but deferred.
// Do not block bring-up on video; Godot or converted assets can replace this later.

struct IVideoBackend
{
    virtual ~IVideoBackend() = default;

    virtual const char *Name() const = 0;
    // True when a full video path is required for gameplay bring-up.
    virtual bool RequiredForGameplay() const = 0;
};
