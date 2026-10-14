// Include the canonical source solely to exercise its private fixed caps table
// and checked native read helper. No independent renderer implementation.
#include "../../src/gfx_d3d/r_caps.cpp"
#include <gfx_d3d/rb_imagetouch.h>
#include <gfx_d3d/rb_stream.h>
#include <gfx_d3d/r_init.h>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/wait.h>
#include <unistd.h>
#include <csignal>
static int warnings{}, errors{};
struct CapFailure {};
static bool returnFromFatal{};
void Com_Printf(int,const char*,...) {}
void Com_PrintWarning(int,const char*,...) { ++warnings; }
void Com_Error(errorParm_t,const char*,...) { ++errors; if(!returnFromFatal) throw CapFailure{}; }
bool Assert_MyHandler(const char*,int,int,const char*,...) { std::abort(); }
char *va(const char*,...) { static char message[]="fixture unexpected caps response"; return message; }
static void Check(bool ok,const char *message) { if(!ok) { std::fprintf(stderr,"FAIL %s\n",message); std::abort(); } }
static unsigned Read(const _D3DCAPS9 &caps,int offset) { unsigned value{};std::memcpy(&value,reinterpret_cast<const unsigned char*>(&caps)+offset,4);return value; }
static void Write(_D3DCAPS9 &caps,int offset,unsigned value) { std::memcpy(reinterpret_cast<unsigned char*>(&caps)+offset,&value,4); }
static _D3DCAPS9 PolicyInput() {
    _D3DCAPS9 input{};
    for(auto &check:s_capsCheckBits) Write(input,check.offset,(Read(input,check.offset)|check.clearBits)&~check.setBits);
    for(auto &check:s_capsCheckInt) { unsigned value=Read(input,check.offset);if(value<check.min)value=check.min;if(value>check.max)value=check.max;Write(input,check.offset,value); }
    return input;
}
extern StreamBackendGlob streamBackendGlob;
#undef main
int main() {
    auto caps=PolicyInput(); R_CheckDxCaps(&caps); Check(errors==0 && warnings==0,"all canonical capability table requirements accepted");
    unsigned char unaligned[sizeof(caps)+1];std::memcpy(unaligned+1,&caps,sizeof(caps));R_CheckDxCaps(reinterpret_cast<const _D3DCAPS9*>(unaligned+1));Check(errors==0,"native fixed-word reads tolerate actual unaligned bytes without enum aliasing");
    auto missing=caps;Write(missing,s_capsCheckBits[0].offset,Read(missing,s_capsCheckBits[0].offset)&~s_capsCheckBits[0].clearBits);
    bool rejected{};try { R_CheckDxCaps(&missing); } catch(CapFailure&) { rejected=true; } Check(rejected && errors==1,"original missing dynamic-texture fatal policy preserved");
    warnings=0;missing=caps;const auto &warn=s_capsCheckBits[1];Write(missing,warn.offset,Read(missing,warn.offset)&~warn.clearBits);R_CheckDxCaps(&missing);Check(warnings==1 && errors==1,"canonical optional gamma warning policy preserved");
    rejected=false;try { R_CheckDxCaps(nullptr); } catch(CapFailure&) { rejected=true; }Check(rejected,"null actual caps input rejects");
    rejected=false;try { R_ReadCapsWord(&caps,sizeof(caps)-3); } catch(CapFailure&) { rejected=true; }Check(rejected,"caps read rejects partial word beyond struct");
    pid_t pid=fork();Check(pid>=0,"fork");if(!pid) { returnFromFatal=true; R_RespondToMissingCaps(DX_CAPS_RESPONSE_QUIT,"fixture returning fatal callback");_Exit(9); }int status{};Check(waitpid(pid,&status,0)==pid && WIFSIGNALED(status) && WTERMSIG(status)==SIGABRT,"fatal response never falls through on unexpected callback return");
    GfxImage a{},b{},c{},d{};a.semantic=0;b.semantic=0;c.semantic=2;d.semantic=4;c.cardMemory.platform[0]=100;d.cardMemory.platform[0]=200;
    GfxImage *images[]{&a,&b,&c,&d};
    for(auto &left:images) for(auto &right:images) { int ab=RB_CompareTouchImages(&left,&right),ba=RB_CompareTouchImages(&right,&left);Check((ab==0 && ba==0)||(ab<0 && ba>0)||(ab>0 && ba<0),"actual image comparator antisymmetry including zero semantics"); }
    qsort(images,4,sizeof(images[0]),[](const void *a,const void *b) { return RB_CompareTouchImages(static_cast<GfxImage *const *>(a),static_cast<GfxImage *const *>(b)); });Check(images[0]==&d && images[1]==&c && !images[2]->semantic && !images[3]->semantic,"actual image ordering memory descending then zero semantics");
    std::memset(streamBackendGlob.imageDirty,0xff,sizeof(streamBackendGlob.imageDirty));std::memset(streamBackendGlob.maxLodBiasDirty,0xff,sizeof(streamBackendGlob.maxLodBiasDirty));RB_StreamInit();for(unsigned value:streamBackendGlob.imageDirty)Check(value==0,"canonical image dirty bits clear");for(unsigned value:streamBackendGlob.maxLodBiasDirty)Check(value==0,"canonical lod dirty bits clear");
    std::puts("PASS actual canonical capability policy/read bounds/fatal-return, image comparator strict ordering and stream bit reset; no GPU capability/device/recording proof");return 0;
}
