#pragma once
#include "native_zone_file.h"
#include "fastfile_initialized_strings.h"
#include <database/db_assetlist_types.h>
#include <vector>
namespace bo1::fastfile {
enum class NativeLoadResult { Ok, Occupied, CallerState, Unsupported, Malformed,
    NameCollision, Capacity, Consumption, Referenced };
struct NativeRootRecord {
    XAssetType type{};
    std::size_t sourceStart{},sourceEnd{},nameSource{},nameBytes{},bufferSource{},bufferBytes{};
    std::uint64_t nativeName{},nativeValues{},nativeIndices{},diskEnd{},nativeEnd{};
    int columns{},rows{},length{};
    XAssetHeader registered{};
};
// External persistent state: it must outlive engine fatal/error recovery and all
// registered borrowed children. Genuine caller bootstrap supplies initialized
// registry pools/hash, DBHASH synchronization, Dvars/useFastFile, zone index and
// no-mod/new-name policy. This component never manufactures those services.
// Supported domain: no script strings, inline RawFile/StringTable roots, ASCII
// strings and exact earlier block4 string references. Other content is refused.
// Run executes stock root/Ptr/Add wrappers; it is not DB_LoadXAssets/Com_Init.
// After an engine longjmp caller must invoke Detach then Remove before file.Close.
// No owner with a nontrivial destructor may be local to that longjmp extent.
class NativeZoneLoad final {
public:
    NativeZoneLoad()=default;
    ~NativeZoneLoad();
    NativeZoneLoad(const NativeZoneLoad &)=delete;
    NativeZoneLoad &operator=(const NativeZoneLoad &)=delete;
    NativeLoadResult Prepare(NativeZoneFile &,unsigned zoneIndex);
    NativeLoadResult Run();
    void Detach();
    NativeLoadResult Remove(); // Scoped primary-entry removal, not full zone unload.
    const std::vector<NativeRootRecord> &Roots() const { return roots_; }
    XAssetList &Assets() { return list_; }
    NativeZoneFile *File() const { return file_; }
private:
    friend int NativeZoneReadCString(unsigned char *);
    friend void NativeZoneValidatePointerOffset(void *);
    NativeZoneFile *file_{};
    unsigned zone_{};
    XAssetList list_{};
    std::vector<NativeRootRecord> roots_;
    std::vector<InitializedStringRange> initialized_;
    std::uint64_t generation_{};
};
void NativeZoneReadData(unsigned char *,int);
int NativeZoneReadCString(unsigned char *);
}
