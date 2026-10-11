#include <clientscript/cscr_readwrite.h>
#include <clientscript/cscr_variable.h>
#include <cstdio>
#include <cstdlib>
#include <vector>

static_assert(sizeof(VariableUnion) == 4 && sizeof(VariableValueInternal) == 28);
static_assert(sizeof(Variable) == 8 && sizeof(scrVarGlob_t) == 128);
// Engine-owned globals with real fixture data; no missing VM service is stubbed.
scrVarGlob_t gScrVarGlob[2];
unsigned int g_scrVarChildBegin[2];
unsigned int g_scrVarChildSize[2];
bool Assert_MyHandler(const char *file, int line, int, const char *, ...)
{
    std::fprintf(stderr, "Engine lookup assertion: %s:%d\n", file, line);
    std::abort();
}
static void check(bool condition) { if (!condition) std::abort(); }
static unsigned run(scriptInstance_t inst, unsigned begin, unsigned size)
{
    g_scrVarChildBegin[inst] = begin;
    g_scrVarChildSize[inst] = size;
    std::vector<VariableValueInternal> table(begin + size);
    gScrVarGlob[inst].variableList = table.data();
    auto *children = table.data() + begin;
    constexpr unsigned parent = 1;
    constexpr unsigned firstName = 17;
    const unsigned bucket = (parent + firstName) % (size - 1) + 1;
    const unsigned other = bucket + 1;
    auto &object = table[parent + 1];
    object.w.status = 0x60 | VAR_OBJECT;
    children[bucket].hash.id = bucket;
    children[bucket].w.status = (firstName << 8) | 0x40 | VAR_INTEGER;
    children[bucket].v.next = bucket;
    check(FindVariableIndexInternal(inst, parent, firstName) == bucket);
    check(FindVariableIndexInternal2(inst, firstName, bucket) == bucket);
    const unsigned collidingName = firstName + size - 1;
    check(FindVariableIndexInternal(inst, parent, collidingName) == 0);
    children[other].hash.id = other;
    children[other].w.status = (collidingName << 8) | 0x20 | VAR_INTEGER;
    children[bucket].v.next = other;
    children[other].v.next = bucket;
    check(FindVariableIndexInternal(inst, parent, collidingName) == other);
    check(FindVariableIndexInternal2(inst, collidingName, bucket) == other);
    check(FindVariableIndexInternal(inst, parent, firstName + 2 * (size - 1)) == 0);
    check(FindVariableIndexInternal2(inst, 0x00ffffffu, bucket) == 0);
    // Empty bucket path.
    check(FindVariableIndexInternal2(inst, 123, bucket + 2) == 0);
    // Empty/nonempty last-sibling paths use the actual parent and hash records.
    object.v.next = 7;
    table[8].hash.u.prev = 0;
    check(FindLastSibling(inst, parent) == 0);
    table[8].hash.u.prev = other;
    check(FindLastSibling(inst, parent) == other);
    // Hash-address entry and value record need not have the same index.
    children[bucket].hash.id = bucket + 3;
    children[bucket + 3].w.status = (firstName << 8) | 0x40 | VAR_INTEGER;
    children[bucket + 3].v.next = bucket;
    check(FindVariableIndexInternal(inst, parent, firstName) == bucket);
    gScrVarGlob[inst].variableList = nullptr;
    return 11;
}
int main()
{
    unsigned checks = run(SCRIPTINSTANCE_SERVER, 0x8000, 0x3fffe)
        + run(SCRIPTINSTANCE_CLIENT, 0x8000, 0x3fffe)
        + run(SCRIPTINSTANCE_SERVER, 0x10000, 0x7fffe);
    std::printf("PASS unchanged cscr_readwrite.cpp: %u lookup checks; direct/collision/miss/empty/sibling/remapped-ID paths, both instances and expanded server pool, four-byte slots\n", checks);
}
