// Actual canonical trace implementation plus real native stdio/path/CRT owners.
#include <game_sp/g_sp_playtrace.cpp>
#undef sprintf_s
#include <thread>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <sys/wait.h>
#include <csignal>

namespace {
dvar_s fixtureHome{};
int checks;
void Check(bool value,const char *why) {
    ++checks;
    if (!value) { std::fprintf(stderr,"FAIL: %s\n",why); std::exit(1); }
}
void ExpectAbort(bool zeroSize) {
    const pid_t child=fork();
    Check(child>=0,"fork overflow proof");
    if (!child) {
        char output[4];
        if(zeroSize)TraceFormat(static_cast<char *>(output),std::size_t{0},"x");
        else TraceFormat(output,"%s","output larger than owned buffer");
        _exit(91);
    }
    int status{};
    Check(waitpid(child,&status,0)==child && WIFSIGNALED(status) && WTERMSIG(status)==SIGABRT,
          "secure-format overflow is fatal, not a negative offset");
}
}
// Isolated settings and logger providers; no simulated game/frame state.
const dvar_s *Dvar_FindVar(const char *name) {
    if (std::strcmp(name,"fs_homepath")) std::abort();
    return &fixtureHome;
}
void Com_Printf(int,const char *format,...) {
    va_list args; va_start(args,format); std::vfprintf(stderr,format,args); va_end(args);
}
int main() {
    char root[]="/data/local/tmp/bo1-sp-trace-XXXXXX";
    Check(mkdtemp(root)!=nullptr,"real temporary home");
    fixtureHome.current.string=root;
    // Reentrant ownership is required by original trace callbacks.
    TraceEnter(); G_SP_PlayTraceLine("{\"proof\":1}"); TraceLeave();
    Check(s_file && !s_openFailed,"canonical line opens actual output");
    const std::string firstPath=s_path;
    std::vector<std::thread> writers;
    for(int i=0;i<4;++i)writers.emplace_back([]{
        for(int j=0;j<16;++j)G_SP_PlayTraceLine("{\"parallel\":1}");
    });
    for(auto &writer:writers)writer.join();
    TraceEnter();bo1::android::CloseStdio(s_file);s_file=nullptr;TraceLeave();
    FILE *read=bo1::android::OpenStdio(firstPath.c_str(),"rb");
    Check(read!=nullptr,"read original JSONL file");
    char line[128]; int count=0; bool intact=true;
    while(std::fgets(line,sizeof(line),read)) {
        intact &= !std::strcmp(line,count ? "{\"parallel\":1}\n" : "{\"proof\":1}\n"); ++count;
    }
    bo1::android::CloseStdio(read);
    Check(count==65 && intact,"serialized complete LF JSONL records");
    // A genuinely read-only stream causes the canonical write path to fail.
    s_file=bo1::android::OpenStdio(firstPath.c_str(),"rb");
    Check(s_file!=nullptr,"read-only failure stream");
    G_SP_PlayTraceLine("cannot write");
    Check(!s_file && s_openFailed,"failed output is closed and retired");
    G_SP_PlayTraceLine("no retry during failed enable");
    Check(!s_file && s_openFailed,"one-attempt ownership preserved");
    char output[8];
    Check(TraceFormat(output,"%d",123)==3 && !std::strcmp(output,"123"),"actual engine formatter");
    ExpectAbort(false); ExpectAbort(true);
    unlink(firstPath.c_str());
    const std::string directory=std::string(root)+"/playtrace";
    rmdir(directory.c_str());rmdir(root);
    std::printf("PASS: %d native SP trace IO/format/ownership checks\n",checks);
}