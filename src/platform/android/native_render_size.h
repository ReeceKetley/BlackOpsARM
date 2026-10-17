#pragma once
#include <algorithm>
#include <cstdint>
namespace bo1::android::native_render_size {
// Set once by the native SDL entry before engine threads/resources exist.
// Zero retains the actual drawable extent; legacy entry never selects a size.
inline unsigned requestedHeight{};
struct Extent { unsigned width,height; };
struct Rect { unsigned x,y,width,height; };
inline Extent Select(unsigned width,unsigned height) {
    if(requestedHeight==480)return {854,480};
    if(requestedHeight==540)return {960,540};
    if(requestedHeight==720)return {1280,720};
    if(requestedHeight==1080)return {1920,1080};
    return {width,height};
}
inline Rect Fit(unsigned sourceWidth,unsigned sourceHeight,unsigned width,unsigned height) {
    if(!sourceWidth||!sourceHeight||!width||!height)return {};
    unsigned w=width,h=height;
    if(std::uint64_t(width)*sourceHeight>std::uint64_t(height)*sourceWidth)
        w=unsigned(std::uint64_t(height)*sourceWidth/sourceHeight);
    else h=unsigned(std::uint64_t(width)*sourceHeight/sourceWidth);
    w=std::max(1u,w);h=std::max(1u,h);
    return {(width-w)/2,(height-h)/2,w,h};
}
inline bool MapPointer(int& x,int& y,unsigned width,unsigned height) {
    if(!requestedHeight)return true;
    const auto source=Select(width,height);const auto rect=Fit(source.width,source.height,width,height);
    if(!rect.width||!rect.height)return false;
    const bool inside=x>=int(rect.x)&&y>=int(rect.y)&&x<int(rect.x+rect.width)&&y<int(rect.y+rect.height);
    x=int(std::clamp<std::int64_t>((std::int64_t(x)-rect.x)*source.width/rect.width,0,source.width-1));
    y=int(std::clamp<std::int64_t>((std::int64_t(y)-rect.y)*source.height/rect.height,0,source.height-1));
    return inside;
}
// Fit in the displayed coordinate system first. A rotated Vulkan surface
// rotates normalized coordinates; it need not report a transposed extent.
inline Rect FitViewport(unsigned sourceWidth,unsigned sourceHeight,unsigned displayWidth,unsigned displayHeight,
        unsigned frameWidth,unsigned frameHeight,bool quarterTurn) {
    const auto fit=Fit(sourceWidth,sourceHeight,displayWidth,displayHeight);
    if(!fit.width||!fit.height||!frameWidth||!frameHeight)return {};
    unsigned w=unsigned(std::uint64_t(quarterTurn?fit.height:fit.width)*frameWidth/(quarterTurn?displayHeight:displayWidth));
    unsigned h=unsigned(std::uint64_t(quarterTurn?fit.width:fit.height)*frameHeight/(quarterTurn?displayWidth:displayHeight));
    w=std::max(1u,w);h=std::max(1u,h);
    return {(frameWidth-w)/2,(frameHeight-h)/2,w,h};
}
}
