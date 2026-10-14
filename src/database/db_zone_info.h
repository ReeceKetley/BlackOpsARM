#pragma once
struct XZoneInfo // sizeof=0xC
{                                       // XREF: DB_UpdateDebugZone(void)+1E/w
    const char *name;                   // XREF: DB_UpdateDebugZone(void)+15/w
    int allocFlags;                     // XREF: DB_UpdateDebugZone(void)+18/w
    int freeFlags;                      // XREF: DB_UpdateDebugZone(void)+1B/w
};
