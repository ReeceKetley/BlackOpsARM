#pragma once

// Audio host surface. Windows backend reports the existing XAudio2/DirectSound stack.
// Godot AudioServer will replace playback; sound-alias semantics stay in engine code.

struct IAudioBackend
{
    virtual ~IAudioBackend() = default;

    virtual const char *Name() const = 0;
    // True when this process should open a real audio device.
    virtual bool ShouldInit() = 0;
};
