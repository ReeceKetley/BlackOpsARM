#include <platform/fastfile_inflate.h>
#include <zlib/zlib.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
using Result = Bo1InflateResult;
static void check(bool value,int line,const char *expression) {
    if(!value) {std::fprintf(stderr,"FAIL %d: %s\n",line,expression);std::exit(1);}
}
#define CHECK(c) check((c),__LINE__,#c)
static std::vector<std::uint8_t> load(const std::string &path) {
    FILE *f=Bo1_OpenFile(path.c_str(),"rb"); CHECK(f);
    std::vector<std::uint8_t> data;
    std::uint8_t buffer[32768]; std::size_t got;
    while((got=std::fread(buffer,1,sizeof(buffer),f))) data.insert(data.end(),buffer,buffer+got);
    CHECK(!std::ferror(f)); CHECK(std::fclose(f)==0); return data;
}
static void fail(const std::vector<std::uint8_t> &data, Result expected, Bo1InflateLimits limits={}) {
    Bo1InflatedFastFile out; out.file.size=123; out.decoded={42}; out.fileBytes=99;
    auto status=Bo1_InflateFastFileMemory(data.data(),data.size(),limits,&out);
    if(status!=expected) std::fprintf(stderr,"expected %s got %s (%zu bytes)\n",Bo1_InflateResultName(expected),Bo1_InflateResultName(status),data.size());
    CHECK(status==expected); CHECK(out.decoded.empty() && !out.file.size && !out.raw.version && !out.fileBytes);
}
static void verify(const Bo1InflatedFastFile &out, unsigned size) {
    CHECK(out.file.size==size && out.file.externalSize==0xfffffffe);
    CHECK(out.file.blockSize[0]==36 && out.file.blockSize[1]==0xffffffff && out.file.blockSize[2]==13 && out.file.blockSize[4]==4097);
    for(unsigned i:{3u,5u,6u}) CHECK(out.file.blockSize[i]==0);
    CHECK(out.decoded.size()==36ull+size && !std::memcmp(&out.file,out.decoded.data(),36));
    CHECK(out.fileBytes==12+out.compressedBytes+out.paddingBytes);
    std::uint32_t state=11;
    for(unsigned i=0;i<size;++i) {state=state*1664525u+1013904223u; CHECK(out.decoded[36+i]==state>>24);}
}
int main(int argc,char **argv) {
    CHECK(argc>0); std::string self=argv[0]; auto slash=self.find_last_of("/\\");
    std::string fixtures=(slash==std::string::npos?".":self.substr(0,slash))+"/inflate-fixtures/";
    for(unsigned size:{0u,1u,127u,32767u,32768u,32769u,524287u,524288u,524289u}) {
        auto path=fixtures+"payload-"+std::to_string(size)+".ff"; auto data=load(path);
        Bo1InflatedFastFile out; Bo1InflateLimits limits;
        CHECK(Bo1_InflateFastFilePath(path.c_str(),limits,&out)==Result::Ok); verify(out,size);
        limits.maxFileBytes=data.size(); limits.maxDecodedBytes=36ull+size;
        CHECK(Bo1_InflateFastFileMemory(data.data(),data.size(),limits,&out)==Result::Ok); verify(out,size);
        --limits.maxFileBytes; fail(data,Result::InputLimit,limits); ++limits.maxFileBytes;
        --limits.maxDecodedBytes; fail(data,Result::OutputLimit,limits);
        if(size==127) for(std::size_t chunk:{1u,2u,7u,31u,32767u}) {
            limits={}; limits.chunkBytes=chunk;
            CHECK(Bo1_InflateFastFileMemory(data.data(),data.size(),limits,&out)==Result::Ok); verify(out,size);
        }
    }
    auto good=load(fixtures+"payload-127.ff"); auto bad=good;
    Bo1InflatedFastFile aligned;
    std::vector<std::uint8_t> unaligned(1,0); unaligned.insert(unaligned.end(),good.begin(),good.end());
    CHECK(Bo1_InflateFastFileMemory(unaligned.data()+1,good.size(),{},&aligned)==Result::Ok); verify(aligned,127);
    auto pattern=load(fixtures+"pattern.ff");
    CHECK(Bo1_InflateFastFileMemory(pattern.data(),pattern.size(),{},&aligned)==Result::Ok);
    CHECK(aligned.file.size==131073 && aligned.decoded.size()==131109);
    for(unsigned i=0;i<131073;++i) CHECK(aligned.decoded[36+i]=="T5 Zombies"[i%10]);
    Bo1InflateLimits bomb; bomb.maxDecodedBytes=65536; fail(pattern,Result::OutputLimit,bomb);
    fail(load(fixtures+"raw-deflate.ff"),Result::CorruptStream);
    fail(load(fixtures+"gzip.ff"),Result::CorruptStream);
    bad[0]='X'; fail(bad,Result::BadMagic); bad=good; bad[8]=0; fail(bad,Result::BadVersion);
    bad=good; std::memcpy(bad.data(),BO1_FF_MAGIC_RETAIL,8); fail(bad,Result::UnsupportedAuthenticated);
    for(unsigned length=0;length<12;++length) fail({good.begin(),good.begin()+length},Result::ShortRawHeader);
    bad=good; bad[12]=0; fail(bad,Result::CorruptStream);
    bad=good; bad.back()^=1; fail(bad,Result::CorruptStream); // Adler32 trailer after full output.
    for(std::size_t length:{12u,13u,14u,20u}) fail({good.begin(),good.begin()+length},Result::TruncatedStream);
    for(std::size_t removed:{1u,2u,4u,30u}) fail({good.begin(),good.end()-removed},Result::TruncatedStream);
    fail(load(fixtures+"short-xfile.ff"),Result::ShortXFile);
    fail(load(fixtures+"dictionary.ff"),Result::DictionaryRequired);
    fail(load(fixtures+"mismatch--1.ff"),Result::SizeMismatch);
    fail(load(fixtures+"mismatch-1.ff"),Result::SizeMismatch);
    bad=good; bad.push_back(1); fail(bad,Result::TrailingData);
    bad=good; bad.insert(bad.end(),good.begin()+12,good.end()); fail(bad,Result::TrailingData);
    Bo1InflatedFastFile out; Bo1InflateLimits limits;
    for(std::size_t padding:{1u,23u,31u}) {
        bad=good; bad.insert(bad.end(),padding,0);
        CHECK(Bo1_InflateFastFileMemory(bad.data(),bad.size(),limits,&out)==Result::Ok); verify(out,127);
        CHECK(out.paddingBytes==padding); Bo1InflateLimits strict; strict.maxZeroPadding=0; fail(bad,Result::TrailingData,strict);
        Bo1InflateLimits exact; exact.maxFileBytes=bad.size();
        CHECK(Bo1_InflateFastFileMemory(bad.data(),bad.size(),exact,&out)==Result::Ok);
        --exact.maxFileBytes; fail(bad,Result::InputLimit,exact);
    }
    bad.push_back(0); fail(bad,Result::TrailingData);
    for(std::size_t chunk:{1u,7u}) {
        bad=good; bad.insert(bad.end(),31,0);
        limits={}; limits.chunkBytes=chunk; limits.maxFileBytes=bad.size();
        CHECK(Bo1_InflateFastFileMemory(bad.data(),bad.size(),limits,&out)==Result::Ok); CHECK(out.paddingBytes==31);
        --limits.maxFileBytes; fail(bad,Result::InputLimit,limits);
    }
    limits={}; limits.chunkBytes=1; limits.maxDecodedBytes=162; fail(good,Result::OutputLimit,limits);
    for(unsigned budget:{0u,11u}) {limits={}; limits.maxFileBytes=budget; fail(good,Result::InputLimit,limits);}
    limits={};
    limits.chunkBytes=0; fail(good,Result::InvalidArgument,limits);
    limits.chunkBytes=32769; fail(good,Result::InvalidArgument,limits);
    limits={}; limits.maxZeroPadding=32; fail(good,Result::InvalidArgument,limits);
    CHECK(Bo1_InflateFastFileMemory(nullptr,1,{},&out)==Result::InvalidArgument);
    CHECK(Bo1_InflateFastFileMemory(good.data(),good.size(),{},nullptr)==Result::InvalidArgument);
    CHECK(Bo1_InflateFastFilePath(nullptr,{},&out)==Result::InvalidArgument);
    CHECK(Bo1_InflateFastFilePath((fixtures+"missing-never-created.ff").c_str(),{},&out)==Result::NotFound);
    CHECK(Bo1_InflateFastFilePath(fixtures.c_str(),{},&out)==Result::IoError);
    out.decoded=good;
    CHECK(Bo1_InflateFastFileMemory(out.decoded.data(),out.decoded.size(),{},&out)==Result::InvalidArgument);
    CHECK(out.decoded.empty());
    static_assert(sizeof(XFile)==36 && sizeof(unsigned)==4);
    std::printf("codec=%s streamBytes=%zu unsignedLongBytes=%zu\n",zlibVersion(),sizeof(z_stream),sizeof(uLong));
    for(int i=1;i<argc;++i) {
        auto status=Bo1_InflateFastFilePath(argv[i],{},&out);
        std::printf("GENUINE status=%s file=%s fileBytes=%llu compressedBytes=%llu padding=%llu decoded=%zu size=%u external=%u blocks=",
            Bo1_InflateResultName(status),argv[i],static_cast<unsigned long long>(out.fileBytes),
            static_cast<unsigned long long>(out.compressedBytes),static_cast<unsigned long long>(out.paddingBytes),
            out.decoded.size(),out.file.size,out.file.externalSize);
        for(unsigned block:out.file.blockSize) std::printf("%u,",block);
        std::puts(""); CHECK(status==Result::Ok);
        auto checksum=adler32(0,nullptr,0);
        for(std::size_t pos=0;pos<out.decoded.size();pos+=32768) {
            auto count=out.decoded.size()-pos; if(count>32768) count=32768;
            checksum=adler32(checksum,out.decoded.data()+pos,static_cast<uInt>(count));
        }
        std::printf("GENUINE decodedAdler32=%08llx\n",static_cast<unsigned long long>(checksum));
    }
    std::puts("PASS bounded inflate: independent fixtures, wrapped checksum/end, disk36, exact budgets, corrupt/truncated/dictionary/size/trailing failures, byte/chunk boundaries, atomic output publication");
}
