#include "sort16.h"
#include <limits>

namespace bo1::compat {
bool Sort16(std::int16_t* a, std::size_t count, Sort16Compare compare,
            const void* context) noexcept {
    constexpr std::size_t domain = std::size_t(INT16_MAX) + 1;
    if (count > domain || !compare || !context) return false;
    if (!count) return true;
    const auto address = reinterpret_cast<std::uintptr_t>(a);
    if (!a || address % alignof(std::int16_t) ||
        count * sizeof(*a) > std::numeric_limits<std::uintptr_t>::max() - address)
        return false;
    if (count < 2) return true;

    // Element positions can represent lo-1/hi+1 without creating invalid pointers.
    using Position = std::ptrdiff_t;
    struct Range { Position lo, hi; };
    Range stack[30];
    std::size_t pending = 0;
    Position lo = 0, hi = static_cast<Position>(count) - 1;
    auto cmp = [&](Position x, Position y) { return compare(a[x], a[y], context); };
    auto swap = [&](Position x, Position y) { const auto v = a[x]; a[x] = a[y]; a[y] = v; };

    for (;;) {
        const Position size = hi - lo + 1;
        if (size <= 8) { // original 0x967121; shortsort 0x967000
            for (Position end = hi; end > lo; --end) {
                Position maximum = lo;
                for (Position scan = lo + 1; scan <= end; ++scan)
                    if (cmp(scan, maximum) > 0) maximum = scan;
                if (maximum != end) swap(maximum, end);
            }
        } else {
            Position mid = lo + size / 2;
            // Original 0x96713b..187: strict median swaps, in this order.
            if (cmp(lo, mid) > 0) swap(lo, mid);
            if (cmp(lo, hi) > 0) swap(lo, hi);
            if (cmp(mid, hi) > 0) swap(mid, hi);
            Position left = lo, right = hi;
            for (;;) { // original 0x967190..209
                if (mid > left) {
                    do { ++left; } while (left < mid && cmp(left, mid) <= 0);
                }
                if (mid <= left) {
                    do { ++left; } while (left <= hi && cmp(left, mid) <= 0);
                }
                do { --right; } while (right > mid && cmp(right, mid) > 0);
                if (right < left) break;
                if (right != left) swap(left, right);
                if (mid == right) mid = left;
            }
            // Original 0x96720d..246: skip pivot-equal suffix of left partition.
            ++right;
            if (mid < right) {
                do { --right; } while (right > mid && cmp(right, mid) == 0);
            }
            if (mid >= right) {
                do { --right; } while (right > lo && cmp(right, mid) == 0);
            }
            // Larger queued, smaller processed, equal lengths queue left.
            // Count<=32768 implies at most15 queued ranges; original capacity30.
            if (right - lo >= hi - left) {
                if (lo < right) stack[pending++] = {lo, right};
                if (left < hi) { lo = left; continue; }
            } else {
                if (left < hi) stack[pending++] = {left, hi};
                if (lo < right) { hi = right; continue; }
            }
        }
        if (!pending) return true;
        const auto range = stack[--pending]; lo = range.lo; hi = range.hi;
    }
}
}
