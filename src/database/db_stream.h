#pragma once
#include "db_registry.h"

struct StreamPosInfo // sizeof=0x8
{                                       // XREF: .data:g_streamPosStack/r
    unsigned __int8 *pos;               // XREF: DB_PopStreamPos(void)+51/r
    unsigned int index;                 // XREF: DB_PushStreamPos(uint)+8F/w
};

void __cdecl DB_InitStreams(XBlock *blocks);
void __cdecl DB_PushStreamPos(unsigned int index);
void __cdecl DB_SetStreamIndex(unsigned int index);
void __cdecl DB_PopStreamPos();
unsigned __int8 *__cdecl DB_GetStreamPos();
unsigned __int8 *__cdecl DB_AllocStreamPos(int alignment);
void __cdecl DB_IncStreamPos(int size);
#if defined(BO1_PORT_X64)
// Next DB_IncStreamPos(nativeSize) advances the Win32 cursor by win32Size (for baked-offset remap).
void DB_SetExpandWin32Size(int win32Size);
void DB_SetExpandElem(int wElem, int nElem);
// Map a Win32 within-block offset to the native address after expansion drift.
unsigned __int8 *DB_StreamMapLookup(unsigned int blockIndex, unsigned int win32Within);
// Exact address written at a known Win32 cursor (e.g. XString start); preferred over drift marks.
void DB_StreamRegisterExact(unsigned int blockIndex, unsigned int win32Off, unsigned __int8 *native);
unsigned __int8 *DB_StreamExactLookup(unsigned int blockIndex, unsigned int win32Within);
unsigned int DB_StreamWin32Pos(unsigned int blockIndex);
const char *DB_StreamDescribe(unsigned int blockIndex, unsigned int win32Within);
// Pointer fields that FF alias offsets can name: Win32 position -> native slot.
void DB_StreamRegisterAliasSlot(unsigned int blockIndex, unsigned int win32Pos, uintptr_t *slot);
uintptr_t *DB_StreamAliasSlotLookup(unsigned int blockIndex, unsigned int win32Pos);
// Ghost cursor undercounted by `bytes`; advance win32 and retarget drift so
// subsequent MapLookup(cooked) hits the native addresses already written.
#endif
const void **__cdecl DB_InsertPointer();

// Stream trace (env BO1_STREAM_TRACE=<file>): same Win32-position log from the Win32 and x64 builds so they can be diffed.
// BO1_TRACE_ZONE=<zone> + BO1_TRACE_ASSET=<n> additionally log every stream op inside that one asset.
// Append a line to docs/port/x64_diag.log (x64 bring-up diagnostics; prefixed with the zone being loaded).
void Bo1_Diag(const char *fmt, ...);
// Log the current call stack as module RVAs (symbolize with llvm-symbolizer --relative-address).
void Bo1_DiagStack(const char *tag);
void DB_Trace_Zone(const char *zone);
void DB_Trace_AssetBegin(int index, int type);
void DB_Trace_AssetEnd(int index, int type);


extern XBlock *g_streamBlocks;
extern unsigned __int8 *g_streamPos;
extern unsigned int g_streamPosIndex;
extern unsigned int g_streamDelayIndex;
extern unsigned __int8 *g_streamPosArray[7];
extern unsigned int g_streamPosStackIndex;
