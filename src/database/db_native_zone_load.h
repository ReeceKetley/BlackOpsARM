#pragma once
#if defined(BO1_NATIVE_ZONE_LOAD) && (!defined(BO1_STRINGTABLE_INDEX_GATE) || !defined(BO1_REGISTRY_ADD_GATE) || !defined(BO1_ASSETLIST_GATE) || !defined(BO1_RAWFILE_PAYLOAD_GATE))
#error BO1_NATIVE_ZONE_LOAD requires the explicit bounded native compilation selection
#endif
#include "db_asset_entry_types.h"
#include "db_rawfile_load.h"
#include "db_scalar_load.h"
#include <universal/com_stringtable.h>
#include <universal/com_errors.h>
extern XAsset *varXAsset;
extern XAssetHeader *varXAssetHeader;
extern RawFile **varRawFilePtr;
extern StringTableCell *varStringTableCell;
extern StringTable *varStringTable;
extern StringTable **varStringTablePtr;
XAsset *__cdecl AllocLoad_FxElemVisStateSample();
unsigned char *__cdecl AllocLoad_XBlendInfo();
void __cdecl Load_RawFilePtr(bool);
void __cdecl Load_StringTableCell(bool);
void __cdecl Load_StringTableCellArray(bool,int);
void __cdecl Load_StringTable(bool);
void __cdecl Load_StringTablePtr(bool);
void __cdecl Load_XAsset(bool);
void __cdecl Load_XAssetHeader(bool);
void __cdecl Load_RawFileAsset(XAssetHeader *);
void __cdecl Load_StringTableAsset(XAssetHeader *);
void __cdecl Com_Error(errorParm_t,const char *,...);
bool DB_NativeZonePolicyReady(unsigned);
namespace bo1::fastfile {
void NativeZoneReadData(unsigned char *,int);
int NativeZoneReadCString(unsigned char *);
void NativeZoneValidatePointerOffset(void *);
}
