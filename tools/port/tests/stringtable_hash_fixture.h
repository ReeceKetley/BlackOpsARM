#pragma once
#include <universal/com_hash.h>
#include <universal/com_stringtable_obj.h>
#if defined(_WIN32)
int __cdecl WindowsOriginalHash(const char*,int);
#endif
static int asciiHashOracle(const char*s,int len,bool*overflow=nullptr){
    if(!s)return 0;std::uint32_t bits=5381;
    for(int i=0;s[i]&&(!len||i<len);++i){auto c=static_cast<unsigned char>(s[i]);require(c<128,"defined ASCII fixture domain");if(c>='A'&&c<='Z')c+=32;
        const auto wide=std::uint64_t(bits)*33+c;if(overflow&&wide>UINT32_MAX)*overflow=true;bits=static_cast<std::uint32_t>(wide);}
    int result;std::memcpy(&result,&bits,4);return result;
}
static void hashFixtures(){
    unsigned vectors=0,overflows=0;const auto before=frontend_fixture::streamState();const auto baseline=frontend_fixture::counts();
    auto check=[&](const char*s,int len){bool overflow=false;const int expected=asciiHashOracle(s,len,&overflow);require(Com_HashString(s,len)==expected,"actual canonical hash matches independent unsigned ASCII oracle");
        require(StringTable_HashString(s)==asciiHashOracle(s,0),"actual unique StringTable wrapper retains unbounded hash semantics");
#if defined(_WIN32)
        require(WindowsOriginalHash(s,len)==expected&&WindowsOriginalHash(s,0)==StringTable_HashString(s),"original optimized MSVC production body signed result bits match canonical leaf");
#endif
        ++vectors;if(overflow)++overflows;
    };
    for(int len:{-1,0,1,INT32_MAX})check(nullptr,len);
    for(int len:{INT32_MIN,-1,0,1,INT32_MAX})check("",len);
    for(const char*s:{"a","abc","ABC","MiXeDCase","0123456789","ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"}){
        const auto size=static_cast<int>(std::strlen(s));for(int len:{INT32_MIN,-1,0,1,2,size,size+1,INT32_MAX})check(s,len);
    }
    for(unsigned c=1;c<128;++c){const char s[]={static_cast<char>(c),0};for(int len:{-1,0,1,2})check(s,len);}
    std::uint32_t seed=0x5a170b1u;std::array<char,257>text{};
    for(unsigned iteration=0;iteration<2048;++iteration){const auto size=static_cast<int>(iteration%257);text.fill(0);for(int i=0;i<size;++i){seed=1664525u*seed+1013904223u;text[std::size_t(i)]=static_cast<char>(1+(seed%127));}
        if(iteration%3==0)text[std::size_t(size/3)]=0;for(int len:{-1,0,1,size/2,size,size+1})check(text.data(),len);
    }
    std::array<char,4097>longString{};longString.fill('Z');longString.back()=0;for(int len:{-1,0,4096,INT32_MAX})check(longString.data(),len);
    require(vectors==12857&&overflows>1000&&before==frontend_fixture::streamState()&&baseline==frontend_fixture::counts(),"hash vectors exercise wrapping without loader/owner mutation");
    std::printf("{\"hash_vectors\":{\"vectors\":%u,\"overflow_vectors\":%u,\"original_windows_vectors\":%u,\"actual_wrapper_vectors\":%u,\"independent_ascii_oracle_matches\":true,\"no_mutation\":true}}\n",vectors,overflows,
#if defined(_WIN32)
        vectors,
#else
        0u,
#endif
        vectors);
}
static void executeCellHashes(const StringTablePayloadPlan&p,const StringTable*header,bool measurement){
    require(header->values&&header->columnCount>=0&&header->rowCount>=0,"live fully resolved first StringTable payload before hash");
    if(measurement)std::printf("{\"hash_measurements\":[");
    for(int i=0;i<p.cells.count;++i){const auto&cell=header->values[i];require(cell.string,"genuine hash input is a proven initialized string");
        for(const auto*c=reinterpret_cast<const unsigned char*>(cell.string);*c;++c)require(*c<128,"genuine first StringTable hash inputs are ASCII");
        const auto computed=StringTable_HashString(cell.string);require(computed==cell.hash&&computed==asciiHashOracle(cell.string,0),"every stored genuine signed hash equals actual wrapper/canonical engine leaf and oracle");
#if defined(_WIN32)
        require(WindowsOriginalHash(cell.string,0)==computed,"every original MSVC stored hash agrees");
#endif
        if(measurement)std::printf("%s{\"index\":%d,\"stored\":%d,\"computed\":%d}",i?",":"",i,cell.hash,computed);
    }if(measurement)std::printf("]}\n");
}
