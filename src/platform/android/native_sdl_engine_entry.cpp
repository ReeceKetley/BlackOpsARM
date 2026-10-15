#include "native_engine_runtime.h"
#include <SDL_main.h>
#include "native_vulkan_entry.h"
#include "native_vulkan_device.h"
#include <cstdio>
#include <cstring>
#include "native_render_size.h"
// Directly link this TU into the SDK engine library. SDLActivity's nativeRunMain
// resolves this C export and invokes it on the actual native SDL owner thread.
extern "C" SDLMAIN_DECLSPEC int SDL_main(int argc, char **argv) {
    if(argc>=5&&argv&&argv[argc-1]) {
        if(!std::strcmp(argv[argc-1],"--kino")){bo1::android::autoStartKino=true;--argc;}
        else if(!std::strcmp(argv[argc-1],"--menu")){bo1::android::autoStartKino=false;--argc;}
    }
    if(argc==4&&argv&&argv[2]&&argv[3]&&!std::strcmp(argv[3],"--vulkan-presentation"))
        return bo1::android::RunNativeVulkanPresentation(argv[2]);
    if((argc==4||argc==5)&&argv&&argv[3]&&!std::strcmp(argv[3],"--native-vulkan")) {
        if(argc==5) {
            if(!argv[4])return 1;
            if(!std::strcmp(argv[4],"540"))bo1::android::native_render_size::requestedHeight=540;
            else if(!std::strcmp(argv[4],"720"))bo1::android::native_render_size::requestedHeight=720;
            else if(!std::strcmp(argv[4],"1080"))bo1::android::native_render_size::requestedHeight=1080;
            else if(std::strcmp(argv[4],"0")) {std::fputs("Invalid native render height: use 0, 540, 720 or 1080\n",stderr);return 1;}
            argc=4;
        }
        std::string error;
        if(!bo1::android::native_device::Configure(true,error)) {
            std::fprintf(stderr,"ENGINE ENTRY native backend selection failed: %s\n",error.c_str());return 1;
        }
        // The common runtime still validates SDK adoption and roots, then owns
        // genuine rendered Com_Init. Backend choice is separate from readiness.
        argv[3]=const_cast<char*>("--rendered");
    }
    return bo1::android::RunNativeEngine(bo1::android::NativeEngineEntry::SDLActivity,argc,argv);
}
