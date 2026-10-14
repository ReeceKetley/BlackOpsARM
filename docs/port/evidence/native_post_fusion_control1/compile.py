from pathlib import Path
import json,re,subprocess
root=Path.cwd();out=Path(__file__).parent;rows=json.loads((root/'build-android-audit/native_engine_entry1/compile_commands.json').read_text());row=next(r for r in rows if Path(r['file']).name=='rb_postfx.cpp')
results={}
for flag in [0,1]:
 cmd=row['command'];cmd=re.sub(r' -DBO1_NATIVE_POST_FUSION_TRIAL(?:=[^ ]+)?','',cmd);cmd=cmd.replace(row['file'],str(root/'src/platform/android/post_fusion_trial.cpp'))
 cmd=re.sub(r' -o .*? -c ',f' -DBO1_NATIVE_POST_FUSION_TRIAL={flag} -o "{(out/f"helper-{flag}.o").resolve().as_posix()}" -c ',cmd,count=1)
 (out/f'helper-{flag}-command.txt').write_text(cmd,encoding='utf8')
 with (out/f'helper-{flag}.log').open('wb') as f:results[str(flag)]=subprocess.run(cmd,cwd=row['directory'],stdout=f,stderr=subprocess.STDOUT).returncode
(out/'compile-results.json').write_text(json.dumps(results),encoding='utf8');print(results)
for flag in [0,1]: print((out/f'helper-{flag}.log').read_text(errors='replace')[-5000:])
