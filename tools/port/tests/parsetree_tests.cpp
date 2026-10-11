#include <clientscript/cscr_parsetree.h>
#include <clientscript/cscr_compiler.h>
#include <universal/mem_userhunk.h>
#include "hunk_test_checks.h"
#include <cstring>
#include <vector>

static_assert(sizeof(sval_u) == 4 && alignof(sval_u) == 4);
extern HunkUser *g_allocNodeUser[2];
static sval_u make(scriptInstance_t inst, unsigned n, sval_u *v)
{
    switch (n) {
    case 0: return node0(inst, ENUM_expression_list);
    case 1: return node1(inst, ENUM_expression_list, v[0]);
    case 2: return node2(inst, ENUM_expression_list, v[0], v[1]);
    case 3: return node3(inst, ENUM_expression_list, v[0], v[1], v[2]);
    case 4: return node4(inst, ENUM_expression_list, v[0], v[1], v[2], v[3]);
    case 5: return node5(inst, ENUM_expression_list, v[0], v[1], v[2], v[3], v[4]);
    case 6: return node6(inst, ENUM_expression_list, v[0], v[1], v[2], v[3], v[4], v[5]);
    case 7: return node7(inst, ENUM_expression_list, v[0], v[1], v[2], v[3], v[4], v[5], v[6]);
    case 8: return node8(inst, ENUM_expression_list, v[0], v[1], v[2], v[3], v[4], v[5], v[6], v[7]);
    default: std::abort();
    }
}
int main()
{
    CHECK(HunkTestReservationCount() == 0);
    Scr_ShutdownAllocNode(SCRIPTINSTANCE_SERVER); // Initial shutdown is safe.
    Scr_InitAllocNode(SCRIPTINSTANCE_SERVER);
    Scr_InitAllocNode(SCRIPTINSTANCE_CLIENT);
    CHECK(g_allocNodeUser[0] != g_allocNodeUser[1]);
    auto client = node1(SCRIPTINSTANCE_CLIENT, ENUM_integer, sval_u(714));
    auto child = node1(SCRIPTINSTANCE_SERVER, ENUM_integer, sval_u(-37));
    sval_u values[8] = {sval_u(0), sval_u(0xffffffffu), sval_u(0x80000000u), sval_u(17), child, sval_u(999), sval_u(0x12345678u), client};
    values[5].floatValue = -3.25f;
    for (unsigned n = 0; n <= 8; ++n) {
        auto result = make(SCRIPTINSTANCE_SERVER, n, values);
        sval_u *p = result.node;
        CHECK(p[0].type == ENUM_expression_list);
        CHECK(reinterpret_cast<std::uintptr_t>(p) % 4 == 0);
        for (unsigned j = 1; j <= n; ++j) {
            CHECK(reinterpret_cast<char *>(&p[j]) - reinterpret_cast<char *>(&p[j - 1]) == 4);
            CHECK(std::memcmp(&p[j], &values[j - 1], 4) == 0);
        }
#if BO1_NATIVE_64BIT
        CHECK(Bo1_NarrowDecode(Bo1_NarrowBits(result.node)) == p);
#endif
    }
    CHECK(node_pos(0xffffffffu).sourcePosValue == 0xffffffffu);
    CHECK(node1_(SCRIPTINSTANCE_SERVER, -17).intValue == -17);
    auto pair = node2_(SCRIPTINSTANCE_SERVER, values[7], child);
    CHECK(std::memcmp(&pair.node[0], &values[7], 4) == 0);
    CHECK(static_cast<sval_u *>(pair.node[1].node) == static_cast<sval_u *>(child.node));
    auto list = linked_list_end(SCRIPTINSTANCE_SERVER, child);
    auto original = static_cast<sval_u *>(list.node[0].node);
    CHECK(original == static_cast<sval_u *>(list.node[1].node));
    CHECK(original[1].node == nullptr);
    prepend_node(SCRIPTINSTANCE_SERVER, values[3], list);
    append_node(SCRIPTINSTANCE_SERVER, list, client);
    sval_u *first = list.node[0].node;
    CHECK(first[0].intValue == 17);
    CHECK(static_cast<sval_u *>(first[1].node) == original);
    sval_u *tail = list.node[1].node;
    CHECK(static_cast<sval_u *>(original[1].node) == tail && tail[1].node == nullptr);
    CHECK(static_cast<sval_u *>(tail[0].node) == static_cast<sval_u *>(client.node));
    // Retain handles across chain growth; every result is real production node8.
    std::vector<sval_u> nodes;
    for (unsigned i = 0; i < 10000; ++i) nodes.push_back(make(SCRIPTINSTANCE_SERVER, 8, values));
    CHECK(HunkTestReservationCount() > 2);
    for (auto result : nodes) {
        CHECK(result.node[0].type == ENUM_expression_list);
        CHECK(static_cast<sval_u *>(result.node[5].node) == static_cast<sval_u *>(child.node));
        CHECK(std::memcmp(&result.node[8], &client, 4) == 0);
    }
#if defined(__ANDROID__)
    CHECK(reinterpret_cast<std::uintptr_t>(static_cast<sval_u *>(child.node)) > UINT32_MAX);
#endif
    Scr_ShutdownAllocNode(SCRIPTINSTANCE_SERVER);
    CHECK(g_allocNodeUser[0] == nullptr && HunkTestReservationCount() == 1);
    // Never decode or dereference released server handles; client remains live.
    CHECK(client.node[0].type == ENUM_integer && client.node[1].intValue == 714);
    Scr_InitAllocNode(SCRIPTINSTANCE_SERVER);
    CHECK(node0(SCRIPTINSTANCE_SERVER, ENUM_NOP).node[0].type == ENUM_NOP);
    Scr_ShutdownAllocNode(SCRIPTINSTANCE_SERVER);
    Scr_ShutdownAllocNode(SCRIPTINSTANCE_CLIENT);
    Scr_ShutdownAllocNode(SCRIPTINSTANCE_CLIENT);
    CHECK(HunkTestReservationCount() == 0);
    std::puts("PASS actual parse-tree: node0..8, raw values, lists, 10000 retained nodes, instance ownership, 4-byte slots/handles");
}
