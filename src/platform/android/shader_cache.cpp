#include "shader_cache.h"
#include "engine_paths.h"
#include <cerrno>
#include <cstring>
#include <string>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {
struct CachePath { std::string directory, leaf; };
bool parse(const char *path, CachePath &result) {
    if (!path) { errno=EINVAL; return false; }
    const char *marker=std::strstr(path,"/pc/shadercache");
    if (!marker) { errno=EINVAL; return false; }
    marker+=4;
    const char *end=std::strchr(marker,'/');
    result.directory=end ? std::string(marker,end) : std::string(marker);
    if (result.directory!="shadercache1" && result.directory!="shadercache2") { errno=EINVAL; return false; }
    result.leaf=end ? end+1 : "";
    if (result.leaf=="." || result.leaf==".." || result.leaf.find_first_of("/\\")!=std::string::npos) {
        errno=EINVAL; return false;
    }
    return true;
}
int openDirectory(const CachePath &path, bool create, int *parent=nullptr) {
    int root=open(Sys_EngineUserRoot(),O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);
    if (root<0) return -1;
    if (create && mkdirat(root,"pc",0700) && errno!=EEXIST) { close(root); return -1; }
    int pc=openat(root,"pc",O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);
    close(root);
    if (pc<0) return -1;
    if (create && mkdirat(pc,path.directory.c_str(),0700) && errno!=EEXIST) { close(pc); return -1; }
    int directory=openat(pc,path.directory.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);
    if (parent && directory>=0) *parent=pc;
    else close(pc);
    return directory;
}
void report(const char *path) {
    std::fprintf(stderr,"Shader cache operation failed: %s (%s)\n",path,std::strerror(errno));
}
}
FILE *bo1::android::OpenShaderCacheFile(const char *path, const char *mode) {
    CachePath cache;
    if (!parse(path,cache) || cache.leaf.empty()) { errno=EINVAL; return nullptr; }
    const bool writing=mode && !std::strcmp(mode,"wb");
    if (!writing && (!mode || std::strcmp(mode,"rb"))) { errno=EINVAL; return nullptr; }
    int directory=openDirectory(cache,writing);
    if (directory<0) return nullptr;
    int file=openat(directory,cache.leaf.c_str(),O_CLOEXEC|O_NOFOLLOW|O_NONBLOCK|
        (writing ? O_WRONLY|O_CREAT : O_RDONLY),0600);
    close(directory);
    if (file<0) return nullptr;
    struct stat info{};
    if (fstat(file,&info) || !S_ISREG(info.st_mode) || info.st_nlink!=1) { close(file); errno=EINVAL; return nullptr; }
    if (writing && ftruncate(file,0)) { close(file); return nullptr; }
    FILE *stream=fdopen(file,mode);
    if (!stream) close(file);
    return stream;
}
bool bo1::android::EnsureShaderCacheDirectory(const char *path) {
    CachePath cache;
    if (!parse(path,cache) || !cache.leaf.empty()) return false;
    int directory=openDirectory(cache,true);
    if (directory<0) return false;
    close(directory); return true;
}
time_t bo1::android::ShaderCacheCutoff(time_t now,unsigned short days) {
    if (days>20) { errno=EINVAL; return static_cast<time_t>(-1); }
    tm utc{};
    if (!gmtime_r(&now,&utc)) return static_cast<time_t>(-1);
    // Preserve Material_SubtractDays' original 28-day rollover policy.
    if (utc.tm_mday<=days) {
        utc.tm_mday+=28-days;
        if (--utc.tm_mon<0) { utc.tm_mon=11; --utc.tm_year; }
    } else utc.tm_mday-=days;
    return timegm(&utc);
}
void bo1::android::CleanShaderCacheDirectory(const char *path,unsigned short days,bool removeDirectory) {
    CachePath cache;
    if (!parse(path,cache) || !cache.leaf.empty()) { report(path); return; }
    const time_t cutoff=removeDirectory ? 0 : ShaderCacheCutoff(time(nullptr),days);
    if (!removeDirectory && cutoff==static_cast<time_t>(-1)) { report(path); return; }
    int parent=-1;
    int directory=openDirectory(cache,false,&parent);
    if (directory<0) { if (errno!=ENOENT) report(path); return; }
    DIR *entries=fdopendir(directory);
    if (!entries) { close(directory); close(parent); report(path); return; }
    while (auto *entry=readdir(entries)) {
        if (!std::strcmp(entry->d_name,".") || !std::strcmp(entry->d_name,"..")) continue;
        struct stat info{};
        if (fstatat(directory,entry->d_name,&info,AT_SYMLINK_NOFOLLOW)) { report(entry->d_name); continue; }
        if (S_ISREG(info.st_mode) && (info.st_mode&S_IWUSR) && (removeDirectory || info.st_atime<cutoff))
            if (unlinkat(directory,entry->d_name,0)) report(entry->d_name);
    }
    closedir(entries);
    if (removeDirectory && unlinkat(parent,cache.directory.c_str(),AT_REMOVEDIR)) report(path);
    close(parent);
}
bool bo1::android::ShaderCacheHasBytes(FILE *stream,unsigned int bytes) {
    struct stat info{};
    const auto position=ftello(stream);
    return position>=0 && !fstat(fileno(stream),&info) && S_ISREG(info.st_mode) &&
        position<=info.st_size && static_cast<unsigned long long>(info.st_size-position)>=bytes;
}
