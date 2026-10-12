"""Expected nonzero refusal proof; never reports genuine registration success."""
import argparse,datetime,hashlib,json,subprocess
from pathlib import Path
from techset_oracle import reference
R=Path(__file__).resolve().parents[2]
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--host-build',type=Path,required=True);ap.add_argument('--android-build',type=Path,required=True);ap.add_argument('--prior-results',type=Path,required=True);ap.add_argument('--output',type=Path,required=True);args=ap.parse_args();out=args.output.resolve();out.mkdir(parents=True,exist_ok=False);result={'schema':'BlackOpsARM.TASK38.default-blocker.v1','genuine_registration_passed':False,'expected_refusal_verified':False,'targets':{},'invocations':[],'collector_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'prior_results_sha256':hashlib.sha256(args.prior_results.read_bytes()).hexdigest()}
 def save(): (out/'results.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
 def run(label,argv,expected=0):
  try:c=subprocess.run([str(a) for a in argv],cwd=R,capture_output=True,text=True,timeout=180)
  except (subprocess.TimeoutExpired,OSError) as error:
   result['invocations'].append({'label':label,'argv':[str(a) for a in argv],'exit':None,'error':str(error)});save();raise
  (out/(label+'.stdout.log')).write_text(c.stdout,encoding='utf-8');(out/(label+'.stderr.log')).write_text(c.stderr,encoding='utf-8');result['invocations'].append({'label':label,'argv':[str(a) for a in argv],'exit':c.returncode});save()
  if expected is not None and c.returncode!=expected:raise RuntimeError(label+' unexpected exit '+str(c.returncode))
  return c
 prior=json.loads(args.prior_results.read_text());ref=prior['oracles']['zombie_pentagon'];oracle=reference(ref);result['oracle']=oracle;adb=Path('D:/Android/Sdk/platform-tools/adb.exe');serial='AKWJ9X5116G04618';device='/data/local/tmp/blackopsarm-techset-blocker-'+datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%d-%H%M%S%f');result['device_directory']=device
 devices=run('adb-devices',[adb,'devices','-l']).stdout
 if serial+' ' not in devices:raise RuntimeError('Authorized HONOR absent')
 def shell(label,words,expected=0):return run(label,[adb,'-s',serial,'shell',*words],expected)
 shell('device-create',['mkdir','-p',device]);source=ref['original_path']
 for target,binary,width in [('windows_x86',args.host_build/'x86/techset.exe',4),('windows_x64',args.host_build/'x64/techset.exe',8),('honor_arm64',args.android_build/'bo1_techset_tests',8)]:
  native=target=='honor_arm64';prefix=out/(target+'.decoded');executable=binary.resolve();result['targets'][target]={'executable_sha256':hashlib.sha256(binary.read_bytes()).hexdigest()}
  if result['targets'][target]['executable_sha256']!=prior['platforms'][target]['binary_sha256']:raise RuntimeError('Prerequisite/current binary SHA differs')
  if native:
   run('device-push',[adb,'-s',serial,'push',executable,device+'/probe']);shell('device-chmod',['chmod','700',device+'/probe']);
   if shell('device-binary-sha',['sha256sum',device+'/probe']).stdout.split()[0]!=result['targets'][target]['executable_sha256']:raise RuntimeError('Pushed probe SHA differs')
   executable=device+'/probe';prefix=device+'/prefix.decoded';source=prior.get('reused_original_device_directory',prior['device_directory'])+'/zombie_pentagon.ff'
   if shell('retail-device-sha',['sha256sum',source]).stdout.split()[0]!=ref['raw_sha256']:raise RuntimeError('Retail device SHA changed')
  call=(lambda label,words,expected=0:shell(label,[executable,*words],expected)) if native else (lambda label,words,expected=0:run(label,[executable,*words],expected))
  decode=json.loads(call(target+'-decode',['--decode-techset',source,str(prefix)]).stdout)
  if decode!={'decoded_bytes':ref['decoded_bytes'],'decoded_adler32':ref['decoded_adler32'],'prefix_bytes':66226,'buffer_released':True}:raise RuntimeError('Native full decode differs')
  digest=shell('device-prefix-sha',['sha256sum',prefix]).stdout.split()[0] if native else hashlib.sha256(prefix.read_bytes()).hexdigest()
  if digest!=oracle['prefix_sha256']:raise RuntimeError('Independent/native prefix differs')
  c=call(target+'-blocked',['--techset',str(prefix)],None);expected_exit=134 if native else 3
  if c.returncode!=expected_exit:raise RuntimeError('Expected exact explicit abort exit differs: '+str(c.returncode))
  rows=[json.loads(line) for line in c.stdout.splitlines() if line.strip()];expected={'schema':'BlackOpsARM.TASK38.blocked.v1','pointer_bytes':width,'source_end':66226,'header_bytes':528 if width==4 else 1056,'null_techniques':130,'stub_name':True,'missing_dependency':'DB_CreateDefaultEntry/preloaded genuine TechniqueSet','registered_assets':1,'techset_registered':False,'root_pop_executed':False,'actual_deserialization_complete':True,'prior_queries':1594}
  def check_record(candidates):
   if len(candidates)!=1 or set(candidates[0])!=set(expected) or any(type(candidates[0][k])!=type(v) or candidates[0][k]!=v for k,v in expected.items()):raise RuntimeError('Exact blocked observer fields/types differ')
  check_record(rows);malformed=[[],rows+rows]
  for key,value in [('registered_assets',True),('root_pop_executed',0),('techset_registered',True),('prior_queries',1594.0)]:
   bad=dict(rows[0]);bad[key]=value;malformed.append([bad])
  for candidate in malformed:
   try:check_record(candidate)
   except RuntimeError:pass
   else:raise RuntimeError('Malformed refusal record accepted')
  result['targets'][target]['malformed_records_rejected']=len(malformed)
  diagnostic='Explicit unsupported standalone registration dependency: DB_CreateDefaultEntry'
  if not c.stderr.splitlines() or c.stderr.splitlines()[-1]!=diagnostic or c.stderr.count(diagnostic)!=1 or any(term in c.stderr for term in ['FAIL ','Assertion ','runtime error:']):raise RuntimeError('Missing terminal exact refusal or unexpected assertion/UBSan failure')
  result['targets'][target].update({'native_decode':decode,'native_prefix_sha256':digest,'expected_exit':expected_exit,'record':rows[0],'expected_refusal_verified':True});save()
 result['expected_refusal_verified']=True;save();print('Verified expected default dependency refusal on all3ABIs; genuine registration remains BLOCKED')
if __name__=='__main__':main()
