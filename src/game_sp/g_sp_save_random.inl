// Logical simulation streams. Capture runs after SV_RunFrame on the game
// thread. A loader must never overwrite its own unrelated TLS CRT stream.
#include <atomic>
#include <server_mp/sv_init_mp.h>
#if defined(__ANDROID__)
#include <platform/android/engine_random.h>
#endif

extern unsigned int holdrand;   // game_mp/g_utils_mp.cpp: G_rand stream
extern unsigned int holdrand_0; // universal/com_math.cpp: flrand/irand stream

struct SPSavedRandomState
{
    unsigned int flags;
    unsigned int game;
    unsigned int math;
    unsigned int crt;
};
static_assert(sizeof(SPSavedRandomState) == 16, "Random stream wire words");
static SPSavedRandomState s_pendingSaveRandom;
static std::atomic<bool> s_saveRandomPending{false};

void G_SP_ClearPendingSaveRandomState()
{
    // Called from ordinary synchronized world init/shutdown. Pending data has
    // no resources; the next world must not consume an abandoned load's seeds.
    s_saveRandomPending.store(false, std::memory_order_release);
}

static void G_WriteSaveRandomState(MemoryFile *file, bool verification)
{
    SPSavedRandomState state{};
    if (verification)
    {
        if (!s_saveRandomPending.load(std::memory_order_acquire))
            Com_Error(ERR_DROP, "Save random: no pending state for decoder verification");
        state = s_pendingSaveRandom;
    }
    else
    {
        SV_CheckThread();
        if (s_saveRandomPending.load(std::memory_order_acquire))
            Com_Error(ERR_DROP, "Save random: simulation has not consumed its pending state");
        state.game = holdrand;
        state.math = holdrand_0;
#if defined(__ANDROID__)
        state.flags = 1;
        state.crt = bo1::android::CrtGetState();
#endif
    }
    MemFile_WriteInt(file, 0x31474E52); // RNG1
    MemFile_WriteData(file, sizeof(state), (unsigned char *)&state);
}

static void G_ReadSaveRandomState(MemoryFile *file)
{
    unsigned int magic = 0;
    SPSavedRandomState state{};
    MemFile_ReadData(file, 4, (unsigned char *)&magic);
    MemFile_ReadData(file, sizeof(state), (unsigned char *)&state);
    if (magic != 0x31474E52 || state.flags > 1)
        Com_Error(ERR_DROP, "Save random: unsupported random section");
#if defined(__ANDROID__)
    if (state.flags != 1)
        Com_Error(ERR_DROP, "Save random: capture lacks Android simulation CRT state");
#else
    if (state.flags != 0)
        Com_Error(ERR_DROP, "Save random: incompatible platform CRT state");
#endif
    if (s_saveRandomPending.load(std::memory_order_acquire))
        Com_Error(ERR_DROP, "Save random: prior pending state has not been consumed");
    // The existing Com_SyncThreads load boundary excludes the simulation owner
    // while this record is published. G_RunFrame acquires it on its own thread.
    s_pendingSaveRandom = state;
    s_saveRandomPending.store(true, std::memory_order_release);
    Com_Printf(15, "loadgame random: staged game/math/CRT streams for simulation-thread consumption; loader TLS unchanged\n");
}

void G_SP_ApplyPendingSaveRandomState()
{
    if (!s_saveRandomPending.load(std::memory_order_acquire))
        return;
    SV_CheckThread();
    const SPSavedRandomState state = s_pendingSaveRandom;
    holdrand = state.game;
    holdrand_0 = state.math;
#if defined(__ANDROID__)
    bo1::android::CrtSrand(state.crt);
#endif
    s_saveRandomPending.store(false, std::memory_order_release);
    Com_Printf(15, "loadgame random: applied saved streams on simulation thread\n");
}
