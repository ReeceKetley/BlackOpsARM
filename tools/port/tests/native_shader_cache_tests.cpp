#include <platform/android/shader_cache.h>
#include <platform/android/engine_paths.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <string>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

static void check(bool value,const char *reason) {
    if (!value) { std::fprintf(stderr,"FAIL %s\n",reason); std::abort(); }
}
static void create(const char *legacy) {
    FILE *stream=bo1::android::OpenShaderCacheFile(legacy,"wb");
    check(stream && fwrite("shader",1,6,stream)==6,"write actual cache");
    check(!fclose(stream),"close actual cache");
}
int main() {
    using namespace bo1::android;
    char templatePath[]="/data/local/tmp/bo1-shader-cache-XXXXXX";
    char *root=mkdtemp(templatePath); check(root,"unique user root");
    InstallEngineUserRoot(root);
    check(EnsureShaderCacheDirectory("../pc/shadercache2"),"create mapped cache directory");
    create("../pc/shadercache2/new");
    FILE *stream=OpenShaderCacheFile("/legacy/game/pc/shadercache2/new","rb");
    check(stream && ShaderCacheHasBytes(stream,6) && !ShaderCacheHasBytes(stream,7),"real remaining file size");
    char payload[7]{};
    check(fread(payload,1,6,stream)==6 && !std::strcmp(payload,"shader"),"read preserved bytes");
    check(!ShaderCacheHasBytes(stream,1),"EOF remaining bound"); fclose(stream);
    check(!OpenShaderCacheFile("../pc/shadercache2/../../outside","wb"),"traversal rejected");
    check(!OpenShaderCacheFile("../pc/shadercache99/new","wb"),"unknown version rejected");
    const std::string base=std::string(root)+"/pc/shadercache2/";
    create("../pc/shadercache2/old"); create("../pc/shadercache2/readonly");
    timespec times[2]{{time(nullptr)-40*86400,0},{time(nullptr)-40*86400,0}};
    check(!utimensat(AT_FDCWD,(base+"old").c_str(),times,0),"age old file");
    check(!utimensat(AT_FDCWD,(base+"readonly").c_str(),times,0) && !chmod((base+"readonly").c_str(),0400),"read-only old file");
    check(!mkdir((base+"nested").c_str(),0700),"nested directory");
    const auto outside=std::string(root)+"/outside";
    FILE *sentinel=fopen(outside.c_str(),"wb"); check(sentinel,"sentinel open"); fwrite("keep",1,4,sentinel); fclose(sentinel);
    check(!symlink(outside.c_str(),(base+"link").c_str()),"symlink fixture");
    check(!OpenShaderCacheFile("../pc/shadercache2/link","wb"),"symlink write rejected");
    const bool hardlinkCreated=link(outside.c_str(),(base+"hardlink").c_str())==0;
    if (hardlinkCreated) check(!OpenShaderCacheFile("../pc/shadercache2/hardlink","wb"),"hardlink write rejected");
    else {
        check(errno==EPERM || errno==EACCES || errno==EOPNOTSUPP,"unexpected hardlink fixture error");
        std::puts("SKIP hardlink fixture: device policy prohibits creating it");
    }
    CleanShaderCacheDirectory("../pc/shadercache2",10,false);
    check(access((base+"old").c_str(),F_OK) && !access((base+"new").c_str(),F_OK),"only old writable file removed");
    check(!access((base+"readonly").c_str(),F_OK) && !access((base+"nested").c_str(),F_OK),"read-only and directory preserved");
    sentinel=fopen(outside.c_str(),"rb"); memset(payload,0,sizeof(payload)); fread(payload,1,4,sentinel); fclose(sentinel);
    check(!std::strcmp(payload,"keep"),"outside bytes preserved");
    create("../pc/shadercache1/old-version"); CleanShaderCacheDirectory("../pc/shadercache1",0,true);
    check(access((std::string(root)+"/pc/shadercache1").c_str(),F_OK),"obsolete cache removed");
    tm date{}; date.tm_year=126; date.tm_mon=9; date.tm_mday=3;
    auto cutoff=ShaderCacheCutoff(timegm(&date),10); tm result{}; gmtime_r(&cutoff,&result);
    check(result.tm_mon==8 && result.tm_mday==21,"reference 28-day rollover");
    for (auto name:{"new","readonly","link"}) check(!unlink((base+name).c_str()),"fixture unlink");
    if(hardlinkCreated) check(!unlink((base+"hardlink").c_str()),"hardlink cleanup");
    check(!rmdir((base+"nested").c_str()) && !rmdir(base.c_str()),"cache cleanup");
    check(!unlink(outside.c_str()) && !rmdir((std::string(root)+"/pc").c_str()) && !rmdir(root),"user fixture cleanup");
    std::puts("PASS native shader cache: IO, bounds, age policy, user-root containment and cleanup");
}
