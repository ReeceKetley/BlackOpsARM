from pathlib import Path
p=Path('docs/port/evidence/native_gpu_phase_profile1/fixture.cpp');s=p.read_text(encoding='utf8');s=s.replace('UINT64 counter=100;','UINT64 counter=100,frequency=1000;');s=s.replace('=1000;else','=d->frequency;else');needle=' // All eight pending sets fill without reissue; ninth Begin has no open set.';s=s.replace(needle,''' {Device d;gpu::Begin(&d);for(int i=0;i<7;++i)gpu::Mark(i);finish();assert(lastStatus==GpuStatus::Valid);gpu::Release();}
 {Device d;d.frequency=0;gpu::Begin(&d);finish();assert(lastStatus==GpuStatus::Disjoint);gpu::Release();}
 {Device d;gpu::Begin(&d);d.counter=0;finish();assert(lastStatus==GpuStatus::Disjoint);gpu::Release();}
#if BO1_NATIVE_GPU_PASS_PROFILE
 {Device d;gpu::Begin(&d);gpu::Mark(0);gpu::BeforePresent();auto &set=gpu::owner.sets[gpu::owner.closed];static_cast<Query*>(set.marks[0])->timestamp=0;gpu::Presented(true);assert(lastStatus==GpuStatus::Valid);gpu::Release();}
 {Device d;gpu::Begin(&d);finish();const auto last=gpu::owner.lastPhaseReport;fixtureClock=last+500000000ULL;gpu::Begin(&d);gpu::BeforePresent();gpu::Presented(true);assert(gpu::owner.lastPhaseReport==last);gpu::Release();fixtureClock+=1000000000ULL;}
#endif
'''+needle);p.write_text(s,encoding='utf8')
