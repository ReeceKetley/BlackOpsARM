#include "platform/android/native_render_size.h"
#include <cassert>
#include <cstdio>
using namespace bo1::android::native_render_size;
int main(){
    requestedHeight=1080;auto size=Select(1920,1200);assert(size.width==1920&&size.height==1080);
    auto fit=Fit(size.width,size.height,1920,1200);assert(fit.x==0&&fit.y==60&&fit.width==1920&&fit.height==1080);
    requestedHeight=720;size=Select(1920,1200);assert(size.width==1280&&size.height==720);
    int x=960,y=600;assert(MapPointer(x,y,1920,1200)&&x==640&&y==360);
    x=0;y=60;assert(MapPointer(x,y,1920,1200)&&x==0&&y==0);
    x=1919;y=1139;assert(MapPointer(x,y,1920,1200)&&x==1279&&y==719);
    x=960;y=0;assert(!MapPointer(x,y,1920,1200)&&x==640&&y==0);
    x=960;y=1199;assert(!MapPointer(x,y,1920,1200)&&x==640&&y==719);
    // Pre-rotated presentation uses the transposed source extent.
    fit=Fit(720,1280,1200,1920);assert(fit.x==60&&fit.y==0&&fit.width==1080&&fit.height==1920);
    fit=Fit(1920,1080,1280,720);assert(fit.x==0&&fit.y==0&&fit.width==1280&&fit.height==720);
    fit=FitViewport(1920,1080,1920,1200,1920,1200,true);assert(fit.x==96&&fit.y==0&&fit.width==1728&&fit.height==1200);
    fit=FitViewport(1280,720,1920,1200,1200,1920,true);assert(fit.x==60&&fit.y==0&&fit.width==1080&&fit.height==1920);
    fit=FitViewport(1920,1080,1920,1200,1920,1200,false);assert(fit.x==0&&fit.y==60&&fit.width==1920&&fit.height==1080);
    requestedHeight=0;size=Select(1920,1200);assert(size.width==1920&&size.height==1200);
    x=1919;y=1199;assert(MapPointer(x,y,1920,1200)&&x==1919&&y==1199);
    assert(Fit(0,1080,1920,1200).width==0);
    puts("native_render_size=pass 1080_720_aspect_fit_letterbox_pointer_rotation_default=pass cpu_fixture_only=1");
}
