#include "gfx_vulkan/upload_bytes.h"
#include <array>
#include <cstdio>
#include <stdexcept>
using bo1::vulkan::UploadBytes;
static unsigned checks;
static void Check(bool ok){++checks;if(!ok)throw std::runtime_error("upload snapshot invariant");}
int main(){try {
    std::string error;
    std::array<unsigned char,35> source{};
    for(unsigned i=0;i<source.size();++i)source[i]=static_cast<unsigned char>(i+1);
    auto packed=UploadBytes::Rows(source.data(),5,5,7,0,error);Check(packed&&packed->size()==25);
    for(unsigned y=0;y<5;++y)for(unsigned x=0;x<5;++x)Check(packed->data()[y*5+x]==source[y*7+x]);
    auto retained=packed;source.fill(99);packed.reset();Check(retained->data()[0]==1&&retained->data()[24]==33);
    auto contiguous=UploadBytes::Copy(source.data(),source.size(),error);source.fill(0);
    for(unsigned i=0;i<contiguous->size();++i)Check(contiguous->data()[i]==99);
    for(unsigned char value:{0,128,255}) {
        auto clear=UploadBytes::Rows(nullptr,1024,1024,0,value,error);Check(clear&&clear->size()==1048576);
        for(std::size_t i=0;i<clear->size();++i)Check(clear->data()[i]==value);
    }
    std::vector<unsigned char> decoded{3,7,11,255};const auto old=decoded.data();
    auto moved=UploadBytes::Decoded(std::move(decoded));Check(moved->data()==old&&moved->size()==4&&moved->data()[3]==255);
    Check(!UploadBytes::Copy(nullptr,1,error));Check(!UploadBytes::Rows(source.data(),5,2,4,0,error));
    Check(!UploadBytes::Rows(source.data(),0,2,7,0,error));Check(!UploadBytes::Rows(source.data(),5,0,7,0,error));
    Check(!UploadBytes::Rows(source.data(),5,3,std::numeric_limits<std::size_t>::max(),0,error));
    Check(!UploadBytes::Rows(nullptr,std::numeric_limits<std::size_t>::max(),2,0,0,error));
    Check(!UploadBytes::Rows(source.data(),5,std::numeric_limits<std::size_t>::max(),7,0,error));
    std::printf("native_upload_bytes=pass checks=%u immutable_pitched_clear_overflow=pass cpu_fixture_only=1\n",checks);return 0;
}catch(const std::exception& e){std::fprintf(stderr,"FAIL %s checks=%u\n",e.what(),checks);return 1;}}
