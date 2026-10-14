#include <universal/com_math.h>
#include <cstdint>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
// Only diagnostics are fixture seams: any assertion is a hard failure. All
// numerical functions and normalization checks come from the original TU.
bool Assert_MyHandler(const char *file,int line,int,const char *,...){std::fprintf(stderr,"Unexpected engine assertion %s:%d\n",file,line);std::abort();}
char *va(const char *format,...){static char text[512];va_list args;va_start(args,format);std::vsnprintf(text,sizeof(text),format,args);va_end(args);return text;}
static void check(bool ok,const char *why){if(!ok){std::fprintf(stderr,"FAIL %s\n",why);std::abort();}}
static bool close(float a,float b){return std::fabs(a-b)<0.00002f;}
template<unsigned R,unsigned C> struct Guarded {
    std::uint64_t before=0xa1b2c3d4e5f67890ull;
    float value[R][C]{};
    std::uint64_t after=0x0918273645546372ull;
    void guards(){check(before==0xa1b2c3d4e5f67890ull && after==0x0918273645546372ull,"actual output canaries");}
};
template<unsigned R,unsigned C> void equal(Guarded<R,C>&m,const float (&expected)[R][C],const char*why){for(unsigned i=0;i<R;++i)for(unsigned j=0;j<C;++j)check(close(m.value[i][j],expected[i][j]),why);m.guards();}
int main(){
    const float h=std::sqrt(0.5f);
    const float quats[5][4]={{0,0,0,1},{h,0,0,h},{0,h,0,h},{0,0,h,h},{.5f,.5f,.5f,.5f}};
    const float expected[5][3][3]={{{1,0,0},{0,1,0},{0,0,1}},{{1,0,0},{0,0,1},{0,-1,0}},{{0,0,-1},{0,1,0},{1,0,0}},{{0,1,0},{-1,0,0},{0,0,1}},{{0,1,0},{0,0,1},{1,0,0}}};
    Guarded<3,3> rotation;
    for(unsigned n=0;n<5;++n){
        UnitQuatToAxis(quats[n],rotation.value);equal(rotation,expected[n],"unit quaternion known rotation");
        float forward[3];UnitQuatToForward(quats[n],forward);
        for(unsigned i=0;i<3;++i)check(close(forward[i],rotation.value[0][i]),"canonical forward matches first row");
        float neg[4];for(unsigned i=0;i<4;++i)neg[i]=-quats[n][i];UnitQuatToAxis(neg,rotation.value);equal(rotation,expected[n],"quaternion sign-equivalent rotation");
    }
    Guarded<3,3> identity,transpose,product,inverse;MatrixIdentity33(identity.value);equal(identity,expected[0],"identity33");
    UnitQuatToAxis(quats[3],rotation.value);MatrixTranspose(rotation.value,transpose.value);MatrixMultiply(rotation.value,transpose.value,product.value);equal(product,expected[0],"rotation times transpose");transpose.guards();
    const float asymmetricA[3][3]={{1,2,3},{0,1,4},{5,6,0}};
    const float asymmetricB[3][3]={{-2,1,0},{3,0,1},{4,5,1}};
    const float asymmetricAB[3][3]={{16,16,5},{19,20,5},{8,5,6}};
    const float asymmetricBA[3][3]={{-2,-3,-2},{8,12,9},{9,19,32}};
    const float asymmetricInverse[3][3]={{-24,18,5},{20,-15,-4},{-5,4,1}};
    MatrixMultiply(asymmetricA,asymmetricB,product.value);equal(product,asymmetricAB,"noncommuting33 A*B");
    MatrixMultiply(asymmetricB,asymmetricA,product.value);equal(product,asymmetricBA,"noncommuting33 B*A");
    MatrixInverse(asymmetricA,inverse.value);equal(inverse,asymmetricInverse,"asymmetric inverse33");
    const float inputVector[3]={2,-1,3};
    const float transformed[1][3]={{17,21,2}},transposeTransformed[1][3]={{9,11,4}};
    Guarded<1,3> vectorResult;
    MatrixTransformVector(inputVector,asymmetricA,vectorResult.value[0]);equal(vectorResult,transformed,"inline transform asymmetric orientation");
    MatrixTransposeTransformVector(inputVector,asymmetricA,vectorResult.value[0]);equal(vectorResult,transposeTransformed,"inline transpose transform asymmetric orientation");
    const float diagonal[3][3]={{2,0,0},{0,4,0},{0,0,8}};const float diagonalInverse[3][3]={{.5f,0,0},{0,.25f,0},{0,0,.125f}};
    MatrixInverse(diagonal,inverse.value);equal(inverse,diagonalInverse,"inverse33 diagonal");
    const float a43[4][3]={{1,0,0},{0,1,0},{0,0,1},{1,2,3}};
    const float b43[4][3]={{1,0,0},{0,1,0},{0,0,1},{-2,4,1}};
    const float c43[4][3]={{1,0,0},{0,1,0},{0,0,1},{-1,6,4}};
    const float ai43[4][3]={{1,0,0},{0,1,0},{0,0,1},{-1,-2,-3}};
    Guarded<4,3> affine,affineInverse;MatrixMultiply43(a43,b43,affine.value);equal(affine,c43,"multiply43 translations");MatrixInverseOrthogonal43(a43,affineInverse.value);equal(affineInverse,ai43,"inverse43 translation");
    const float rotationA43[4][3]={{0,1,0},{-1,0,0},{0,0,1},{1,2,3}};
    const float rotationB43[4][3]={{1,0,0},{0,0,1},{0,-1,0},{-2,4,1}};
    const float rotationAB43[4][3]={{0,0,1},{-1,0,0},{0,-1,0},{-1,1,3}};
    MatrixMultiply43(rotationA43,rotationB43,affine.value);equal(affine,rotationAB43,"noncommuting affine43");
    const float a44[4][4]={{1,0,0,0},{0,1,0,0},{0,0,1,0},{9,12,15,1}};
    const float b44[4][4]={{2,0,0,0},{0,3,0,0},{0,0,4,0},{0,0,0,1}};
    const float c44[4][4]={{2,0,0,0},{0,3,0,0},{0,0,4,0},{18,36,60,1}};
    const float ai44[4][4]={{1,0,0,0},{0,1,0,0},{0,0,1,0},{-9,-12,-15,1}};
    Guarded<4,4> homogeneous,homogeneousInverse,homogeneousTranspose;
    MatrixMultiply44(a44,b44,homogeneous.value);equal(homogeneous,c44,"multiply44 translation/scale");MatrixInverse44(a44,homogeneousInverse.value);equal(homogeneousInverse,ai44,"inverse44 translation");MatrixTranspose44(a44,homogeneousTranspose.value);for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j)check(homogeneousTranspose.value[i][j]==a44[j][i],"transpose44");homogeneousTranspose.guards();
    const float asymmetricA44[4][4]={{1,2,0,0},{0,1,3,0},{0,0,1,0},{4,5,6,1}};
    const float asymmetricB44[4][4]={{2,0,0,0},{1,3,0,0},{0,2,4,0},{7,8,9,1}};
    const float asymmetricAB44[4][4]={{4,6,0,0},{1,9,12,0},{0,2,4,0},{20,35,33,1}};
    const float asymmetricBA44[4][4]={{2,4,0,0},{1,5,9,0},{0,2,10,0},{11,27,39,1}};
    const float asymmetricInv44[4][4]={{1,-2,6,0},{0,1,-3,0},{0,0,1,0},{-4,3,-15,1}};
    MatrixMultiply44(asymmetricA44,asymmetricB44,homogeneous.value);equal(homogeneous,asymmetricAB44,"noncommuting44 A*B");
    MatrixMultiply44(asymmetricB44,asymmetricA44,homogeneous.value);equal(homogeneous,asymmetricBA44,"noncommuting44 B*A");
    MatrixInverse44(asymmetricA44,homogeneousInverse.value);equal(homogeneousInverse,asymmetricInv44,"asymmetric inverse44");
    const float planes[2][4]={{1,0,0,-2},{-1,0,0,-2}},center[3]={0,0,0},outside[3]={-3,0,0},mins[3]={-1,-1,-1},maxs[3]={1,1,1},outsideMins[3]={-4,-1,-1},outsideMaxs[3]={-3,1,1};
    check(SphereInPlanes(planes,2,center,.25f)==1 && SphereInPlanes(planes,2,outside,.25f)==0,"two plane sphere decisions");check(BoxInPlanes(planes,2,mins,maxs)==1 && BoxInPlanes(planes,2,outsideMins,outsideMaxs)==0,"two plane box decisions");
    const float bounds[2][3]={{-1,-2,-3},{4,5,6}},origin[3]={10,20,30},rotatedExpected[2][3]={{5,19,27},{12,24,36}};
    Guarded<2,3> rotated;GetRotatedBounds(bounds,origin,rotation.value,rotated.value);equal(rotated,rotatedExpected,"rotated bounds choose correct rows");
    const float knots[3][2]={{0,0},{.5f,1},{1,0}};check(close(GraphGetValueFromFraction(3,knots,.25f),.5f),"three knot two-column row access");
    std::puts("PASS canonical matrix rows:10 quaternion signs,3x3/4x3/4x4 multiply/transpose/inverse,2plane decisions,rotated2x3bounds,3x2knots and output canaries");
}
