"""TASK37 production trees unchanged; prerequisite return/target additive only."""
from pathlib import Path
import argparse,hashlib,json,subprocess
ROOT=Path(__file__).resolve().parents[2];PARENT='96f37fc'
def old(p):return subprocess.check_output(['git','show',PARENT+':'+p],cwd=ROOT).replace(b'\r\n',b'\n').decode('utf-8-sig')
def prove():
 subprocess.run(['git','diff','--exit-code',PARENT,'--','src','tl','DemonWare'],cwd=ROOT,check=True,capture_output=True)
 if subprocess.check_output(['git','ls-files','--others','--exclude-standard','--','src','tl','DemonWare'],cwd=ROOT).strip():raise RuntimeError('Unexpected new production source')
 p='tools/port/tests/raw_strings_tests.cpp';s=old(p);a=s.rindex('\n}');expected=s[:a]+'\n    return 0;'+s[a:]
 if (ROOT/p).read_text(encoding='utf-8-sig')!=expected:raise RuntimeError('Unexpected prerequisite fixture change')
 p='android/cmake/arm64-audit/CMakeLists.txt'
 if not (ROOT/p).read_text(encoding='utf-8-sig').startswith(old(p)):raise RuntimeError('Existing target changed')
 return {'schema':'BlackOpsARM.TASK37.source.v1','status':'passed','parent':PARENT,'production_trees_unchanged':['src','tl','DemonWare'],'explicit_prerequisite_success_return':True,'existing_cmake_targets_unchanged':True,'new_integration_source_sha256':hashlib.sha256((ROOT/'tools/port/tests/code_zone_tests.cpp').read_bytes()).hexdigest(),'finite_reader_not_async_engine_loader':True}
if __name__=='__main__':
 ap=argparse.ArgumentParser();ap.add_argument('--output',required=True);args=ap.parse_args();p=Path(args.output);p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(prove(),indent=2),encoding='utf-8');print('TASK37 source proof passed')
