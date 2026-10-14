#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <chrono>
#include <new>
#include <cstdlib>
#include <cstring>
#include <system_error>
#include <qcommon/threads.h>
bool mjpeg_run_encoder;
bool mjpeg_inited;
namespace {
using FrameCallback=void(*)(unsigned char*,unsigned char*,unsigned char*);
struct EncoderWorker {
    std::mutex mutex;std::condition_variable changed;std::thread thread;
    bool stopped{},pending{},busy{};unsigned captures{};int width{},height{};unsigned fps{},frames{};
    FrameCallback callback=mjpeg_add_frame;
    std::vector<unsigned char> planes;
    std::chrono::steady_clock::time_point next{};
    ~EncoderWorker(){Stop();}
    void Stop(){ {std::lock_guard<std::mutex> lock(mutex);stopped=true;}changed.notify_all();if(thread.joinable() && thread.get_id()!=std::this_thread::get_id())thread.join(); }
    void Run(){
        for(;;){std::vector<unsigned char> frame;FrameCallback call=nullptr;std::size_t luma=0;
            {std::unique_lock<std::mutex> lock(mutex);changed.wait(lock,[&]{return stopped||pending;});if(stopped&&!pending)return;
                frame.swap(planes);pending=false;busy=true;call=callback;luma=std::size_t(width)*height;}
            call(frame.data(),frame.data()+luma,frame.data()+luma+luma/4);
            bool finalize=false;
            {std::lock_guard<std::mutex> lock(mutex);++frames;if(frames>=8192)__atomic_store_n(&mjpeg_run_encoder,false,__ATOMIC_RELEASE);finalize=!__atomic_load_n(&mjpeg_run_encoder,__ATOMIC_ACQUIRE);}
            if(finalize){
                {std::unique_lock<std::mutex> lock(mutex);changed.wait(lock,[&]{return captures==0;});}
                mjpeg_close_int();
            }
            {std::lock_guard<std::mutex> lock(mutex);busy=false;}
            changed.notify_all();
        }
    }
};
EncoderWorker &Worker(){static EncoderWorker value;return value;}
thread_local unsigned ownedCaptures;
struct CaptureLease {
    EncoderWorker *worker{};
    ~CaptureLease(){if(!worker)return;{std::lock_guard<std::mutex> lock(worker->mutex);--worker->captures;--ownedCaptures;}worker->changed.notify_all();}
};
}
static void SetEncoding(bool enabled){__atomic_store_n(&mjpeg_run_encoder,enabled,__ATOMIC_RELEASE);if(!enabled)Worker().changed.notify_all();}
void mjpeg_stop_encoding(){SetEncoding(false);}
bool mjpeg_is_encoding(){return __atomic_load_n(&mjpeg_run_encoder,__ATOMIC_ACQUIRE);}
void mjpeg_initonce(){
    auto &worker=Worker();bool failed=false;
    {std::lock_guard<std::mutex> lock(worker.mutex);if(worker.thread.joinable())return;
        try{worker.stopped=false;worker.thread=std::thread(mjpeg_thread,&worker);mjpeg_inited=true;}catch(const std::system_error&){failed=true;}}
    if(failed){Com_Error(ERR_FATAL,"Could not create native MJPEG encoder worker");std::abort();}
}
void mjpeg_set_callback(FrameCallback callback){if(!callback){Com_Error(ERR_FATAL,"MJPEG frame callback is null");std::abort();}auto &worker=Worker();std::lock_guard<std::mutex> lock(worker.mutex);worker.callback=callback;}
void mjpeg_init(){mjpeg_set_callback(mjpeg_add_frame);}
void mjpeg_start_encoding(int width,int height,unsigned fps){
    if(width<=0 || height<=0 || width>65500 || height>65500 || (width&1) || (height&1) || !fps || fps>1000000 ||
       mjpeg_is_encoding() || !yuv_is_ready() || !mjpeg_writer_ready(width,height,fps)) {
        Com_PrintError(16,"MJPEG start rejected invalid dimensions/FPS, active recording or missing actual resources\n");return;
    }
    mjpeg_initonce();auto &worker=Worker();bool rejected=false;
    {std::lock_guard<std::mutex> lock(worker.mutex);rejected=!worker.thread.joinable()||worker.pending||worker.busy||worker.captures;
        if(!rejected){worker.width=width;worker.height=height;worker.fps=fps;worker.frames=0;worker.next={};SetEncoding(true);}}
    if(rejected)Com_PrintError(16,"MJPEG start rejected unfinished previous worker ownership\n");
}
void mjpeg_close(){
    auto &worker=Worker();SetEncoding(false);
    if(ownedCaptures || (worker.thread.joinable()&&worker.thread.get_id()==std::this_thread::get_id())){Com_Error(ERR_FATAL,"MJPEG close cannot wait on its own capture or encoder callback thread");std::abort();}
    {std::unique_lock<std::mutex> lock(worker.mutex);worker.changed.wait(lock,[&]{return !worker.pending&&!worker.busy&&worker.captures==0;});}
    mjpeg_close_int();
}
void mjpeg_shutdown(){auto &worker=Worker();if(ownedCaptures || (worker.thread.joinable()&&worker.thread.get_id()==std::this_thread::get_id())){Com_Error(ERR_FATAL,"MJPEG shutdown cannot join its own worker");std::abort();}mjpeg_close();worker.Stop();mjpeg_inited=false;}
void mjpeg_draw(){
    if(!mjpeg_is_encoding())return;
    const bool waitForCodec=mjpeg_writer_uses_vpx();
    auto &worker=Worker();int width,height;bool backpressure=false;CaptureLease capture;
    {std::unique_lock<std::mutex> lock(worker.mutex);if(!mjpeg_is_encoding())return;
        if(waitForCodec){worker.changed.wait(lock,[&]{return !mjpeg_is_encoding()||worker.stopped||(!worker.pending&&!worker.busy&&!worker.captures);});if(!mjpeg_is_encoding()||worker.stopped)return;}
        const auto now=std::chrono::steady_clock::now();if(now<worker.next)return;
        backpressure=worker.pending||worker.busy||worker.captures;width=worker.width;height=worker.height;
        if(!backpressure){worker.next=now+std::chrono::nanoseconds(1000000000ULL/worker.fps);++worker.captures;++ownedCaptures;capture.worker=&worker;}}
    if(backpressure){Com_PrintWarning(16,"MJPEG encoder busy; capture frame dropped\n");return;}
    if(!yuv_encode_frame())return;
    unsigned char *y=nullptr,*u=nullptr,*v=nullptr;yuv_get_buffers(&y,&u,&v);
    if(!y||!u||!v){SetEncoding(false);Com_PrintError(16,"MJPEG capture resources disappeared\n");return;}
    std::vector<unsigned char> frame;const std::size_t luma=std::size_t(width)*height;
    try{frame.resize(luma+luma/2);}catch(const std::bad_alloc&){SetEncoding(false);Com_PrintError(16,"MJPEG owned frame allocation failed\n");return;}
    std::memcpy(frame.data(),y,luma);std::memcpy(frame.data()+luma,u,luma/4);std::memcpy(frame.data()+luma+luma/4,v,luma/4);
    {std::lock_guard<std::mutex> lock(worker.mutex);if(!mjpeg_is_encoding()||worker.stopped)return;worker.planes.swap(frame);worker.pending=true;}
    worker.changed.notify_one();
}
void __stdcall mjpeg_thread(void *context){if(context!=&Worker()){Com_Error(ERR_FATAL,"Native MJPEG worker requires its actual owned context");std::abort();}static_cast<EncoderWorker*>(context)->Run();}
