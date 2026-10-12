#pragma once
#include <platform/bo1_native_width.h>
#include <cstddef>
#include <cstdint>
#include "db_assetlist_types.h"
#include <universal/com_stringtable.h>
#include "db_rawfile_types.h"

#define POOLSIZE_STRINGTABLE      576
#define POOLSIZE_STRINGTABLE_MOD 768 // mod: capacity for bo1_mod_zones; in use only with it on the command line (DB_InitXAssets)

#define POOLSIZE_RAWFILE          1024
#define POOLSIZE_RAWFILE_MOD 2048 // mod: capacity for bo1_mod_zones; in use only with it on the command line (DB_InitXAssets)

template <typename T>
union XAssetPoolEntry // sizeof=0x10
{                                       // ...
    XAssetPoolEntry()
    {
        next = NULL;
    }
    T entry;
    XAssetPoolEntry<T> *next;
};

template <typename T, int LEN>
struct XAssetPool
{
    XAssetPoolEntry<T> *freeHead;
    XAssetPoolEntry<T> entries[LEN];
};

template <typename T>
XAssetHeader DB_AllocXAsset(void *pool)
{
    XAssetHeader header;
    XAssetPool<T, 1> *castPool = (XAssetPool<T, 1> *) pool;
    XAssetPoolEntry<T> *freeHead = castPool->freeHead;

    if (freeHead)
    {
        header.data = &freeHead->entry;
        castPool->freeHead = freeHead->next;
    }
    else
    {
        header.data = NULL;
    }

    return header;
}

template <typename T>
void __cdecl DB_InitPool(void *arg, int32_t size)
{
    //XAssetPool<RawFile, POOLSIZE_RAWFILE_MOD> *pool = (XAssetPool<RawFile, POOLSIZE_RAWFILE_MOD>*)arg;
    T *pool = (T *)arg;
    pool->freeHead = &pool->entries[0];
    for (int32_t i = 0; i < size - 1; i++)
    {
        pool->entries[i].next = &pool->entries[i + 1];
    }
    pool->entries[size - 1].next = NULL;
}

struct EmblemSet;
extern XAssetPool<StringTable, POOLSIZE_STRINGTABLE_MOD> g_StringTablePool;
void __cdecl DB_FreeXAssetHeader_EmblemSet_(void *pool, XAssetHeader header);
void __cdecl DB_CloneXAssetInternal(const XAsset *from, XAsset *to);
int __cdecl DB_SizeofXAsset_StringTable_();
int __cdecl DB_GetXAssetTypeSize(int type);
extern int(__cdecl *DB_GetXAssetSizeHandler[43])();

extern XAssetPool<RawFile, POOLSIZE_RAWFILE_MOD> g_RawFilePool;
int __cdecl DB_SizeofXAsset_RawFile_();
