#include <cstdio>
#include <cstdlib>
#include <cstddef>
extern "C" {
#include <jpeg/jpeglib.h>
}
#include <array>
#include <atomic>
#include <thread>
#include <csignal>
#include <sys/wait.h>
#include <unistd.h>
static std::atomic<int> callsA{},callsB{};
static void *AllocateA(unsigned int bytes) { ++callsA; return std::malloc(bytes); }
static void *AllocateB(unsigned int bytes) { ++callsB; return std::malloc(bytes); }
static void Free(void *memory) { std::free(memory); }
static void Error() { std::abort(); }
static void Print(char *text) { std::fprintf(stderr,"jpeg: %s\n",text); }
static void Check(bool ok) { if(!ok) std::abort(); }
struct Destination {
    jpeg_destination_mgr manager{};
    std::array<unsigned char,4096> bytes{};
};
static void InitDestination(j_compress_ptr context) {
    auto *dest=reinterpret_cast<Destination *>(context->dest);
    dest->manager.next_output_byte=dest->bytes.data(); dest->manager.free_in_buffer=dest->bytes.size();
}
static boolean FullDestination(j_compress_ptr) { std::abort(); }
static void FinishDestination(j_compress_ptr) {}
static void RoundTrip(void *(*allocator)(unsigned int), unsigned char red, bool checkTruncated=true) {
    jpeg_set_jpeg_alloc(allocator,Free);
    jpeg_compress_struct encoder{}; jpeg_error_mgr error{};
    encoder.err=jpeg_std_error(&error,Error,Print);
    jpeg_CreateCompress(&encoder,JPEG_LIB_VERSION,sizeof(encoder));
    // Context owns captured hooks; clearing this thread's next-context default
    // must not break allocation/free for an already created codec.
    jpeg_set_jpeg_alloc(nullptr,nullptr);
    Destination destination;
    destination.manager.init_destination=InitDestination;
    destination.manager.empty_output_buffer=FullDestination;
    destination.manager.term_destination=FinishDestination;
    encoder.dest=&destination.manager;
    encoder.image_width=16; encoder.image_height=16;
    encoder.input_components=3; encoder.in_color_space=JCS_RGB;
    jpeg_set_defaults(&encoder); jpeg_set_quality(&encoder,90,TRUE);
    jpeg_start_compress(&encoder,TRUE);
    std::array<unsigned char,48> row;
    for(int i=0;i<16;++i) { row[i*3]=red; row[i*3+1]=90; row[i*3+2]=180; }
    JSAMPROW rows[]={row.data()};
    while(encoder.next_scanline<encoder.image_height) Check(jpeg_write_scanlines(&encoder,rows,1)==1);
    jpeg_finish_compress(&encoder);
    const int encoded=destination.bytes.size()-destination.manager.free_in_buffer;
    Check(encoded>100 && destination.bytes[0]==0xFF && destination.bytes[1]==0xD8);
    jpeg_destroy_compress(&encoder);
    jpeg_set_jpeg_alloc(allocator,Free);
    jpeg_decompress_struct decoder{};
    decoder.err=jpeg_std_error(&error,Error,Print);
    jpeg_CreateDecompress(&decoder,JPEG_LIB_VERSION,sizeof(decoder));
    jpeg_set_jpeg_alloc(nullptr,nullptr);
    jpeg_memory_src(&decoder,destination.bytes.data(),encoded);
    Check(jpeg_read_header(&decoder,TRUE)==JPEG_HEADER_OK);
    Check(jpeg_start_decompress(&decoder));
    Check(decoder.output_width==16 && decoder.output_height==16 && decoder.output_components==3);
    std::array<unsigned char,50> output; output.fill(0xA5);
    JSAMPROW outputs[]={output.data()+1};
    while(decoder.output_scanline<decoder.output_height) {
        Check(jpeg_read_scanlines(&decoder,outputs,1)==1);
        for(int i=0;i<16;++i) {
            Check(std::abs(int(output[i*3+1])-red)<=4);
            Check(std::abs(int(output[i*3+2])-90)<=4);
            Check(std::abs(int(output[i*3+3])-180)<=4);
        }
        Check(output.front()==0xA5 && output.back()==0xA5);
    }
    Check(jpeg_finish_decompress(&decoder)); jpeg_destroy_decompress(&decoder);
    if (!checkTruncated) return;
    const auto child=fork(); Check(child>=0);
    if(!child) {
        jpeg_set_jpeg_alloc(allocator,Free);
        jpeg_decompress_struct truncated{};
        truncated.err=jpeg_std_error(&error,Error,Print);
        jpeg_CreateDecompress(&truncated,JPEG_LIB_VERSION,sizeof(truncated));
        jpeg_memory_src(&truncated,destination.bytes.data(),12);
        jpeg_read_header(&truncated,TRUE); std::_Exit(91);
    }
    int status; Check(waitpid(child,&status,0)==child);
    Check(WIFSIGNALED(status) && WTERMSIG(status)==SIGABRT);
}
int main() {
    static_assert(sizeof(INT32)==4);
    jpeg_set_jpeg_alloc(AllocateA,Free);
    std::atomic<bool> bound{},release{};
    std::thread other([&] {
        Check(jpeg_get_jpeg_alloc()->malloc==nullptr);
        jpeg_set_jpeg_alloc(AllocateB,Free); bound.store(true);
        while(!release.load()) std::this_thread::yield();
        Check(jpeg_get_jpeg_alloc()->malloc==AllocateB);
    });
    while(!bound.load()) std::this_thread::yield();
    Check(jpeg_get_jpeg_alloc()->malloc==AllocateA);
    release.store(true); other.join();
    RoundTrip(AllocateA,42);
    std::thread job([] { RoundTrip(AllocateB,84,false); }); job.join();
    Check(callsA>0 && callsB>0);
    std::puts("PASS actual modified JPEG native ABI encode/decode/pixel canaries, captured allocator lifetime, thread-local hooks and strict truncated-input error");
}
