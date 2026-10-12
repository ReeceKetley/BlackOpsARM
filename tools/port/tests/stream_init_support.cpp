// Standalone stream gate only. Trace is disabled; crossing into the decoder's
// runtime byte-accounting callback is an explicit unsupported failure.
#include <cstdio>
#include <cstdlib>
unsigned long long DB_XFileBytesLoaded(){std::fputs("Unavailable engine decode byte accounting\n",stderr);std::abort();}
#if !defined(BO1_REGISTRY_ADD_GATE)
int g_dbTopAssetType{};
#endif
