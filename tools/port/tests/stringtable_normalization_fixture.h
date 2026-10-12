#pragma once
#include <universal/com_stringtable_sort.h>
#include <platform/win32_compat/sort16.h>
#include <atomic>
#include <thread>
#include <cstdlib>
#include <numeric>

using bo1::stringtable::NormalizeIndices;
using bo1::stringtable::NormalizeResult;
static int normalizationCompare(std::int16_t a, std::int16_t b, const void* p) noexcept {
    const auto& t=*static_cast<const StringTable*>(p);
    if(t.values[a].hash!=t.values[b].hash)return t.values[a].hash<t.values[b].hash?-1:1;
    return a%t.columnCount-b%t.columnCount;
}
#if !defined(__ANDROID__)
static const StringTable* normalizationControl;
static int __cdecl normalizationCrtCompare(const void* a,const void* b){
    return normalizationCompare(*static_cast<const std::int16_t*>(a),*static_cast<const std::int16_t*>(b),normalizationControl);
}
#endif
static void normalizationFixtures(){
    const auto stream=frontend_fixture::streamState();const auto owners=frontend_fixture::counts();
    unsigned cases=0,sortedCases=0;std::uint32_t digest=5381;
    for(unsigned count:{0u,1u,7u,8u,9u,16u,31u,32u,127u,32768u})for(unsigned pattern=0;pattern<6;++pattern){
        std::vector<StringTableCell> cells(count);std::vector<std::int16_t> indices(count);
        for(unsigned i=0;i<count;++i){
            std::uint32_t bits=0;
            switch(pattern){case 0:bits=i;break;case 1:bits=count-i;break;case 2:bits=i%3;break;
            case 3:bits=42;break;case 4:bits=(i%2?0x80000000u:0x7fffffffu);break;
            default:bits=i*2654435761u;break;}
            std::memcpy(&cells[i].hash,&bits,4);indices[i]=static_cast<std::int16_t>(count-1-i);
        }
        StringTable t{"controlled-sort",count&&count%2==0?2:1,static_cast<int>(count/(count&&count%2==0?2:1)),cells.data(),indices.data()};
        std::vector<unsigned char> cellCopy(cells.size()*sizeof(cells[0]));
        if(!cells.empty())std::memcpy(cellCopy.data(),cells.data(),cellCopy.size());
        std::array<unsigned char,sizeof(t)> oldHeader;std::memcpy(oldHeader.data(),&t,sizeof(t));
#if !defined(__ANDROID__)
        auto expected=indices;normalizationControl=&t;
        if(count)std::qsort(expected.data(),expected.size(),sizeof(std::int16_t),normalizationCrtCompare);
        normalizationControl=nullptr;
#endif
        require(NormalizeIndices(&t,cells.size()*sizeof(cells[0]),indices.size()*sizeof(indices[0]))==NormalizeResult::Ok,"actual controlled normalization");
        require(!std::memcmp(&t,oldHeader.data(),sizeof(t))&&(cells.empty()||!std::memcmp(cells.data(),cellCopy.data(),cellCopy.size())),"normalization leaves header and values unchanged");
        for(unsigned i=1;i<count;++i)require(normalizationCompare(indices[i-1],indices[i],&t)<=0,"normalized signed hash and column order");
#if !defined(__ANDROID__)
        require(indices==expected,"exact original Windows CRT duplicate ordering");
#endif
        for(auto v:indices){digest=digest*33u+static_cast<unsigned>(v);}
        ++cases;++sortedCases;
    }
    StringTableCell cells[9]{};std::int16_t indices[9]{8,7,6,5,4,3,2,1,0};
    StringTable valid{"invalid-fixtures",1,9,cells,indices};
    auto test=[&](StringTable* t,std::size_t vb,std::size_t ib,NormalizeResult result){
        std::array<unsigned char,sizeof(valid)> copy;std::memcpy(copy.data(),&valid,sizeof(valid));
        std::array<unsigned char,sizeof(cells)> c;std::array<unsigned char,sizeof(indices)> ix;
        std::array<unsigned char,sizeof(valid)> target;
        if(result!=NormalizeResult::InvalidTable)std::memcpy(target.data(),t,sizeof(*t));
        std::memcpy(c.data(),cells,sizeof(cells));std::memcpy(ix.data(),indices,sizeof(indices));
        require(NormalizeIndices(t,vb,ib)==result,"controlled normalization failure status");
        require(!std::memcmp(copy.data(),&valid,sizeof(valid))&&!std::memcmp(c.data(),cells,sizeof(cells))&&!std::memcmp(ix.data(),indices,sizeof(indices)),"invalid normalization fails before any mutation");++cases;
        if(result!=NormalizeResult::InvalidTable)require(!std::memcmp(target.data(),t,sizeof(*t)),"actual passed malformed header remains unchanged");
    };
    test(nullptr,sizeof(cells),sizeof(indices),NormalizeResult::InvalidTable);
    test(reinterpret_cast<StringTable*>(reinterpret_cast<std::uintptr_t>(&valid)+1),sizeof(cells),sizeof(indices),NormalizeResult::InvalidTable);
    test(reinterpret_cast<StringTable*>(UINTPTR_MAX-(alignof(StringTable)-1)),sizeof(cells),sizeof(indices),NormalizeResult::InvalidTable);
    auto bad=valid;bad.columnCount=-1;test(&bad,sizeof(cells),sizeof(indices),NormalizeResult::NegativeDimension);
    bad=valid;bad.rowCount=-1;test(&bad,sizeof(cells),sizeof(indices),NormalizeResult::NegativeDimension);
    bad=valid;bad.columnCount=INT32_MAX;bad.rowCount=INT32_MAX;test(&bad,sizeof(cells),sizeof(indices),NormalizeResult::IndexDomain);
    bad=valid;bad.rowCount=32769;test(&bad,sizeof(cells),sizeof(indices),NormalizeResult::IndexDomain);
    test(&valid,sizeof(cells)-1,sizeof(indices),NormalizeResult::InvalidSpan);
    test(&valid,sizeof(cells),sizeof(indices)-1,NormalizeResult::InvalidSpan);
    bad=valid;bad.values=nullptr;test(&bad,sizeof(cells),sizeof(indices),NormalizeResult::InvalidSpan);
    bad=valid;bad.cellIndex=nullptr;test(&bad,sizeof(cells),sizeof(indices),NormalizeResult::InvalidSpan);
    bad=valid;bad.values=reinterpret_cast<StringTableCell*>(reinterpret_cast<std::uintptr_t>(cells)+1);test(&bad,sizeof(cells),sizeof(indices),NormalizeResult::InvalidAlignment);
    bad=valid;bad.cellIndex=reinterpret_cast<std::int16_t*>(reinterpret_cast<std::uintptr_t>(indices)+1);test(&bad,sizeof(cells),sizeof(indices),NormalizeResult::InvalidAlignment);
    bad=valid;bad.values=reinterpret_cast<StringTableCell*>(UINTPTR_MAX-(alignof(StringTableCell)-1));test(&bad,sizeof(cells),sizeof(indices),NormalizeResult::InvalidSpan);
    bad=valid;bad.cellIndex=reinterpret_cast<std::int16_t*>(UINTPTR_MAX-1);test(&bad,sizeof(cells),sizeof(indices),NormalizeResult::InvalidSpan);
    bad=valid;bad.cellIndex=reinterpret_cast<std::int16_t*>(cells);test(&bad,sizeof(cells),sizeof(indices),NormalizeResult::OverlappingSpan);
    bad=valid;bad.cellIndex=reinterpret_cast<std::int16_t*>(&bad);test(&bad,sizeof(cells),sizeof(indices),NormalizeResult::OverlappingSpan);
    bad=valid;bad.values=reinterpret_cast<StringTableCell*>(&bad);test(&bad,sizeof(cells),sizeof(indices),NormalizeResult::OverlappingSpan);
    for(auto value:{std::int16_t(-1),std::int16_t(9),std::int16_t(1)}){indices[8]=value;test(&valid,sizeof(cells),sizeof(indices),value==1?NormalizeResult::DuplicateIndex:NormalizeResult::IndexRange);}
    indices[8]=0;
    bad=valid;bad.columnCount=0;bad.rowCount=INT32_MAX;bad.values=nullptr;bad.cellIndex=nullptr;
    test(&bad,0,0,NormalizeResult::Ok);
    require(NormalizeIndices(&valid)==NormalizeResult::Ok,"actual production contract overload");++cases;
    int context=0;auto dummy=[](std::int16_t,std::int16_t,const void*)noexcept{return 0;};
    require(!bo1::compat::Sort16(indices,32769,dummy,&context)&&!bo1::compat::Sort16(indices,9,nullptr,&context)&&!bo1::compat::Sort16(indices,9,dummy,nullptr),"lowlevel invalid callback/domain fails");cases+=3;
    std::atomic<unsigned> ready{0},failures{0};std::atomic<bool> go{false};
    auto worker=[&](unsigned identity){std::vector<StringTableCell> c(4096);std::vector<std::int16_t> ix(4096);
        for(unsigned i=0;i<c.size();++i){c[i].hash=identity?static_cast<int>(4096-i):static_cast<int>(i);ix[i]=static_cast<std::int16_t>(4095-i);}
        StringTable t{"overlap-independent",1,4096,c.data(),ix.data()};
        std::array<unsigned char,sizeof(t)> savedHeader;std::memcpy(savedHeader.data(),&t,sizeof(t));
        std::vector<unsigned char> savedCells(c.size()*sizeof(c[0]));std::memcpy(savedCells.data(),c.data(),savedCells.size());
        ready.fetch_add(1,std::memory_order_release);
        while(!go.load(std::memory_order_acquire))std::this_thread::yield();
        for(unsigned pass=0;pass<8;++pass){std::reverse(ix.begin(),ix.end());if(NormalizeIndices(&t)!=NormalizeResult::Ok)failures.fetch_add(1);
            for(unsigned i=0;i<ix.size();++i)if(ix[i]!=static_cast<std::int16_t>(identity?4095-i:i))failures.fetch_add(1);
            if(std::memcmp(savedHeader.data(),&t,sizeof(t))||std::memcmp(savedCells.data(),c.data(),savedCells.size()))failures.fetch_add(1);}
    };
    std::thread a(worker,0),b(worker,1);while(ready.load(std::memory_order_acquire)!=2)std::this_thread::yield();go.store(true,std::memory_order_release);a.join();b.join();
    require(!failures.load()&&stream==frontend_fixture::streamState()&&owners==frontend_fixture::counts(),"normalizer explicit contexts and owner counters independent");
    std::printf("{\"normalization_fixtures\":{\"cases\":%u,\"sorted_cases\":%u,\"order_digest\":%u,\"concurrent_tables\":2,\"concurrent_passes\":16,\"no_global_context\":true,\"native_width_positions\":true,\"invalid_no_mutation\":true}}\n",cases,sortedCases,digest);
}
static void executeNormalization(const StringTablePayloadPlan&p,StringTable* t,XBlock(&blocks)[7],bool measurement){
    const auto stream=frontend_fixture::streamState();const auto owners=frontend_fixture::counts();
    std::array<unsigned char,sizeof(*t)> header;std::memcpy(header.data(),t,sizeof(*t));
    // Actual live zone children and checked extents; not fake registry execution.
    require(reinterpret_cast<unsigned char*>(t->cellIndex)==blocks[4].data+p.indices.nativeStart&&reinterpret_cast<unsigned char*>(t->values)==blocks[4].data+p.cells.nativeStart,"normalization spans belong to current live zone");
    std::vector<unsigned char> earlier(static_cast<std::size_t>(p.indices.nativeStart));std::memcpy(earlier.data(),blocks[4].data,earlier.size());
    require(NormalizeIndices(t,static_cast<std::size_t>(p.cells.nativeBytes),static_cast<std::size_t>(p.indices.nativeBytes))==NormalizeResult::Ok,"actual production normalization on genuine loaded indices");
    require(!std::memcmp(t,header.data(),header.size())&&!std::memcmp(earlier.data(),blocks[4].data,earlier.size())&&stream==frontend_fixture::streamState()&&owners==frontend_fixture::counts(),"only index words changed; header children stream and owner unchanged");
    if(measurement){std::printf("{\"normalization_measurements\":{\"cell_count\":%d,\"element_bytes\":2,\"only_indices_changed\":true,\"exact_policy\":true,\"sorted_indices\":[",p.cells.count);for(int i=0;i<p.cells.count;++i)std::printf("%s%d",i?",":"",static_cast<int>(t->cellIndex[i]));std::printf("]}}\n");}
}
