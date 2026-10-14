from pathlib import Path
import subprocess,json,re,shutil
root=Path.cwd();out=Path(__file__).parent;old=root/'docs/port/evidence/native_gpu_phase_profile1'
shutil.copyfile(old/'d3d9.h',out/'d3d9.h')
fixture=(old/'fixture.cpp').read_text()
fixture=fixture.replace('gpu::Mark(', 'fixtureMark(')
helper='''static void fixtureMark(int i) {
#if BO1_NATIVE_GPU_POST_PROFILE
 gpu::Mark(i >= 0 && i < 7 ? i + 100 : i);
#else
 gpu::Mark(i);
#endif
}
'''
fixture=fixture.replace('static void finish()',helper+'static void finish()',1)
test='''
 // Selection must ignore the other mode without consuming any slot or Issue.
 {Device d;gpu::Begin(&d);const auto before=d.counter;
  gpu::Mark(99);gpu::Mark(107);gpu::Mark(-2147483647);gpu::Mark(2147483647);
#if BO1_NATIVE_GPU_POST_PROFILE
  for(int i=0;i<7;++i)gpu::Mark(i);
#else
  for(int i=100;i<107;++i)gpu::Mark(i);
#endif
  assert(d.counter==before);
#if BO1_NATIVE_GPU_PASS_PROFILE
  assert(gpu::owner.sets[gpu::owner.open].markMask==0);
#endif
  for(int i=0;i<7;++i){fixtureMark(i);fixtureMark(i);
#if BO1_NATIVE_GPU_PASS_PROFILE
   auto &s=gpu::owner.sets[gpu::owner.open];
   assert(s.markMask==((1u<<(i+1))-1));
   assert(static_cast<Query*>(s.marks[i])->issues==1);
   assert(static_cast<Query*>(s.marks[i])->timestamp==before+static_cast<unsigned>(i)+1);
   assert(d.counter==before+static_cast<unsigned>(i)+1);
#else
   assert(d.counter==before);
#endif
  }finish();assert(lastStatus==GpuStatus::Valid);assert(!d.unissued);gpu::Release();}
'''
fixture=fixture.replace(' std::printf(',test+' std::printf(',1)
(out/'fixture.cpp').write_text(fixture,encoding='utf8')
rows=json.loads((root/'build-android-audit/native_engine_entry1/compile_commands.json').read_text());row=next(r for r in rows if Path(r['file']).name=='performance_gpu_overlay.cpp')
results={}
for flag,post in [(0,0),(0,1),(1,0),(1,1)]:
 label=f'provider-{flag}-post-{post}';cmd=row['command'];cmd=re.sub(r' -DBO1_NATIVE_GPU_(?:PASS|POST)_PROFILE(?:=[^ ]+)?','',cmd)
 cmd=re.sub(r' -o .*? -c ',f' -DBO1_NATIVE_GPU_PASS_PROFILE={flag} -DBO1_NATIVE_GPU_POST_PROFILE={post} -Wall -Wextra -Werror -o "{(out/(label+".o")).resolve().as_posix()}" -c ',cmd,count=1)
 (out/(label+'-command.txt')).write_text(cmd,encoding='utf8')
 with (out/(label+'.log')).open('wb') as f: native=subprocess.run(cmd,cwd=row['directory'],stdout=f,stderr=subprocess.STDOUT).returncode
 label=f'fixture-{flag}-post-{post}';exe=(out/(label+'.exe')).resolve()
 command=f'"C:/BuildTools/VC/Auxiliary/Build/vcvarsall.bat" x64 >nul && cl /nologo /W4 /WX /std:c++20 /EHsc /MTd /Od /DBO1_NATIVE_GPU_PASS_PROFILE={flag} /DBO1_NATIVE_GPU_POST_PROFILE={post} /I"{out.resolve()}" "{(out / "fixture.cpp").resolve()}" /Fo"{(out / (label+".obj")).resolve()}" /Fe"{exe}"'
 (out/(label+'-command.txt')).write_text(command,encoding='utf8')
 with (out/(label+'-compile.log')).open('wb') as f: compilecode=subprocess.run('cmd.exe /d /s /c "'+command+'"',stdout=f,stderr=subprocess.STDOUT).returncode
 result={'native_compile':native,'fixture_compile':compilecode}
 if compilecode==0:
  with (out/(label+'-run.log')).open('wb') as f: result['run']=subprocess.run([str(exe)],stdout=f,stderr=subprocess.STDOUT).returncode
  log=(out/(label+'-run.log')).read_text();expected='post' if post else 'coarse'
  result['log_profile_correct']=('BO1_NATIVE_GPU_PHASE' not in log if not flag else f'BO1_NATIVE_GPU_PHASE profile={expected} seq=' in log and f'profile={"coarse" if post else "post"}' not in log)
  result['missing_is_na']=not flag or bool(re.search(r'mask=0x01 .*p1_ms=n/a',log))
  result['invalid_order_na']=not flag or bool(re.search(r'phase_valid=0 p0_ms=n/a',log))
 results[label]=result
(out/'results.json').write_text(json.dumps(results,indent=2),encoding='utf8');print(json.dumps(results,indent=2))
assert all(r['native_compile']==0 and r['fixture_compile']==0 and r.get('run')==0 and r.get('log_profile_correct') and r.get('missing_is_na') and r.get('invalid_order_na') for r in results.values())
