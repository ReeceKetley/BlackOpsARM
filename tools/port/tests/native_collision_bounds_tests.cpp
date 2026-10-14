#include <qcommon/cm_test.h>
#include <qcommon/cm_trace.h>
#include <universal/com_math.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <cstddef>
// Genuine data owners and collision/math functions; assertions hard-fail.
clipMap_t cm{};
bool Assert_MyHandler(const char *file,int line,int,const char *,...){std::fprintf(stderr,"Unexpected engine assertion %s:%d\n",file,line);std::abort();}
char *va(const char *format,...){static char text[512];va_list args;va_start(args,format);std::vsnprintf(text,sizeof(text),format,args);va_end(args);return text;}
static unsigned checks;
static void check(bool ok,const char *why){++checks;if(!ok){std::fprintf(stderr,"FAIL %s\n",why);std::abort();}}
struct alignas(8) Bounds {std::uint32_t before=0x13572468;float value[3]{};std::uint32_t after=0x24681357;};
struct List {std::uint16_t before=0x1357;std::uint16_t value[4]={0xabcd,0xabcd,0xabcd,0xabcd};std::uint16_t after=0x2468;};
static_assert(offsetof(Bounds,value)==4);
static_assert(sizeof(cLeaf_s)==44 && offsetof(cLeaf_s,cluster)==40);
static cplane_s planes[3];static cNode_t nodes[3];static cLeaf_s leaves[4];
static void setLeafWord(unsigned leaf,std::uint32_t word){std::memcpy(reinterpret_cast<unsigned char *>(&leaves[leaf])+offsetof(cLeaf_s,cluster),&word,sizeof(word));}
static void bounds(Bounds &a,float x,float y,float z){a.value[0]=x;a.value[1]=y;a.value[2]=z;check(reinterpret_cast<std::uintptr_t>(a.value)%8==4,"actual float3 source is 4 modulo 8");}
static void query(Bounds &lo,Bounds &hi,int capacity,const std::uint16_t *expected,int count,int expectedLast){
    List out;int last=-77;Bounds oldLo=lo,oldHi=hi;
    check(CM_BoxLeafnums(lo.value,hi.value,out.value,capacity,&last)==count,"actual query count");
    check(last==expectedLast,"actual query lastLeaf");
    for(int i=0;i<count;++i)check(out.value[i]==expected[i],"actual front-before-back leaf order");
    for(int i=count;i<4;++i)check(out.value[i]==0xabcd,"list beyond count unchanged");
    check(out.before==0x1357 && out.after==0x2468,"list canaries");
    check(std::memcmp(&lo,&oldLo,sizeof(lo))==0 && std::memcmp(&hi,&oldHi,sizeof(hi))==0,"source bounds and canaries unchanged");
}
int main(){
    cm.nodes=nodes;cm.numNodes=3;cm.planes=planes;cm.planeCount=3;cm.leafs=leaves;cm.numLeafs=4;
    planes[0]={{1,0,0},0,0,0,{0,0}};planes[1]={{0,1,0},0,1,0,{0,0}};planes[2]={{0,0,1},0,2,0,{0,0}};
    nodes[0]={&planes[0],{1,2}};nodes[1]={&planes[1],{-1,-2}};nodes[2]={&planes[2],{-3,-4}};
    for(unsigned i=0;i<4;++i)setLeafWord(i,i);setLeafWord(3,UINT32_MAX);
    Bounds lo,hi;const std::uint16_t all[]={0,1,2,3},front[]={0,1},one0[]={0},one1[]={1},one2[]={2},one3[]={3};
    bounds(lo,-2,-3,-4);bounds(hi,2,3,4);query(lo,hi,4,all,4,2);query(lo,hi,2,all,2,2);query(lo,hi,0,all,0,2);
    bounds(lo,1,-3,-4);bounds(hi,2,3,4);query(lo,hi,4,front,2,1);
    bounds(lo,1,1,-4);bounds(hi,2,3,4);query(lo,hi,4,one0,1,0);check(CM_PointLeafnum(lo.value)==0,"actual axial point traversal");
    bounds(lo,1,-3,-4);bounds(hi,2,-1,4);query(lo,hi,4,one1,1,1);
    bounds(lo,-2,-3,1);bounds(hi,-1,3,4);query(lo,hi,4,one2,1,2);
    bounds(lo,-2,-3,-4);bounds(hi,-1,3,-1);query(lo,hi,4,one3,1,0);
    // Cluster -1 with zero tail is not all-FF; preserve the four-byte decision.
    setLeafWord(3,0x0000ffff);query(lo,hi,4,one3,1,3);setLeafWord(3,UINT32_MAX);
    struct GuardedLeafList {std::uint64_t before=0x123456789abcdef0ULL;leafList_s value{};std::uint64_t after=0xfedcba9876543210ULL;} guarded;
    List output;auto &ll=guarded.value;ll.list=output.value;ll.maxcount=2;
    bounds(lo,-2,-3,-4);bounds(hi,2,3,4);std::memcpy(ll.bounds[0],lo.value,12);std::memcpy(ll.bounds[1],hi.value,12);
    CM_BoxLeafnums_r(&ll,0);check(ll.count==2 && ll.overflowed==1 && ll.lastLeaf==2,"real traversal overflow still updates lastLeaf");
    check(output.value[0]==0 && output.value[1]==1 && output.value[2]==0xabcd && output.value[3]==0xabcd,"overflow writes no extra leaves");
    check(guarded.before==0x123456789abcdef0ULL && guarded.after==0xfedcba9876543210ULL && output.before==0x1357 && output.after==0x2468,"traversal owner canaries");
    // General planes: all eight original signbit branches with real normals.
    cm.numNodes=1;nodes[0]={&planes[0],{-1,-2}};
    const std::uint16_t both[]={0,1};
    for(unsigned bits=0;bits<8;++bits){
        planes[0]={{(bits&1)?-1.f:1.f,(bits&2)?-2.f:2.f,(bits&4)?-3.f:3.f},0,3,static_cast<std::uint8_t>(bits),{0,0}};
        bounds(lo,-.5f,-.5f,-.5f);bounds(hi,.5f,.5f,.5f);query(lo,hi,4,both,2,1);
        float center[3];for(unsigned i=0;i<3;++i)center[i]=planes[0].normal[i];
        bounds(lo,center[0]-.1f,center[1]-.1f,center[2]-.1f);bounds(hi,center[0]+.1f,center[1]+.1f,center[2]+.1f);query(lo,hi,4,one0,1,0);check(CM_PointLeafnum(center)==0,"actual nonaxial point front");
        for(unsigned i=0;i<3;++i)center[i]=-center[i];bounds(lo,center[0]-.1f,center[1]-.1f,center[2]-.1f);bounds(hi,center[0]+.1f,center[1]+.1f,center[2]+.1f);query(lo,hi,4,one1,1,1);check(CM_PointLeafnum(center)==1,"actual nonaxial point back");
    }
    // Original strict comparison at a zero-width on-plane bound returns side0:
    // real traversal consequently visits both children.
    planes[0]={{1,0,0},0,0,0,{0,0}};bounds(lo,0,-1,-1);bounds(hi,0,1,1);query(lo,hi,4,both,2,1);
    std::printf("PASS genuine collision bounds/traversal %u checks; sources4mod8; actual math/node/leaf owners\n",checks);
}
