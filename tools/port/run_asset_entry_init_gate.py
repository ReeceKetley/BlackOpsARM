"""Strict three-ABI TASK31 entry-list/real DBHASH gate; no assets or registration."""
from pathlib import Path
import argparse,datetime,hashlib,json,re,subprocess,traceback
from run_rawfile_payload_gate import verify_binary_architecture,verify_source_manifest,exact_record
ROOT=Path(__file__).resolve().parents[2]
def digest(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
def validate(text,pb):
 rows=[json.loads(x) for x in text.splitlines() if x.strip()]
 if len(rows)!=8:raise RuntimeError('Expected exactly8 ordered records')
 exact_record(rows[0],{'locks':{'critical_section':74,'initialize_threads':8,'recursive_depth':3,'locked_updates':16000,'partial_release_excludes_worker':True,'script_sections_independent':True}},'locks')
 # Check nested field types too; bool must never substitute for an integer.
 expected=[]
 for i,mod in enumerate([False,True,False]*2):
  last=65535 if mod else 32767
  value={'entry_init':{'round':i//3,'mod':mod,'head_index':16,'last_index':last,'free_nodes':last-15,'stride':24 if pb==8 else 16,'reserved_sentinel_untouched':True,'payload_tails_unchanged':True,'hash_buckets_unchanged':True}}
  expected.append(value);exact_record(rows[i+1],value,'init'+str(i))
 expected_summary={'schema':'BlackOpsARM.TASK31.entry-init.v1','pointer_bytes':pb,'entry_bytes':24 if pb==8 else 16,'pool_bytes':65536*(24 if pb==8 else 16),'hash_bytes':65536,'initializations':6,'full_db_init_executed':False,'entry_allocation_executed':False,'registry_publication_executed':False}
 exact_record(rows[-1],expected_summary,'terminal')
 def deep(actual,want):
  if type(actual)!=type(want):raise RuntimeError('Nested record type differs')
  if isinstance(want,dict):
   if set(actual)!=set(want):raise RuntimeError('Nested fields differ')
   for k in want:deep(actual[k],want[k])
  elif actual!=want:raise RuntimeError('Nested record value differs')
 deep(rows[0],{'locks':{'critical_section':74,'initialize_threads':8,'recursive_depth':3,'locked_updates':16000,'partial_release_excludes_worker':True,'script_sections_independent':True}})
 for got,want in zip(rows[1:7],expected):deep(got,want)
 return rows

def main():
 parser=argparse.ArgumentParser();parser.add_argument('--host-build',type=Path,required=True);parser.add_argument('--android-build',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--serial',required=True);args=parser.parse_args()
 out=args.output.resolve()
 if not out.is_relative_to(ROOT) or out.exists() or not re.fullmatch(r'[a-zA-Z0-9_-]+',out.name):raise RuntimeError('Fresh safe worktree output required')
 out.mkdir(parents=True);calls=[];result={'schema':'BlackOpsARM.TASK31.entry-init-gate.v1','utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'platforms':{},'all_tests_completed':False,'scope':'Actual extracted DB_Init entry-list helper and real native recursive DBHASH; no full DB_Init/entry allocation/registration/asset loading','tools':{str(p.relative_to(ROOT)):digest(p) for p in [Path(__file__).resolve(),ROOT/'tools/port/run_rawfile_payload_gate.py',ROOT/'tools/port/verify_asset_entry_init_source.py']}}
 def run(label,argv,allowed=(0,)):
  from types import SimpleNamespace
  outcome='completed'
  try:
   cp=subprocess.run([str(x) for x in argv],cwd=out,capture_output=True,text=True,encoding='utf-8',errors='replace',timeout=45)
  except subprocess.TimeoutExpired as error:
   def readable(value):return value.decode('utf-8','replace') if isinstance(value,bytes) else(value or '')
   cp=SimpleNamespace(stdout=readable(error.stdout),stderr=readable(error.stderr)+'\nCollector timeout after45s',returncode=None);outcome='timeout'
  except OSError as error:
   cp=SimpleNamespace(stdout='',stderr=str(error),returncode=None);outcome='launch_error'
  (out/(label+'.stdout.log')).write_text(cp.stdout);(out/(label+'.stderr.log')).write_text(cp.stderr);calls.append({'label':label,'argv':[str(x) for x in argv],'exit':cp.returncode,'outcome':outcome})
  if cp.returncode not in allowed:raise RuntimeError(label+' exit '+str(cp.returncode)+' '+cp.stderr[-1000:])
  return cp
 adb=['D:/Android/Sdk/platform-tools/adb.exe','-s',args.serial]
 try:
  devices=run('devices',[adb[0],'devices']).stdout.splitlines()
  if args.serial+'\tdevice' not in devices:raise RuntimeError('Requested serial is not currently authorized')
  remote='/data/local/tmp/blackopsarm-task31-'+out.name;run('device-create',adb+['shell','mkdir','-p',remote]);result['device_metadata']=run('device-metadata',adb+['shell','getprop ro.product.model; getprop ro.build.version.sdk; getconf PAGESIZE']).stdout
  for target in ['windows_x86','windows_x64','honor_arm64']:
   pb=4 if target=='windows_x86' else 8
   binary=(args.android_build/'bo1_asset_entry_init_tests') if target=='honor_arm64' else(args.host_build/target.removeprefix('windows_')/'asset_entry_init.exe')
   verify_binary_architecture(binary,'arm64' if target=='honor_arm64' else target.removeprefix('windows_'))
   manifest=verify_source_manifest(binary.parent/'source-manifest.json')
   command=[str(binary.resolve())]
   if target=='honor_arm64':
    path=remote+'/entry-tests';run('probe-push',adb+['push',binary.resolve(),path]);run('probe-chmod',adb+['shell','chmod','700',path]);devhash=run('probe-sha',adb+['shell','sha256sum',path]).stdout.split()[0]
    if devhash!=digest(binary):raise RuntimeError('Device binary hash mismatch')
    command=adb+['shell',path]
   cp=run(target+'-normal',command)
   if cp.stderr.strip():raise RuntimeError('Normal stderr/UBSan diagnostics')
   rows=validate(cp.stdout,pb)
   bad=run(target+'-unsupported',command+['--unsupported-section'],(134,) if target=='honor_arm64' else(3,))
   if bad.stdout.strip() or 'unsupported critical section 75' not in bad.stderr:raise RuntimeError('Missing unsupported diagnostic or unexpected success output')
   result['platforms'][target]={'rows':rows,'binary_sha256':digest(binary),'source_manifest':manifest,'unsupported_exit':bad.returncode}
  # Negative collector controls: deleted/extra records, nested bool-as-int and bad sentinel.
  good=result['platforms']['windows_x86']['rows'];rejected=0
  import copy
  candidates=[good[:-1],good+[good[-1]]];bad=copy.deepcopy(good);bad[1]['entry_init']['head_index']=True;candidates.append(bad);bad=copy.deepcopy(good);bad[1]['entry_init']['last_index']=32768;candidates.append(bad)
  for candidate in candidates:
   try:validate('\n'.join(json.dumps(x) for x in candidate),4)
   except (RuntimeError,ValueError):rejected+=1
   else:raise RuntimeError('Malformed collector control accepted')
  result['malformed_records_rejected']=rejected;result['all_tests_completed']=True;result['status']='passed'
 except Exception:
  result['status']='failed';result['error']=traceback.format_exc();raise
 finally:
  (out/'results.json').write_text(json.dumps(result,indent=2));(out/'invocations.json').write_text(json.dumps(calls,indent=2))
 print('PASS TASK31 canonical entry initialization and DBHASH on three ABIs: '+str(out))
if __name__=='__main__':main()
