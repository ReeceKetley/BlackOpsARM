#include "surface_image_save.h"
#include <gfx_d3d/r_jpeg.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <csetjmp>
#include <unistd.h>

namespace {
bool Supported(D3DFORMAT format) {
    return format==D3DFMT_A8R8G8B8 || format==D3DFMT_X8R8G8B8;
}
HRESULT ValidateStorage(D3DFORMAT pixelsFormat,
    const void *pixels,UINT width,UINT height,std::ptrdiff_t pitch) {
    if(!Supported(pixelsFormat)) return D3DERR_NOTAVAILABLE;
    if(!pixels || !width || !height || pitch<=0 || std::uint64_t(width)*4>std::uint64_t(pitch))
        return D3DERR_INVALIDCALL;
    const auto rowBytes=std::uint64_t(width)*4;
    if(rowBytes>PTRDIFF_MAX || std::uint64_t(height-1)>
       (std::uint64_t(PTRDIFF_MAX)-rowBytes)/std::uint64_t(pitch))
        return D3DERR_INVALIDCALL;
    return S_OK;
}
HRESULT ValidatePixels(D3DXIMAGE_FILEFORMAT format,D3DFORMAT pixelsFormat,
    const void *pixels,UINT width,UINT height,std::ptrdiff_t pitch) {
    if(format!=D3DXIFF_JPG && format!=D3DXIFF_TGA) return D3DERR_NOTAVAILABLE;
    const HRESULT storage=ValidateStorage(pixelsFormat,pixels,width,height,pitch);
    if(FAILED(storage)) return storage;
    if((format==D3DXIFF_JPG && (width>JPEG_MAX_DIMENSION || height>JPEG_MAX_DIMENSION)) ||
       (format==D3DXIFF_TGA && (width>65535 || height>65535))) return D3DERR_INVALIDCALL;
    return S_OK;
}
void *CodecAlloc(unsigned int size) { return std::malloc(size); }
void CodecFree(void *memory) { std::free(memory); }
void CodecFatal() { std::abort(); }
void CodecPrint(char *message) { std::fprintf(stderr,"surface JPEG: %s\n",message); }
struct JpegWriter {
    jpeg_compress_struct codec;
    jpeg_error_mgr errors;
    jpeg_destination_mgr destination;
    std::jmp_buf failure;
    jpeg_alloc previous;
    FILE *file;
    unsigned char *row;
    unsigned char buffer[8192];
    bool installing;
};
void CodecError(jpeg_common_struct *codec) {
    auto *state=static_cast<JpegWriter *>(codec->client_data);
    (*codec->err->output_message)(codec);
    std::longjmp(state->failure,1);
}
void InitOutput(jpeg_compress_struct *codec) {
    auto *state=static_cast<JpegWriter *>(codec->client_data);
    state->destination.next_output_byte=state->buffer;
    state->destination.free_in_buffer=sizeof(state->buffer);
}
jpeg_boolean WriteOutput(jpeg_compress_struct *codec) {
    auto *state=static_cast<JpegWriter *>(codec->client_data);
    if(std::fwrite(state->buffer,1,sizeof(state->buffer),state->file)!=sizeof(state->buffer))
        std::longjmp(state->failure,1);
    InitOutput(codec);
    return 1;
}
void FinishOutput(jpeg_compress_struct *codec) {
    auto *state=static_cast<JpegWriter *>(codec->client_data);
    const auto count=sizeof(state->buffer)-state->destination.free_in_buffer;
    if(std::fwrite(state->buffer,1,count,state->file)!=count) std::longjmp(state->failure,1);
}
HRESULT WriteJpeg(FILE *file,const unsigned char *pixels,UINT width,UINT height,std::ptrdiff_t pitch) {
    // All mutable state across setjmp lives on the heap; there are no C++
    // objects requiring destructors between the jump target and codec callbacks.
    auto *state=static_cast<JpegWriter *>(std::calloc(1,sizeof(JpegWriter)));
    if(!state) return E_OUTOFMEMORY;
    state->file=file;
    if(setjmp(state->failure)) {
        jpeg_destroy_compress(&state->codec);
        if(state->installing) jpeg_set_jpeg_alloc(state->previous.malloc,state->previous.free);
        std::free(state->row); std::free(state);
        return E_FAIL;
    }
    state->codec.err=jpeg_std_error(&state->errors,CodecFatal,CodecPrint);
    state->codec.err->error_exit=CodecError;
    state->codec.client_data=state;
    state->previous=*jpeg_get_jpeg_alloc(); state->installing=true;
    jpeg_set_jpeg_alloc(CodecAlloc,CodecFree);
    jpeg_CreateCompress(&state->codec,JPEG_LIB_VERSION,sizeof(state->codec));
    jpeg_set_jpeg_alloc(state->previous.malloc,state->previous.free); state->installing=false;
    state->destination.init_destination=InitOutput;
    state->destination.empty_output_buffer=WriteOutput;
    state->destination.term_destination=FinishOutput;
    state->codec.dest=&state->destination;
    state->codec.image_width=width; state->codec.image_height=height;
    state->codec.input_components=3; state->codec.in_color_space=JCS_RGB;
    jpeg_set_defaults(&state->codec); jpeg_set_quality(&state->codec,90,1);
    state->row=static_cast<unsigned char *>(std::malloc(std::size_t(width)*3));
    if(!state->row) {
        jpeg_destroy_compress(&state->codec); std::free(state); return E_OUTOFMEMORY;
    }
    jpeg_start_compress(&state->codec,1);
    while(state->codec.next_scanline<height) {
        const auto *source=pixels+std::size_t(state->codec.next_scanline)*pitch;
        for(std::size_t i=0;i<width;++i) {
            state->row[i*3]=source[i*4+2]; state->row[i*3+1]=source[i*4+1]; state->row[i*3+2]=source[i*4];
        }
        if(jpeg_write_scanlines(&state->codec,&state->row,1)!=1) std::longjmp(state->failure,1);
    }
    jpeg_finish_compress(&state->codec); jpeg_destroy_compress(&state->codec);
    std::free(state->row); std::free(state);
    return S_OK;
}
HRESULT WriteTga(FILE *file,D3DFORMAT format,const unsigned char *pixels,UINT width,UINT height,std::ptrdiff_t pitch) {
    unsigned char header[18]{};
    header[2]=2; header[12]=width; header[13]=width>>8;
    header[14]=height; header[15]=height>>8; header[16]=32; header[17]=0x28;
    if(std::fwrite(header,1,sizeof(header),file)!=sizeof(header)) return E_FAIL;
    auto *row=static_cast<unsigned char *>(std::malloc(std::size_t(width)*4));
    if(!row) return E_OUTOFMEMORY;
    HRESULT result=S_OK;
    for(UINT y=0;y<height;++y) {
        std::memcpy(row,pixels+std::size_t(y)*pitch,std::size_t(width)*4);
        if(format==D3DFMT_X8R8G8B8) for(UINT x=0;x<width;++x) row[x*4+3]=255;
        if(std::fwrite(row,4,width,file)!=width) { result=E_FAIL; break; }
    }
    std::free(row); return result;
}
HRESULT StagePixels(const char *filename,D3DXIMAGE_FILEFORMAT format,D3DFORMAT pixelFormat,
    const void *pixels,UINT width,UINT height,std::ptrdiff_t pitch,char **temporary) {
    *temporary=nullptr;
    if(!filename || !*filename) return D3DERR_INVALIDCALL;
    const HRESULT valid=ValidatePixels(format,pixelFormat,pixels,width,height,pitch);
    if(FAILED(valid)) return valid;
    const auto length=std::strlen(filename);
    if(length>SIZE_MAX-12) return E_OUTOFMEMORY;
    auto *path=static_cast<char *>(std::malloc(length+12));
    if(!path) return E_OUTOFMEMORY;
    std::memcpy(path,filename,length); std::memcpy(path+length,".tmp.XXXXXX",12);
    const int descriptor=mkstemp(path);
    if(descriptor<0) { std::free(path); return E_FAIL; }
    FILE *file=fdopen(descriptor,"wb");
    if(!file) { close(descriptor); unlink(path); std::free(path); return E_FAIL; }
    HRESULT result=format==D3DXIFF_JPG ? WriteJpeg(file,static_cast<const unsigned char *>(pixels),width,height,pitch) :
        WriteTga(file,pixelFormat,static_cast<const unsigned char *>(pixels),width,height,pitch);
    if(std::fflush(file)!=0) result=E_FAIL;
    if(SUCCEEDED(result) && fsync(fileno(file))!=0) result=E_FAIL;
    if(std::fclose(file)!=0) result=E_FAIL;
    if(FAILED(result)) { unlink(path); std::free(path); return result; }
    *temporary=path; return S_OK;
}
HRESULT CommitPixels(const char *filename,char *temporary,HRESULT result) {
    if(temporary) {
        if(SUCCEEDED(result) && std::rename(temporary,filename)!=0) result=E_FAIL;
        if(FAILED(result)) unlink(temporary);
        std::free(temporary);
    }
    return result;
}
}

HRESULT BO1_SaveSurfacePixels(const char *filename,D3DXIMAGE_FILEFORMAT fileFormat,
    D3DFORMAT pixelFormat,const void *pixels,UINT width,UINT height,std::ptrdiff_t pitch) {
    char *temporary=nullptr;
    const HRESULT result=StagePixels(filename,fileFormat,pixelFormat,pixels,width,height,pitch,&temporary);
    return CommitPixels(filename,temporary,result);
}

extern "C" HRESULT WINAPI D3DXSaveSurfaceToFileA(const char *filename,D3DXIMAGE_FILEFORMAT format,
    IDirect3DSurface9 *surface,const PALETTEENTRY *palette,const RECT *rect) {
    if(!filename || !*filename || !surface) return D3DERR_INVALIDCALL;
    if(palette || (format!=D3DXIFF_JPG && format!=D3DXIFF_TGA)) return D3DERR_NOTAVAILABLE;
    D3DSURFACE_DESC description{};
    HRESULT result=surface->GetDesc(&description);
    if(FAILED(result)) return result;
    if(!Supported(description.Format)) return D3DERR_NOTAVAILABLE;
    if(!description.Width || !description.Height || description.Width>INT32_MAX || description.Height>INT32_MAX)
        return D3DERR_INVALIDCALL;
    RECT region{0,0,static_cast<LONG>(description.Width),static_cast<LONG>(description.Height)};
    if(rect) region=*rect;
    if(region.left<0 || region.top<0 || region.right<=region.left || region.bottom<=region.top ||
       std::uint64_t(region.right)>description.Width || std::uint64_t(region.bottom)>description.Height)
        return D3DERR_INVALIDCALL;
    D3DLOCKED_RECT locked{};
    result=surface->LockRect(&locked,nullptr,D3DLOCK_READONLY);
    if(FAILED(result)) return result;
    char *temporary=nullptr;
    result=ValidateStorage(description.Format,locked.pBits,description.Width,description.Height,locked.Pitch);
    if(SUCCEEDED(result)) {
        const auto offset=std::size_t(region.top)*static_cast<unsigned int>(locked.Pitch)+std::size_t(region.left)*4;
        result=StagePixels(filename,format,description.Format,static_cast<const unsigned char *>(locked.pBits)+offset,
            region.right-region.left,region.bottom-region.top,locked.Pitch,&temporary);
    }
    const HRESULT unlocked=surface->UnlockRect();
    if(FAILED(unlocked)) result=unlocked;
    return CommitPixels(filename,temporary,result);
}
