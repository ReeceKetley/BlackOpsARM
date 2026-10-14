#include <platform/android/command_line_services.h>
#include <platform/android/engine_paths.h>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
// Exercise the actual global-construction timing used by r_stream.cpp.
std::string constructorCommandLine(GetCommandLineA());
int main(int argc, char **argv) {
    if (argc != 5) return 2;
    std::printf("OS %s\n", constructorCommandLine.c_str());
    for (int i=0; i<argc; ++i) std::printf("ARG %d %s\n", i, argv[i]);
    if (std::strcmp(constructorCommandLine.c_str(), GetCommandLineA())) return 3;
    char *storage=GetCommandLineA();
    bo1::android::InstallEngineCommandLine(argv[0], "+set bo1_headless 1 +set dedicated 1");
    if (storage!=GetCommandLineA()
        || !std::strstr(storage,"+set bo1_headless 1 +set dedicated 1")) return 4;
    std::puts("PASS actual OS argv available before main and stable policy installation");
    const char *root="/data/local/tmp/blackopsarm-game-data";
    bo1::android::InstallEngineDataRoot(root);
    if(std::strcmp(root,Sys_DefaultInstallPath()) || !*Sys_Cwd())return 5;
    std::puts("PASS actual staged data root resolved independently of executable directory");
}
