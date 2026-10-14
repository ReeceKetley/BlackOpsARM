#include "../../../src/clientscript/cscr_compiler.cpp"
#include <clientscript/cscr_tempmemory.h>
#include <xanim/xanim.h>
#include <universal/com_memory.h>
#include <universal/mem_userhunk.h>
#include <universal/mem_firstfit.h>
#include <platform/android/thread_services.h>
#include <platform/android/virtual_memory.h>
#include <qcommon/common.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <cstdint>
#include <climits>
#include <atomic>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <csignal>
extern unsigned char *s_hunkData;extern int s_hunkTotal;extern hunkUsed_t hunk_low,hunk_high;extern fileData_s *com_hunkData;extern bool g_anim_developer;
static unsigned checks;static int *fatalMarker;
static std::atomic<unsigned long long> entered{},exited{};
extern "C" void __cyg_profile_func_enter(void*,void*){entered.fetch_add(1,std::memory_order_relaxed);}
extern "C" void __cyg_profile_func_exit(void*,void*){exited.fetch_add(1,std::memory_order_relaxed);}
void Scr_TerminalError(const char*,scriptInstance_t){std::fputs("UNEXECUTED script terminal error\n",stderr);std::abort();}
static void Check(bool ok,const char *why){if(!ok){std::fprintf(stderr,"FAIL: %s\n",why);std::abort();}++checks;}
void Com_Error(errorParm_t,const char *format,...){if(fatalMarker)*fatalMarker=1;va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);std::fputc('\n',stderr);} // Deliberately returning fatal reporter.
bool Assert_MyHandler(const char *file,int line,int,const char *format,...){std::fprintf(stderr,"assert %s:%d ",file,line);va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);std::abort();}
void Com_Printf(int,const char *format,...){va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);}
void Com_PrintWarning(int,const char *format,...){va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);}
void Com_PrintError(int,const char *format,...){va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);}
void Com_DPrintf(int,const char *format,...){va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);}
void Bo1_Diag(const char *format,...){va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);}
void Bo1_DiagStack(const char*){std::fputs("UNEXECUTED hunk OOM stack path\n",stderr);std::abort();}

static int LinkedOperandCase(){
 Sys_InitMainThread();alignas(HunkUserDefault) unsigned char arena[512];std::memset(arena,0xa5,sizeof(arena));
 HunkUser *user=Hunk_UserCreateFromBuffer(arena,sizeof(arena),HU_SCHEME_DEFAULT,0,nullptr,"linked-call-fixture",0);TempMemoryReset(user);char *start=TempMallocAlignStrict(0);
 EmitByte(SCRIPTINSTANCE_SERVER,0x61);EmitUInt32(SCRIPTINSTANCE_SERVER,1);EmitByte(SCRIPTINSTANCE_SERVER,0xe7);char *end=TempMallocAlignStrict(0);
 const uint32_t operandHandle=BO1_NARROW_ENCODE(start+1);Check(reinterpret_cast<uintptr_t>(start+1)%4!=0,"actual call operand address odd");
 Check(Scr_ReadLinkedCodeOperand(operandHandle)==1,"actual LinkThread sentinel load from decoded packed address");
 const uint32_t codeHandle=BO1_NARROW_ENCODE(start+5);Scr_WriteLinkedCodeOperand(operandHandle,codeHandle);Check(Scr_ReadLinkedCodeOperand(operandHandle)==codeHandle&&BO1_NARROW_DECODE(char,codeHandle)==start+5,"actual linked code handle exact bits and native decode");
 Scr_WriteLinkedCodeOperand(operandHandle,0x87654321u);const unsigned char expected[]={0x61,0x21,0x43,0x65,0x87,0xe7};Check(!std::memcmp(start,expected,6),"actual high-bit four-byte patch exact bytes and neighboring opcode canaries");Check(TempMallocAlignStrict(0)==end&&end-start==6&&static_cast<unsigned char>(*end)==0xa5,"linked patch cursor and trailing canary unchanged");
 Hunk_UserDestroy(user);TempMemoryReset(nullptr);std::printf("PASS: %u actual linked operand codec/producer/canary observations\n",checks);return 0;
}
int main(int argc,char **argv){
 if(argc==2&&!std::strcmp(argv[1],"linked"))return LinkedOperandCase();
 alarm(20);Sys_InitMainThread();alignas(HunkUserDefault) unsigned char arena[4096];std::memset(arena,0xa5,sizeof(arena));
 HunkUser *user=Hunk_UserCreateFromBuffer(arena,sizeof(arena),HU_SCHEME_DEFAULT,0,nullptr,"packed-compiler-fixture",0);TempMemoryReset(user);auto inst=SCRIPTINSTANCE_SERVER;
 char *start=TempMallocAlignStrict(0);EmitByte(inst,0x81);EmitShort(inst,static_cast<short>(0x9327));EmitUInt32(inst,0x76543210);EmitFloat(inst,-2.5f);EmitCodepos(inst,start+1);EmitCodepos(inst,nullptr);
 const unsigned char expected[]={0x81,0x27,0x93,0x10,0x32,0x54,0x76,0,0,0x20,0xc0};
 Check(!std::memcmp(start,expected,sizeof(expected)),"actual emit exact little endian bytes at odd operands");Check(TempMallocAlignStrict(0)-start==19,"actual cursor19 bytes without padding");Check(BO1_NARROW_DECODE(char,bo1::bytecode::Load<uint32_t>(start+11))==start+1,"actual four byte Narrow32 absolute code handle");Check(!bo1::bytecode::Load<uint32_t>(start+15),"actual null handle remains zero");Check(static_cast<unsigned char>(start[19])==0xa5,"emit trailing canary");
 char *patch=TempMallocAlignStrict(4);EmitByte(inst,0xcc);char *end=TempMallocAlignStrict(0);BreakStatementInfo br{patch,patch+4,nullptr};ContinueStatementInfo co{patch,patch+4,nullptr};gScrCompileGlob[inst].currentBreakStatement=&br;ConnectBreakStatements(inst);Check(bo1::bytecode::Load<uint32_t>(patch)==1 && patch[4]==static_cast<char>(0xcc),"actual odd break backpatch exact4 plus canary");gScrCompileGlob[inst].currentContinueStatement=&co;ConnectContinueStatements(inst);Check(bo1::bytecode::Load<uint32_t>(patch)==1 && TempMallocAlignStrict(0)==end,"actual continue backpatch preserves cursor");
 unsigned char cases[26];std::memset(cases,0xa5,sizeof(cases));unsigned keys[]={7,UINT_MAX,1};for(unsigned i=0;i<3;++i){bo1::bytecode::Store<uint32_t>(cases+1+8*i,keys[i]);bo1::bytecode::Store<uint32_t>(cases+5+8*i,keys[i]^0x12345678);}qsort(cases+1,3,8,ComparePackedCaseInfo);unsigned sorted[]={UINT_MAX,7,1};for(unsigned i=0;i<3;++i){Check(bo1::bytecode::Load<uint32_t>(cases+1+8*i)==sorted[i],"actual packed comparator descending unsigned key");Check(bo1::bytecode::Load<uint32_t>(cases+5+8*i)==(sorted[i]^0x12345678),"sort preserves associated four byte code record");}Check(cases[0]==0xa5&&cases[25]==0xa5,"packed sort canaries");
 struct OwnedBlock{uint64_t left; scr_block_s block;uint64_t right;};OwnedBlock dst{0x12345678,{},0x87654321};scr_block_s from{};from.localVarsCount=3;from.localVarsCreateCount=3;from.localVarsPublicCount=3;dst.block.localVarsCount=3;dst.block.localVarsPublicCount=3;for(unsigned i=0;i<3;++i){from.localVars[i]={10+i,100+i};dst.block.localVars[i]={12-i,102-i};}from.localVarsInitBits[0]=7;Scr_TransferBlock(&from,&dst.block);for(unsigned i=0;i<3;++i)Check(dst.block.localVars[i].name==10+i&&dst.block.localVars[i].sourcePos==100+i,"actual transfer typed previous row shift");Check(dst.block.localVarsInitBits[0]==7&&dst.left==0x12345678&&dst.right==0x87654321,"real transfer initbits and owner canaries");
 dst.block={};dst.block.localVarsCount=3;for(unsigned i=0;i<3;++i)dst.block.localVars[i]={12-i,102-i};ScrBlockPtr child{};child=&dst.block;Scr_MergeChildBlocks(&child,1,&from);for(unsigned i=0;i<3;++i)Check(dst.block.localVars[i].name==10+i&&dst.block.localVars[i].sourcePos==100+i,"actual merge typed previous row shift");Check(dst.block.localVarsPublicCount==3&&dst.left==0x12345678&&dst.right==0x87654321,"merge real public count and owner canaries");
 Hunk_UserDestroy(user);TempMemoryReset(nullptr);std::printf("PASS: %u actual packed compiler emission/backpatch/sort/row observations\n",checks);
}

