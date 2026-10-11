// check_weapon_fields.py appends the actual production tables, initializers and
// KeyValueToField implementation. Asset registrars below return opaque pointers;
// this checks field memory access, not loading loose assets or special enums.
#include <Windows.h>
#include <intrin.h>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "universal/csp_field_offset.h"
#include "bgame/bg_weapons_load_obj.h"
#include "flame/flame_system.h"
#include "clientscript/cscr_stringlist.h"

extern cspField_t weaponDefFields[748];
extern cspField_t flameTableFields[119];
void InitWeaponDef(WeaponFullDef *);
void InitFlameTable(flameTable *);
static void BG_CopyWeaponFullDefForUpdate(WeaponFullDef *, const WeaponVariantDef *);
static void BG_CommitWeaponFullDefUpdate(WeaponVariantDef *, WeaponFullDef *);

// Unused header-level network tables reference these symbols.
#define UNUSED_NET_FIELD(name) decltype(name) name{}
UNUSED_NET_FIELD(eventEntityStateFields);
UNUSED_NET_FIELD(actorStateFields);
UNUSED_NET_FIELD(vehicleEntityStateFields);
UNUSED_NET_FIELD(planeStateFields);
UNUSED_NET_FIELD(helicopterEntityStateFields);
UNUSED_NET_FIELD(turretEntityStateFields);
UNUSED_NET_FIELD(loopFxEntityStateFields);
UNUSED_NET_FIELD(fxStateFields);
UNUSED_NET_FIELD(soundBlendEntityStateFields);
UNUSED_NET_FIELD(scriptMoverStateFields);
UNUSED_NET_FIELD(missileEntityStateFields);
UNUSED_NET_FIELD(itemEntityStateFields);
UNUSED_NET_FIELD(corpseEntityStateFields);
UNUSED_NET_FIELD(playerEntityStateFields);
UNUSED_NET_FIELD(archivedEntityFields);
UNUSED_NET_FIELD(entityStateFields);
UNUSED_NET_FIELD(clientStateFields);
UNUSED_NET_FIELD(playerStateFields);
UNUSED_NET_FIELD(objectiveFields);
UNUSED_NET_FIELD(matchStateFields);

static void Check(bool ok, const char *message)
{
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}

const dvar_t *dedicated = nullptr;
bool Assert_MyHandler(const char *, int, int, const char *, ...)
{
    Check(false, "engine assertion");
    return true;
}
void Com_Error(errorParm_t, const char *, ...) { Check(false, "engine error"); }
char *va(const char *, ...) { static char message[] = "field error"; return message; }
void I_strncpyz(char *dest, const char *src, int size)
{
    std::strncpy(dest, src, size - 1);
    dest[size - 1] = 0;
}

static void *opaqueAsset;
const FxEffectDef *FX_Register(const char *) { return static_cast<FxEffectDef *>(opaqueAsset); }
Material *Material_RegisterHandle(char *, int) { return static_cast<Material *>(opaqueAsset); }
XModel *R_RegisterModel(char *) { return static_cast<XModel *>(opaqueAsset); }
PhysPreset *PhysPreset_Register(const char *) { return static_cast<PhysPreset *>(opaqueAsset); }
unsigned int SL_GetLowercaseString(const char *, unsigned int, scriptInstance_t) { return 0x1234; }
static char copiedString[] = "native string";
static void CopyString(unsigned char *member, const char *)
{
    *reinterpret_cast<const char **>(member) = copiedString;
}

template <typename T> static T *GuardedObject(unsigned char *allocation)
{
    // One committed page followed by an inaccessible page. Place the object
    // against the guard and check a prefix canary after each operation.
    constexpr size_t pages = (sizeof(T) + 256 + 4095) / 4096;
    auto *region = allocation;
    Check(VirtualAlloc(region, pages * 4096, MEM_COMMIT, PAGE_READWRITE) == region, "commit test pages");
    auto *object = reinterpret_cast<T *>(region + pages * 4096 - sizeof(T));
    std::memset(reinterpret_cast<unsigned char *>(object) - 256, 0x5a, 256);
    return object;
}

template <typename T> static void CheckCanary(T *object)
{
    auto *prefix = reinterpret_cast<unsigned char *>(object) - 256;
    for (int i = 0; i < 256; ++i) Check(prefix[i] == 0x5a, "prefix canary");
}

static const cspField_t *Find(const cspField_t *fields, size_t count, const char *name)
{
    for (size_t i = 0; i < count; ++i)
        if (!std::strcmp(fields[i].szName, name)) return &fields[i];
    Check(false, name);
    return nullptr;
}

template <typename T> static size_t TestWrites(T *object, const cspField_t *fields, size_t count)
{
    size_t checked = 0;
    for (size_t i = 0; i < count; ++i)
    {
        const auto &field = fields[i];
        if (field.iFieldType >= 15) continue; // Weapon enums/maps use a separate parser.
        size_t width = 4;
        if (field.iFieldType == 0 || (field.iFieldType >= 9 && field.iFieldType <= 13)) width = sizeof(void *);
        if (field.iFieldType == 5) width = 1;
        if (field.iFieldType == 14) width = 2;
        Check(field.iFieldType != 1 && field.iFieldType != 2 && field.iFieldType != 3, "unexpected inline string");
        Check(field.iOffset >= 0 && field.iOffset + width <= sizeof(T), field.szName);
        std::memset(object, 0xa5, sizeof(T));
        char value[] = "1.25";
        Check(KeyValueToField(reinterpret_cast<unsigned char *>(object), &field, value, 0, nullptr, CopyString), field.szName);
        const auto *bytes = reinterpret_cast<unsigned char *>(object);
        for (size_t j = 0; j < sizeof(T); ++j)
            if (j < field.iOffset || j >= field.iOffset + width)
                Check(bytes[j] == 0xa5, field.szName);
        if (width == sizeof(void *) && (field.iFieldType == 0 || field.iFieldType >= 9))
        {
            void *actual;
            std::memcpy(&actual, bytes + field.iOffset, sizeof(actual));
            Check(actual == (field.iFieldType == 0 ? copiedString : opaqueAsset), field.szName);
        }
        CheckCanary(object);
        ++checked;
    }
    return checked;
}

static void TestUpdate(WeaponFullDef *original)
{
    std::memset(original, 0, sizeof(*original));
    InitWeaponDef(original);
    auto *oldModel = reinterpret_cast<XModel *>(static_cast<unsigned char *>(opaqueAsset) + 32);
    for (auto &model : original->gunXModel) model = oldModel;
    for (auto &animation : original->szXAnims) animation = "original animation";
    for (auto &damage : original->locationDamageMultipliers) damage = 2.0f;
    original->hideTags[3] = 123;
    original->notetrackSoundMapKeys[2] = 456;
    original->notetrackSoundMapValues[2] = 789;
    original->weapDef.damage = 77;
    std::vector<unsigned char> before(sizeof(*original));
    std::memcpy(before.data(), original, before.size());
    WeaponFullDef copy;
    BG_CopyWeaponFullDefForUpdate(&copy, &original->weapVariantDef);
    Check(!std::memcmp(before.data(), original, before.size()), "snapshot changed source");
    Check(copy.weapVariantDef.weapDef == &copy.weapDef && copy.weapDef.gunXModel == copy.gunXModel,
        "snapshot must use local backing arrays");
    Check(copy.weapVariantDef.hideTags == copy.hideTags, "local hide tags");
    char value[] = "1.25";
    for (const char *name : {"gunModel16", "idleAnim", "locGun", "displayName"})
        Check(KeyValueToField(reinterpret_cast<unsigned char *>(&copy), Find(weaponDefFields, 748, name),
            value, 0, nullptr, CopyString), name);
    copy.hideTags[3] = 321;
    copy.notetrackSoundMapKeys[2] = 654;
    copy.notetrackSoundMapValues[2] = 987;
    Check(!std::memcmp(before.data(), original, before.size()), "uncommitted edit changed source");
    BG_CommitWeaponFullDefUpdate(&original->weapVariantDef, &copy);
    Check(original->weapVariantDef.weapDef == &original->weapDef, "definition pointer escaped");
    Check(original->weapDef.gunXModel == original->gunXModel, "model pointer escaped");
    Check(original->weapVariantDef.szXAnims == original->szXAnims, "animation pointer escaped");
    Check(original->weapDef.locationDamageMultipliers == original->locationDamageMultipliers, "damage pointer escaped");
    Check(original->weapVariantDef.szDisplayName == copiedString, "header edit lost");
    Check(original->szXAnims[1] == copiedString && !std::strcmp(original->szXAnims[0], "original animation"),
        "animation edit lost or adjacent element changed");
    Check(original->gunXModel[15] == opaqueAsset && original->gunXModel[14] == oldModel, "model edit lost or adjacent element changed");
    Check(original->locationDamageMultipliers[18] == 1.25f && original->locationDamageMultipliers[17] == 2.0f,
        "damage multiplier edit lost or adjacent element changed");
    Check(original->hideTags[3] == 321 && original->notetrackSoundMapKeys[2] == 654 &&
        original->notetrackSoundMapValues[2] == 987, "tag/map edit lost");
    Check(original->weapDef.damage == 77, "unmodified scalar changed");
    CheckCanary(original);
    // Missing optional arrays must retain null storage after unrelated edits.
    original->weapDef.worldModel = nullptr;
    BG_CopyWeaponFullDefForUpdate(&copy, &original->weapVariantDef);
    BG_CommitWeaponFullDefUpdate(&original->weapVariantDef, &copy);
    Check(original->weapDef.worldModel == nullptr, "optional array nullness changed");
}

int main()
{
    void *address = nullptr;
#ifdef BO1_PORT_X64
    address = reinterpret_cast<void *>(0x180000000ull);
#endif
    auto *region = static_cast<unsigned char *>(VirtualAlloc(address, 0x10000, MEM_RESERVE, PAGE_NOACCESS));
    Check(region != nullptr, "reserve guarded memory");
#ifdef BO1_PORT_X64
    Check(reinterpret_cast<uintptr_t>(region) > 0x80000000ull, "native memory must exceed 2 GB");
#endif
    opaqueAsset = region;
    auto *weapon = GuardedObject<WeaponFullDef>(region);
    auto *flame = GuardedObject<flameTable>(region + 0x8000);
    std::memset(weapon, 0, sizeof(*weapon));
    InitWeaponDef(weapon);
    Check(weapon->weapVariantDef.weapDef == &weapon->weapDef, "weapon definition pointer");
    Check(weapon->weapDef.gunXModel == weapon->gunXModel, "model array pointer");
    Check(weapon->weapVariantDef.szXAnims == weapon->szXAnims, "animation array pointer");
    Check(weapon->weapDef.damage == 0 && weapon->weapDef.maxGibDistance == 0, "initial scalar fields");
    for (const auto &field : weaponDefFields)
        if (field.iFieldType == 0)
            Check(*reinterpret_cast<const char **>(reinterpret_cast<unsigned char *>(weapon) + field.iOffset) != nullptr &&
                **reinterpret_cast<const char **>(reinterpret_cast<unsigned char *>(weapon) + field.iOffset) == 0, field.szName);
    CheckCanary(weapon);
    std::memset(flame, 0, sizeof(*flame));
    InitFlameTable(flame);
    Check(flame->name && !*flame->name && flame->flameCooldownSound && !*flame->flameCooldownSound, "tail flame strings");
    Check(flame->fire == nullptr && flame->streamFlame2 == nullptr, "flame material slots intact");
    CheckCanary(flame);
    size_t writes = TestWrites(weapon, weaponDefFields, 748) + TestWrites(flame, flameTableFields, 119);

    // Independently check names against representative members around pointers,
    // nested structures and arrays, including the last member of each object.
    Check(Find(weaponDefFields, 748, "displayName")->iOffset == offsetof(WeaponFullDef, weapVariantDef.szDisplayName), "displayName mapping");
    Check(Find(weaponDefFields, 748, "gunModel16")->iOffset == offsetof(WeaponFullDef, gunXModel[15]), "gunModel16 mapping");
    Check(Find(weaponDefFields, 748, "locGun")->iOffset == offsetof(WeaponFullDef, locationDamageMultipliers[18]), "locGun mapping");
    Check(Find(weaponDefFields, 748, "maxGibDistance")->iOffset == offsetof(WeaponFullDef, weapDef.maxGibDistance), "maxGibDistance mapping");
    Check(Find(flameTableFields, 119, "flameCooldownSound")->iOffset == offsetof(flameTable, flameCooldownSound), "flame tail mapping");
    TestUpdate(weapon);
    std::printf("PASS: 867 field mappings, %zu generic field writes, guarded initialization, array edits; pointer=%p\n", writes, region);
    VirtualFree(region, 0, MEM_RELEASE);
}
