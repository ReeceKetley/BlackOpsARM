#pragma once

// Online / platform-services surface for offline Zombies.
// Windows backend is a local stub: no Steam ownership, matchmaking, or DemonWare.

struct IPlatformServices
{
    virtual ~IPlatformServices() = default;

    virtual const char *Name() const = 0;
    virtual bool IsOnline() const = 0;
    virtual bool HasSteam() const = 0;
    virtual bool HasDemonWare() const = 0;
};
