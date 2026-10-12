#pragma once
#include <csetjmp>
#include <qcommon/cmd_args.h>
#include <qcommon/trace_thread_info.h>

struct va_info_t
{
    char va_string[4][1024]; // see va(), the modulo is 4
    int index;
};

extern va_info_t va_info[15];
extern TraceThreadInfo g_traceThreadInfo[15];
extern jmp_buf g_com_error[15];
extern CmdArgs g_cmd_args[2];
void __cdecl Com_InitThreadData(int threadContext);
void __cdecl Cmd_ResetArgs(CmdArgs *args);
