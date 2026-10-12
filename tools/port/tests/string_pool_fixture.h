#pragma once
#include <platform/atomic32.h>
#include <clientscript/cscr_stringlist.h>
#include <clientscript/cscr_memorytree.h>
#include <cstdio>
#include <cstdlib>
#include <vector>
namespace pool_fixture {
inline void require(bool ok,const char*why){if(!ok){std::fprintf(stderr,"FAIL actual pool %s\n",why);std::abort();}}
inline void baseline(scriptInstance_t inst){
    require(gScrMemTreeGlob[inst].totalAlloc==0&&gScrMemTreeGlob[inst].totalAllocBuckets==0,"actual MT live counters zero BEFORE shutdown/reinit");
    require(gScrStringDebugGlob[inst]&&bo1::atomic32::AtomicLoad32(&gScrStringDebugGlob[inst]->totalRefCount)==0,"debug reference count zero before shutdown");
    for(unsigned i=0;i<65536;++i)require(bo1::atomic32::AtomicLoad32(&gScrStringDebugGlob[inst]->refCount[i])==0,"all debug references zero before shutdown");
    require(!gScrStringDebugGlob[inst]->ignoreLeaks,"leak checking stays enabled");
    const unsigned capacity=inst==SCRIPTINSTANCE_SERVER?0x8000u:0x4000u;auto*hash=gScrStringGlob[inst].hashTable;
    for(unsigned i=1;i<capacity;++i)require((hash[i].status_next&0x30000u)==0,"all hash entries free before shutdown");
    std::vector<bool>seen(capacity);unsigned index=hash[0].status_next&0xffffu,previous=0,count=0;
    while(index){require(index<capacity&&!seen[index]&&hash[index].u.prev==previous,"actual hash free-list range/uniqueness/backlink");seen[index]=true;++count;previous=index;index=hash[index].status_next&0xffffu;}
    require(count==capacity-1&&hash[0].u.prev==previous,"all hash entries returned exactly once before reinit");

}
}
