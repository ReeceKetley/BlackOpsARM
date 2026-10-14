#pragma once
#include <string.h>

struct bdSecurityKey // sizeof=0x10
{                                                                             // XREF: .data:g_secKey/r
    unsigned __int8 ab[16];                         // XREF: dwCreateSession(overlappedTask * const,MatchMakingInfo * const)+139/r

    bdSecurityKey(const struct bdSecurityKey *src)
    {
        memcpy(this->ab, src->ab, sizeof(bdSecurityKey));
    }
    bdSecurityKey()
    {
        memset(this->ab, 1u, sizeof(bdSecurityKey));
    }
};

struct bdSecurityID // sizeof=0x8
{                                                                             // XREF: bdQoSProbe::bdQoSProbeEntryWrapper/r
    unsigned __int8 ab[8];                            // XREF: PM_Weapon_FireWeapon+17D/o

    bdSecurityID(const struct bdSecurityID *src)
    {
        memcpy(this->ab, src->ab, sizeof(bdSecurityID));
    }

    bdSecurityID()
    {
        memset(this->ab, 1u, sizeof(bdSecurityID));
    }

    bool operator==(const bdSecurityID &other) const
    {
        return memcmp(this->ab, other.ab, sizeof(bdSecurityID)) == 0;
    }

    bool operator!=(const bdSecurityID &other) const
    {
        return !(*this == other);
    }
};

