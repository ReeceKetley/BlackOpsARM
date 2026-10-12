"""TASK32 adapter: strict actual entry allocation/free and FF diagnostic contracts.
Reuse TASK31 ABI/manifest/device/invocation collection without duplicating it.
"""
from pathlib import Path
base=Path(__file__).with_name('run_asset_entry_init_gate.py')
source=base.read_text(encoding='utf-8-sig')
a=source.index('def validate(');b=source.index('\ndef main():',a)
source=source[:a]+'''def strict_nested(actual,want):
 if type(actual) is not type(want):raise RuntimeError('Nested record type differs')
 if isinstance(want,dict):
  if set(actual)!=set(want):raise RuntimeError('Nested fields differ')
  for key in want:strict_nested(actual[key],want[key])
 elif actual!=want:raise RuntimeError('Nested record value differs')
def validate(text,pb):
 rows=[json.loads(x) for x in text.splitlines() if x.strip()]
 if len(rows)!=1:raise RuntimeError('Exactly one allocation summary required')
 exact_record(rows[0],{'schema':'BlackOpsARM.TASK32.entry-alloc.v1','pointer_bytes':pb,
  'header_entry_allocations':1600,'invalid_caller_guards':6,'high_count_after_release':0,
  'high_water_retained':1600,'free_entry_nodes':32752,'raw_free':1024,'table_free':576,
  'lifo_verified':True,'registry_publication_executed':False},'entry allocation summary')
 return rows
''' +source[b:]
source=source.replace('bo1_asset_entry_init_tests','bo1_entry_alloc_tests').replace('asset_entry_init.exe','entry_alloc.exe').replace('BlackOpsARM.TASK31.entry-init-gate.v1','BlackOpsARM.TASK32.entry-alloc-gate.v1').replace('Actual extracted DB_Init entry-list helper and real native recursive DBHASH; no full DB_Init/entry allocation/registration/asset loading','Actual header dispatch,entry allocation/free,FF diagnostic traversal and fatal partial progress; no registry publication or engine startup')
source=source.replace("ROOT/'tools/port/verify_asset_entry_init_source.py']", "ROOT/'tools/port/run_asset_entry_init_gate.py']")
source=source.replace("cp=run(target+'-normal',command)","cp=run(target+'-normal',command+['normal'])")
a=source.index("   bad=run(target+'-unsupported'");b=source.index("  # Negative collector controls",a)
source=source[:a]+'''   diagnostic=run(target+'-diagnostic',command+['diagnostic'])
   if diagnostic.stderr!="'name-one','zone-one'\\n'name-one','zone-one'\\n'name-two','zone-two'\\nname-one\\n":
    raise RuntimeError('Actual primary/override/nonmatching/callback diagnostic differs')
   diagnostic_rows=[json.loads(line) for line in diagnostic.stdout.splitlines() if line.strip()]
   if len(diagnostic_rows)!=1:raise RuntimeError('Diagnostic terminal missing/extra')
   strict_nested(diagnostic_rows[0],{'diagnostic':{'primary_override_nonmatching_inputs':True,'actual_ff_traversal':True,'typed_callback_executed':True,'logical_baseline_restored':True}})
   fatals={}
   for case,high,token in [('--empty-entry',0,'Could not allocate asset - increase XASSET_ENTRY_POOL_SIZE'),('--raw-limit',1026,"Exceeded limit of 1024 'rawfile' assets."),('--table-limit',578,"Exceeded limit of 576 'stringtable' assets."),('--loadobj-unavailable',None,'Explicit unsupported standalone LoadObj diagnostic')]:
    bad=run(target+'-'+case[2:],command+[case],(134,) if target=='honor_arm64' else(3,))
    if token not in bad.stderr:raise RuntimeError('Missing actual fatal diagnostic '+case)
    if high is None:
     if bad.stdout.strip():raise RuntimeError('Unsupported diagnostic claims success')
    else:
     values=[json.loads(line) for line in bad.stdout.splitlines() if line.strip()]
     want={'fatal_state':{'empty_head_injected':high==0,'entry_high_count':high,'lock_released':True,'atomic_rollback':False}}
     if len(values)!=1:raise RuntimeError('Fatal state missing/extra')
     strict_nested(values[0],want)
     expected_error=(token+'\\n') if high==0 else(token+\"\\n'name-one','zone-one'\\n'name-two','zone-two'\\n\"+token+'\\n\\n')
     expected_error+='Fixture fatal endpoint; production longjmp recovery not certified\\n'
     if bad.stderr!=expected_error:raise RuntimeError('Actual fatal traversal/order/error diagnostics differ')
    fatals[case]={'exit':bad.returncode,'high_count':high,'required_diagnostic':token}
   result['platforms'][target]={'rows':rows,'binary_sha256':digest(binary),'source_manifest':manifest,'diagnostic':diagnostic_rows,'fatal_controls':fatals}
''' +source[b:]
a=source.index('  # Negative collector controls');b=source.index("  result['malformed_records_rejected']",a)
source=source[:a]+'''  import copy
  good=result['platforms']['windows_x86']['rows'];candidates=[[],good+good]
  bad=copy.deepcopy(good);bad[0]['header_entry_allocations']=True;candidates.append(bad)
  bad=copy.deepcopy(good);bad[0]['raw_free']=1025;candidates.append(bad);rejected=0
  for candidate in candidates:
   try:validate('\\n'.join(json.dumps(x) for x in candidate),4)
   except (RuntimeError,ValueError):rejected+=1
   else:raise RuntimeError('Malformed collector control accepted')
''' +source[b:]
source=source.replace("ROOT/'tools/port/run_asset_entry_init_gate.py']", "ROOT/'tools/port/run_asset_entry_init_gate.py',ROOT/'tools/port/verify_entry_alloc_source.py']")
source=source.replace("  result['malformed_records_rejected']=rejected;", '''  for want in [{'value':True},{'value':False},{'value':0},{'value':1026}]:
   bad={'value':1 if want['value'] is True else0 if want['value'] is False elseFalse}
   try:strict_nested(bad,want)
   except RuntimeError:rejected+=1
   else:raise RuntimeError('Nested bool/integer corruption accepted')
  result['malformed_records_rejected']=rejected;'''.replace('else0','else 0').replace('elseFalse','else False'))
source=source.replace('PASS TASK31 canonical entry initialization and DBHASH','PASS TASK32 actual entry allocation/free and FF diagnostics')
exec(compile(source,str(base),'exec'),globals())
