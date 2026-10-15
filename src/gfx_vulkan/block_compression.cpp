#include "block_compression.h"
#include <limits>
#include <cstring>
namespace bo1::vulkan {
namespace {
std::uint64_t Read(const unsigned char* p,unsigned bytes) {
    std::uint64_t value=0;for(unsigned i=0;i<bytes;++i)value|=std::uint64_t(p[i])<<(8*i);return value;
}
void Color(unsigned value,unsigned char* p) {
    const unsigned r=(value>>11)&31,g=(value>>5)&63,b=value&31;
    p[0]=(r<<3)|(r>>2);p[1]=(g<<2)|(g>>4);p[2]=(b<<3)|(b>>2);p[3]=255;
}
}
bool DecodeBlocks(BlockCompression format,unsigned width,unsigned height,unsigned depth,
    const void* source,std::size_t sourceBytes,std::vector<unsigned char>& rgba,std::string& error) {
    error.clear();rgba.clear();
    if(!source||!width||!height||!depth||(format!=BlockCompression::BC1&&format!=BlockCompression::BC2&&format!=BlockCompression::BC3)) {
        error="Invalid block texture input";return false;
    }
    const std::uint64_t rows=(std::uint64_t(height)+3)/4,columns=(std::uint64_t(width)+3)/4;
    const unsigned blockBytes=format==BlockCompression::BC1?8:16;
    const auto limit=std::numeric_limits<std::size_t>::max();
    // Divide before multiplying: dimensions may originate in malformed files.
    if(columns>limit/rows||columns*rows>limit/depth||columns*rows*depth>limit/blockBytes
        ||std::uint64_t(width)>limit/height||std::uint64_t(width)*height>limit/depth
        ||std::uint64_t(width)*height*depth>limit/4) {error="Block texture size overflow";return false;}
    const auto required=std::size_t(columns*rows*depth*blockBytes);
    if(sourceBytes!=required) {error="Block texture byte count mismatch";return false;}
    std::vector<unsigned char> result(std::size_t(width)*height*depth*4);
    const auto* block=static_cast<const unsigned char*>(source);
    for(unsigned z=0;z<depth;++z)for(std::uint64_t by=0;by<rows;++by)for(std::uint64_t bx=0;bx<columns;++bx,block+=blockBytes) {
        const auto* color=block+(format==BlockCompression::BC1?0:8);
        const unsigned c0=unsigned(Read(color,2)),c1=unsigned(Read(color+2,2));
        unsigned char palette[4][4]{};Color(c0,palette[0]);Color(c1,palette[1]);
        if(c0>c1||format!=BlockCompression::BC1) {
            for(unsigned channel=0;channel<3;++channel) {
                palette[2][channel]=(2*palette[0][channel]+palette[1][channel])/3;
                palette[3][channel]=(palette[0][channel]+2*palette[1][channel])/3;
            }
            palette[2][3]=palette[3][3]=255;
        } else {
            for(unsigned channel=0;channel<3;++channel)palette[2][channel]=(palette[0][channel]+palette[1][channel])/2;
            palette[2][3]=255; // palette[3] is transparent black.
        }
        unsigned char alphas[8]{};
        if(format==BlockCompression::BC3) {
            alphas[0]=block[0];alphas[1]=block[1];
            const unsigned denominator=alphas[0]>alphas[1]?7:5;
            for(unsigned i=1;i<denominator;++i)alphas[i+1]=((denominator-i)*alphas[0]+i*alphas[1])/denominator;
            if(denominator==5){alphas[6]=0;alphas[7]=255;}
        }
        const auto indices=Read(color+4,4);
        const auto alphaBits=format==BlockCompression::BC2?Read(block,8):(format==BlockCompression::BC3?Read(block+2,6):0);
        for(unsigned y=0;y<4;++y)for(unsigned x=0;x<4;++x) {
            const auto dx=bx*4+x,dy=by*4+y;if(dx>=width||dy>=height)continue;
            const unsigned index=y*4+x;auto* pixel=result.data()+((std::size_t(z)*height+std::size_t(dy))*width+std::size_t(dx))*4;
            std::memcpy(pixel,palette[(indices>>(2*index))&3],4);
            if(format==BlockCompression::BC2)pixel[3]=unsigned((alphaBits>>(4*index))&15)*17;
            else if(format==BlockCompression::BC3)pixel[3]=alphas[(alphaBits>>(3*index))&7];
        }
    }
    rgba=std::move(result);return true;
}
}
