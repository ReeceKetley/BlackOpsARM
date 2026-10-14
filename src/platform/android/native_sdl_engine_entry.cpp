#include "native_engine_runtime.h"
#include <SDL_main.h>
// Directly link this TU into the SDK engine library. SDLActivity's nativeRunMain
// resolves this C export and invokes it on the actual native SDL owner thread.
extern "C" SDLMAIN_DECLSPEC int SDL_main(int argc, char **argv) {
    return bo1::android::RunNativeEngine(bo1::android::NativeEngineEntry::SDLActivity,argc,argv);
}
