#include "r_jpeg.h"
#include <universal/com_memory.h>
#include <qcommon/common.h>
#include <universal/com_files.h>

#if defined(__ANDROID__)
#include <cstring>
#include <cstdlib>
#include <climits>
#include <new>
thread_local int hackSize;
struct NativeJpegDestination {
    jpeg_destination_mgr pub{};
    unsigned char *buffer{};
    std::size_t capacity{};
    bool owned{};
};
static jpeg_boolean NativeJpegGrow(jpeg_compress_struct *context) {
    auto *dest=reinterpret_cast<NativeJpegDestination *>(context->dest);
    if (!dest->owned || dest->capacity >= INT_MAX) {
        Com_Error(ERR_FATAL,"JPEG destination capacity exhausted"); std::abort();
    }
    const auto capacity=dest->capacity > INT_MAX/2 ? std::size_t(INT_MAX) : dest->capacity*2;
    auto *buffer=static_cast<unsigned char *>(Z_MallocJpeg(static_cast<unsigned int>(capacity)));
    if (!buffer) { Com_Error(ERR_FATAL,"JPEG output allocation failed"); std::abort(); }
    std::memcpy(buffer,dest->buffer,dest->capacity);
    Z_FreeJpeg(dest->buffer);
    dest->buffer=buffer;
    dest->pub.next_output_byte=buffer+dest->capacity;
    dest->pub.free_in_buffer=capacity-dest->capacity;
    dest->capacity=capacity;
    return 1;
}

// A setup/read pair owns its codec and scratch storage. Registry membership is
// checked before dereferencing the public pointer, enforcing the creating thread.
struct NativeJpegRead {
    jpeg_decompress_struct codec{};
    jpeg_error_mgr errors{};
    NativeJpegRead *next{};
    unsigned char *scanline{};
    jpeg_alloc previous{};
    bool installingAllocator{};
};
static thread_local NativeJpegRead *nativeJpegReads;

static NativeJpegRead *NativeJpegFindRead(jpeg_decompress_struct *codec) {
    for(auto *read=nativeJpegReads; read; read=read->next)
        if(&read->codec==codec) return read;
    return nullptr;
}

static void NativeJpegReleaseRead(NativeJpegRead *read) {
    jpeg_destroy_decompress(&read->codec);
    if(read->installingAllocator)
        jpeg_set_jpeg_alloc(read->previous.malloc,read->previous.free);
    if(read->scanline) Z_FreeJpeg(read->scanline);
    auto **link=&nativeJpegReads;
    while(*link!=read) link=&(*link)->next;
    *link=read->next;
    read->~NativeJpegRead();
    Z_FreeJpeg(read);
}

static void NativeJpegReadError(jpeg_common_struct *codec) {
    auto *read=NativeJpegFindRead(reinterpret_cast<jpeg_decompress_struct *>(codec));
    if(read) {
        (*codec->err->output_message)(codec);
        NativeJpegReleaseRead(read);
    }
    Jpeg_Error();
    std::abort();
}

static void NativeJpegRejectRead(NativeJpegRead *read,const char *message) {
    if(read) NativeJpegReleaseRead(read);
    Com_Error(ERR_FATAL,"%s",message);
    std::abort();
}
#else
int hackSize;
#endif
#if !defined(__ANDROID__)
int s_hunk_mark;
jpeg_decompress_struct s_read_cinfo;
jpeg_error_mgr s_read_jerr;
#endif

void *__cdecl Z_MallocJpeg(unsigned int size)
{
    return Z_Malloc(size, "Z_MallocJpeg", 11);
}

void __cdecl Z_FreeJpeg(void *ptr)
{
    Z_Free((char*)ptr, 11);
}

void __cdecl R_SaveJpg(
                char *filename,
                int quality,
                unsigned int image_width,
                unsigned int image_height,
                unsigned __int8 *image_buffer,
                unsigned __int8 *output_buffer)
{
#if defined(__ANDROID__)
    if (image_width>JPEG_MAX_DIMENSION || image_height>JPEG_MAX_DIMENSION) {
        Com_Error(ERR_FATAL,"JPEG dimensions exceed the codec limit"); std::abort();
    }
    const std::uint64_t rawSize=std::uint64_t(image_width)*image_height*3;
    if (!filename || !image_buffer || !image_width || !image_height || rawSize>INT_MAX) {
        Com_Error(ERR_FATAL,"Invalid JPEG image dimensions or storage"); std::abort();
    }
    jpeg_compress_struct cinfo{}; jpeg_error_mgr jerr{};
    const auto previous=*jpeg_get_jpeg_alloc();
    cinfo.err=jpeg_std_error(&jerr,Jpeg_Error,Jpeg_Print);
    jpeg_set_jpeg_alloc(Z_MallocJpeg,Z_FreeJpeg);
    jpeg_CreateCompress(&cinfo,JPEG_LIB_VERSION,sizeof(cinfo));
    jpegDest(reinterpret_cast<jpeg_common_struct *>(&cinfo),output_buffer,
        output_buffer ? static_cast<int>(rawSize) : 512);
    cinfo.image_width=image_width; cinfo.image_height=image_height;
    cinfo.input_components=3; cinfo.in_color_space=JCS_RGB;
    jpeg_set_defaults(&cinfo); jpeg_set_quality(&cinfo,quality,1);
    jpeg_start_compress(&cinfo,1);
    while(cinfo.next_scanline<cinfo.image_height) {
        JSAMPROW row=image_buffer+std::size_t(cinfo.next_scanline)*image_width*3;
        if(jpeg_write_scanlines(&cinfo,&row,1)!=1) {
            Com_Error(ERR_FATAL,"JPEG scanline suspended unexpectedly"); std::abort();
        }
    }
    jpeg_finish_compress(&cinfo);
    auto *dest=reinterpret_cast<NativeJpegDestination *>(cinfo.dest);
    const int written=FS_WriteFile(filename,reinterpret_cast<char *>(dest->buffer),hackSize);
    if(dest->owned) { Z_FreeJpeg(dest->buffer); dest->buffer=nullptr; }
    jpeg_destroy_compress(&cinfo);
    jpeg_set_jpeg_alloc(previous.malloc,previous.free);
    if(!written) { Com_Error(ERR_FATAL,"JPEG file write failed"); std::abort(); }
#else
    unsigned __int8 *out; // [esp+0h] [ebp-214h]
    unsigned __int8 *row_pointer[1]; // [esp+8h] [ebp-20Ch] BYREF
    jpeg_compress_struct cinfo; // [esp+Ch] [ebp-208h] BYREF
    jpeg_error_mgr jerr; // [esp+17Ch] [ebp-98h] BYREF

    cinfo.err = jpeg_std_error(&jerr, Jpeg_Error, Jpeg_Print);
    jpeg_set_jpeg_alloc(Z_MallocJpeg, Z_FreeJpeg);
    jpeg_CreateCompress(&cinfo, 62, 0x170u);
    if ( output_buffer )
    {
        out = output_buffer;
        jpegDest((jpeg_common_struct *)&cinfo, output_buffer, 3 * image_height * image_width);
    }
    else
    {
        out = (unsigned __int8 *)Hunk_AllocateTempMemory(3 * image_height * image_width, "SaveJPG");
        jpegDest((jpeg_common_struct *)&cinfo, out, 3 * image_height * image_width);
    }
    cinfo.image_width = image_width;
    cinfo.image_height = image_height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, 1u);
    jpeg_start_compress(&cinfo, 1u);
    while ( cinfo.next_scanline < cinfo.image_height )
    {
        row_pointer[0] = &image_buffer[3 * image_width * cinfo.next_scanline];
        jpeg_write_scanlines(&cinfo, row_pointer, 1u);
    }
    jpeg_finish_compress(&cinfo);
    FS_WriteFile(filename, (char *)out, hackSize);
    if ( !output_buffer )
        Hunk_FreeTempMemory((char *)out);
    //jpeg_destroy_decompress(&cinfo); // bug?
    jpeg_destroy_compress(&cinfo);
    jpeg_set_jpeg_alloc(0, 0);
#endif
}

void __cdecl jpegDest(jpeg_common_struct *cinfo, unsigned __int8 *outfile, int size)
{
#if defined(__ANDROID__)
    if(!cinfo || cinfo->is_decompressor || size<=0) {
        Com_Error(ERR_FATAL,"Invalid JPEG output destination"); std::abort();
    }
    auto *encoder=reinterpret_cast<jpeg_compress_struct *>(cinfo);
    if(encoder->dest && encoder->dest->init_destination!=init_destination) {
        Com_Error(ERR_FATAL,"JPEG destination has a different owner"); std::abort();
    }
    auto *dest=reinterpret_cast<NativeJpegDestination *>(encoder->dest);
    if(!dest) {
        dest=static_cast<NativeJpegDestination *>(cinfo->mem->alloc_small(cinfo,0,sizeof(NativeJpegDestination)));
        new (dest) NativeJpegDestination{};
        encoder->dest=&dest->pub;
    } else if(dest->owned && dest->buffer) Z_FreeJpeg(dest->buffer);
    dest->owned=outfile==nullptr; dest->capacity=static_cast<std::size_t>(size);
    dest->buffer=outfile ? outfile : static_cast<unsigned char *>(Z_MallocJpeg(size));
    if(!dest->buffer) { Com_Error(ERR_FATAL,"JPEG output allocation failed"); std::abort(); }
    dest->pub.init_destination=init_destination;
    dest->pub.empty_output_buffer=NativeJpegGrow;
    dest->pub.term_destination=term_destination;
#else
    my_destination_mgr *dest; // [esp+0h] [ebp-4h]

    if ( !cinfo[1].err )
        cinfo[1].err = (jpeg_error_mgr *)cinfo->mem->alloc_small(cinfo, 0, 28);
    dest = (my_destination_mgr *)cinfo[1].err;
    dest->pub.init_destination = init_destination;
    dest->pub.empty_output_buffer = (jpeg_boolean (__cdecl *)(jpeg_compress_struct *))Flame_GetLocalClientSourceRange;
    dest->pub.term_destination = term_destination;
    dest->outfile = (FILE*)outfile;
    dest->size = size;
#endif
}

void __cdecl init_destination(jpeg_compress_struct *cinfo)
{
#if defined(__ANDROID__)
    auto *dest=reinterpret_cast<NativeJpegDestination *>(cinfo->dest);
    dest->pub.next_output_byte=dest->buffer;
    dest->pub.free_in_buffer=dest->capacity;
#else
    my_destination_mgr *dest; // [esp+0h] [ebp-4h]

    dest = (my_destination_mgr *)cinfo->dest;
    dest->pub.next_output_byte = (JOCTET*)dest->outfile;
    dest->pub.free_in_buffer = dest->size;
#endif
}

void __cdecl term_destination(jpeg_compress_struct *cinfo)
{
#if defined(__ANDROID__)
    const auto *dest=reinterpret_cast<NativeJpegDestination *>(cinfo->dest);
    hackSize=static_cast<int>(dest->capacity-dest->pub.free_in_buffer);
#else
    hackSize = cinfo->dest[1].free_in_buffer - cinfo->dest->free_in_buffer;
#endif
}

void __cdecl Jpeg_Print(char *message)
{
    Com_Printf(8, "%s\n", message);
}

jpeg_decompress_struct *__cdecl R_ReadJpgSetup(
                const unsigned __int8 *data,
                unsigned int dataLen,
                int *width,
                int *height,
                _D3DFORMAT *imageFormat)
{
#if defined(__ANDROID__)
    if(!data || !dataLen || dataLen>INT_MAX || !width || !height || !imageFormat)
        NativeJpegRejectRead(nullptr,"Invalid JPEG input or output parameters");
    *width=0; *height=0; *imageFormat=D3DFMT_UNKNOWN;
    auto *storage=Z_MallocJpeg(sizeof(NativeJpegRead));
    if(!storage) NativeJpegRejectRead(nullptr,"JPEG read context allocation failed");
    auto *read=new(storage) NativeJpegRead{};
    read->next=nativeJpegReads; nativeJpegReads=read;
    auto *codec=&read->codec;
    codec->err=jpeg_std_error(&read->errors,Jpeg_Error,Jpeg_Print);
    codec->err->error_exit=NativeJpegReadError;
    read->previous=*jpeg_get_jpeg_alloc();
    read->installingAllocator=true;
    jpeg_set_jpeg_alloc(Z_MallocJpeg,Z_FreeJpeg);
    jpeg_CreateDecompress(codec,JPEG_LIB_VERSION,sizeof(*codec));
    // The modified codec captures these callbacks in its own memory manager.
    jpeg_set_jpeg_alloc(read->previous.malloc,read->previous.free);
    read->installingAllocator=false;
    jpeg_memory_src(codec,data,dataLen);
    if(jpeg_read_header(codec,1)!=JPEG_HEADER_OK)
        NativeJpegRejectRead(read,"JPEG input suspended unexpectedly");
    // The screenshot consumer allocates 4*width*height through signed32
    // Z_VirtualAlloc; reject an unrepresentable image before decompression.
    if(std::uint64_t(codec->image_width)*codec->image_height*4>INT_MAX)
        NativeJpegRejectRead(read,"JPEG image exceeds the renderer allocation limit");
    if(!jpeg_start_decompress(codec))
        NativeJpegRejectRead(read,"JPEG input suspended unexpectedly");
    if(codec->output_components!=3) {
        Com_PrintWarning(8,"WARNING: jpeg image is not RGB\n");
        NativeJpegReleaseRead(read);
        return nullptr;
    }
    if(!codec->output_width || !codec->output_height ||
       codec->output_width>INT_MAX/4 || codec->output_height>INT_MAX)
        NativeJpegRejectRead(read,"JPEG output dimensions exceed the renderer limits");
    *width=static_cast<int>(codec->output_width);
    *height=static_cast<int>(codec->output_height);
    *imageFormat=D3DFMT_A8R8G8B8;
    return codec;
#else
    iassert(s_hunk_mark == 0);

    s_read_cinfo.err = jpeg_std_error(&s_read_jerr, Jpeg_Error, Jpeg_Print);
    jpeg_set_jpeg_alloc(Jpeg_HunkAlloc, BLOPS_NULLSUB);
    s_hunk_mark = Hunk_SetMark();
    jpeg_CreateDecompress(&s_read_cinfo, 62, 0x1B8u);
    jpeg_memory_src(&s_read_cinfo, data, dataLen);
    jpeg_read_header(&s_read_cinfo, 1u);
    jpeg_start_decompress(&s_read_cinfo);
    if ( s_read_cinfo.output_components == 3 )
    {
        *width = s_read_cinfo.output_width;
        *height = s_read_cinfo.output_height;
        *imageFormat = D3DFMT_A8R8G8B8;
        return &s_read_cinfo;
    }
    else
    {
        Com_PrintWarning(8, "WARNING: jpeg image is not RGB\n");
        jpeg_destroy_decompress(&s_read_cinfo);
        jpeg_set_jpeg_alloc(0, 0);
        Hunk_ClearToMark(s_hunk_mark);
        s_hunk_mark = 0;
        return 0;
    }
#endif
}

void*__cdecl Jpeg_HunkAlloc(unsigned int size)
{
    return Hunk_Alloc(size, "ReadJpg", 11);
}

void Jpeg_Error()
{
    Com_Error(ERR_FATAL, "jpeg internal error");
}

#if defined(__ANDROID__)
void __cdecl R_ReadJpgAbort(jpeg_decompress_struct *context)
{
    auto *read=NativeJpegFindRead(context);
    if(!read) NativeJpegRejectRead(nullptr,"JPEG read context is not owned by the calling thread");
    NativeJpegReleaseRead(read);
}
#endif

char __cdecl R_ReadJpg(jpeg_decompress_struct *context, unsigned __int8 *pixels, int pitch, int height)
{
#if defined(__ANDROID__)
    auto *read=NativeJpegFindRead(context);
    if(!read) NativeJpegRejectRead(nullptr,"JPEG read context is not owned by the calling thread");
    const std::size_t rowBytes=std::size_t(context->output_width)*4;
    if(!pixels || pitch<=0 || std::size_t(pitch)<rowBytes ||
       height<0 || static_cast<unsigned int>(height)<context->output_height)
        NativeJpegRejectRead(read,"JPEG output pitch or height is too small");
    const std::uint64_t span=std::uint64_t(context->output_height-1)*static_cast<unsigned int>(pitch)+rowBytes;
    if(span>PTRDIFF_MAX)
        NativeJpegRejectRead(read,"JPEG output span exceeds native address limits");
    read->scanline=static_cast<unsigned char *>(Z_MallocJpeg(context->output_width*3));
    if(!read->scanline) NativeJpegRejectRead(read,"JPEG scanline allocation failed");
    while(context->output_scanline<context->output_height) {
        const auto row=context->output_scanline;
        if(jpeg_read_scanlines(context,&read->scanline,1)!=1)
            NativeJpegRejectRead(read,"JPEG scanline suspended unexpectedly");
        auto *output=pixels+std::size_t(row)*static_cast<unsigned int>(pitch);
        for(std::size_t i=0;i<context->output_width;++i) {
            output[4*i]=read->scanline[3*i];
            output[4*i+1]=read->scanline[3*i+1];
            output[4*i+2]=read->scanline[3*i+2];
            output[4*i+3]=255;
        }
    }
    if(!jpeg_finish_decompress(context))
        NativeJpegRejectRead(read,"JPEG finish suspended unexpectedly");
    NativeJpegReleaseRead(read);
    return 1;
#else
    unsigned __int8 *scanline; // [esp+8h] [ebp-Ch] BYREF
    jpeg_decompress_struct *cinfo; // [esp+Ch] [ebp-8h]
    unsigned int i; // [esp+10h] [ebp-4h]

    cinfo = context;
    if ( !s_hunk_mark
        && !Assert_MyHandler("C:\\projects_pc\\cod\\codsrc\\src\\gfx_d3d\\r_jpeg.cpp", 515, 0, "%s", "s_hunk_mark != 0") )
    {
        __debugbreak();
    }
    scanline = (unsigned __int8 *)Hunk_AllocateTempMemory(cinfo->output_components * cinfo->output_width, "R_ReadJpg");
    while ( cinfo->output_scanline < cinfo->output_height && cinfo->output_scanline != height )
    {
        jpeg_read_scanlines(cinfo, &scanline, 1u);
        for ( i = 0; i < cinfo->output_width; ++i )
        {
            pixels[4 * i] = scanline[cinfo->output_components * i];
            pixels[4 * i + 1] = scanline[cinfo->output_components * i + 1];
            pixels[4 * i + 2] = scanline[cinfo->output_components * i + 2];
            pixels[4 * i + 3] = -1;
        }
        pixels += pitch;
    }
    jpeg_finish_decompress(cinfo);
    jpeg_destroy_decompress(cinfo);
    jpeg_set_jpeg_alloc(0, 0);
    Hunk_FreeTempMemory((char *)scanline);
    Hunk_ClearToMark(s_hunk_mark);
    s_hunk_mark = 0;
    return 1;
#endif
}

