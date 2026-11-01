#include <platform/android/zombie_save_store.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace bo1::android::save_store;
static unsigned checks;
static int fault;
static bool interrupted;
static void Check(bool ok, const char *what) {
    ++checks;
    if (!ok) { std::fprintf(stderr,"FAIL %s errno=%d\n",what,errno); std::exit(1); }
}
extern "C" ssize_t __real_write(int,const void*,size_t);
extern "C" int __real_fsync(int);
extern "C" int __real_renameat(int,const char*,int,const char*);
extern "C" ssize_t __wrap_write(int fd,const void *p,size_t size) {
    if (fault==1) { errno=ENOSPC; return -1; }
    if (fault==4 && !interrupted) { interrupted=true; errno=EINTR; return -1; }
    if (fault==4 && size>7) size=7;
    return __real_write(fd,p,size);
}
extern "C" int __wrap_fsync(int fd) {
    struct stat info{};
    if (fault==2 && !fstat(fd,&info) && S_ISREG(info.st_mode)) { errno=EIO; return -1; }
    return __real_fsync(fd);
}
extern "C" int __wrap_renameat(int a,const char *b,int c,const char *d) {
    if (fault==3) { errno=EIO; return -1; }
    return __real_renameat(a,b,c,d);
}
static std::vector<unsigned char> Bytes(const std::string &path) {
    FILE *file=std::fopen(path.c_str(),"rb"); Check(file,"read fixture file");
    Check(!std::fseek(file,0,SEEK_END),"seek fixture"); auto length=std::ftell(file);
    Check(length>=0,"fixture length"); std::rewind(file);
    std::vector<unsigned char> data(length);
    Check(std::fread(data.data(),1,data.size(),file)==data.size(),"read fixture bytes");
    Check(!std::fclose(file),"close fixture read"); return data;
}
static void Replace(const std::string &path,const std::vector<unsigned char> &data) {
    FILE *file=std::fopen(path.c_str(),"wb"); Check(file,"write fixture file");
    Check(std::fwrite(data.data(),1,data.size(),file)==data.size(),"write fixture bytes");
    Check(!std::fclose(file),"close fixture write");
}
int main(int argc,char **argv) {
    if (argc==3 && !std::strcmp(argv[1],"--read")) {
        Snapshot loaded; std::string error;
        Check(Read(argv[2],loaded,error),"new process reads snapshot");
        Check(loaded.generation==2 && loaded.metadata==std::vector<unsigned char>(31,0x22)
            && loaded.game==std::vector<unsigned char>(4097,0x23)
            && loaded.client==std::vector<unsigned char>(511,0x24),"new process exact payload");
        std::printf("fresh-process payload verified\n"); return 0;
    }
    if (argc!=2) return 2;
    std::string pattern=std::string(argv[1])+"/save-store-XXXXXX";
    std::vector<char> temp(pattern.begin(),pattern.end()); temp.push_back(0);
    const char *root=mkdtemp(temp.data()); Check(root,"isolated fixture root");
    const std::string directory=std::string(root)+"/save-states/";
    Snapshot input,loaded; std::string error;
    loaded.generation=987;
    Check(!Read(root,loaded,error) && loaded.generation==987,"missing keeps output");
    Check(!Write(root,input,error),"empty payload rejected");
    input.metadata.assign(31,0x11); input.game.assign(4097,0x12); input.client.assign(511,0x13);
    Check(Write(root,input,error),"first durable generation");
    Check(Read(root,loaded,error) && loaded.generation==1 && loaded.game==input.game,"first exact read");
    input.metadata.assign(31,0x22); input.game.assign(4097,0x23); input.client.assign(511,0x24);
    Check(Write(root,input,error),"second durable generation");
    Check(Read(root,loaded,error) && loaded.generation==2 && loaded.client==input.client,"newest exact read");
    const pid_t pid=fork(); Check(pid>=0,"spawn fresh process");
    if (!pid) { execl(argv[0],argv[0],"--read",root,nullptr); _exit(127); }
    int status=0; Check(waitpid(pid,&status,0)==pid && WIFEXITED(status) && WEXITSTATUS(status)==0,"independent process read");
    const auto first=Bytes(directory+"suspend.0"),second=Bytes(directory+"suspend.1");
    for (int mode : {1,2,3}) {
        fault=mode;
        Check(!Write(root,input,error),"injected write/flush/rename failure reported");
        fault=0;
        Check(Read(root,loaded,error) && loaded.generation==2,"previous generation survives failed commit");
    }
    fault=4; interrupted=false;
    Check(Write(root,input,error) && interrupted,"EINTR and short writes retried"); fault=0;
    Replace(directory+"suspend.0",first);
    for (std::size_t offset=0;offset<64;++offset) {
        auto damaged=second; damaged[offset]^=0x80; Replace(directory+"suspend.1",damaged);
        Check(Read(root,loaded,error) && loaded.generation==1,"header corruption falls back");
    }
    for (std::size_t offset : {std::size_t(64),std::size_t(95),second.size()-1}) {
        auto damaged=second; damaged[offset]^=1; Replace(directory+"suspend.1",damaged);
        Check(Read(root,loaded,error) && loaded.generation==1,"payload corruption falls back");
    }
    for (std::size_t size : {std::size_t(0),std::size_t(63),second.size()-1}) {
        auto damaged=second; damaged.resize(size); Replace(directory+"suspend.1",damaged);
        Check(Read(root,loaded,error) && loaded.generation==1,"truncation falls back");
    }
    auto trailing=second; trailing.push_back(0); Replace(directory+"suspend.1",trailing);
    Check(Read(root,loaded,error) && loaded.generation==1,"trailing bytes rejected");
    Replace(directory+"pending",second);
    Check(Read(root,loaded,error) && loaded.generation==1,"unpublished pending ignored");
    Replace(directory+"suspend.0",{});
    loaded.generation=987;
    Check(!Read(root,loaded,error) && loaded.generation==987,"both corrupt fail without output mutation");
    Check(!unlink((directory+"suspend.0").c_str()),"remove fixture slot");
    Check(!symlink("pending",(directory+"suspend.0").c_str()),"create fixture symlink");
    Check(!Read(root,loaded,error),"symlink generation rejected");
    Check(Write(root,input,error),"recover from invalid generations");
    input.flags=0; Check(!Write(root,input,error),"incomplete snapshot cannot claim playable flags"); input.flags=DiagnosticPayload;
    input.game.resize(MaxGame+1); Check(!Write(root,input,error),"oversize rejected");
    input.game.resize(MaxGame); input.client.resize(MaxClient); input.metadata.resize(MaxMetadata);
    Check(Write(root,input,error) && Read(root,loaded,error) && loaded.game==input.game && loaded.client==input.client,"maximum bounded payload");
    input.flags=SoloResumePayload;
    Check(Write(root,input,error) && Read(root,loaded,error) && loaded.flags==SoloResumePayload
        && loaded.client==input.client,"normal solo resume payload preserves its distinct type");
    const auto publicGeneration=loaded.generation;
    for (int mode : {1,2,3}) {
        fault=mode;Check(!Write(root,input,error),"normal save failed write is reported");fault=0;
        Check(Read(root,loaded,error) && loaded.generation==publicGeneration && loaded.flags==SoloResumePayload,
            "last normal save survives failed commit");
    }
    input.client.clear();Check(!Write(root,input,error),"normal solo save cannot omit CLIENT");
    input.flags=3;Check(!Write(root,input,error),"unknown mixed payload type rejected");
    std::printf("save-store checks %u PASS root %s\n",checks,root);
}
