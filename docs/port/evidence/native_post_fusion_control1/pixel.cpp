#include <algorithm>
#include <cstdint>
#include <climits>
#include <cstdlib>
#include <cassert>
#include <cstdio>
struct D3DLOCKED_RECT { int Pitch; void* pBits; };
struct PixelDiff {
    std::uint64_t histogram[256]{},alphaMismatch{},pixelsDifferent{},channels{};
    unsigned maxRgb{},maxAlpha{};
};
bool ScanRGBA(unsigned width,unsigned height,const D3DLOCKED_RECT& a,const D3DLOCKED_RECT& b,PixelDiff& result) {
    if (!a.pBits || !b.pBits || width>static_cast<unsigned>(INT_MAX)/4
        || a.Pitch<static_cast<int>(width*4) || b.Pitch<static_cast<int>(width*4)) return false;
    for (unsigned y=0;y<height;++y) {
        const auto* left=static_cast<const unsigned char*>(a.pBits)+static_cast<std::size_t>(y)*a.Pitch;
        const auto* right=static_cast<const unsigned char*>(b.pBits)+static_cast<std::size_t>(y)*b.Pitch;
        for (unsigned x=0;x<width;++x) {
            bool changed=false;
            for (unsigned c=0;c<4;++c) {
                unsigned diff=static_cast<unsigned>(std::abs(static_cast<int>(left[x*4+c])-right[x*4+c]));
                changed|=diff!=0; ++result.histogram[diff]; ++result.channels;
                if (c==3) { result.alphaMismatch+=diff!=0; result.maxAlpha=std::max(result.maxAlpha,diff); }
                else result.maxRgb=std::max(result.maxRgb,diff);
            }
            result.pixelsDifferent+=changed;
        }
    }
    return true;
}

int main(){
 unsigned char a[24]={},b[24]={};D3DLOCKED_RECT left{12,a},right{12,b};
 {PixelDiff r;assert(ScanRGBA(2,2,left,right,r));assert(r.channels==16&&r.histogram[0]==16&&!r.pixelsDifferent);}
 b[0]=1;b[12+4+2]=1;b[8]=255;b[20]=255;
 {PixelDiff r;assert(ScanRGBA(2,2,left,right,r));assert(r.channels==16&&r.histogram[1]==2&&r.histogram[0]==14&&r.pixelsDifferent==2&&r.maxRgb==1&&!r.alphaMismatch);}
 b[0]=2;
 {PixelDiff r;assert(ScanRGBA(2,2,left,right,r));assert(r.maxRgb==2&&!r.alphaMismatch);}
 b[0]=0;b[12+4+3]=1;
 {PixelDiff r;assert(ScanRGBA(2,2,left,right,r));assert(r.maxRgb==1&&r.alphaMismatch==1&&r.maxAlpha==1);}
 {PixelDiff r;auto invalid=right;invalid.Pitch=7;assert(!ScanRGBA(2,2,left,invalid,r));invalid.Pitch=-1;assert(!ScanRGBA(2,2,left,invalid,r));invalid.Pitch=12;invalid.pBits=nullptr;assert(!ScanRGBA(2,2,left,invalid,r));}
 std::puts("production ScanRGBA pitch/fullRGBA/histogram/acceptance-boundary PASS");
}
