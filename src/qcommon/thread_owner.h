#pragma once
enum ThreadOwner : __int32
{                                       // XREF: ?Sys_SuspendDiscReads@@YAXW4ThreadOwner@@@Z/r
    THREAD_OWNER_NONE       = 0x0,
    THREAD_OWNER_DATABASE   = 0x1,
    THREAD_OWNER_CINEMATICS = 0x2,
    THREAD_OWNER_SHUTDOWN   = 0x3,
};
