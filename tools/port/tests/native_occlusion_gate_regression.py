"""Actual gate/Cmd/Update functions with real threads and synthetic collision work.

Uses canonical sound types; mocked collision/event/TLS services are explicit.
Tablet integration separately proves the actual engine worker and collision code.
"""
from pathlib import Path
import argparse,json,hashlib
root=Path(__file__).resolve().parents[3]
p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);a=p.parse_args()
source=root/'src/sound/snd_occlusion.cpp';snd=root/'src/sound/snd.cpp'
owners=root/'src/game_sp/g_sp_native_owners_checkpoint.inl'
def extract(s,key):
 b=s.index(key);e=s.index('{',b)+1;d=1
 while d:d+=(s[e]=='{')-(s[e]=='}');e+=1
 return s[b:e]
s=source.read_text()
functions='\n'.join(extract(s,k) for k in ['static int SND_OcclusionRunning()', 'static void SND_SetOcclusionRunning(', 'void __cdecl SND_LosOcclusionCmd()', 'void __cdecl SND_LosOcclusionUpdate()', 'bool SND_OcclusionCheckpointActive()', 'bool SND_BeginOcclusionCheckpoint()', 'bool SND_EndOcclusionCheckpoint()', 'bool SND_AbortOcclusionCheckpoint()'])
functions+='\n'+extract(owners.read_text(),'static bool SP_FreezeAudioCheckpoint()')
fixture=r'''
#include <sound/snd.h>
#include <sound/snd_dvar.h>
#include <sound/snd_occlusion.h>
#include <qcommon/common.h>
#include <qcommon/threads.h>
#include <win32/win_common.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <cstdio>
#include <cstdlib>
#include <csetjmp>
#define InterlockedExchange _InterlockedExchange
snd_local_t g_snd{};
static dvar_t setting{};
const dvar_t *snd_losOcclusion=&setting;
static std::mutex traceMutex,eventMutex;
static std::condition_variable event;
static bool signaled;
static std::atomic<bool> stopWorker,allowTrace,inTrace,workerEnabled{true};
static std::atomic<unsigned> traces,wakeups;
static std::thread::id mainId;
static unsigned checks;
static bool deviceStopped,deviceFailure;
static unsigned deviceBegins,deviceEnds;
bool SD_BeginCheckpoint(){++deviceBegins;if(deviceFailure)return false;deviceStopped=true;return true;}
bool SD_EndCheckpoint(){++deviceEnds;deviceStopped=false;return true;}
static void check(bool b){if(!b){fprintf(stderr,"failed check %u\n",checks);std::abort();}++checks;}
static std::atomic<bool> s_occlusionCheckpoint;
static std::atomic<unsigned> s_occlusionWorkerBatches;
bool Sys_IsMainThread(){return std::this_thread::get_id()==mainId;}
void Sys_EnterCriticalSection(CriticalSection c){if(c!=CRITSEC_SOUND_OCCLUSION)std::abort();traceMutex.lock();}
void Sys_LeaveCriticalSection(CriticalSection){traceMutex.unlock();}
bool Sys_TryEnterCriticalSection(CriticalSection){return traceMutex.try_lock();}
void Sys_SetOcclusion(){std::lock_guard lock(eventMutex);signaled=true;event.notify_one();}
void Com_Printf(int,const char*,...){}
bool Assert_MyHandler(const char*,int,int,const char*,...){std::abort();}
double SND_LosOcclusionTrace(bool,int *cache,const float*,const float*){
 if(s_occlusionCheckpoint.load())std::abort();inTrace=true;
 while(!allowTrace.load())std::this_thread::yield();
 *cache=42;++traces;return .375;
}
void SND_TraceProximity(float*,float*,float *distance,float *reflection){for(int i=0;i<4;++i){distance[i]=500.f;reflection[i]=.5f;}}
void SND_UpdateProximity(){}
FADER
FUNCTIONS
static void waitFor(auto predicate){auto end=std::chrono::steady_clock::now()+std::chrono::seconds(3);while(!predicate()){if(std::chrono::steady_clock::now()>end)std::abort();std::this_thread::yield();}}
int main(){
 mainId=std::this_thread::get_id();setting.current.enabled=true;SND_SetOcclusionRunning(1);
 for(auto &t:g_snd.occlusionTraces)t.id=-1;
 static snd_alias_t alias{};alias.flags=2;g_snd.voiceAliasHash[0]=17;g_snd.voice[0].alias=&alias;g_snd.voice[0].closestListenerIndex=0;g_snd.voice[0].firstPlaybackId=9;
 std::thread worker([]{while(!stopWorker){std::unique_lock lock(eventMutex);event.wait(lock,[]{return signaled||stopWorker;});if(stopWorker)break;signaled=false;lock.unlock();if(workerEnabled)SND_LosOcclusionCmd();++wakeups;}});
 SND_LosOcclusionUpdate();waitFor([]{return inTrace.load();});
 std::thread release([]{std::this_thread::sleep_for(std::chrono::milliseconds(20));allowTrace=true;});
 check(SND_BeginOcclusionCheckpoint());release.join();
 check(g_snd.traceSequence==1&&g_snd.lastTraceSequence==1&&traces==1&&g_snd.occlusionTraces[0].cache==42);
 auto count=traces.load();auto wakes=wakeups.load();Sys_SetOcclusion();waitFor([&]{return wakeups.load()>wakes;});check(traces==count);
 auto sequence=g_snd.traceSequence;SND_LosOcclusionUpdate();check(g_snd.traceSequence==sequence);check(!SND_BeginOcclusionCheckpoint());check(SND_EndOcclusionCheckpoint());check(!SND_EndOcclusionCheckpoint());
 wakes=wakeups.load();Sys_SetOcclusion();waitFor([&]{return wakeups.load()>wakes;});check(traces==count);
 for(unsigned i=0;i<64;++i){auto batches=s_occlusionWorkerBatches.load();for(int retry=0;retry<32;++retry){SND_LosOcclusionUpdate();check(SND_BeginOcclusionCheckpoint());if(g_snd.traceSequence==i+2)break;check(SND_EndOcclusionCheckpoint());}check(g_snd.traceSequence==g_snd.lastTraceSequence&&g_snd.traceSequence==i+2);check(s_occlusionWorkerBatches.load()>batches);check(g_snd.occlusionTraces[0].occlusion==.375f);check(SND_EndOcclusionCheckpoint());}
 // An intentionally disabled worker may retain one pending logical batch.
 check(SND_BeginOcclusionCheckpoint());workerEnabled=false;setting.current.enabled=false;++g_snd.traceSequence;check(SND_EndOcclusionCheckpoint());
 check(SND_BeginOcclusionCheckpoint());check(g_snd.traceSequence-g_snd.lastTraceSequence==1);setting.current.enabled=true;workerEnabled=true;check(SND_EndOcclusionCheckpoint());
 check(SND_BeginOcclusionCheckpoint());check(g_snd.traceSequence==g_snd.lastTraceSequence);
 // Error abort clears derived work before releasing a longjmp checkpoint.
 check(SND_AbortOcclusionCheckpoint());check(!SND_OcclusionCheckpointActive()&&g_snd.traceSequence==0&&g_snd.lastTraceSequence==0);check(!SND_AbortOcclusionCheckpoint());
 for(auto &t:g_snd.occlusionTraces)check(t.id==-1&&t.cache==0);
 std::thread wrong([]{if(SND_BeginOcclusionCheckpoint())std::abort();});wrong.join();check(true);
 std::jmp_buf jump;if(!setjmp(jump)){check(SND_BeginOcclusionCheckpoint());check(SND_AbortOcclusionCheckpoint());longjmp(jump,1);}check(!SND_OcclusionCheckpointActive());
 SND_LosOcclusionUpdate();check(SND_BeginOcclusionCheckpoint());check(g_snd.traceSequence==1&&g_snd.lastTraceSequence==1);check(SND_EndOcclusionCheckpoint());
 // Actual composite freeze rolls back its first owner when the second refuses.
 SND_SetOcclusionRunning(0);auto ends=deviceEnds;check(!SP_FreezeAudioCheckpoint());check(!deviceStopped&&deviceEnds==ends+1&&!SND_OcclusionCheckpointActive());SND_SetOcclusionRunning(1);
 deviceFailure=true;ends=deviceEnds;check(!SP_FreezeAudioCheckpoint());check(!deviceStopped&&deviceEnds==ends);deviceFailure=false;
 check(SP_FreezeAudioCheckpoint());check(deviceStopped&&SND_OcclusionCheckpointActive());auto begins=deviceBegins;check(!SP_FreezeAudioCheckpoint());check(deviceBegins==begins&&deviceStopped);
 check(SND_EndOcclusionCheckpoint());check(SD_EndCheckpoint());check(!deviceStopped);
 stopWorker=true;Sys_SetOcclusion();worker.join();
 check(traces==67&&s_occlusionWorkerBatches==67);
 printf("actual occlusion gate/Cmd/Update %u checks PASS, %u synthetic collision callbacks / %u completed worker batches, real threads\n",checks,traces.load(),s_occlusionWorkerBatches.load());
}
'''
a.out.mkdir(parents=True,exist_ok=False)
(a.out/'fixture.cpp').write_text(fixture.replace('FADER',extract(snd.read_text(),'void __cdecl SND_FaderSetGoal(')).replace('FUNCTIONS',functions),encoding='utf-8')
(a.out/'source-manifest.json').write_text(json.dumps([{'file':str(f),'sha256':hashlib.sha256(f.read_bytes()).hexdigest()} for f in [source,snd,owners]],indent=2),encoding='utf-8')
