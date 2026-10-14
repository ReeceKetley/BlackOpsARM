#include "d3d9.h"
#include "../../../../src/platform/android/post_filter_depth.cpp"
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <crtdbg.h>
using namespace bo1::android::post_filter_depth;
int main(int argc,char** argv){
 _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
 _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
 _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
 IDirect3DSurface9 surface;IDirect3DDevice9 device;device.bound=&surface;
 if(argc>1){
  if(!std::strcmp(argv[1],"state"))failState=1;
  if(!std::strcmp(argv[1],"get"))getResult=E_FAIL;
  if(!std::strcmp(argv[1],"get-positive"))getResult=1;
  if(!std::strcmp(argv[1],"unbind"))unbindResult=E_FAIL;
  if(!std::strcmp(argv[1],"restore"))restoreResult=E_FAIL;
  FilterScope filter;DrawScope draw(&device);return 0;
 }
 {DrawScope draw(&device);}assert(!calls);
#if BO1_NATIVE_FILTER_DEPTH_UNBIND
 {FilterScope filter;DrawScope draw(nullptr);}assert(!calls);
 for(int state=0;state<3;++state){device.states[state]=1;{FilterScope filter;DrawScope draw(&device);}assert(!unbinds);device.states[state]=0;}
 {FilterScope outer;{FilterScope inner;DrawScope draw(&device);assert(!device.bound&&surface.refs==2);}assert(device.bound==&surface&&surface.refs==1);{DrawScope draw(&device);assert(!device.bound);}}
 assert(unbinds==2&&restores==2&&releases==2&&surface.refs==1);
 {FilterScope filter;device.bound=nullptr;DrawScope draw(&device);}assert(unbinds==2&&releases==2);
 device.bound=&surface;int before=calls;{DrawScope draw(&device);}assert(calls==before);
#else
 {FilterScope outer;FilterScope inner;DrawScope draw(&device);}assert(!calls&&surface.refs==1&&device.bound==&surface);
#endif
 std::puts("PASS production helper ownership/state/OFF checks");
}
