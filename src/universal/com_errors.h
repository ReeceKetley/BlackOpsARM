#pragma once

enum errorParm_t : __int32
{                                                                             // XREF: ?Com_Error@@YAXW4errorParm_t@@PBDZZ/r
        ERR_FATAL                        = 0x0,
        ERR_DROP                         = 0x1,
        ERR_SERVERDISCONNECT = 0x2,
        ERR_DISCONNECT             = 0x3,
        ERR_SCRIPT                     = 0x4,
        ERR_SCRIPT_DROP            = 0x5,
        ERR_LOCALIZATION         = 0x6,
};
