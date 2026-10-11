// check_brush_edges.py appends actual production geometry and comparison code.
#include <Windows.h>
#include <intrin.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "common/brush_edges.h"

static void Check(bool value, const char *name)
{
    if (!value) { std::fprintf(stderr, "FAIL: %s\n", name); std::exit(1); }
}
bool Assert_MyHandler(const char *, int, int, const char *, ...)
{
    Check(false, "engine assertion");
    return true;
}
int VecNCompareCustomEpsilon(const float *, const float *, float, int);

int main()
{
    void *address = nullptr;
#ifdef BO1_PORT_X64
    address = reinterpret_cast<void *>(0x180000000ull);
#endif
    auto *region = static_cast<unsigned char *>(VirtualAlloc(address, 0x40000,
        MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    Check(region != nullptr, "allocate test geometry");
    DWORD previousProtect;
    Check(VirtualProtect(region + 0x3f000, 0x1000, PAGE_NOACCESS, &previousProtect), "guard output page");
    auto *points = reinterpret_cast<SimplePlaneIntersection *>(region + 0x1000);
    auto **cycle = reinterpret_cast<const SimplePlaneIntersection **>(region + 0x3f000 - 1024 * sizeof(void *));
    points[0] = {{0, 0, 0}, {0, 1, 4}};
    points[1] = {{1, 0, 0}, {0, 1, 2}};
    points[2] = {{1, 1, 0}, {0, 2, 3}};
    points[3] = {{0, 1, 0}, {0, 3, 4}};
    SimplePlaneIntersection *square[] = {&points[0], &points[1], &points[2], &points[3]};
    int count = -1;
    Check(FindCycleBFS(0, square, 4, square[0], square[1], 1, cycle, &count) && count == 4, "square cycle");
    Check(cycle[0] == square[1] && cycle[1] == square[0] && cycle[2] == square[3] && cycle[3] == square[2], "cycle order/full pointers");
    Check(!FindCycleBFS(0, square, 2, square[0], square[1], 1, cycle, &count) && count == 0, "disconnected cycle");
    const SimplePlaneIntersection *removeList[] = {square[0], square[2], square[1], square[3]};
    Check(RemoveNextPointFormedByThisPlane(2, removeList, removeList + 4) == square[2], "remove middle point");
    Check(removeList[0] == square[0] && removeList[1] == square[1] && removeList[2] == square[3], "shift pointer elements");
    Check(RemoveNextPointFormedByThisPlane(99, removeList, removeList + 3) == nullptr, "missing plane");
    points[4] = points[0];
    points[4].xyz[0] = 0.005f;
    const SimplePlaneIntersection *duplicates[] = {square[0], square[1], square[2], square[3], square[0], &points[4]};
    Check(NumberOfUniquePoints(duplicates, 6) == 4, "epsilon unique count");
    points[4].xyz[0] = 0.02f;
    Check(NumberOfUniquePoints(duplicates, 6) == 5, "distinct nearby point");
    points[4] = {{2, 0, 0}, {0, 1, 5}};
    points[5] = {{3, 0, 0}, {0, 1, 6}};
    points[6] = {{3, 1, 0}, {0, 6, 7}};
    points[7] = {{2, 1, 0}, {0, 7, 5}};
    SimplePlaneIntersection *twoSquares[8];
    for (int i = 0; i < 8; ++i) twoSquares[i] = &points[i];
    const SimplePlaneIntersection *edges[] = {&points[0], &points[4], &points[1], &points[5]};
    int partition[4] = {};
    Check(PartitionEdges(0, 1, twoSquares, 8, edges, 4, partition) == 2, "partition disconnected loops");
    Check(partition[0] == 2 && partition[1] == 4 && edges[0] == &points[0] && edges[1] == &points[1] &&
        edges[2] == &points[4] && edges[3] == &points[5], "partition reordering");
    for (int i = 0; i < 4; ++i) {
        points[i].planeIndex[0] = 0;
        points[i].planeIndex[1] = 1;
        points[i].planeIndex[2] = 2;
    }
    const SimplePlaneIntersection *list[] = {&points[0], &points[1], &points[2], &points[3]};
    Check(Remove(list, 4, &points[1]) == 3 && list[0] == &points[0] && list[1] == &points[2] && list[2] == &points[3], "remove preserves remaining pointer order");
    points[4].planeIndex[0] = 0; points[4].planeIndex[1] = 100; points[4].planeIndex[2] = 101;
    const SimplePlaneIntersection *rare[] = {&points[0], &points[4], &points[1], &points[2], &points[3]};
    Check(RemovePtsWithPlanesThatOccurLessThanTwice(rare, 5) == 4 && rare[1] == &points[1] && rare[3] == &points[3], "remove rare-plane point");

    // A maximum-length cycle exercises queue indexing and output bounds.
    points[0] = {{0, 0, 0}, {0, 1, 10}};
    points[1] = {{0, 0, 0}, {0, 1, 1032}};
    SimplePlaneIntersection *chain[1024];
    chain[0] = &points[0]; chain[1] = &points[1];
    for (int i = 0; i < 1022; ++i) {
        points[i + 2] = {{static_cast<float>(i), 0, 0}, {0, i + 10, i + 11}};
        chain[i + 2] = &points[i + 2];
    }
    Check(FindCycleBFS(0, chain, 1024, &points[0], &points[1], 1, cycle, &count) && count == 1024, "maximum-length cycle");
    Check(cycle[0] == &points[1] && cycle[1] == &points[0] && cycle[1023] == &points[1023], "maximum cycle order");
    std::printf("PASS: brush BFS, uniqueness, removals, partitioning and 1024-point guarded cycle; pointer=%p\n", region);
    VirtualFree(region, 0, MEM_RELEASE);
}
