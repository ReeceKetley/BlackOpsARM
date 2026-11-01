// GSL1: logical server pane state. Asset and renderer pointers remain local.
#include <Glass/glass_server.h>
#include <Glass/glass_load_obj.h>
#include <cmath>
struct SPGlassState
{
    unsigned state = 0;
    int changeTime = 0;
    float health = 0, crackedRate = 0, hitPos[3]{}, hitDir[3]{};
};
static_assert(sizeof(SPGlassState) == 40);
struct SPGlassRecord
{
    unsigned brush = 0;
    std::string definition;
    float origin[3]{};
    SPGlassState value;
};
static std::vector<SPGlassRecord> s_savedGlass;
static bool s_savedGlassPresent;
static int s_savedGlassNextShatter, s_savedGlassShatterInterval;

static bool SP_ValidGlassState(const SPGlassState &v, unsigned index)
{
    if ((v.state & 15) > 2 || ((v.state >> 4) & 4095) != index || v.changeTime < 0
        || !std::isfinite(v.health) || !std::isfinite(v.crackedRate) || v.crackedRate < 0) return false;
    for (int i = 0; i < 3; ++i)
        if (!std::isfinite(v.hitPos[i]) || !std::isfinite(v.hitDir[i])) return false;
    return true;
}
static SPGlassState SP_GetGlassState(const GlassServer &g)
{
    SPGlassState v; v.state = g.state.val.i; v.changeTime = g.stateChangeTime;
    v.health = g.health; v.crackedRate = g.crackedDamageRate;
    memcpy(v.hitPos, g.hitPos, sizeof(v.hitPos)); memcpy(v.hitDir, g.hitDir, sizeof(v.hitDir));
    return v;
}
static void SP_CaptureGlass()
{
    s_savedGlass.clear(); s_savedGlassPresent = true;
    s_savedGlassNextShatter = svGlasses.nextShatterTime; s_savedGlassShatterInterval = svGlasses.shatterIntervalsMS;
    if (svGlasses.numGlasses >= 1000) Com_Error(ERR_DROP, "Glass checkpoint pane count invalid");
    for (unsigned i = 0; i < svGlasses.numGlasses; ++i)
    {
        const auto &g = svGlasses.glasses[i]; SPGlassRecord r;
        if (!g.glass || g.glass->index != i || !g.glass->glassDef || !g.glass->glassDef->name)
            Com_Error(ERR_DROP, "Glass checkpoint lost its immutable pane owner");
        r.brush = g.glass->brushModel; r.definition = g.glass->glassDef->name;
        memcpy(r.origin, g.glass->origin, sizeof(r.origin)); r.value = SP_GetGlassState(g);
        if (!SP_ValidGlassState(r.value, i)) Com_Error(ERR_DROP, "Glass checkpoint invalid live state");
        s_savedGlass.push_back(std::move(r));
    }
}
static void SP_WriteGlass(MemoryFile *file)
{
    MemFile_WriteInt(file, 0x314C5347); // GSL1
    MemFile_WriteInt(file, (int)s_savedGlass.size());
    MemFile_WriteInt(file, s_savedGlassNextShatter); MemFile_WriteInt(file, s_savedGlassShatterInterval);
    for (const auto &r : s_savedGlass)
    {
        MemFile_WriteInt(file, r.brush); MemFile_WriteCString(file, const_cast<char *>(r.definition.c_str()));
        MemFile_WriteData(file, sizeof(r.origin), (unsigned char *)r.origin);
        MemFile_WriteData(file, sizeof(r.value), (unsigned char *)&r.value);
    }
}
static void SP_ReadGlass(MemoryFile *file)
{
    const auto read = [file]() { int v; MemFile_ReadData(file, 4, (unsigned char *)&v); return v; };
    if (read() != 0x314C5347) Com_Error(ERR_DROP, "Glass checkpoint schema invalid");
    const int count = read(); const int next = read(), interval = read();
    const auto *asset = GetGlasses();
    if (count < 0 || count >= 1000 || (unsigned)count != (asset ? asset->numGlasses : 0)
        || (count && (!asset || !asset->glasses)) || (next > 0 && interval <= 0))
        Com_Error(ERR_DROP, "Glass checkpoint map/count/timer mismatch");
    std::vector<SPGlassRecord> decoded;
    for (int i = 0; i < count; ++i)
    {
        SPGlassRecord r; r.brush = read(); r.definition = MemFile_ReadCString(file);
        MemFile_ReadData(file, sizeof(r.origin), (unsigned char *)r.origin);
        MemFile_ReadData(file, sizeof(r.value), (unsigned char *)&r.value);
        const auto &g = asset->glasses[i];
        if (g.index != (unsigned)i || !g.glassDef || !g.glassDef->name || r.definition.empty()
            || r.definition != g.glassDef->name || r.brush != g.brushModel
            || memcmp(r.origin, g.origin, sizeof(r.origin)) || !SP_ValidGlassState(r.value, i))
            Com_Error(ERR_DROP, "Glass checkpoint rejected pane identity/state %i", i);
        decoded.push_back(std::move(r));
    }
    s_savedGlass = std::move(decoded); s_savedGlassNextShatter = next; s_savedGlassShatterInterval = interval;
    s_savedGlassPresent = true;
}
static void SP_PublishGlass()
{
    if (!s_savedGlassPresent) return; // Legacy formats contain no pane records.
    if (svGlasses.numGlasses != s_savedGlass.size()) Com_Error(ERR_DROP, "Glass checkpoint lost fresh server panes");
    // Validate the entire owner set before mutation. Preserve the live asset and
    // dvar owners; SetState would emit new script events and randomize damage.
    for (unsigned i = 0; i < s_savedGlass.size(); ++i)
    {
        const auto *g = svGlasses.glasses[i].glass; const auto &r = s_savedGlass[i];
        if (!g || g->index != i || !g->glassDef || r.definition != g->glassDef->name
            || r.brush != g->brushModel || memcmp(r.origin, g->origin, sizeof(r.origin)))
            Com_Error(ERR_DROP, "Glass checkpoint lost publication identity");
    }
    svGlasses.nextShatterTime = s_savedGlassNextShatter; svGlasses.shatterIntervalsMS = s_savedGlassShatterInterval;
    unsigned changed = 0;
    for (unsigned i = 0; i < s_savedGlass.size(); ++i)
    {
        auto &g = svGlasses.glasses[i]; const auto &v = s_savedGlass[i].value;
        g.state.val.i = v.state; g.stateChangeTime = v.changeTime;
        g.health = v.health; g.crackedDamageRate = v.crackedRate;
        memcpy(g.hitPos, v.hitPos, sizeof(g.hitPos)); memcpy(g.hitDir, v.hitDir, sizeof(g.hitDir));
        const auto actual = SP_GetGlassState(g);
        if (memcmp(&actual, &v, sizeof(v))) Com_Error(ERR_DROP, "Glass checkpoint publication differs");
        changed += (v.state & 15) != 0;
    }
    Com_Printf(15, "Glass checkpoint: %u server panes (%u changed) LIVE logical state IDENTICAL; normal gamestate owns client reconstruction\n",
        (unsigned)s_savedGlass.size(), changed);
}
