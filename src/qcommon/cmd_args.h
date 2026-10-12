#pragma once

struct CmdArgs // sizeof=0x28EC
{                                                                             // XREF: Cmd_Init(void)+10/o
                                                                                // Com_InitThreadData(int)+50/o ...
        int nesting;                                                // XREF: SV_Cmd_Argc(void)+3/r
                                                                                // SV_Cmd_Argc(void)+E/r ...
        int localClientNum[8];
        int controllerIndex[8];
        struct itemDef_s *itemDef[8];
        int argshift[8];
        int argc[8];                                                // XREF: SV_Cmd_Argc(void)+3A/r
                                                                                // SV_Cmd_Argv(int)+6E/r
        const char **argv[8];                             // XREF: SV_Cmd_Argv(int)+7D/r
        char textPool[8192];
        const char *argvPool[512];
        int usedTextPool[8];
        int totalUsedArgvPool;                            // XREF: SV_Cmd_TokenizeString(char const *)+F/r
        int totalUsedTextPool;
};


