#include <gfx_d3d/r_jpeg.h>
#include <qcommon/common.h>
#include <universal/memory_services.h>
#include <array>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <csignal>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

static std::atomic<int> live{};
static bool requireCleanup;
static int allocationBudget=-1;
unsigned int *Z_Malloc(int bytes,const char *,int tag) {
    if(bytes<=0 || tag!=11) std::abort();
    if(allocationBudget==0) return nullptr;
    if(allocationBudget>0) --allocationBudget;
    auto *memory=static_cast<unsigned int *>(std::malloc(bytes));
    if(!memory) std::abort(); ++live; return memory;
}
void Z_Free(char *memory,int tag) {
    if(tag!=11 || !memory) std::abort(); --live; std::free(memory);
}
void Com_Printf(int,const char *,...) {}
void Com_PrintWarning(int,const char *,...) {}
void Com_Error(errorParm_t,const char *,...) {
    if(requireCleanup && live!=0) std::_Exit(93);
    std::abort();
}
static void Check(bool value) { if(!value) std::abort(); }
static void *PriorAlloc(unsigned int count) { return std::malloc(count); }
static void PriorFree(void *memory) { std::free(memory); }
static void Fatal() { std::abort(); }
static void Print(char *) {}

struct Encoded { std::array<unsigned char,4096> bytes{}; unsigned int length{}; };
static Encoded Encode(bool grayscale) {
    Encoded image;
    jpeg_compress_struct codec{}; jpeg_error_mgr errors{};
    codec.err=jpeg_std_error(&errors,Fatal,Print);
    jpeg_CreateCompress(&codec,JPEG_LIB_VERSION,sizeof(codec));
    jpegDest(reinterpret_cast<jpeg_common_struct *>(&codec),image.bytes.data(),image.bytes.size());
    codec.image_width=2; codec.image_height=2;
    codec.input_components=grayscale?1:3; codec.in_color_space=grayscale?JCS_GRAYSCALE:JCS_RGB;
    jpeg_set_defaults(&codec); jpeg_set_quality(&codec,90,1); jpeg_start_compress(&codec,1);
    unsigned char data[]{42,90,180,42,90,180}; JSAMPROW row=data;
    while(codec.next_scanline<codec.image_height) Check(jpeg_write_scanlines(&codec,&row,1)==1);
    jpeg_finish_compress(&codec);
    image.length=image.bytes.size()-codec.dest->free_in_buffer;
    jpeg_destroy_compress(&codec);
    return image;
}
static jpeg_decompress_struct *Setup(const Encoded &image) {
    int width=-1,height=-1; _D3DFORMAT format=D3DFMT_UNKNOWN;
    auto *codec=R_ReadJpgSetup(image.bytes.data(),image.length,&width,&height,&format);
    Check(codec && width==2 && height==2 && format==D3DFMT_A8R8G8B8);
    Check(jpeg_get_jpeg_alloc()->malloc==PriorAlloc && jpeg_get_jpeg_alloc()->free==PriorFree);
    return codec;
}
static void Decode(jpeg_decompress_struct *codec) {
    std::array<unsigned char,26> output; output.fill(0xA5);
    Check(R_ReadJpg(codec,output.data()+1,12,2)==1);
    for(int row=0;row<2;++row) {
        for(int column=0;column<2;++column) {
            const auto index=1+row*12+column*4;
            Check(std::abs(int(output[index])-42)<=4 && std::abs(int(output[index+1])-90)<=4 &&
                  std::abs(int(output[index+2])-180)<=4 && output[index+3]==255);
        }
        for(int pad=0;pad<4;++pad) Check(output[1+row*12+8+pad]==0xA5);
    }
    Check(output.front()==0xA5 && output.back()==0xA5);
}
int main() {
    jpeg_set_jpeg_alloc(PriorAlloc,PriorFree);
    const auto image=Encode(false),gray=Encode(true);
    Check(live==0);
    auto *first=Setup(image),*second=Setup(image);
    Decode(second); Decode(first); Check(live==0);
    R_ReadJpgAbort(Setup(image)); Check(live==0);
    int width=-1,height=-1; _D3DFORMAT format=D3DFMT_UNKNOWN;
    Check(!R_ReadJpgSetup(gray.bytes.data(),gray.length,&width,&height,&format));
    Check(width==0 && height==0 && format==D3DFMT_UNKNOWN && live==0);
    std::array<std::thread,2> threads;
    for(auto &thread:threads) thread=std::thread([&] {
        jpeg_set_jpeg_alloc(PriorAlloc,PriorFree);
        for(int i=0;i<8;++i) Decode(Setup(image));
    });
    for(auto &thread:threads) thread.join(); Check(live==0);
    auto *shared=static_cast<unsigned char *>(mmap(nullptr,4096,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0));
    Check(shared!=MAP_FAILED);
    for(int mode=0;mode<10;++mode) {
        std::memset(shared,0xA5,4096);
        const auto child=fork(); Check(child>=0);
        if(!child) {
            requireCleanup=true;
            if(mode==0) { unsigned char junk[]{1,2,3}; R_ReadJpgSetup(junk,3,&width,&height,&format); }
            if(mode==6) allocationBudget=0;
            if(mode==7) allocationBudget=1;
            auto truncated=image;
            if(mode==1) truncated.length-=2;
            if(mode==9) {
                bool patched=false;
                for(unsigned int i=0;i+8<truncated.length;++i) {
                    if(truncated.bytes[i]==0xFF && truncated.bytes[i+1]==0xC0) {
                        truncated.bytes[i+5]=0xFF; truncated.bytes[i+6]=0xDC;
                        truncated.bytes[i+7]=0xFF; truncated.bytes[i+8]=0xDC;
                        patched=true; break;
                    }
                }
                Check(patched);
            }
            auto *codec=Setup(mode==1 || mode==9?truncated:image);
            if(mode==8) allocationBudget=0;
            if(mode==2) R_ReadJpg(codec,shared+1,7,2);
            else if(mode==3) R_ReadJpg(codec,shared+1,8,1);
            else if(mode==4) {
                requireCleanup=false;
                std::thread wrongThread([&] { R_ReadJpg(codec,shared+1,8,2); }); wrongThread.join();
            } else if(mode==5) { R_ReadJpgAbort(codec); R_ReadJpg(codec,shared+1,8,2); }
            else R_ReadJpg(codec,shared+1,8,2);
            std::_Exit(91);
        }
        int status; Check(waitpid(child,&status,0)==child);
        Check(WIFSIGNALED(status) && WTERMSIG(status)==SIGABRT);
        if(mode>=2) for(int i=0;i<4096;++i) Check(shared[i]==0xA5);
        Check(live==0);
    }
    Check(!munmap(shared,4096));
    std::puts("PASS actual native R_ReadJpg setup/read/abort, RGB/padded rows/canaries, interleaved contexts, two threads, hook restoration, grayscale retirement, strict EOF and fatal rejection cleanup/ownership");
}
