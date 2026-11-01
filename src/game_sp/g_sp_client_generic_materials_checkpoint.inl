// Native map_material ordinals are borrowed by the saved CLIENT script graph.
// Reconnect those asset owners by name; transient filter/overlay visuals may
// reset, but callbacks must never observe an empty or differently indexed map.
#include <gfx_d3d/r_material.h>

struct SPClientGenericMaterials
{
    std::string names[64];
    Material *resolved[64]{}; // fresh DB-owned assets, never written to disk
};
static SPClientGenericMaterials s_clientGenericMaterials;
static bool s_clientGenericMaterialsPresent;

static void SP_ClientGenericMaterialsCodec(SPNativeCheckpointCodec &c, SPClientGenericMaterials &state)
{
    if (c.Int(0x314D4743) != 0x314D4743 || c.Int(64) != 64) // CGM1
        Com_Error(ERR_DROP, "Client generic material checkpoint rejected schema/count");
    for (auto &name : state.names)
    {
        bool present = !name.empty(); c.Field(present);
        if (present) name = c.Name(name.c_str());
        else name.clear();
    }
}
static void SP_WriteClientGenericMaterials(MemoryFile *file)
{
    SPClientGenericMaterials state;
    const auto *cg = CG_GetLocalClientGlobals(0);
    for (int i = 0; i < 64; ++i) if (const auto *material = cg->genericMaterialMap[i])
    {
        if (!material->info.name || !material->info.name[0])
            Com_Error(ERR_DROP, "Client generic material checkpoint lost asset identity at %i", i);
        state.names[i] = material->info.name;
    }
    SPNativeCheckpointCodec c{nullptr, file}; SP_ClientGenericMaterialsCodec(c, state);
}
static void SP_DecodeClientGenericMaterials(SPNativeCheckpointCodec &c)
{
    s_clientGenericMaterials = {}; s_clientGenericMaterialsPresent = false;
    SP_ClientGenericMaterialsCodec(c, s_clientGenericMaterials);
    // Resolve every entry before publishing any table slot. Empty slots and
    // duplicate material names retain their exact original numeric identity.
    for (int i = 0; i < 64; ++i) if (!s_clientGenericMaterials.names[i].empty())
    {
        auto *material = Material_Register(s_clientGenericMaterials.names[i].data(), 0);
        if (!material || !material->info.name
            || I_stricmp(material->info.name, s_clientGenericMaterials.names[i].c_str()))
            Com_Error(ERR_DROP, "Client generic material checkpoint asset differs at %i", i);
        s_clientGenericMaterials.resolved[i] = material;
    }
    s_clientGenericMaterialsPresent = true;
}
static void SP_PublishClientGenericMaterials()
{
    if (!s_clientGenericMaterialsPresent) return; // CLP1..14 retain their legacy behavior.
    auto *cg = CG_GetLocalClientGlobals(0); int count = 0;
    for (int i = 0; i < 64; ++i)
    {
        cg->genericMaterialMap[i] = s_clientGenericMaterials.resolved[i];
        if (cg->genericMaterialMap[i]) ++count;
    }
    for (int i = 0; i < 64; ++i)
        if (cg->genericMaterialMap[i] != s_clientGenericMaterials.resolved[i])
            Com_Error(ERR_DROP, "Client generic material checkpoint publication differs at %i", i);
    Com_Printf(15, "Client generic materials: %i DB asset owners reconnected at original 64 script ordinals before callbacks\n", count);
}
