#include <gfx_d3d/r_init.h>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>
#include <csignal>
void Com_Printf(int,const char *,...) {}
void Sys_DirectXFatalError() {} // Deliberately returns to verify the native fatal fallback.
static void Check(bool ok,const char *message) { if(!ok) { std::fprintf(stderr,"FAIL %s\n",message); std::abort(); } }
using FatalTask=void(*)();
static void Fatal(FatalTask task) { pid_t pid=fork(); Check(pid>=0,"fork"); if(!pid) { task(); _Exit(9); } int status{}; Check(waitpid(pid,&status,0)==pid,"wait"); Check(WIFSIGNALED(status)&&WTERMSIG(status)==SIGABRT,"fatal callback returning never permits execution/publication"); }
void MissingMode(){ R_ClosestRefreshRateForMode(1,1,60); }
void BadCount(){ int rows[256][2]; R_AddValidResolution(800,600,-1,rows); }
void FullCount(){ int rows[256][2]{}; R_AddValidResolution(800,600,256,rows); }
void FatalReturning(){ R_FatalInitError("fixture returning fatal callback"); }
#undef main
int main() {
 struct { int rows[256][2]; unsigned canary; } table{}; table.canary=0x51a3e7c9;
 int count=0; for(int i=0;i<256;++i) count=R_AddValidResolution(800+i,600,count,table.rows);
 Check(count==256 && table.canary==0x51a3e7c9 && table.rows[255][0]==1055,"real multidimensional table stores all rows in bounds");
 Check(R_AddValidResolution(1055,600,count,table.rows)==256,"full table still accepts duplicate");
 Check(R_AddValidResolution(799,600,0,table.rows)==0,"canonical minimum dimensions preserved");
 int refresh[257]{}; int rates=0; for(int i=0;i<256;++i) rates=R_AddValidRefreshRate(255-i,rates,refresh);
 Check(rates==256 && refresh[0]==0 && refresh[255]==255,"actual refresh insertion sorted and bounded");
 Check(R_AddValidRefreshRate(100,rates,refresh)==256,"full refresh table deduplicates");
 dx.displayModeCount=3;
 dx.displayModes[0]={800,600,60,D3DFMT_X8R8G8B8}; dx.displayModes[1]={800,600,120,D3DFMT_X8R8G8B8}; dx.displayModes[2]={1920,1080,60,D3DFMT_X8R8G8B8};
 Check(R_ClosestRefreshRateForMode(800,600,0)==60,"lower search boundary actual mode");
 Check(R_ClosestRefreshRateForMode(800,600,90)==60,"canonical lower refresh selection");
 Check(R_ClosestRefreshRateForMode(800,600,120)==120,"exact refresh");
 Check(R_ClosestRefreshRateForMode(1920,1080,240)==60,"upper search boundary stays in bounds");
 Fatal(MissingMode); Fatal(BadCount); Fatal(FullCount); Fatal(FatalReturning);
 std::puts("PASS actual canonical renderer mode helpers: 256-row bounds, dedup/sort, refresh endpoints, missing-mode rejection and fatal-return retirement; no GPU proof"); return 0;
}
