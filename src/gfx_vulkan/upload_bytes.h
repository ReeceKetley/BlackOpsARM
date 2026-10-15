#pragma once
#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace bo1::vulkan {
// Immutable CPU upload storage. Raw planes use bulk copies without constructing
// or destroying an STL element for every byte in instrumented Debug builds.
// Compressed-image decoding can transfer its existing vector without a copy.
class UploadBytes {
public:
    static std::shared_ptr<const UploadBytes> Copy(const void* source,std::size_t bytes,std::string& error) {
        if(!source) {error="Missing upload bytes";return {};}
        return Rows(source,bytes,1,bytes,0,error);
    }
    static std::shared_ptr<const UploadBytes> Rows(const void* source,std::size_t rowBytes,
        std::size_t rows,std::size_t pitch,unsigned char clearValue,std::string& error) {
        const auto maximum=std::size_t(std::numeric_limits<std::ptrdiff_t>::max());
        if(!rowBytes||!rows||rowBytes>maximum/rows
            ||(source&&(pitch<rowBytes||pitch>maximum||(rows>1&&pitch>(maximum-rowBytes)/(rows-1))))) {
            error="Invalid upload row range/pitch";return {};
        }
        auto result=std::shared_ptr<UploadBytes>(new UploadBytes);
        result->size_=rowBytes*rows;
        // No value initialization: every byte is filled below before publication.
        result->raw_.reset(new unsigned char[result->size_]);
        if(!source)std::memset(result->raw_.get(),clearValue,result->size_);
        else if(pitch==rowBytes)std::memcpy(result->raw_.get(),source,result->size_);
        else for(std::size_t row=0;row<rows;++row)
            std::memcpy(result->raw_.get()+row*rowBytes,static_cast<const unsigned char*>(source)+row*pitch,rowBytes);
        return result;
    }
    static std::shared_ptr<const UploadBytes> Decoded(std::vector<unsigned char>&& bytes) {
        auto result=std::shared_ptr<UploadBytes>(new UploadBytes);
        result->size_=bytes.size();result->decoded_=std::move(bytes);return result;
    }
    const unsigned char* data()const{return raw_?raw_.get():decoded_.data();}
    std::size_t size()const{return size_;}
private:
    UploadBytes()=default;
    std::unique_ptr<unsigned char[]> raw_;
    std::vector<unsigned char> decoded_;
    std::size_t size_{};
};
}
