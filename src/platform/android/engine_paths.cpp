#include "engine_paths.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits.h>
#include <mutex>
#include <cerrno>
#include <sys/stat.h>
#include <unistd.h>
namespace {
char dataRoot[PATH_MAX + 1];
bool installed;
char userRoot[PATH_MAX + 1];
bool userInstalled;
std::mutex mutex;
[[noreturn]] void fail(const char *reason) {
    std::fprintf(stderr,"Native engine paths: %s\n",reason);std::abort();
}
}
void bo1::android::InstallEngineDataRoot(const char *directory) {
    std::lock_guard<std::mutex> lock(mutex);
    if(installed || !directory || !*directory)fail("invalid or duplicate data root");
    char *resolved=realpath(directory,nullptr);
    if(!resolved)fail("cannot resolve data root");
    struct stat info{};
    if(stat(resolved,&info) || !S_ISDIR(info.st_mode) || access(resolved,R_OK|X_OK)) {
        std::free(resolved);fail("data root is not a readable directory");
    }
    const auto size=std::strlen(resolved);
    if(size>=sizeof(dataRoot)){std::free(resolved);fail("data root too long");}
    std::memcpy(dataRoot,resolved,size+1);std::free(resolved);installed=true;
}
char *Sys_DefaultInstallPath() {
    std::lock_guard<std::mutex> lock(mutex);
    if(!installed)fail("data root must be installed before use");
    return dataRoot;
}
char *Sys_Cwd() {
    thread_local char directory[PATH_MAX + 1];
    if(!getcwd(directory,sizeof(directory)))fail("cannot read working directory");
    return directory;
}
void bo1::android::InstallEngineUserRoot(const char *directory) {
    std::lock_guard<std::mutex> lock(mutex);
    if(userInstalled || !directory || !*directory)fail("invalid or duplicate user root");
    if(mkdir(directory,0700) && errno!=EEXIST)fail("cannot create user root");
    char *resolved=realpath(directory,nullptr);
    if(!resolved)fail("cannot resolve user root");
    struct stat info{};
    if(stat(resolved,&info) || !S_ISDIR(info.st_mode) || access(resolved,R_OK|W_OK|X_OK)){
        std::free(resolved);fail("user root is not writable directory");}
    auto size=std::strlen(resolved);
    if(size>=sizeof(userRoot)){std::free(resolved);fail("user root too long");}
    std::memcpy(userRoot,resolved,size+1);std::free(resolved);userInstalled=true;
}
char *Sys_EngineUserRoot() {
    std::lock_guard<std::mutex> lock(mutex);
    if(!userInstalled)fail("user root must be installed before use");
    return userRoot;
}
