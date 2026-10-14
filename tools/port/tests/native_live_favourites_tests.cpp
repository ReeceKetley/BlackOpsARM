#include <live/live_win.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <array>
#include <cstdint>

extern favourite_t s_favourites[42];
static int refreshes;
// Isolated observer; production retains its actual client favourites callback.
extern "C" void __wrap__Z18CL_SetFavourites_fv() { ++refreshes; }
static void Check(bool ok) { if (!ok) std::abort(); }

int main() {
    std::memset(s_favourites, 0, sizeof(s_favourites));
    constexpr unsigned long long uid = 0xFEDCBA9800000100ULL;
    constexpr unsigned long long address = 0x9999060504030201ULL;
    const unsigned char expected[6] = {1,2,3,4,5,6};
    Live_AddFavourite(address, uid);
    Check(refreshes == 1 && s_favourites[0].uid == uid);
    Check(!std::memcmp(s_favourites[0].addressblob, expected, sizeof(expected)));
    for (unsigned int i = 1; i < 42; ++i) Live_AddFavourite(address + i, uid + i);
    Check(refreshes == 42 && s_favourites[41].uid == uid + 41);
    const auto last = s_favourites[41];
    Live_AddFavourite(address, 11);
    Check(refreshes == 43 && !std::memcmp(&last, &s_favourites[41], sizeof(last)));

    std::array<unsigned char, 255> addresses;
    std::array<unsigned char, 339> ids;
    addresses.fill(0xA5); ids.fill(0x5A);
    Live_SetFavsBlobs(addresses.data()+1, ids.data()+1);
    Check(addresses.front() == 0xA5 && addresses.back() == 0xA5);
    Check(ids.front() == 0x5A && ids.back() == 0x5A);
    Check(addresses[253] == 0 && ids[337] == 0);
    Check(!std::memcmp(addresses.data()+1, expected, 6));
    const auto saved = std::to_array(s_favourites);
    std::memset(s_favourites, 0, sizeof(s_favourites));
    Live_ParseFavsBlobs(addresses.data()+1, ids.data()+1);
    for (unsigned int i = 0; i < 42; ++i) {
        Check(s_favourites[i].uid == saved[i].uid);
        Check(!std::memcmp(s_favourites[i].addressblob, saved[i].addressblob, 6));
    }
    Live_DeleteFavourite(uid+17);
    Check(refreshes == 44 && s_favourites[17].uid == 0);
    Live_DeleteFavourite(7);
    Check(refreshes == 44);
    Live_SetFavsBlobs(addresses.data()+1, ids.data()+1);
    std::memset(s_favourites, 0, sizeof(s_favourites));
    Live_ParseFavsBlobs(addresses.data()+1, ids.data()+1);
    Check(s_favourites[17].uid == uid+18 && s_favourites[40].uid == uid+41);
    Check(s_favourites[41].uid == 0);
    Check(addresses.front() == 0xA5 && addresses.back() == 0xA5);
    Check(ids.front() == 0x5A && ids.back() == 0x5A);
    std::puts("PASS actual favourites: high/zero-low-byte IDs, 42-slot limit, compact packed unaligned blobs, canaries, add/delete callbacks");
}
