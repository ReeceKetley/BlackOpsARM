#include <qcommon/common.h>
#include <mutex>
#include <string>
#include <cstdint>
#include <climits>
#include <unistd.h>
#include <new>
namespace {
struct NativeVpx {
    vpx_codec_ctx_t codec{};vpx_codec_enc_cfg_t cfg{};vpx_image_t image{};
    FILE *file{};std::string target,temporary;bool imageOwned{},codecOwned{},ready{};
    std::uint64_t submitted{},packets{};
};
std::mutex vpxOwner;
NativeVpx state;
void Clear() {
    state.ready=false;
    if(state.codecOwned || state.codec.priv)vpx_codec_destroy(&state.codec);
    state.codecOwned=false;state.codec={};
    if(state.imageOwned)vpx_img_free(&state.image);state.imageOwned=false;state.image={};
    if(state.file)std::fclose(state.file);state.file=nullptr;
    if(!state.temporary.empty())::unlink(state.temporary.c_str());
    state.target.clear();state.temporary.clear();state.submitted=state.packets=0;
}
bool Header(FILE *file,const vpx_codec_enc_cfg_t *cfg,unsigned frames) {
    if(!file || !cfg || !cfg->g_w || !cfg->g_h || cfg->g_w>65535 || cfg->g_h>65535 || cfg->g_timebase.num<=0 || cfg->g_timebase.den<=0)return false;
    char header[32]{};std::memcpy(header,"DKIF",4);mem_put_le16(header+6,32);mem_put_le32(header+8,0x30385056U);
    mem_put_le16(header+12,static_cast<short>(cfg->g_w));mem_put_le16(header+14,static_cast<short>(cfg->g_h));
    mem_put_le32(header+16,cfg->g_timebase.den);mem_put_le32(header+20,cfg->g_timebase.num);mem_put_le32(header+24,frames);
    return std::fwrite(header,1,sizeof(header),file)==sizeof(header);
}
bool PacketHeader(FILE *file,const vpx_codec_cx_pkt_t *packet) {
    if(!file || !packet || packet->kind!=VPX_CODEC_CX_FRAME_PKT || packet->data.frame.sz>UINT32_MAX || packet->data.frame.pts<0)return false;
    char header[12];const std::uint64_t pts=static_cast<std::uint64_t>(packet->data.frame.pts);
    mem_put_le32(header,static_cast<unsigned>(packet->data.frame.sz));mem_put_le32(header+4,static_cast<unsigned>(pts));mem_put_le32(header+8,static_cast<unsigned>(pts>>32));
    return std::fwrite(header,1,sizeof(header),file)==sizeof(header);
}
bool Encode(bool eof,const unsigned char *y,const unsigned char *u,const unsigned char *v) {
    if(!state.ready || state.submitted>=UINT32_MAX)return false;
    if(!eof) {
        if(!y||!u||!v)return false;
        const unsigned char *planes[3]={y,u,v};
        for(int plane=0;plane<3;++plane){const unsigned width=plane?state.cfg.g_w/2:state.cfg.g_w;const unsigned height=plane?state.cfg.g_h/2:state.cfg.g_h;
            if(!state.image.planes[plane] || state.image.stride[plane]<static_cast<int>(width))return false;
            for(unsigned row=0;row<height;++row)std::memcpy(state.image.planes[plane]+std::size_t(row)*state.image.stride[plane],planes[plane]+std::size_t(row)*width,width);
        }
    }
    if(vpx_codec_encode(&state.codec,eof?nullptr:&state.image,static_cast<vpx_codec_pts_t>(state.submitted),1,0,VPX_DL_REALTIME)!=VPX_CODEC_OK)return false;
    vpx_codec_iter_t iterator=nullptr;const vpx_codec_cx_pkt_t *packet;
    while((packet=vpx_codec_get_cx_data(&state.codec,&iterator))!=nullptr){if(packet->kind!=VPX_CODEC_CX_FRAME_PKT)continue;
        if(!packet->data.frame.buf || !packet->data.frame.sz || state.packets>=UINT32_MAX || !PacketHeader(state.file,packet) ||
           std::fwrite(packet->data.frame.buf,1,packet->data.frame.sz,state.file)!=packet->data.frame.sz)return false;
        ++state.packets;
    }
    if(!eof)++state.submitted;
    return true;
}
void Report(){Com_PrintError(16,"Native VP8/IVF operation failed; no codec substitution or successful output reported\n");}
}
void mem_put_le16(char *memory,short value){const auto bits=static_cast<unsigned short>(value);memory[0]=bits;memory[1]=bits>>8;}
void mem_put_le32(char *memory,unsigned value){for(unsigned i=0;i<4;++i)memory[i]=value>>(i*8);}
void write_ivf_file_header(FILE *file,const vpx_codec_enc_cfg_t *cfg,unsigned frames){if(!Header(file,cfg,frames))Report();}
void write_ivf_frame_header(FILE *file,const vpx_codec_cx_pkt_t *packet){if(!PacketHeader(file,packet))Report();}
bool vpx_init_native(const char *filename,int width,int height,unsigned fps) {
    if(!filename||!*filename||width<=0||height<=0||(width&1)||(height&1)||width>16383||height>16383||!fps||fps>1000000||std::uint64_t(width)*height>INT_MAX)return false;
    std::lock_guard<std::mutex> lock(vpxOwner);if(state.ready)return false;
    Clear();
    if(!vpx_img_alloc(&state.image,VPX_IMG_FMT_I420,width,height,16)){Clear();return false;}state.imageOwned=true;
    if(vpx_codec_enc_config_default(vpx_codec_vp8_cx(),&state.cfg,0)!=VPX_CODEC_OK){Clear();return false;}
    state.cfg.g_w=width;state.cfg.g_h=height;state.cfg.g_timebase.num=1;state.cfg.g_timebase.den=fps;
    state.cfg.g_lag_in_frames=0;state.cfg.rc_dropframe_thresh=0;
    if(vpx_codec_enc_init(&state.codec,vpx_codec_vp8_cx(),&state.cfg,0)!=VPX_CODEC_OK){Clear();return false;}state.codecOwned=true;
    if(vpx_codec_control(&state.codec,VP8E_SET_ENABLEAUTOALTREF,0)!=VPX_CODEC_OK){Clear();return false;}
    try{state.target=filename;state.temporary=state.target+".tmp.XXXXXX";}catch(const std::bad_alloc&){Clear();return false;}
    const int fd=mkstemp(state.temporary.data());if(fd<0){Clear();return false;}
    state.file=fdopen(fd,"w+b");if(!state.file){::close(fd);Clear();return false;}
    if(!Header(state.file,&state.cfg,0)||fflush(state.file)!=0||fsync(fileno(state.file))!=0){Clear();return false;}
    state.ready=true;return true;
}
bool vpx_is_ready(){std::lock_guard<std::mutex> lock(vpxOwner);return state.ready;}
bool vpx_ready_for(int width,int height,unsigned fps){std::lock_guard<std::mutex> lock(vpxOwner);return state.ready&&state.cfg.g_w==unsigned(width)&&state.cfg.g_h==unsigned(height)&&state.cfg.g_timebase.den==static_cast<int>(fps);}
bool vpx_encode_native(const unsigned char *y,const unsigned char *u,const unsigned char *v,bool eof){std::lock_guard<std::mutex> lock(vpxOwner);if(!Encode(eof,y,u,v)){Clear();return false;}return true;}
bool vpx_shutdown_native(){
    std::lock_guard<std::mutex> lock(vpxOwner);if(!state.ready){Clear();return true;}
    bool okay=true;
    for(;;){const auto prior=state.packets;if(!Encode(true,nullptr,nullptr,nullptr)){okay=false;break;}if(state.packets==prior)break;}
    if(state.packets!=state.submitted)okay=false;
    if(vpx_codec_destroy(&state.codec)!=VPX_CODEC_OK)okay=false;state.codecOwned=false;state.codec={};
    if(okay&&(fseeko(state.file,0,SEEK_SET)!=0||!Header(state.file,&state.cfg,static_cast<unsigned>(state.packets))))okay=false;
    if(fflush(state.file)!=0)okay=false;if(fsync(fileno(state.file))!=0)okay=false;
    if(fclose(state.file)!=0)okay=false;state.file=nullptr;
    if(okay&&std::rename(state.temporary.c_str(),state.target.c_str())!=0)okay=false;
    Clear();return okay;
}
void vpx_init(const char *filename,int width,int height){if(vpx_is_ready())return;if(!vpx_init_native(filename,width,height,30))Report();}
void vpx_encode_frame(unsigned char *y,unsigned char *u,unsigned char *v,bool eof){if(!vpx_encode_native(y,u,v,eof))Report();}
void vpx_shutdown(){if(!vpx_shutdown_native())Report();}
