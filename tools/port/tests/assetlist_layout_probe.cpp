// Full Windows registry inclusion, including legacy flag-off x64 selection.
#if defined(BO1_ASSETLIST_LIGHT_LAYOUT)
#include <database/db_assetlist_types.h>
#else
#include <database/db_registry.h>
#endif
#include <platform/bo1_native_width.h>
#include <cstdio>
int main(){
#if defined(BO1_ASSETLIST_LIGHT_LAYOUT)
    const char*scope="actual lightweight envelope header";
#else
    const char*scope="actual full registry";
#endif
    std::printf("PASS %s: pointer=%zu XAssetHeader=%zu XAsset=%zu ScriptStringList=%zu XAssetList=%zu native-selection=%d\n",scope,sizeof(void*),sizeof(XAssetHeader),sizeof(XAsset),sizeof(ScriptStringList),sizeof(XAssetList),BO1_NATIVE_64BIT);}
