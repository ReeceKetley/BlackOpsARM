#include "native_jpeg.h"
#include <gfx_d3d/r_jpeg.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <climits>
#include <csetjmp>
namespace {
void *Allocate(unsigned n){return std::malloc(n);}void Free(void *p){std::free(p);}void Fatal(){std::abort();}
void Print(char *text){std::fprintf(stderr,"MJPEG: %s\n",text);}
struct Encoder {jpeg_compress_struct codec;jpeg_error_mgr errors;jpeg_destination_mgr destination;std::jmp_buf failure;jpeg_alloc previous;bool installing;unsigned char *row,*bytes;std::size_t size,capacity;unsigned char block[8192];};
void Error(jpeg_common_struct *codec){auto *state=static_cast<Encoder*>(codec->client_data);(*codec->err->output_message)(codec);std::longjmp(state->failure,1);}
void Begin(jpeg_compress_struct *codec){auto *s=static_cast<Encoder*>(codec->client_data);s->destination.next_output_byte=s->block;s->destination.free_in_buffer=sizeof(s->block);}
void Append(Encoder *s,std::size_t count){
    if(count>INT_MAX-s->size) std::longjmp(s->failure,1);
    const std::size_t total=s->size+count;
    if(total>s->capacity){std::size_t cap=s->capacity?s->capacity:8192;while(cap<total)cap=cap>INT_MAX/2?INT_MAX:cap*2;void *p=std::realloc(s->bytes,cap);if(!p)std::longjmp(s->failure,1);s->bytes=static_cast<unsigned char*>(p);s->capacity=cap;}
    std::memcpy(s->bytes+s->size,s->block,count);s->size=total;
}
jpeg_boolean Full(jpeg_compress_struct *codec){auto *s=static_cast<Encoder*>(codec->client_data);Append(s,sizeof(s->block));Begin(codec);return 1;}
void End(jpeg_compress_struct *codec){auto *s=static_cast<Encoder*>(codec->client_data);Append(s,sizeof(s->block)-s->destination.free_in_buffer);}
}
bool MjpegEncodePlanes(const unsigned char *y,const unsigned char *u,const unsigned char *v,unsigned width,unsigned height,unsigned char **bytes,std::size_t *size){
    if(bytes)*bytes=nullptr;if(size)*size=0;
    if(!bytes || !size || !y || !u || !v || !width || !height || (width&1) || (height&1) || width>JPEG_MAX_DIMENSION || height>JPEG_MAX_DIMENSION) return false;
    // Heap POD jump state: no C++ destructors crossed by codec failure.
    auto *s=static_cast<Encoder*>(std::calloc(1,sizeof(Encoder)));if(!s)return false;
    if(setjmp(s->failure)){jpeg_destroy_compress(&s->codec);if(s->installing)jpeg_set_jpeg_alloc(s->previous.malloc,s->previous.free);std::free(s->row);std::free(s->bytes);std::free(s);return false;}
    s->codec.err=jpeg_std_error(&s->errors,Fatal,Print);s->codec.err->error_exit=Error;s->codec.client_data=s;
    s->previous=*jpeg_get_jpeg_alloc();s->installing=true;jpeg_set_jpeg_alloc(Allocate,Free);
    jpeg_CreateCompress(&s->codec,JPEG_LIB_VERSION,sizeof(s->codec));jpeg_set_jpeg_alloc(s->previous.malloc,s->previous.free);s->installing=false;
    s->destination.init_destination=Begin;s->destination.empty_output_buffer=Full;s->destination.term_destination=End;s->codec.dest=&s->destination;
    s->codec.image_width=width;s->codec.image_height=height;s->codec.input_components=3;s->codec.in_color_space=JCS_YCbCr;
    jpeg_set_defaults(&s->codec);jpeg_set_quality(&s->codec,90,1);
    s->row=static_cast<unsigned char*>(std::malloc(std::size_t(width)*3));if(!s->row)std::longjmp(s->failure,1);
    jpeg_start_compress(&s->codec,1);
    while(s->codec.next_scanline<height){const std::size_t row=s->codec.next_scanline;for(unsigned x=0;x<width;++x){s->row[x*3]=y[row*width+x];s->row[x*3+1]=u[(row/2)*(width/2)+x/2];s->row[x*3+2]=v[(row/2)*(width/2)+x/2];}if(jpeg_write_scanlines(&s->codec,&s->row,1)!=1)std::longjmp(s->failure,1);}
    jpeg_finish_compress(&s->codec);jpeg_destroy_compress(&s->codec);std::free(s->row);*bytes=s->bytes;*size=s->size;std::free(s);return true;
}
