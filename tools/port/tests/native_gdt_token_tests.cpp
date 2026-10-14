#include <tl/gdt_remote.h>
#include <qcommon/common.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>
#include <csignal>
// Isolated fatal-return provider verifies the canonical parser does not proceed
// if the error callback unexpectedly returns. No asset-update handler is replaced.
void Com_Error(errorParm_t, const char *, ...) {}
static void check(bool condition) { if (!condition) std::abort(); }
static void invalid(int which) {
    char output[8];
    if (which == 0) GDTCmd_NextToken("x", output, 0);
    else if (which == 1) GDTCmd_NextToken("x", nullptr, 8);
    else { char input[8]; std::memset(input, 'x', sizeof(input)); GDTCmd_NextToken(input, output, 8); }
}
int main() {
    struct { char before; char text[128]; char after; } output{'a', {}, 'z'};
    const char *command=" \tvehicle  jeep mass 1500";
    const char *tail=GDTCmd_NextToken(command,output.text,sizeof(output.text));
    check(!std::strcmp(output.text,"vehicle") && tail && !std::strcmp(tail,"jeep mass 1500"));
    tail=GDTCmd_NextToken(tail,output.text,sizeof(output.text));
    check(!std::strcmp(output.text,"jeep") && tail && !std::strcmp(tail,"mass 1500"));
    check(output.before=='a' && output.after=='z');
    char exact[128]; std::memset(exact,'q',127); exact[127]=0;
    check(!GDTCmd_NextToken(exact,output.text,128) && !std::strcmp(exact,output.text));
    check(!GDTCmd_NextToken(nullptr,output.text,128) && !output.text[0]);
    check(!GDTCmd_NextToken(" \t",output.text,128) && !output.text[0]);
    const char signedInput[]={static_cast<char>(0xff),' ', 'x',0};
    check(GDTCmd_NextToken(signedInput,output.text,128)==signedInput+2 && static_cast<unsigned char>(output.text[0])==255);
    for (int i=0;i<3;++i) {
        const pid_t pid=fork(); check(pid>=0);
        if(!pid) { invalid(i); _Exit(9); }
        int status; check(waitpid(pid,&status,0)==pid && WIFSIGNALED(status) && WTERMSIG(status)==SIGABRT);
    }
    std::puts("PASS canonical GDT token bounds, ARM ctype, canaries and fatal-return rejection");
}
