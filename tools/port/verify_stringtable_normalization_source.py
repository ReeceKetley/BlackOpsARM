"""Prove the sole production restoration and unchanged surrounding source."""
from pathlib import Path
import argparse,json,hashlib,subprocess,sys
sys.dont_write_bytecode=True
from verify_arm64_audit import sha
from verify_string_pool_source import functions
ROOT=Path(__file__).resolve().parents[2]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--baseline',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
 b=json.loads(a.baseline.read_text(encoding='utf-8'));parent=b['BlackOpsARM']['head'];out=a.output.resolve()
 if not out.is_relative_to(ROOT/'build-android-audit') or out.exists():raise RuntimeError('Use fresh ignored proof output')
 report={'schema':'BlackOpsARM.TASK-0025.source-preservation.v1','parent':parent,'files':{},'verifier_sha256':sha(Path(__file__))}
 def lf(data):return data.replace(b'\r\n',b'\n')
 def original(n):
  raw=subprocess.check_output(['git','--no-optional-locks','show',parent+':'+n],cwd=ROOT)
  return lf(raw)
 def restored(n,edits):
  data=(ROOT/n).read_bytes()
  for new,old in edits:
   candidates=[(new,old),(new.replace(b'\n',b'\r\n'),old.replace(b'\n',b'\r\n'))]
   candidates=list(dict.fromkeys(candidates));matches=[pair for pair in candidates if data.count(pair[0])==1]
   assert len(matches)==1,(n,new);x,y=matches[0];data=data.replace(x,y,1)
  assert hashlib.sha256(data).hexdigest().upper()==b['BlackOpsARM']['files'][n],n
  assert lf(data)==original(n),n
  report['files'][n]={'sha256':sha(ROOT/n),'whole_source_restored_exact_after_documented_edits':True}
 restored('src/database/db_registry.cpp',[
  (b'#include <universal/com_stringtable_sort.h>\n',b''),
  (b'    const auto normalized = bo1::stringtable::NormalizeIndices(stringTable->stringTable);\n    if (normalized != bo1::stringtable::NormalizeResult::Ok)\n        Com_Error(ERR_DROP, "Invalid registered StringTable indices (%s)", bo1::stringtable::NormalizeResultName(normalized));\n',b'')])
 restored('cmake_files.cmake',[(b'\n\t"${SRC_DIR}/universal/com_stringtable_sort.cpp"\n\t"${SRC_DIR}/platform/win32_compat/sort16.cpp"',b'')])
 n='android/cmake/arm64-audit/CMakeLists.txt';data=lf((ROOT/n).read_bytes());start=data.index(b'        add_executable(bo1_stringtable_normalization_tests ');end=data.index(b'        add_custom_command(',start)
 assert data[:start]+data[end:]==original(n);report['files'][n]={'only_new_target_added':True,'sha256':sha(ROOT/n)}
 restored('tools/port/tests/stringtable_cells_fixture.h',[(b'\n#if defined(BO1_STRINGTABLE_NORMALIZATION_GATE)\n#include "stringtable_normalization_fixture.h"\n#endif',b''),(b'#if defined(BO1_STRINGTABLE_NORMALIZATION_GATE)\n    executeNormalization(p,header,blocks,measurement);\n#endif\n',b'')])
 restored('tools/port/tests/raw_strings_tests.cpp',[(b'\n#if defined(BO1_STRINGTABLE_NORMALIZATION_GATE)\n    normalizationFixtures();\n#endif',b'')])
 for n in ['src/database/db_expand.cpp','src/database/db_stream_load.cpp','src/database/db_load.cpp','src/universal/com_stringtable.cpp','src/universal/com_stringtable_obj.cpp','src/universal/q_shared.cpp','src/universal/com_hash.cpp','src/universal/com_stringtable.h']:
  assert sha(ROOT/n)==b['BlackOpsARM']['files'][n],n
  report['files'][n]={'whole_working_source_byte_unchanged':True,'sha256':sha(ROOT/n)}
 for n,count in [('src/database/db_expand.cpp',163),('src/database/db_stream_load.cpp',6)]:
  fs=functions((ROOT/n).read_bytes());assert len(fs)==count,(n,len(fs));report['files'][n]['unchanged_function_bodies']=count
 report.update(status='passed',registry_execution_certified=False,only_production_caller_change='normalize returned registered StringTable header after DB_AddXAsset',legacy_csv_query_hash_and_all_loader_bodies_unchanged=True)
 out.parent.mkdir(parents=True,exist_ok=True);out.write_bytes((json.dumps(report,indent=2)+'\n').encode());print('PASS source preservation: sole registration restoration; CSV/query/hash/163expander/6stream bodies unchanged')
if __name__=='__main__':main()
