#include <ik/ik_math.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
int main() {
    struct GuardedMatrix { float before[4]; float matrix[4][4]; float after[4]; } value{};
    for(int i=0;i<4;++i) { value.before[i]=91; value.after[i]=93; }
    const float axis[3][3]={{0,1,0},{-1,0,0},{0,0,1}};
    const float origin[3]={3,4,5};
    const float expected[4][4]={{0,2,0,0},{-2,0,0,0},{0,0,2,0},{3,4,5,1}};
    ikMatrixSet44(value.matrix,origin,axis,2);
    if(std::memcmp(value.matrix,expected,sizeof(expected))) std::abort();
    for(int i=0;i<4;++i) if(value.before[i]!=91 || value.after[i]!=93) std::abort();
    std::puts("PASS original IK matrix setter: rotation, scale, translation, homogeneous row and bounds");
}
