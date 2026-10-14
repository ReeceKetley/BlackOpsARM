#include <qcommon/cm_showcollision.h>
#include <qcommon/cm_trace.h>
#include <common/brush_edges.h>
#include <qcommon/common.h>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <cstdint>
#include <sys/wait.h>
#include <unistd.h>
#include <csignal>

namespace {
unsigned checks,faces,maxFace;
bool nested;
cbrush_t box{};
constexpr std::uint64_t guard=0xCAB01234DEADBEEFull;
void Check(bool ok,const char *reason){if(!ok){std::fprintf(stderr,"FAIL: %s\n",reason);std::exit(1);}++checks;}
void Collect(int count,float (*points)[3],const float *color) {
    Check(count>=3 && count<=1024 && points && color,"actual polygon callback bounds");
    std::array<float,3072> snapshot{};
    std::memcpy(snapshot.data(),points,count*3*sizeof(float));
    if(!nested){nested=true;CM_ShowSingleBrushCollision(&box,color,Collect);nested=false;
        Check(!std::memcmp(snapshot.data(),points,count*3*sizeof(float)),"nested viewer retains caller winding ownership");}
    if(nested)return;
    ++faces;if(static_cast<unsigned>(count)>maxFace)maxFace=count;
    for(int point=0;point<count;++point)for(int axis=0;axis<3;++axis)
        Check(std::isfinite(points[point][axis]) && std::fabs(points[point][axis])<=1.001f,"finite bounded actual polygon vertex");
}
void FatalBounds(int mode) {
    const pid_t child=fork();Check(child>=0,"bounds fault fork");
    if(!child){
        if(mode==0){CMShowCollisionWinding winding{};winding.numpoints=1025;CM_ReverseWinding(&winding);}
        if(mode==1){ShowCollisionBrushPt pts[1024]{};float xyz[1024][3]{};CM_GetXyzList(0,pts,1025,xyz,1024);}
        if(mode==2){cbrush_t malformed=box;malformed.numsides=32762;float planes[6][4];CM_BuildAxialPlanes(&malformed,planes);}
        if(mode==3){cbrush_t malformed=box;malformed.mins[0]=2;const float color[4]={1,1,1,1};CM_ShowSingleBrushCollision(&malformed,color,Collect);}
        if(mode==4){ShowCollisionBrushPt pts[1024]{};float planes[6][4];CM_BuildAxialPlanes(&box,planes);const short indices[3]={0,2,4};const float xyz[3]={-1,-1,-1};CM_AddSimpleBrushPoint(&box,planes,indices,xyz,1024,pts);}
        if(mode==5){float plane[4];CM_GetPlaneVec4Form(nullptr,nullptr,-1,plane);}
        _exit(91);
    }
    int status{};Check(waitpid(child,&status,0)==child && (mode==4 ? WIFSIGNALED(status) && WTERMSIG(status)==SIGABRT : WIFEXITED(status) && WEXITSTATUS(status)==2),"malformed bounds fail before continuing writes");
}
}
// The fixture's recoverable-error reporter deliberately returns. Canonical
// native guards must still abort; actual assertion/fatal owners are linked.
void Com_Error(errorParm_t,const char *format,...){va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);}
int main(){
    static_assert(sizeof(winding_t)==52);static_assert(sizeof(ShowCollisionBrushPt)==20);
    for(int axis=0;axis<3;++axis){box.mins[axis]=-1;box.maxs[axis]=1;}
    const float color[4]={1,0.5f,0.25f,1};
    struct AxialScratch {std::uint64_t before=guard;float rows[6][4]{};std::uint64_t after=guard;} axial;
    struct IntersectionScratch {std::uint64_t before=guard;ShowCollisionBrushPt rows[1024]{};std::uint64_t after=guard;} pts;
    CM_BuildAxialPlanes(&box,axial.rows);
    Check(axial.before==guard && axial.after==guard,"six axial rows preserve scratch canaries");
    Check(CM_ForEachBrushPlaneIntersection(&box,axial.rows,pts.rows)==8,"actual box triple-plane intersections");
    Check(pts.before==guard && pts.after==guard,"intersection array preserves canaries");
    CM_ShowSingleBrushCollision(&box,color,Collect);
    std::printf("BOX actual faces=%u max=%u\n",faces,maxFace);Check(faces==6 && maxFace==4,"actual box produces six four-point faces");
    cbrush_t octagon=box;cplane_s planes[4]{};cbrushside_t sides[4]{};
    const float n=std::sqrt(0.5f);
    for(int side=0;side<4;++side){planes[side].normal[0]=(side&1)?n:-n;planes[side].normal[1]=(side&2)?n:-n;planes[side].dist=1.2f*n;sides[side].plane=&planes[side];}
    octagon.numsides=4;octagon.sides=sides;
    CM_BuildAxialPlanes(&octagon,axial.rows);
    Check(CM_ForEachBrushPlaneIntersection(&octagon,axial.rows,pts.rows)==16,"actual beveled triple-plane intersections");
    Check(pts.before==guard && pts.after==guard && axial.before==guard && axial.after==guard,"beveled scratch canaries");
    faces=maxFace=0;CM_ShowSingleBrushCollision(&octagon,color,Collect);
    std::printf("OCT actual faces=%u max=%u\n",faces,maxFace);Check(faces==10 && maxFace==8,"beveled prism produces genuine eight-point polygon beyond legacy p4");
    struct WindingScratch {std::uint64_t before=guard;CMShowCollisionWinding winding{};std::uint64_t after=guard;} owned;
    owned.winding.numpoints=1024;
    for(int i=0;i<1024;++i)owned.winding.p[i][0]=static_cast<float>(i);
    CM_ReverseWinding(&owned.winding);
    Check(owned.winding.p[0][0]==1023 && owned.winding.p[1023][0]==0 && owned.before==guard && owned.after==guard,"actual maximal owned tail reversal preserves canaries");
    const float negativeNormal[3]={0,-1,0};int i,j;CM_PickProjectionAxes(negativeNormal,&i,&j);
    Check(i==0 && j==2,"negative floating normal chooses original projection axes");
    for(int mode=0;mode<6;++mode)FatalBounds(mode);
    std::printf("PASS: %u native collision intersection/winding/storage checks\n",checks);
}
