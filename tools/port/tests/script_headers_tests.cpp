#include <clientscript/cscr_variable.h>
#include <clientscript/cscr_debugger.h>
#include <clientscript/cscr_instance.h>
#include <clientscript/cscr_stringlist.h>
#include <database/db_expand.hpp>
#include <cstddef>
#include <initializer_list>
#include <cstdio>
#include <cstdlib>

static_assert(sizeof(VariableUnion) == 4 && alignof(VariableUnion) == 4);
static_assert(sizeof(sval_u) == 4 && alignof(sval_u) == 4);
static_assert(sizeof(VariableValue) == 8);
static_assert(sizeof(VariableValueInternal) == 0x1C);
static_assert(offsetof(VariableStackBuffer, buf) == 13);
static_assert(sizeof(scr_localVar_t) == 8 && sizeof(scr_block_s) == 0x218);
static_assert(sizeof(PrecacheEntry) == 8 && sizeof(RefString) == 8);

// These are the actual utility's engine-owned pointer slots, supplied by the
// harness; no allocator, string-table or VM service is replaced by a stub.
scrVarDebugPub_t *gScrVarDebugPub[2];
scrStringDebugGlob_t *gScrStringDebugGlob[2];

int main()
{
    char code[] = "code";
    VariableUnion value{};
    value.codePosValue = code;
    VariableUnion copy{};
    Bo1_NarrowSetBits(copy.codePosValue, Bo1_NarrowBits(value.codePosValue));
    if (static_cast<const char *>(copy.codePosValue) != code) std::abort();
    sval_u tree[2];
    tree[0].node = &tree[1];
    if (static_cast<sval_u *>(tree[0].node) != &tree[1]) std::abort();
#if defined(__ANDROID__)
    if (SVAL_PTR_AT(sval_u, BO1_NARROW_ENCODE(tree)) != &tree[1]) std::abort();
#endif
#if BO1_NATIVE_64BIT
    if (DB_ExpandPtr32(0xFFFFFFFFu) != ~uintptr_t(0)
        || DB_ExpandPtr32(0xFFFFFFFEu) != ~uintptr_t(1)) std::abort();
#endif
    for (scriptInstance_t inst : {SCRIPTINSTANCE_SERVER, SCRIPTINSTANCE_CLIENT}) {
        Scr_ResetScrVarDebugPub(inst);
        Scr_ResetScrStringDebugGlob(inst);
        if (!gScrVarDebugPub[inst] || !gScrStringDebugGlob[inst]) std::abort();
        auto *var = gScrVarDebugPub[inst];
        auto *str = gScrStringDebugGlob[inst];
        Scr_ClearScrVarDebugPub(inst);
        Scr_ClearScrStringDebugGlob(inst);
        if (gScrVarDebugPub[inst] || gScrStringDebugGlob[inst]) std::abort();
        Scr_ResetScrVarDebugPub(inst);
        Scr_ResetScrStringDebugGlob(inst);
        if (gScrVarDebugPub[inst] != var || gScrStringDebugGlob[inst] != str) std::abort();
    }
    if (gScrVarDebugPub[0] == gScrVarDebugPub[1]
        || gScrStringDebugGlob[0] == gScrStringDebugGlob[1]) std::abort();
    std::printf("PASS actual script headers and cscr_instance.cpp: VM/parse slots=4, fixed records retained; both instance pointer reset/clear paths\n");
}
