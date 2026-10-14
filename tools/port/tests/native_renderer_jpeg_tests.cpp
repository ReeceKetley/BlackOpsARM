#include <gfx_d3d/r_jpeg.h>
#include <qcommon/common.h>
#include <universal/memory_services.h>
#include <array>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
static std::atomic<int> live{};
static bool failWrite,requireCleanup;
// Isolate engine initialization; actual tagged allocations and files use OS memory/I/O.
unsigned int *Z_Malloc(int bytes,const char *,int tag) {
    if(bytes<=0 || tag!=11) std::abort();
    auto *memory=static_cast<unsigned int *>(std::malloc(bytes));
    if(!memory) std::abort(); ++live; return memory;
}
void Z_Free(char *memory,int tag) { if(tag!=11 || !memory) std::abort(); --live; std::free(memory); }
int FS_WriteFile(char *filename,char *data,unsigned int size) {
    if(failWrite) return 0;
    FILE *file=std::fopen(filename,"wb"); if(!file) return 0;
    const auto written=std::fwrite(data,1,size,file);
    const int closed=std::fclose(file);
    return written==size && closed==0;
}
void Com_Printf(int,const char *,...) {}
void Com_Error(errorParm_t,const char *,...) {
    if(requireCleanup && live!=0) std::_Exit(93);
    std::abort();
}
static void Check(bool ok) { if(!ok) std::abort(); }
static void *PriorAlloc(unsigned int count) { return std::malloc(count); }
static void PriorFree(void *memory) { std::free(memory); }
static void Fatal() { std::abort(); }
static void Print(char *) {}
extern thread_local int hackSize;
int main() {
    char filename[]="/data/local/tmp/blackopsarm-renderer-jpeg-fixture1.jpg";
    std::array<unsigned char,12> pixels{42,90,180,42,90,180,42,90,180,42,90,180};
    jpeg_set_jpeg_alloc(PriorAlloc,PriorFree);
    R_SaveJpg(filename,90,2,2,pixels.data(),nullptr);
    Check(live==0 && hackSize>512);
    Check(jpeg_get_jpeg_alloc()->malloc==PriorAlloc && jpeg_get_jpeg_alloc()->free==PriorFree);
    std::array<unsigned char,4096> fileBytes{};
    FILE *file=std::fopen(filename,"rb"); Check(file!=nullptr);
    const int length=std::fread(fileBytes.data(),1,fileBytes.size(),file); Check(!std::fclose(file));
    Check(length==hackSize);
    jpeg_decompress_struct decoder{}; jpeg_error_mgr errors{};
    decoder.err=jpeg_std_error(&errors,Fatal,Print);
    jpeg_CreateDecompress(&decoder,JPEG_LIB_VERSION,sizeof(decoder));
    jpeg_memory_src(&decoder,fileBytes.data(),length);
    Check(jpeg_read_header(&decoder,TRUE)==JPEG_HEADER_OK && jpeg_start_decompress(&decoder));
    Check(decoder.output_width==2 && decoder.output_height==2 && decoder.output_components==3);
    std::array<unsigned char,6> row; JSAMPROW rows[]={row.data()};
    while(decoder.output_scanline<decoder.output_height) {
        Check(jpeg_read_scanlines(&decoder,rows,1)==1);
        for(int i=0;i<2;++i) {
            Check(std::abs(int(row[i*3])-42)<=4 && std::abs(int(row[i*3+1])-90)<=4 && std::abs(int(row[i*3+2])-180)<=4);
        }
    }
    Check(jpeg_finish_decompress(&decoder)); jpeg_destroy_decompress(&decoder);
    auto *borrowed=static_cast<unsigned char *>(mmap(nullptr,4096,PROT_READ|PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS,-1,0));
    Check(borrowed!=MAP_FAILED); std::memset(borrowed,0xA5,5);
    for(int mode=0;mode<2;++mode) {
        const auto child=fork(); Check(child>=0);
        if(!child) {
            failWrite=mode==1; requireCleanup=mode==1;
            R_SaveJpg(filename,90,mode==0?1:2,mode==0?1:2,pixels.data(),mode==0?borrowed+1:nullptr);
            std::_Exit(91);
        }
        int status; Check(waitpid(child,&status,0)==child);
        Check(WIFSIGNALED(status) && WTERMSIG(status)==SIGABRT);
    }
    Check(borrowed[0]==0xA5 && borrowed[4]==0xA5);
    Check(!munmap(borrowed,4096) && !std::remove(filename));
    std::puts("PASS actual R_SaveJpg owned growth/real file/pixel decode/allocator restore and cleanup; bounded borrowed overflow and file-write failure");
}
