from pathlib import Path
import subprocess,json,re
root=Path.cwd();out=root/'docs/port/evidence/native_gpu_phase_profile1';results={}
for flag in [0,1]:
 label=f'fixture-{flag}';exe=out/(label+'.exe')
 command=f'"C:/BuildTools/VC/Auxiliary/Build/vcvarsall.bat" x64 >nul && cl /nologo /W4 /WX /std:c++20 /EHsc /MTd /Od /DBO1_NATIVE_GPU_PASS_PROFILE={flag} /I"{out}" "{out / "fixture.cpp"}" /Fo"{out / (label+".obj")}" /Fe"{exe}"'
 (out/(label+'-command.txt')).write_text(command,encoding='utf8')
 with (out/(label+'-compile.log')).open('wb') as f: code=subprocess.run('cmd.exe /d /s /c "'+command+'"',stdout=f,stderr=subprocess.STDOUT).returncode
 results[label]={'compile':code}
 if code==0:
  with (out/(label+'-run.log')).open('wb') as f:results[label]['run']=subprocess.run([str(exe)],stdout=f,stderr=subprocess.STDOUT).returncode
  log=(out/(label+'-run.log')).read_text()
  results[label]['disabled_no_phase_log']= flag!=0 or 'BO1_NATIVE_GPU_PHASE' not in log
  results[label]['missing_is_na']=flag==0 or bool(re.search(r'mask=0x01 .*p1_ms=n/a',log))
  results[label]['invalid_order_na']=flag==0 or bool(re.search(r'phase_valid=0 p0_ms=n/a',log))
(out/'fixture-results.json').write_text(json.dumps(results,indent=2),encoding='utf8');print(json.dumps(results));
