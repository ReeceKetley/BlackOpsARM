#pragma once

enum netadrtype_t : __int32
{                                                                             // XREF: netadr_t/r
        NA_BOT             = 0x0,
        NA_BAD             = 0x1,
        NA_LOOPBACK    = 0x2,
        NA_BROADCAST = 0x3,
        NA_IP                = 0x4,
};

struct netadr_t // sizeof=0x10
{                                                                             // XREF: G_InitGrenadeEntity(gentity_s *,gentity_s *)+355/o
        netadrtype_t type;                                    // XREF: CL_ConnectionlessPacket(int,netadr_t,msg_t *,int)+1FF/r
                                                                                // CL_ConnectionlessPacket(int,netadr_t,msg_t *,int)+24B/r ...
        unsigned __int8 ip[4];                            // XREF: CL_ConnectionlessPacket(int,netadr_t,msg_t *,int)+204/r
                                                                                // CL_ConnectionlessPacket(int,netadr_t,msg_t *,int)+250/r ...
        unsigned __int16 port;                            // XREF: CL_ConnectionlessPacket(int,netadr_t,msg_t *,int)+20A/r
                                                                                // CL_ConnectionlessPacket(int,netadr_t,msg_t *,int)+256/r ...
        // padding byte
        // padding byte
        int addrHandleIndex;                                // XREF: CL_ConnectionlessPacket(int,netadr_t,msg_t *,int)+210/r
};

