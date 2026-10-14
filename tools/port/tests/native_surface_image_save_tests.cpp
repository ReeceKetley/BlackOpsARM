#include <platform/android/surface_image_save.h>
#include <gfx_d3d/r_jpeg.h>
#include <array>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <thread>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>
static void Check(bool good) { if(!good) std::abort(); }
// Linker wrappers observe the actual writer/codec allocation lifecycle without
// replacing either algorithm. Fixed bookkeeping avoids recursive allocation.
static bool tracking,allocationFailed;
static int budget=-1;
static std::array<void *,256> allocations;
extern "C" void *__real_malloc(std::size_t);
extern "C" void *__real_calloc(std::size_t,std::size_t);
extern "C" void __real_free(void *);
static bool RejectAllocation() {
    if(!tracking) return false;
    if(budget==0) { allocationFailed=true; return true; }
    if(budget>0) --budget;
    return false;
}
static void Track(void *memory) {
    if(!tracking || !memory) return;
    for(auto &slot:allocations) if(!slot) { slot=memory; return; }
    std::abort();
}
extern "C" void *__wrap_malloc(std::size_t bytes) {
    if(RejectAllocation()) return nullptr;
    auto *memory=__real_malloc(bytes); Track(memory); return memory;
}
extern "C" void *__wrap_calloc(std::size_t count,std::size_t bytes) {
    if(RejectAllocation()) return nullptr;
    auto *memory=__real_calloc(count,bytes); Track(memory); return memory;
}
extern "C" void __wrap_free(void *memory) {
    if(memory) for(auto &slot:allocations) if(slot==memory) { slot=nullptr; break; }
    __real_free(memory);
}
static void EndTracking() {
    tracking=false; budget=-1;
    for(auto *memory:allocations) Check(!memory);
}
static HRESULT SavePixels(const char *filename,D3DXIMAGE_FILEFORMAT fileFormat,D3DFORMAT pixelFormat,
    const void *pixels,UINT width,UINT height,std::ptrdiff_t pitch) {
    tracking=true; allocationFailed=false;
    const auto result=BO1_SaveSurfacePixels(filename,fileFormat,pixelFormat,pixels,width,height,pitch);
    EndTracking(); return result;
}
static HRESULT SaveSurface(const char *filename,D3DXIMAGE_FILEFORMAT format,IDirect3DSurface9 *surface,
    const PALETTEENTRY *palette,const RECT *rect) {
    tracking=true; allocationFailed=false;
    const auto result=D3DXSaveSurfaceToFileA(filename,format,surface,palette,rect);
    EndTracking(); return result;
}
static void *Allocate(unsigned int size) { return std::malloc(size); }
static void Release(void *memory) { std::free(memory); }
static void Fatal() { std::abort(); }
static void Print(char *) {}
static std::vector<unsigned char> Read(const char *path) {
    FILE *file=std::fopen(path,"rb"); Check(file);
    Check(!std::fseek(file,0,SEEK_END)); const auto length=std::ftell(file); Check(length>=0);
    Check(!std::fseek(file,0,SEEK_SET)); std::vector<unsigned char> bytes(length);
    Check(std::fread(bytes.data(),1,bytes.size(),file)==bytes.size()); Check(!std::fclose(file)); return bytes;
}
static void Sentinel(const char *path) {
    FILE *file=std::fopen(path,"wb"); Check(file);
    Check(std::fwrite("keep",1,4,file)==4 && !std::fclose(file));
}
static void IsSentinel(const char *path) {
    const auto bytes=Read(path); Check(bytes.size()==4 && !std::memcmp(bytes.data(),"keep",4));
}
static void DecodeJpeg(const char *path) {
    const auto bytes=Read(path);
    jpeg_decompress_struct codec{}; jpeg_error_mgr errors{};
    codec.err=jpeg_std_error(&errors,Fatal,Print);
    jpeg_CreateDecompress(&codec,JPEG_LIB_VERSION,sizeof(codec));
    jpeg_memory_src(&codec,bytes.data(),bytes.size());
    Check(jpeg_read_header(&codec,1)==JPEG_HEADER_OK && jpeg_start_decompress(&codec));
    Check(codec.output_width==2 && codec.output_height==2 && codec.output_components==3);
    unsigned char row[6]; JSAMPROW rows=row;
    while(codec.output_scanline<codec.output_height) {
        Check(jpeg_read_scanlines(&codec,&rows,1)==1);
        for(int x=0;x<2;++x) Check(std::abs(int(row[x*3])-42)<=4 &&
            std::abs(int(row[x*3+1])-90)<=4 && std::abs(int(row[x*3+2])-180)<=4);
    }
    Check(jpeg_finish_decompress(&codec)); jpeg_destroy_decompress(&codec);
}
// CPU surface fixture checks the actual COM adapter contracts and HRESULTs.
// It is deliberately not evidence of DXVK/Vulkan or a real GPU readback.
struct CpuSurface final : IDirect3DSurface9 {
    std::array<unsigned char,48> bytes{};
    HRESULT descResult=S_OK,lockResult=S_OK,unlockResult=S_OK;
    int locks{},unlocks{}; int pitch=24; D3DFORMAT format=D3DFMT_A8R8G8B8;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID,void **) override { return E_NOINTERFACE; }
    ULONG STDMETHODCALLTYPE AddRef() override { return 1; }
    ULONG STDMETHODCALLTYPE Release() override { return 1; }
    HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9 **) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID,const void *,DWORD,DWORD) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID,void *,DWORD *) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE FreePrivateData(REFGUID) override { return E_NOTIMPL; }
    DWORD STDMETHODCALLTYPE SetPriority(DWORD) override { return 0; }
    DWORD STDMETHODCALLTYPE GetPriority() override { return 0; }
    void STDMETHODCALLTYPE PreLoad() override {}
    D3DRESOURCETYPE STDMETHODCALLTYPE GetType() override { return D3DRTYPE_SURFACE; }
    HRESULT STDMETHODCALLTYPE GetContainer(REFIID,void **) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE GetDesc(D3DSURFACE_DESC *desc) override {
        if(FAILED(descResult)) return descResult;
        *desc={}; desc->Width=4; desc->Height=2; desc->Format=format; desc->Pool=D3DPOOL_SYSTEMMEM; return S_OK;
    }
    HRESULT STDMETHODCALLTYPE LockRect(D3DLOCKED_RECT *locked,const RECT *rect,DWORD flags) override {
        ++locks; Check(!rect && flags==D3DLOCK_READONLY);
        if(FAILED(lockResult)) return lockResult;
        locked->Pitch=pitch; locked->pBits=bytes.data(); return S_OK;
    }
    HRESULT STDMETHODCALLTYPE UnlockRect() override { ++unlocks; return unlockResult; }
    HRESULT STDMETHODCALLTYPE GetDC(HDC *) override { return E_NOTIMPL; }
    HRESULT STDMETHODCALLTYPE ReleaseDC(HDC) override { return E_NOTIMPL; }
};
static bool NoTemps(const char *directory) {
    DIR *dir=opendir(directory); Check(dir); bool clean=true;
    while(auto *entry=readdir(dir)) if(std::strstr(entry->d_name,".tmp.")) clean=false;
    Check(!closedir(dir)); return clean;
}
int main() {
    char directory[]="/data/local/tmp/blackopsarm-surface-save-XXXXXX"; Check(mkdtemp(directory));
    char jpg[256],tga[256],large[256];
    std::snprintf(jpg,sizeof(jpg),"%s/test.jpg",directory);
    std::snprintf(tga,sizeof(tga),"%s/test.tga",directory);
    std::snprintf(large,sizeof(large),"%s/large.jpg",directory);
    std::array<unsigned char,24> pixels; pixels.fill(0xA5);
    for(int y=0;y<2;++y) for(int x=0;x<2;++x) {
        auto *pixel=pixels.data()+y*12+x*4;
        pixel[0]=180; pixel[1]=90; pixel[2]=42; pixel[3]=17+x+y;
    }
    jpeg_set_jpeg_alloc(Allocate,Release);
    Check(SavePixels(jpg,D3DXIFF_JPG,D3DFMT_A8R8G8B8,pixels.data(),2,2,12)==S_OK);
    Check(jpeg_get_jpeg_alloc()->malloc==Allocate && jpeg_get_jpeg_alloc()->free==Release);
    DecodeJpeg(jpg);
    Check(SavePixels(tga,D3DXIFF_TGA,D3DFMT_A8R8G8B8,pixels.data(),2,2,12)==S_OK);
    auto bytes=Read(tga); Check(bytes.size()==34 && bytes[2]==2 && bytes[12]==2 && bytes[14]==2 && bytes[16]==32 && bytes[17]==0x28);
    for(int y=0;y<2;++y) Check(!std::memcmp(bytes.data()+18+y*8,pixels.data()+y*12,8));
    Check(SavePixels(tga,D3DXIFF_TGA,D3DFMT_X8R8G8B8,pixels.data(),2,2,12)==S_OK);
    bytes=Read(tga); for(int i=0;i<4;++i) Check(bytes[18+i*4+3]==255);
    std::vector<unsigned char> noisy(128*128*4); unsigned int random=1;
    for(auto &byte:noisy) { random=random*1664525u+1013904223u; byte=random>>24; }
    Check(SavePixels(large,D3DXIFF_JPG,D3DFMT_X8R8G8B8,noisy.data(),128,128,512)==S_OK);
    Check(Read(large).size()>8192);
    Sentinel(tga);
    Check(FAILED(SavePixels(tga,D3DXIFF_PNG,D3DFMT_A8R8G8B8,pixels.data(),2,2,12)));
    Check(FAILED(SavePixels(tga,D3DXIFF_TGA,D3DFMT_R5G6B5,pixels.data(),2,2,12)));
    Check(FAILED(SavePixels(tga,D3DXIFF_TGA,D3DFMT_A8R8G8B8,pixels.data(),2,2,7)));
    Check(FAILED(SavePixels(tga,D3DXIFF_TGA,D3DFMT_A8R8G8B8,pixels.data(),2,2,PTRDIFF_MAX)));
    Check(FAILED(SavePixels("/missing/parent/test.tga",D3DXIFF_TGA,D3DFMT_A8R8G8B8,pixels.data(),2,2,12)));
    IsSentinel(tga);
    for(int injection=0;injection<4;++injection) {
        budget=injection;
        Check(FAILED(SavePixels(tga,D3DXIFF_JPG,D3DFMT_A8R8G8B8,pixels.data(),2,2,12)) && allocationFailed);
        Check(jpeg_get_jpeg_alloc()->malloc==Allocate && jpeg_get_jpeg_alloc()->free==Release);
        Check(NoTemps(directory)); IsSentinel(tga);
    }
    CpuSurface surface;
    for(int y=0;y<2;++y) for(int x=0;x<4;++x) {
        auto *pixel=surface.bytes.data()+y*24+x*4;
        pixel[0]=x; pixel[1]=y; pixel[2]=99; pixel[3]=255;
    }
    RECT roi{1,0,3,2};
    Check(SaveSurface(tga,D3DXIFF_TGA,&surface,nullptr,&roi)==S_OK);
    bytes=Read(tga); Check(bytes.size()==34 && surface.locks==1 && surface.unlocks==1);
    for(int y=0;y<2;++y) Check(!std::memcmp(bytes.data()+18+y*8,surface.bytes.data()+y*24+4,8));
    Sentinel(tga); surface.unlockResult=E_FAIL;
    Check(SaveSurface(tga,D3DXIFF_TGA,&surface,nullptr,&roi)==E_FAIL); IsSentinel(tga);
    surface.unlockResult=S_OK; surface.pitch=7;
    Check(FAILED(SaveSurface(tga,D3DXIFF_TGA,&surface,nullptr,&roi))); IsSentinel(tga);
    surface.pitch=24; surface.lockResult=D3DERR_INVALIDCALL;
    const auto unlocks=surface.unlocks;
    Check(SaveSurface(tga,D3DXIFF_TGA,&surface,nullptr,&roi)==D3DERR_INVALIDCALL && surface.unlocks==unlocks);
    surface.lockResult=S_OK; surface.descResult=E_FAIL;
    Check(SaveSurface(tga,D3DXIFF_TGA,&surface,nullptr,&roi)==E_FAIL);
    surface.descResult=S_OK; roi.right=5;
    Check(FAILED(SaveSurface(tga,D3DXIFF_TGA,&surface,nullptr,&roi))); IsSentinel(tga);
    for(int mode=0;mode<2;++mode) {
        const auto child=fork(); Check(child>=0);
        if(!child) {
            std::signal(SIGXFSZ,SIG_IGN); const rlimit limit{0,0}; Check(!setrlimit(RLIMIT_FSIZE,&limit));
            const auto result=SavePixels(tga,mode?D3DXIFF_JPG:D3DXIFF_TGA,D3DFMT_X8R8G8B8,noisy.data(),128,128,512);
            Check(FAILED(result)); Check(NoTemps(directory)); IsSentinel(tga); std::_Exit(0);
        }
        int status; Check(waitpid(child,&status,0)==child && WIFEXITED(status) && WEXITSTATUS(status)==0);
    }
    Check(NoTemps(directory)); Check(!unlink(jpg) && !unlink(tga) && !unlink(large) && !rmdir(directory));
    std::puts("PASS real JPG/TGA files, decoded RGB pixels/TGA bytes/alpha/pitch, buffered JPEG flush, invalid inputs/storage span, CPU COM ROI/error/unlock contracts, write failures preserve target and retire temporary files; no GPU proof");
}
