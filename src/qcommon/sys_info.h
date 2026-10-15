#pragma once
struct __declspec(align(8)) SysInfo // sizeof=0x260
{                                                                             // XREF: .data:sys_info/r
                                                                                // ?Com_SetRecommended@@YAXHH@Z/r
        double cpuGHz;                                 // XREF: Sys_RegisterInfoDvars+DA/r
                                                                                // Sys_Init(void)+1B7/r ...
        double configureGHz;                     // XREF: Com_SetRecommended(int,int)+31/r
                                                                                // Com_SetRecommended(int,int)+41/w ...
        int logicalCpuCount;                                // XREF: Sys_Init(void)+13F/r
                                                                                // Sys_Init(void)+165/r ...
        int physicalCpuCount;                             // XREF: Sys_Init(void)+17B/r
                                                                                // Sys_Init(void)+1A1/r
        int sysMB;                                                    // XREF: Com_SetRecommended(int,int)+49/r
                                                                                // Com_SetRecommended(int,int)+55/w ...
        char gpuDescription[512];                     // XREF: Com_SetRecommended(int,int)+125/o
                                                                                // Sys_HasInfoChanged(void):loc_7D9644/o ...
        bool SSE;                                                     // XREF: Sys_RegisterInfoDvars+A5/r
                                                                                // Sys_Init(void)+211/r ...
        char cpuVendor[13];                                 // XREF: Sys_Init(void):loc_7DA2D7/o
                                                                                // Sys_FindInfo+90/o
        char cpuName[49];                                     // XREF: Sys_RegisterInfoDvars+FC/o
                                                                                // Sys_Init(void)+12B/o ...
        // padding byte
        // padding byte
        // padding byte
        // padding byte
        // padding byte
};
extern SysInfo sys_info;
