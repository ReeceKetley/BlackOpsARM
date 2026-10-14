#include <CubeMapGenLib/CCubeMapProcessor.h>
#include <array>
#include <vector>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <thread>
static void Check(bool ok) { if(!ok) std::abort(); }
static void Prepare(CCubeMapProcessor &cube, int size, float color) {
    cube.Init(size,size,3,4);
    std::vector<float> face(size*size*4);
    for(int i=0;i<size*size;++i) {
        face[i*4]=color; face[i*4+1]=color/2; face[i*4+2]=color/4; face[i*4+3]=1;
    }
    for(int i=0;i<6;++i) cube.SetInputFaceData(i,CP_VAL_FLOAT32,4,size*4*sizeof(float),face.data(),10,1,1);
    cube.m_NumFilterThreads=2;
}
static void Start(CCubeMapProcessor &cube) {
    cube.InitiateFiltering(20,20,2,CP_FILTER_TYPE_DISC,CP_FIXUP_NONE,0,TRUE);
}
static void Wait(CCubeMapProcessor &cube) {
    const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(10);
    while(cube.GetStatus()==CP_STATUS_PROCESSING) {
        Check(cube.GetFilterProgressString()!=nullptr);
        Check(std::chrono::steady_clock::now()<deadline);
        std::this_thread::yield();
    }
    Check(cube.GetStatus()==CP_STATUS_FILTER_COMPLETED);
}
static void Verify(CCubeMapProcessor &cube, float color) {
    std::array<float,4*4*4> output;
    for(int face=0;face<6;++face) {
        cube.GetOutputFaceData(face,0,CP_VAL_FLOAT32,4,4*4*sizeof(float),output.data(),1,1);
        for(int pixel=0;pixel<16;++pixel) {
            Check(std::fabs(output[pixel*4]-color)<0.0001);
            Check(std::fabs(output[pixel*4+1]-color/2)<0.0001);
            Check(std::fabs(output[pixel*4+2]-color/4)<0.0001);
            Check(std::fabs(output[pixel*4+3]-1)<0.0001);
        }
    }
}
int main() {
    CCubeMapProcessor first,second;
    Prepare(first,4,0.8f); Prepare(second,4,0.4f);
    Start(first); Start(second); Wait(first); Wait(second);
    Verify(first,0.8f); Verify(second,0.4f);
    Prepare(first,128,0.7f); Start(first);
    const bool activeBeforeStop=first.IsFilterThreadActive(0);
    first.TerminateActiveThreads();
    std::printf("stop observed active=%d resulting_status=%d\n",activeBeforeStop,first.GetStatus());
    Check(!first.IsFilterThreadActive(0) && !first.IsFilterThreadActive(1));
    Check(first.GetStatus()==CP_STATUS_FILTER_TERMINATED || first.GetStatus()==CP_STATUS_FILTER_COMPLETED);
    Prepare(first,4,0.6f); Start(first); Wait(first); Verify(first,0.6f);
    first.Clear(); second.Clear();
    std::puts("PASS actual CubeMapGen two-worker/two-instance filtered colors, atomic progress, cancellation/join and restart");
}
