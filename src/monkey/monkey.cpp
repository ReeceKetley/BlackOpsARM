#include "monkey.h"
#include "monkey_comm.h"

#include <cstring>
#include <universal/q_shared.h>
#include <Windows.h>
#include <universal/dvar.h>
#include <universal/assertive.h>
#include <qcommon/common.h>
#include <qcommon/cmd.h>
#include <win32/win_common.h>
#include <universal/timing.h>
#if defined(__ANDROID__)
#include <platform/profile_services.h>
#include <cstdlib>
#endif

char g_MonkeyPrintBuffer[0x20000];
unsigned int g_MonkeyPrintBufferSize;
bool g_MonkeyGrabPrints;
extern bool g_MonkeyIsRunning;
unsigned __int64 g_MonkeyOokTimer;
unsigned __int64 g_MonkeyOokTimeoutTicks;

const dvar_s *in_monkey;

void __cdecl Monkey_Error(const char *text)
{
    if ( Monkey_IsRunning() )
    {
        if ( text )
            Monkey_Send("error", text);
        else
            Monkey_Send("error", g_MonkeyPrintBuffer);
        Monkey_DisconnectAndExit();
    }
}

void __cdecl Monkey_GrabComPrints(bool enable)
{
#if defined(__ANDROID__)
    if(!Monkey_IsRunning()) return;
    Sys_EnterCriticalSection(CRITSECT_CONSOLE);
    if(!enable || !g_MonkeyGrabPrints) { g_MonkeyPrintBuffer[0]=0; g_MonkeyPrintBufferSize=0; }
    g_MonkeyGrabPrints=enable;
    Sys_LeaveCriticalSection(CRITSECT_CONSOLE);
#else
    if ( Monkey_IsRunning() )
    {
        if ( enable )
        {
            if ( !g_MonkeyGrabPrints )
            {
                g_MonkeyPrintBuffer[0] = 0;
                g_MonkeyPrintBufferSize = 0;
            }
            g_MonkeyGrabPrints = 1;
        }
        else
        {
            g_MonkeyPrintBuffer[0] = 0;
            g_MonkeyPrintBufferSize = 0;
            g_MonkeyGrabPrints = 0;
        }
    }
#endif
}

void __cdecl Monkey_ComPrintHook(const char *msg)
{
#if defined(__ANDROID__)
    if(!msg) { Com_Error(ERR_FATAL,"Monkey print capture received null text"); std::abort(); }
    Sys_EnterCriticalSection(CRITSECT_CONSOLE);
    if(g_MonkeyGrabPrints) {
        constexpr char marker[]="\n(Message truncated)\n";
        constexpr std::size_t capacity=sizeof(g_MonkeyPrintBuffer);
        const std::size_t count=strnlen(msg,capacity);
        std::size_t used=g_MonkeyPrintBufferSize;
        if(used>=capacity) used=capacity-1;
        if(count>=capacity-used-1) {
            constexpr std::size_t markerBytes=sizeof(marker)-1;
            if(used>capacity-markerBytes-1) used=capacity-markerBytes-1;
            memcpy(g_MonkeyPrintBuffer+used,marker,sizeof(marker));
            g_MonkeyPrintBufferSize=static_cast<unsigned int>(used+markerBytes);
        } else {
            memcpy(g_MonkeyPrintBuffer+used,msg,count);
            g_MonkeyPrintBuffer[used+count]=0;
            g_MonkeyPrintBufferSize=static_cast<unsigned int>(used+count);
        }
    }
    Sys_LeaveCriticalSection(CRITSECT_CONSOLE);
#else
    unsigned int v1; // kr00_4
    char v2; // dl
    char *v3; // [esp+10h] [ebp-58h]
    const char *v4; // [esp+14h] [ebp-54h]
    int v5; // [esp+18h] [ebp-50h]
    int msg_len; // [esp+64h] [ebp-4h]

    if ( g_MonkeyGrabPrints )
    {
        msg_len = strlen(msg);
        if ( g_MonkeyPrintBufferSize + msg_len >= 0x1FFFF )
        {
            v1 = strlen("\n(Message truncated)\n");
            if ( 0x20000 - (v1 + 1) >= g_MonkeyPrintBufferSize )
                v5 = g_MonkeyPrintBufferSize;
            else
                v5 = 0x20000 - (v1 + 1);
            v4 = "\n(Message truncated)\n";
            v3 = &g_MonkeyPrintBuffer[v5];
            do
            {
                v2 = *v4;
                *v3++ = *v4++;
            }
            while ( v2 );
            g_MonkeyPrintBufferSize = 0x20000;
        }
        else
        {
            strcat(g_MonkeyPrintBuffer, msg);
            g_MonkeyPrintBufferSize += msg_len;
        }
    }
#endif
}

void __cdecl Monkey_Start()
{
#if defined(__ANDROID__)
    in_monkey=_Dvar_RegisterBool("in_monkey",0,0,"Randomly generate input");
    const char *zookeeper=Dvar_GetString("zookeeper");
    if(zookeeper && *zookeeper) {
        if(!Monkey_InitCommunication(zookeeper)) {
            Com_Error(ERR_FATAL,"Monkey communication initialization failed"); std::abort();
        }
        g_MonkeyOokTimer=BO1_ProfileTicks();
        g_MonkeyOokTimeoutTicks=BO1_PROFILE_TICKS_PER_SECOND;
        Assert_SetMonkeyCallbackHandler(Monkey_AssertCallback);
        __atomic_store_n(&g_MonkeyIsRunning,true,__ATOMIC_RELEASE);
    }
#else
    LARGE_INTEGER PerformanceCount; // [esp+0h] [ebp-Ch] BYREF
    const char *zookeeper; // [esp+8h] [ebp-4h]

    in_monkey = _Dvar_RegisterBool("in_monkey", 0, 0, "Randomly generate input");
    zookeeper = Dvar_GetString("zookeeper");
    if ( zookeeper )
    {
        if ( *zookeeper )
        {
            Monkey_InitCommunication(zookeeper);
            g_MonkeyIsRunning = 1;
            QueryPerformanceCounter(&PerformanceCount);
            g_MonkeyOokTimer = PerformanceCount.QuadPart;
            g_MonkeyOokTimeoutTicks = (unsigned __int64)(1000.0 / msecPerRawTimerTick);
            Assert_SetMonkeyCallbackHandler(Monkey_AssertCallback);
        }
    }
#endif
}

void __cdecl Monkey_AssertCallback(const char *assertMsg)
{
#if defined(__ANDROID__)
    if(!Monkey_IsRunning()) return;
    Monkey_Send("assert",assertMsg);
    Monkey_ShutdownCommunication();
    __atomic_store_n(&g_MonkeyIsRunning,false,__ATOMIC_RELEASE);
#else
    Monkey_Send("assert", assertMsg);
    Monkey_ShutdownCommunication();
#endif
}

void __cdecl Monkey_Frame()
{
    char message[1024]; // [esp+0h] [ebp-488h] BYREF
    char type[132]; // [esp+400h] [ebp-88h] BYREF

    if ( Monkey_IsRunning() )
    {
        if ( Monkey_Recv(type, 128, message, 1024) )
        {
            if ( !I_stricmp(type, "command") )
            {
                Com_Printf(16, "Monkey: Recieved command '%s'\n", message);
                Cbuf_AddText(0, message);
                Cbuf_AddText(0, "\n");
            }
            if ( I_stricmp(type, "command_capture") )
            {
                Com_Printf(16, "Monkey: Unknown command type '%s', message='%s'\n", type, message);
            }
            else
            {
                Com_Printf(16, "Monkey: Recieved command_capture '%s'\n", message);
                Cbuf_AddText(0, message);
                Cbuf_AddText(0, "\n");
                Sys_EnterCriticalSection(CRITSECT_CONSOLE);
                Monkey_GrabComPrints(0);
                Monkey_GrabComPrints(1);
                Cbuf_Execute(0, 0);
                Monkey_Send("output", g_MonkeyPrintBuffer);
                Monkey_GrabComPrints(0);
                Sys_LeaveCriticalSection(CRITSECT_CONSOLE);
            }
        }
        Monkey_KeepAlive();
    }
}

void __cdecl Monkey_KeepAlive()
{
#if defined(__ANDROID__)
    if(!Monkey_IsRunning()) return;
    const unsigned __int64 now=BO1_ProfileTicks();
    if(now>g_MonkeyOokTimer) {
        g_MonkeyOokTimer=now+g_MonkeyOokTimeoutTicks;
        Monkey_Send("ook","Ook, ook, eek!\n");
    } else if(g_MonkeyOokTimer-now>g_MonkeyOokTimeoutTicks) {
        g_MonkeyOokTimer=now+g_MonkeyOokTimeoutTicks;
        Monkey_Send("ook","Ook, ook, eek (strange happenings)!\n");
    }
#else
    LARGE_INTEGER PerformanceCount; // [esp+8h] [ebp-18h] BYREF
    const char *string; // [esp+10h] [ebp-10h]
    const char *ook; // [esp+14h] [ebp-Ch]
    unsigned __int64 now; // [esp+18h] [ebp-8h]

    if ( Monkey_IsRunning() )
    {
        QueryPerformanceCounter(&PerformanceCount);
        now = PerformanceCount.QuadPart;
        if ( PerformanceCount.QuadPart <= g_MonkeyOokTimer )
        {
            if ( g_MonkeyOokTimer > g_MonkeyOokTimeoutTicks + now )
            {
                g_MonkeyOokTimer = g_MonkeyOokTimeoutTicks + now;
                string = "Ook, ook, eek (strange happenings)!\n";
                Monkey_Send("ook", "Ook, ook, eek (strange happenings)!\n");
            }
        }
        else
        {
            g_MonkeyOokTimer = g_MonkeyOokTimeoutTicks + now;
            ook = "Ook, ook, eek!\n";
            Monkey_Send("ook", "Ook, ook, eek!\n");
        }
    }
#endif
}

void __cdecl Monkey_Event(const char *event)
{
    if ( Monkey_IsRunning() )
        Monkey_Send("event", event);
}

bool __cdecl Monkey_UseRandomInput()
{
    return in_monkey->current.enabled;
}


bool g_MonkeyIsRunning;
bool __cdecl Monkey_IsRunning()
{
#if defined(__ANDROID__)
    return __atomic_load_n(&g_MonkeyIsRunning,__ATOMIC_ACQUIRE);
#else
    return g_MonkeyIsRunning;
#endif
}
