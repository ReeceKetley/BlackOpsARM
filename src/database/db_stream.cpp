#include <platform/bo1_native_width.h>
#include "db_stream.h"
#include "db_file_load.h"
#include <universal/assertive.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#if BO1_NATIVE_64BIT
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <cstdio>
#endif

XBlock *g_streamBlocks;
unsigned __int8 *g_streamPos;
unsigned int g_streamPosIndex;
unsigned int g_streamDelayIndex;
unsigned __int8 *g_streamPosArray[7];
unsigned int g_streamPosStackIndex;

extern int g_dbTopAssetType;
StreamPosInfo g_streamPosStack[64];

// ---- stream trace (identical output format in Win32 and x64 builds) ----
static FILE *g_traceFile;
static bool g_traceInit;
static char g_traceZone[64];
static char g_traceZoneFilter[64];
static int g_traceAssetFilter = -1;
static int g_traceCurAsset = -1;
static bool g_traceOpsOn;
static int g_traceOpNo;
static int g_traceStackOp = -1;

static void DB_TraceInit()
{
    if (g_traceInit)
        return;
    g_traceInit = true;
    char buf[512];
    size_t n = 0;
    if (getenv_s(&n, buf, sizeof(buf), "BO1_STREAM_TRACE") == 0 && n > 1)
        fopen_s(&g_traceFile, buf, "w");
    if (getenv_s(&n, g_traceZoneFilter, sizeof(g_traceZoneFilter), "BO1_TRACE_ZONE") != 0)
        g_traceZoneFilter[0] = 0;
    if (getenv_s(&n, buf, sizeof(buf), "BO1_TRACE_ASSET") == 0 && n > 1)
        g_traceAssetFilter = atoi(buf);
    if (getenv_s(&n, buf, sizeof(buf), "BO1_TRACE_STACK_OP") == 0 && n > 1)
        g_traceStackOp = atoi(buf);
}

static unsigned int DB_TraceW32(unsigned int idx);
static void DB_TraceOp(char kind)
{
    if (!g_traceFile || !g_traceOpsOn)
        return;
    fprintf(g_traceFile, "%c %u %08X f%llX\n", kind, g_streamPosIndex, DB_TraceW32(g_streamPosIndex), DB_XFileBytesLoaded());
    // BO1_TRACE_STACK_OP=<n>: dump the call stack (module RVAs) at the n-th non-block-0 op of the traced asset.
    if (g_streamPosIndex != 0 && ++g_traceOpNo == g_traceStackOp)
    {
        void *frames[24];
        const unsigned short n = RtlCaptureStackBackTrace(1, 24, frames, nullptr);
        const unsigned char *base = reinterpret_cast<const unsigned char *>(GetModuleHandleA(nullptr));
        char line[512];
        int len = snprintf(line, sizeof(line), "STACK op#%d %c:", g_traceOpNo, kind);
        for (unsigned short i = 0; i < n && len < 480; ++i)
            len += snprintf(line + len, sizeof(line) - len, " %llx", (unsigned long long)(static_cast<const unsigned char *>(frames[i]) - base));
        Bo1_Diag("%s", line);
    }
}

void Bo1_Diag(const char *fmt, ...)
{
    // x64 bring-up diagnostics: BO1_DIAG_LOG names the file, default is x64_diag.log in the working directory.
    static char s_path[MAX_PATH];
    if (!s_path[0] && !GetEnvironmentVariableA("BO1_DIAG_LOG", s_path, sizeof(s_path)))
        strcpy_s(s_path, "x64_diag.log");
    FILE *f = nullptr;
    if (fopen_s(&f, s_path, "a") != 0 || !f)
        return;
    fprintf(f, "[%s] ", g_traceZone);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(f, fmt, ap);
    va_end(ap);
    fputc('\n', f);
    fclose(f);
}

void Bo1_DiagStack(const char *tag)
{
    void *frames[32];
    const unsigned short n = RtlCaptureStackBackTrace(1, 32, frames, nullptr);
    const unsigned char *exeBase = reinterpret_cast<const unsigned char *>(GetModuleHandleA(nullptr));
    char line[1200];
    int len = snprintf(line, sizeof(line), "STACK %s:", tag ? tag : "");
    for (unsigned short i = 0; i < n && len < 1100; ++i)
    {
        const unsigned char *addr = static_cast<const unsigned char *>(frames[i]);
        HMODULE mod = nullptr;
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               reinterpret_cast<LPCSTR>(addr), &mod) && mod && reinterpret_cast<const unsigned char *>(mod) != exeBase)
        {
            char path[260] = {};
            GetModuleFileNameA(mod, path, sizeof(path));
            const char *base = strrchr(path, '\\');
            len += snprintf(line + len, sizeof(line) - len, " %s+%llx", base ? base + 1 : path,
                            (unsigned long long)(addr - reinterpret_cast<const unsigned char *>(mod)));
        }
        else
            len += snprintf(line + len, sizeof(line) - len, " %llx", (unsigned long long)(addr - exeBase));
    }
    Bo1_Diag("%s", line);
}

void DB_Trace_Zone(const char *zone)
{
    DB_TraceInit();
    strncpy_s(g_traceZone, sizeof(g_traceZone), zone ? zone : "", _TRUNCATE);
    if (g_traceFile)
        fprintf(g_traceFile, "ZONE %s\n", g_traceZone);
}

void DB_Trace_AssetBegin(int index, int type)
{
    DB_TraceInit();
    g_traceCurAsset = index;
    g_traceOpNo = 0;
    g_traceOpsOn = g_traceFile && g_traceAssetFilter == index && g_traceZoneFilter[0] && !strcmp(g_traceZone, g_traceZoneFilter);
    if (g_traceOpsOn)
        fprintf(g_traceFile, "B %d %d\n", index, type);
}

void DB_Trace_AssetEnd(int index, int type)
{
    if (!g_traceFile)
        return;
    fprintf(g_traceFile, "S %d %d", index, type);
    for (unsigned int i = 0; i < 7; ++i)
        fprintf(g_traceFile, " %08X", DB_TraceW32(i));
    fprintf(g_traceFile, " f%llX\n", DB_XFileBytesLoaded());
    fflush(g_traceFile);
    g_traceOpsOn = false;
}

#if BO1_NATIVE_64BIT
namespace
{
// Drift mark: for Win32 offsets >= win32Off, add `drift` to get the native address.
// drift = (nativePos - block.data) - win32Pos after the commit that created the mark.
struct DriftMark
{
    unsigned int win32Off;
    int drift;
};

std::vector<DriftMark> g_driftMarks[7];
unsigned int g_streamWin32Pos[7];
int g_expandWin32Size = -1;
int g_expandElemW = 0;
int g_expandElemN = 0;

static int g_totalDriftDelta[7];

static void DB_StreamNoteDrift(unsigned int blockIndex, unsigned int win32Off, int drift)
{
    std::vector<DriftMark> &marks = g_driftMarks[blockIndex];
    const int prev = marks.empty() ? 0 : marks.back().drift;
    if (!marks.empty() && marks.back().win32Off == win32Off)
        marks.back().drift = drift;
    else
        marks.push_back(DriftMark{win32Off, drift});
    g_totalDriftDelta[blockIndex] += (drift - prev);
}
} // namespace

void DB_SetExpandWin32Size(int win32Size)
{
    g_expandWin32Size = win32Size;
}

void DB_SetExpandElem(int wElem, int nElem)
{
    g_expandElemW = wElem;
    g_expandElemN = nElem;
}

namespace
{
// Exact Win32-position -> native-address segments, one per stream commit (merged when contiguous and unexpanded).
// Every object a FastFile pointer/alias can name starts at a commit, so lookups of object starts are exact and the
// drift-mark reconstruction below is only a (logged) fallback.
struct StreamSeg
{
    unsigned int w;      // Win32 start within the block
    unsigned int wSize;  // Win32 footprint
    unsigned int n;      // native start within the block
    unsigned int nSize;  // native footprint
    unsigned int wElem;  // Win32 element stride when this commit is an expanded array (0 otherwise)
    unsigned int nElem;  // native element stride
};
std::vector<StreamSeg> g_streamSegs[7];
int g_segFallbackLogged;

static void DB_StreamNoteSeg(unsigned int blockIndex, unsigned int w, unsigned int wSize, unsigned int n, unsigned int nSize,
                             unsigned int wElem, unsigned int nElem)
{
    std::vector<StreamSeg> &segs = g_streamSegs[blockIndex];
    if (!segs.empty())
    {
        StreamSeg &last = segs.back();
        if (!wElem && !last.wElem && last.wSize == last.nSize && wSize == nSize && last.w + last.wSize == w &&
            last.n + last.nSize == n)
        {
            last.wSize += wSize;
            last.nSize += nSize;
            return;
        }
    }
    segs.push_back(StreamSeg{w, wSize, n, nSize, wElem, nElem});
}
}

void DB_StreamRegisterExact(unsigned int blockIndex, unsigned int win32Off, unsigned __int8 *native)
{
    (void)blockIndex;
    (void)win32Off;
    (void)native; // superseded by the commit segments
}

namespace
{
// Pointer-field slots that FastFile alias offsets may name: Win32 stream position -> native slot.
// Appended mostly in increasing order; kept as a sorted prefix plus a small unsorted tail.
struct AliasSlot
{
    unsigned int w;
    uintptr_t *slot;
};
std::vector<AliasSlot> g_aliasSlots[7];
size_t g_aliasSorted[7];

static bool AliasSlotLess(const AliasSlot &a, const AliasSlot &b)
{
    return a.w < b.w;
}

static void DB_AliasMergeTail(unsigned int blockIndex)
{
    std::vector<AliasSlot> &v = g_aliasSlots[blockIndex];
    size_t &sorted = g_aliasSorted[blockIndex];
    if (sorted == v.size())
        return;
    std::sort(v.begin() + sorted, v.end(), AliasSlotLess);
    std::inplace_merge(v.begin(), v.begin() + sorted, v.end(), AliasSlotLess);
    sorted = v.size();
}
}

void DB_StreamRegisterAliasSlot(unsigned int blockIndex, unsigned int win32Pos, uintptr_t *slot)
{
    if (blockIndex >= 7)
        return;
    g_aliasSlots[blockIndex].push_back(AliasSlot{win32Pos, slot});
}

uintptr_t *DB_StreamAliasSlotLookup(unsigned int blockIndex, unsigned int win32Pos)
{
    if (blockIndex >= 7)
        return nullptr;
    DB_AliasMergeTail(blockIndex);
    const std::vector<AliasSlot> &v = g_aliasSlots[blockIndex];
    const auto it = std::lower_bound(v.begin(), v.end(), AliasSlot{win32Pos, nullptr}, AliasSlotLess);
    if (it == v.end() || it->w != win32Pos)
        return nullptr;
    // Later registrations of the same position (re-expansions) win.
    auto last = it;
    while (last + 1 != v.end() && (last + 1)->w == win32Pos)
        ++last;
    return last->slot;
}

// Exact lookup only: nullptr when `win32Within` is not inside a recorded commit, or lies inside an expanded object
// (a size-changing struct/array) at an offset other than its start.
unsigned __int8 *DB_StreamExactLookup(unsigned int blockIndex, unsigned int win32Within)
{
    if (blockIndex >= 7 || !g_streamBlocks || !g_streamBlocks[blockIndex].data)
        return nullptr;
    const std::vector<StreamSeg> &segs = g_streamSegs[blockIndex];
    size_t lo = 0, hi = segs.size();
    while (lo < hi)
    {
        const size_t mid = (lo + hi) / 2;
        if (segs[mid].w <= win32Within)
            lo = mid + 1;
        else
            hi = mid;
    }
    if (lo == 0)
        return nullptr;
    const StreamSeg &seg = segs[lo - 1];
    if (win32Within >= seg.w + seg.wSize)
        return nullptr;
    const unsigned int delta = win32Within - seg.w;
    if (delta != 0 && seg.wSize != seg.nSize)
    {
        // Interior of an expanded array: element starts map exactly (stride changes per element).
        if (seg.wElem && delta % seg.wElem == 0)
            return g_streamBlocks[blockIndex].data + seg.n + (delta / seg.wElem) * seg.nElem;
        return nullptr;
    }
    return g_streamBlocks[blockIndex].data + seg.n + delta;
}

unsigned int DB_StreamWin32Pos(unsigned int blockIndex)
{
    return blockIndex < 7 ? g_streamWin32Pos[blockIndex] : 0;
}

// Debug: describe the commit segments around a Win32 position.
const char *DB_StreamDescribe(unsigned int blockIndex, unsigned int win32Within)
{
    static char buf[512];
    buf[0] = 0;
    if (blockIndex >= 7)
        return buf;
    const std::vector<StreamSeg> &segs = g_streamSegs[blockIndex];
    size_t lo = 0, hi = segs.size();
    while (lo < hi)
    {
        const size_t mid = (lo + hi) / 2;
        if (segs[mid].w <= win32Within)
            lo = mid + 1;
        else
            hi = mid;
    }
    int len = 0;
    for (size_t i = (lo >= 2 ? lo - 2 : 0); i < segs.size() && i < lo + 2; ++i)
        len += snprintf(buf + len, sizeof(buf) - len, "%s[w=%X+%u n=%X+%u]", i == lo - 1 ? ">" : " ",
                        segs[i].w, segs[i].wSize, segs[i].n, segs[i].nSize);
    return buf;
}

unsigned __int8 *DB_StreamMapLookup(unsigned int blockIndex, unsigned int win32Within)
{
    if (blockIndex >= 7 || !g_streamBlocks || !g_streamBlocks[blockIndex].data)
        return nullptr;

    if (unsigned __int8 *exact = DB_StreamExactLookup(blockIndex, win32Within))
        return exact;
    if (g_segFallbackLogged < 64)
    {
        ++g_segFallbackLogged;
        Bo1_Diag("MapLookup: no exact segment block=%u win32=%08X topType=%d cur=%u (interior of expanded object or gap) - drift fallback",
                 blockIndex, win32Within, g_dbTopAssetType, g_streamPosIndex);
    }

    const std::vector<DriftMark> &marks = g_driftMarks[blockIndex];
    int drift = 0;
    for (size_t i = 0; i < marks.size(); ++i)
    {
        if (marks[i].win32Off <= win32Within)
            drift = marks[i].drift;
        else
            break;
    }
    return g_streamBlocks[blockIndex].data + win32Within + drift;
}
#endif

static unsigned int DB_TraceW32(unsigned int idx)
{
#if BO1_NATIVE_64BIT
    return g_streamWin32Pos[idx];
#else
    if (!g_streamBlocks || !g_streamBlocks[idx].data)
        return 0;
    const unsigned __int8 *p = (idx == g_streamPosIndex) ? g_streamPos : g_streamPosArray[idx];
    return p ? (unsigned int)(p - g_streamBlocks[idx].data) : 0;
#endif
}

void __cdecl DB_InitStreams(XBlock *blocks)
{
    int i; // [esp+0h] [ebp-4h]

    g_streamBlocks = blocks;
    g_streamPos = blocks->data;
    g_streamPosIndex = 0;
    g_streamDelayIndex = 0;
    g_streamPosArray[0] = 0;
    g_streamPosStackIndex = 0;
    for ( i = 0; i < 7; ++i )
    {
        if ( i )
            g_streamPosArray[i] = blocks[i].data;
#if BO1_NATIVE_64BIT
        g_streamWin32Pos[i] = 0;
        g_driftMarks[i].clear();
        g_driftMarks[i].push_back(DriftMark{0, 0});
#endif
    }
#if BO1_NATIVE_64BIT
    g_expandWin32Size = -1;
    for (int bi = 0; bi < 7; ++bi)
    {
        g_streamSegs[bi].clear();
        g_aliasSlots[bi].clear();
        g_aliasSorted[bi] = 0;
    }
#endif
}

void __cdecl DB_PushStreamPos(unsigned int index)
{
    iassert(index < ARRAY_COUNT(g_streamPosArray));
    iassert(g_streamPosIndex < ARRAY_COUNT(g_streamPosArray));
    iassert(g_streamPosStackIndex < ARRAY_COUNT(g_streamPosStack));

    g_streamPosStack[g_streamPosStackIndex].index = g_streamPosIndex;
    DB_SetStreamIndex(index);
    g_streamPosStack[g_streamPosStackIndex++].pos = g_streamPos;
}

void __cdecl DB_SetStreamIndex(unsigned int index)
{
    if ( index != g_streamPosIndex )
    {
        g_streamPosArray[g_streamPosIndex] = g_streamPos;
        g_streamPosIndex = index;
        g_streamPos = g_streamPosArray[index];
        g_streamPosArray[index] = 0;
    }
}

void __cdecl DB_PopStreamPos()
{
    if ( !g_streamPosStackIndex
        && !Assert_MyHandler(
                    "C:\\projects_pc\\cod\\codsrc\\src\\database\\db_stream.cpp",
                    83,
                    0,
                    "%s\n\t(g_streamPosStackIndex) = %i",
                    "(g_streamPosStackIndex > 0)",
                    0) )
    {
        __debugbreak();
    }
    --g_streamPosStackIndex;
    if ( !g_streamPosIndex )
        g_streamPos = g_streamPosStack[g_streamPosStackIndex].pos;
    DB_SetStreamIndex(g_streamPosStack[g_streamPosStackIndex].index);
}

unsigned __int8 *__cdecl DB_GetStreamPos()
{
    return g_streamPos;
}

unsigned __int8 *__cdecl DB_AllocStreamPos(int alignment)
{
    if ( !g_streamPos
        && !Assert_MyHandler("C:\projects_pc\cod\codsrc\src\database\db_stream.cpp", 110, 0, "%s", "g_streamPos") )
    {
        __debugbreak();
    }
    const unsigned __int8 *before = g_streamPos;
#if BO1_NATIVE_64BIT
    // `alignment` is the Win32 request, taken literally by the Win32 shadow cursor (the FastFile's offsets were baked
    // with it). The native cursor is aligned to at least 8 so expanded structs holding pointers are naturally aligned.
    const int nativeAlignment = alignment | static_cast<int>(sizeof(void *) - 1);
#else
    const int nativeAlignment = alignment;
#endif
    const uintptr_t aligned =
        static_cast<uintptr_t>(~(uintptr_t)nativeAlignment) &
        (reinterpret_cast<uintptr_t>(g_streamPos) + static_cast<uintptr_t>(nativeAlignment));
#if BO1_NATIVE_64BIT
    {
        const int nativePad = static_cast<int>(reinterpret_cast<unsigned __int8 *>(aligned) - before);
        const unsigned int win32Before = g_streamWin32Pos[g_streamPosIndex];
        const unsigned int win32Aligned =
            (win32Before + static_cast<unsigned int>(alignment)) &
            ~static_cast<unsigned int>(alignment);
        const int win32Pad = static_cast<int>(win32Aligned - win32Before);
        if (nativePad != win32Pad)
        {
            const int driftBefore =
                static_cast<int>(before - g_streamBlocks[g_streamPosIndex].data) -
                static_cast<int>(win32Before);
            DB_StreamNoteDrift(
                g_streamPosIndex,
                win32Before + static_cast<unsigned int>(win32Pad),
                driftBefore + (nativePad - win32Pad));
        }
        g_streamWin32Pos[g_streamPosIndex] = win32Before + static_cast<unsigned int>(win32Pad);
    }
#endif
    g_streamPos = reinterpret_cast<unsigned __int8 *>(aligned);
    DB_TraceOp('A');
    return g_streamPos;
}

void __cdecl DB_IncStreamPos(int size)
{
    iassert(g_streamPos);
    iassert(g_streamPos + size <= g_streamBlocks[g_streamPosIndex].data + g_streamBlocks[g_streamPosIndex].size);
#if BO1_NATIVE_64BIT
    {
        const int wSize = (g_expandWin32Size >= 0) ? g_expandWin32Size : size;
        g_expandWin32Size = -1;
        const unsigned int w0 = g_streamWin32Pos[g_streamPosIndex];
        if (size > 0 || wSize > 0)
            DB_StreamNoteSeg(g_streamPosIndex, w0, static_cast<unsigned int>(wSize),
                             static_cast<unsigned int>(g_streamPos - g_streamBlocks[g_streamPosIndex].data),
                             static_cast<unsigned int>(size), static_cast<unsigned int>(g_expandElemW),
                             static_cast<unsigned int>(g_expandElemN));
        g_expandElemW = g_expandElemN = 0;
        if (wSize != size)
        {
            const int driftBefore =
                static_cast<int>(g_streamPos - g_streamBlocks[g_streamPosIndex].data) -
                static_cast<int>(w0);
            const int driftAfter = driftBefore + (size - wSize);
            DB_StreamNoteDrift(g_streamPosIndex, w0 + static_cast<unsigned int>(wSize), driftAfter);
        }
        g_streamWin32Pos[g_streamPosIndex] = w0 + static_cast<unsigned int>(wSize);
    }
#endif
    g_streamPos += size;
    DB_TraceOp('I');
}

const void **__cdecl DB_InsertPointer()
{
    const void **pData; // [esp+0h] [ebp-4h]

    DB_PushStreamPos(4u);
    pData = (const void **)DB_AllocStreamPos(3); // Win32 pointer slot alignment; native cursor aligns to 8
#if BO1_NATIVE_64BIT
    // Retail FFs reserve 4-byte alias slots; store a full pointer but advance Win32 cursor by 4.
    DB_SetExpandWin32Size(4);
#endif
    DB_IncStreamPos(static_cast<int>(sizeof(void *)));
    DB_PopStreamPos();
    return pData;
}
