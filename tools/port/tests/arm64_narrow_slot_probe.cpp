#include <universal/narrow32.h>

// This originally failed 8 == 4 on Android. The native-width selector must now
// pass with no BO1_PORT_X64 definition and without widening any VM slot.
static_assert(sizeof(BO1_NARROW(char)) == 4,
              "A narrow slot must remain four bytes on every supported ABI.");
static_assert(alignof(BO1_NARROW(char)) == 4);
