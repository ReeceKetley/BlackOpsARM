#include <cstdint>
#include <ctime>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
static std::uint64_t fixtureClock=1000000000ULL;
int fakeClock(int, timespec *v) { v->tv_sec=static_cast<time_t>(fixtureClock/1000000000ULL); v->tv_nsec=static_cast<long>(fixtureClock%1000000000ULL); return 0; }
#define CLOCK_MONOTONIC 1
#define clock_gettime fakeClock
#include "../../../..//src/platform/android/performance_gpu_overlay.cpp"
#undef clock_gettime
using namespace bo1::android::performance;
static unsigned epochValue=1,validCount=0; static GpuStatus lastStatus=GpuStatus::Unavailable;
namespace bo1::android::performance { unsigned Epoch(){return epochValue;} void GpuObservation(unsigned,GpuStatus s,double,std::uint64_t){lastStatus=s;if(s==GpuStatus::Valid)++validCount;} }
struct Device;
struct Query:IDirect3DQuery9 { Device *d; unsigned type; bool issued=false,forcePending=false; unsigned issues=0; UINT64 timestamp=0; Query(Device *a,unsigned b):d(a),type(b){} HRESULT Issue(unsigned) override; HRESULT GetData(void*,unsigned,unsigned) override; unsigned Release() override; };
struct Device:IDirect3DDevice9 {
 std::vector<Query*> q; int failCreate=-1,failIssue=-1,failRead=-1; unsigned creates=0,releases=0,gets=0,unissued=0; bool pending=false,disjoint=false; UINT64 counter=100,frequency=1000;
 HRESULT CreateQuery(unsigned type,IDirect3DQuery9 **out) override { unsigned n=creates++; if(static_cast<int>(n)==failCreate)return -1; auto *v=new Query(this,type);q.push_back(v);*out=v;return S_OK; }
 ~Device(){assert(releases==q.size());}
};
HRESULT Query::Issue(unsigned flag){if(d->failIssue==static_cast<int>(type))return -1;issued=true;++issues;if(type==D3DQUERYTYPE_TIMESTAMP&&flag==D3DISSUE_END)timestamp=++d->counter;return S_OK;}
HRESULT Query::GetData(void *p,unsigned,unsigned flags){assert(flags==0);++d->gets;if(!issued){++d->unissued;return -1;}if(d->failRead==static_cast<int>(type))return -1;if(d->pending||forcePending)return S_FALSE;if(type==D3DQUERYTYPE_TIMESTAMPDISJOINT)*static_cast<BOOL*>(p)=d->disjoint;else if(type==D3DQUERYTYPE_TIMESTAMPFREQ)*static_cast<UINT64*>(p)=d->frequency;else *static_cast<UINT64*>(p)=timestamp;return S_OK;}
unsigned Query::Release(){++d->releases;delete this;return 0;}
static void finish(){gpu::BeforePresent();gpu::Presented(true);fixtureClock+=1000000000ULL;}
int main(){
 constexpr unsigned perSet=4
#if BO1_NATIVE_GPU_PASS_PROFILE
 +7
#endif
 ;
 // Every partial allocation failure must release exactly those already acquired.
 for(unsigned i=0;i<8*perSet;++i){Device d;d.failCreate=static_cast<int>(i);gpu::Begin(&d);assert(lastStatus==GpuStatus::Unsupported);assert(d.releases==d.q.size());gpu::Release();}
 {Device d;gpu::Begin(&d);assert(d.creates==8*perSet);auto before=d.counter;gpu::Mark(-1);gpu::Mark(7);gpu::Mark(0);gpu::Mark(0);
#if BO1_NATIVE_GPU_PASS_PROFILE
 assert(d.counter==before+1);
#else
 assert(d.counter==before);
#endif
 finish();assert(lastStatus==GpuStatus::Valid);assert(!d.unissued);gpu::Release();assert(d.releases==8*perSet);}
 {Device d;gpu::Begin(&d);for(int i=0;i<7;++i)gpu::Mark(i);finish();assert(lastStatus==GpuStatus::Valid);gpu::Release();}
 {Device d;d.frequency=0;gpu::Begin(&d);finish();assert(lastStatus==GpuStatus::Disjoint);gpu::Release();}
 {Device d;gpu::Begin(&d);d.counter=0;finish();assert(lastStatus==GpuStatus::Disjoint);gpu::Release();}
#if BO1_NATIVE_GPU_PASS_PROFILE
 {Device d;gpu::Begin(&d);gpu::Mark(0);gpu::BeforePresent();auto &set=gpu::owner.sets[gpu::owner.closed];static_cast<Query*>(set.marks[0])->timestamp=0;gpu::Presented(true);assert(lastStatus==GpuStatus::Valid);gpu::Release();}
 {Device d;gpu::Begin(&d);finish();const auto last=gpu::owner.lastPhaseReport;fixtureClock=last+500000000ULL;gpu::Begin(&d);gpu::BeforePresent();gpu::Presented(true);assert(gpu::owner.lastPhaseReport==last);gpu::Release();fixtureClock+=1000000000ULL;}
#endif
 // All eight pending sets fill without reissue; ninth Begin has no open set.
 {Device d;d.pending=true;for(unsigned i=0;i<8;++i){gpu::Begin(&d);gpu::Mark(2);finish();}const auto issued=d.counter;gpu::Begin(&d);gpu::Mark(3);finish();assert(d.counter==issued);assert(lastStatus==GpuStatus::Pending);d.pending=false;gpu::Begin(&d);assert(lastStatus==GpuStatus::Pending);finish();assert(lastStatus==GpuStatus::Valid);assert(!d.unissued);gpu::Release();}
 {Device d;d.disjoint=true;gpu::Begin(&d);gpu::Mark(4);finish();assert(lastStatus==GpuStatus::Disjoint);gpu::Release();}
 {Device d;gpu::Begin(&d);const auto before=d.counter;++epochValue;gpu::Mark(0);assert(d.counter==before);gpu::BeforePresent();assert(d.releases==d.q.size());gpu::Begin(&d);finish();gpu::Release();}
 {Device d;gpu::Begin(&d);epochValue=0;gpu::Mark(0);gpu::BeforePresent();assert(d.releases==d.q.size());gpu::Begin(&d);epochValue=1;gpu::Release();}
#if BO1_NATIVE_GPU_PASS_PROFILE
 {Device d;gpu::Begin(&d);d.failIssue=D3DQUERYTYPE_TIMESTAMP;gpu::Mark(0);assert(lastStatus==GpuStatus::Failed);assert(d.releases==d.q.size());gpu::Release();}
 {Device d;gpu::Begin(&d);gpu::Mark(2);gpu::Mark(1);finish();assert(lastStatus==GpuStatus::Valid);gpu::Release();}
 {Device d;gpu::Begin(&d);gpu::Mark(0);gpu::BeforePresent();auto &set=gpu::owner.sets[gpu::owner.closed];auto *mark=static_cast<Query*>(set.marks[0]);const auto value=mark->timestamp;mark->forcePending=true;gpu::Presented(true);assert(lastStatus==GpuStatus::Pending);mark->forcePending=false;mark->timestamp=value;gpu::Presented(true);assert(lastStatus==GpuStatus::Valid);gpu::Release();}
#endif
 {Device d;gpu::Begin(&d);d.failRead=D3DQUERYTYPE_TIMESTAMP;finish();assert(lastStatus==GpuStatus::Failed);assert(d.releases==d.q.size());gpu::Release();}
 std::printf("production GPU owner fixture PASS flag=%d valid=%u\n",BO1_NATIVE_GPU_PASS_PROFILE,validCount);
}
