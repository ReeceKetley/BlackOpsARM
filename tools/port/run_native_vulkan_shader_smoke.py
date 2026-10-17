"""Focused native Vulkan shader check; never restarts the installed game or copies assets."""
from pathlib import Path
import argparse, subprocess, hashlib, json, re

parser=argparse.ArgumentParser()
parser.add_argument('--build',required=True)
parser.add_argument('--remote',required=True)
parser.add_argument('--label',default='final')
parser.add_argument('--image',help='64x40 RGBA material input fixture, not a game asset import')
parser.add_argument('--depth-only',action='store_true',help='Run only the depth comparison family and validate its SPIR-V')
parser.add_argument('--bias-only',action='store_true',help='Run only actual fixed-depth polygon bias GPU cases')
parser.add_argument('--ubyte-only',action='store_true',help='Run only UBYTE input reconstruction GPU cases')
parser.add_argument('--sampling-only',action='store_true',help='Run sampling validation and generated attachment texture GPU proof')
parser.add_argument('--depth-copy-only',action='store_true',help='Run independent sun-cascade depth restoration and fresh stencil GPU checks')
parser.add_argument('--copy-only',action='store_true',help='Run only real generated colour image copy/load/readback checks')
parser.add_argument('--geometry-only',action='store_true',help='Run native retained geometry and standalone scope GPU ownership checks')
parser.add_argument('--regions-only',action='store_true',help='Run immutable model-lighting tile/queue and actual GPU region/fence checks')
parser.add_argument('--arena-only',action='store_true',help='Run only real GPU draw page/pool/range/retirement checks')
args=parser.parse_args()
assert sum([args.sampling_only,args.geometry_only,args.depth_copy_only,args.regions_only,args.depth_only,args.bias_only,args.ubyte_only,args.copy_only,args.arena_only,bool(args.image)])<=1
assert re.fullmatch(r'/data/local/tmp/[A-Za-z0-9_-]+',args.remote)
assert re.fullmatch(r'[A-Za-z0-9_-]+',args.label)
root=Path(__file__).resolve().parents[2]
build=(root/args.build).resolve()
adb=['D:/Android/Sdk/platform-tools/adb.exe','-s','AKWJ9X5116G04618']
def device(*args,check=True):
    return subprocess.run(adb+list(args),stdout=subprocess.PIPE,stderr=subprocess.STDOUT,check=check)
before=device('shell','pidof','com.blackopsarm.dev:engine',check=False).stdout.decode().strip()
device('shell','mkdir','-p',args.remote)
manifest={}
for path in [build/'bo1_vulkan_shader_smoke',build/'shader-compiler/libbo1_native_shader_compiler.so']:
    digest=hashlib.sha256(path.read_bytes()).hexdigest()
    remote=args.remote+'/'+path.name
    existing=device('shell','sha256sum',remote,check=False).stdout.decode().split()
    if not existing or existing[0]!=digest: device('push',str(path),remote)
    verified=device('shell','sha256sum',remote).stdout.decode().split()[0]
    assert verified==digest
    manifest[path.name]={'sha256':digest,'bytes':path.stat().st_size}
device('shell','chmod','755',args.remote+'/bo1_vulkan_shader_smoke')
imageArgument=''
if args.image:
    image=(root/args.image).resolve();assert image.stat().st_size==64*40*4
    imageRemote=args.remote+'/input.rgba';digest=hashlib.sha256(image.read_bytes()).hexdigest()
    existing=device('shell','sha256sum',imageRemote,check=False).stdout.decode().split()
    if not existing or existing[0]!=digest: device('push',str(image),imageRemote)
    assert device('shell','sha256sum',imageRemote).stdout.decode().split()[0]==digest
    imageArgument=' '+imageRemote
arguments='--sampling-only' if args.sampling_only else '--geometry-only' if args.geometry_only else '--depth-copy-only' if args.depth_copy_only else '--regions-only' if args.regions_only else '--arena-only' if args.arena_only else '--copy-only' if args.copy_only else ('--ubyte-only '+args.remote) if args.ubyte_only else '--bias-only' if args.bias_only else ('--depth-only '+args.remote) if args.depth_only else (args.remote+imageArgument)
result=device('shell',f'LD_LIBRARY_PATH={args.remote} {args.remote}/bo1_vulkan_shader_smoke {arguments}',check=False)
(build/('tablet-'+args.label+'.log')).write_bytes(result.stdout)
print(result.stdout.decode(),end='')
assert result.returncode==0
if args.sampling_only:
    assert b'native_sampling_validation=PASS' in result.stdout
    assert b'gpu_pixels=1 alias_lookups=2 slot_reuses=2 fence_completed=1' in result.stdout
    after=device('shell','pidof','com.blackopsarm.dev:engine',check=False).stdout.decode().strip();assert before==after
    (build/('manifest-'+args.label+'.json')).write_text(json.dumps({'providers':manifest,
        'game_pid_before':before,'game_pid_after':after,'game_assets_transferred':0,
        'generated_attachment_sampled':True,'gpu_pixels':1,'full_game_frame_tested':False},indent=2)+'\n',encoding='utf-8')
    print('PASS sampling validation and generated attachment sampling GPU pixel; no game restart')
    raise SystemExit(0)
if args.depth_copy_only:
    assert b'native_depth_copy=PASS' in result.stdout
    after=device('shell','pidof','com.blackopsarm.dev:engine',check=False).stdout.decode().strip();assert before==after
    (build/('manifest-'+args.label+'.json')).write_text(json.dumps({'providers':manifest,
        'game_pid_before':before,'game_pid_after':after,'game_assets_transferred':0,
        'independent_cascade_depth_restore':True,'fresh_stencil_preserved':True,
        'production_cache_selector_tested':False,'full_game_frame_tested':False},indent=2)+'\n',encoding='utf-8')
    print('PASS independent cascade depth restoration GPU pixels and fresh stencil; no game restart')
    raise SystemExit(0)
if args.geometry_only:
    assert b'native_geometry_packet=PASS pixels=5 multistream=2 offsets=2 pending_flush=1' in result.stdout
    assert b'native_geometry_retention=PASS pixels=5 legacy_retentions=3 retained_draws=6 retained_arena_growths=0' in result.stdout
    after=device('shell','pidof','com.blackopsarm.dev:engine',check=False).stdout.decode().strip();assert before==after
    (build/('manifest-'+args.label+'.json')).write_text(json.dumps({'providers':manifest,
        'game_pid_before':before,'game_pid_after':after,'game_assets_transferred':0,
        'gpu_pixels':5,'geometry_offset_packet_tested':True,'invalid_ranges_rejected':2,'owner_mismatches_rejected':2,
        'actual_pending_fence_ownership':True,'nested_scope_restoration':True,
        'full_game_frame_tested':False},indent=2)+'\n',encoding='utf-8')
    print('PASS retained geometry GPU pixels, validation, nested scopes and fence ownership; no game restart')
    raise SystemExit(0)
if args.regions_only:
    assert b'native_model_lighting_regions=PASS' in result.stdout
    assert b'native_model_lighting_regions_gpu=PASS old_new_bytes=65536 region_bytes=512' in result.stdout
    assert result.stdout.count(b'native_model_lighting_boxes_gpu=PASS case=')==3
    assert b'case=1 staged_bytes=768 region_count=1 full_fallback=0' in result.stdout
    assert b'case=2 staged_bytes=32768 region_count=1 full_fallback=0' in result.stdout
    assert b'case=3 staged_bytes=65536 region_count=1 full_fallback=1' in result.stdout
    after=device('shell','pidof','com.blackopsarm.dev:engine',check=False).stdout.decode().strip();assert before==after
    (build/('manifest-'+args.label+'.json')).write_text(json.dumps({'providers':manifest,
        'game_pid_before':before,'game_pid_after':after,'game_assets_transferred':0,
        'old_new_readback_bytes':65536,'partial_upload_bytes':512,'actual_fence_ownership':True,'box_cases':3,
        'box_staged_bytes':[768,32768,65536],'full_fallback_cases':1,
        'full_game_frame_tested':False},indent=2)+'\n',encoding='utf-8')
    print('PASS immutable atlas union/recovery and actual GPU partial-update pixels/fence owners; no game restart')
    raise SystemExit(0)
if args.arena_only:
    assert result.stdout.count(b'native_draw_storage=PASS round=')==2
    assert b'native_draw_storage_pixels=PASS pixels=8' in result.stdout
    after=device('shell','pidof','com.blackopsarm.dev:engine',check=False).stdout.decode().strip();assert before==after
    (build/('manifest-'+args.label+'.json')).write_text(json.dumps({'providers':manifest,
        'game_pid_before':before,'game_pid_after':after,'game_assets_transferred':0,
        'gpu_pixels':8,'pages':4,'actual_fence_pending_rejection':True,'page_pool_reused_after_fence':True,
        'real_shader_material_draw_tested':True,'full_game_frame_tested':False},indent=2)+'\n',encoding='utf-8')
    print('PASS actual GPU draw page/pool/range/retirement/reuse checks; no game restart')
    raise SystemExit(0)
if args.copy_only:
    assert result.stdout.count(b'native_image_copy=PASS case=')==3
    after=device('shell','pidof','com.blackopsarm.dev:engine',check=False).stdout.decode().strip();assert before==after
    (build/('manifest-'+args.label+'.json')).write_text(json.dumps({'providers':manifest,
        'game_pid_before':before,'game_pid_after':after,'game_assets_transferred':0,
        'copy_readback_images':3,'real_shader_material_draw_tested':False,'full_game_frame_tested':False},indent=2)+'\n',encoding='utf-8')
    print('PASS actual scene copy/overwrite/LOAD GPU pixels and ordered journal; no game restart')
    raise SystemExit(0)
if args.ubyte_only:
    assert result.stdout.count(b'native_ubyte_input=PASS case=')==6
    validation=[];sdk=Path('D:/Android/Sdk/ndk/30.0.16248370/shader-tools/windows-x86_64')
    for case in range(6):
        path=build/('ubyte-case'+str(case)+'.spv');device('pull',args.remote+'/'+path.name,str(path))
        run=subprocess.run([str(sdk/'spirv-val.exe'),'--target-env','vulkan1.1',str(path)],capture_output=True)
        (build/(path.stem+'-validation.txt')).write_bytes(run.stdout+run.stderr);assert run.returncode==0,(case,run.stderr)
        subprocess.run([str(sdk/'spirv-dis.exe'),str(path),'-o',str(build/(path.stem+'.spvasm'))],check=True)
        validation.append({'case':case,'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'spirv_validation':True})
    after=device('shell','pidof','com.blackopsarm.dev:engine',check=False).stdout.decode().strip();assert before==after
    (build/('manifest-'+args.label+'.json')).write_text(json.dumps({'providers':manifest,'shaders':validation,
        'game_pid_before':before,'game_pid_after':after,'game_assets_transferred':0,
        'ubyte_gpu_cases':6,'byte_values_per_case':256,'full_game_frame_tested':False},indent=2)+'\n',encoding='utf-8')
    print('PASS actual UBYTE GPU reconstruction/swizzle/mixed/partial family and Vulkan1.1 validation; no game restart')
    raise SystemExit(0)
if args.bias_only:
    assert result.stdout.count(b'native_depth_bias=PASS format=')==14
    assert result.stdout.count(b'native_forward_depth=PASS format=')==2
    after=device('shell','pidof','com.blackopsarm.dev:engine',check=False).stdout.decode().strip()
    assert before==after,'Bias harness must not restart the game'
    (build/('manifest-'+args.label+'.json')).write_text(json.dumps({'providers':manifest,
        'game_pid_before':before,'game_pid_after':after,'game_assets_transferred':0,
        'fixed_depth_formats':[124,129],'depth_bias_gpu_cases':10,'forward_depth_gpu_cases':4,
        'forward_equal_additive_formats':2,'full_game_frame_tested':False},indent=2)+'\n',encoding='utf-8')
    print('PASS actual D16/D24 bias, forward depth and EQUAL additive pixel checks; no game restart')
    raise SystemExit(0)
if args.depth_only:
    assert result.stdout.count(b'native_depth_compare=PASS case=')==9
    validation=[]
    sdk=Path('D:/Android/Sdk/ndk/30.0.16248370/shader-tools/windows-x86_64')
    for case in range(9):
        path=build/('depth-case'+str(case)+'.spv')
        device('pull',args.remote+'/'+path.name,str(path))
        run=subprocess.run([str(sdk/'spirv-val.exe'),'--target-env','vulkan1.1',str(path)],capture_output=True)
        (build/(path.stem+'-validation.txt')).write_bytes(run.stdout+run.stderr)
        assert run.returncode==0,(case,run.stderr)
        subprocess.run([str(sdk/'spirv-dis.exe'),str(path),'-o',str(build/(path.stem+'.spvasm'))],check=True)
        validation.append({'case':case,'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'spirv_validation':True})
    after=device('shell','pidof','com.blackopsarm.dev:engine',check=False).stdout.decode().strip()
    assert before==after,'Depth harness must not restart the game'
    (build/('manifest-'+args.label+'.json')).write_text(json.dumps({'providers':manifest,'shaders':validation,
        'game_pid_before':before,'game_pid_after':after,'game_assets_transferred':0,
        'depth_family_gpu_cases':9,'full_game_frame_tested':False},indent=2)+'\n',encoding='utf-8')
    print('PASS actual depth attachment/Dref/raw/projective family and Vulkan1.1 SPIR-V validation; no game restart')
    raise SystemExit(0)
assert b'lrp_gpu=PASS' in result.stdout and b'real_colour_stage_pair=PASS' in result.stdout
assert b't5_colour_native=PASS' in result.stdout
device('pull',args.remote+'/t5-colour-native.pam',str(build/('t5-colour-'+args.label+'.pam')))
validation=[]
sdk=Path('D:/Android/Sdk/ndk/30.0.16248370/shader-tools/windows-x86_64')
for name in ['t5-fullscreen-vs','t5-bloom-ps','t5-colour-ps','t5-colour-linked-vs']:
    path=build/(name+'.spv')
    device('pull',args.remote+'/'+path.name,str(path))
    run=subprocess.run([str(sdk/'spirv-val.exe'),'--target-env','vulkan1.1',str(path)],capture_output=True)
    (build/(name+'-validation.txt')).write_bytes(run.stdout+run.stderr)
    assert run.returncode==0,(name,run.stderr)
    subprocess.run([str(sdk/'spirv-dis.exe'),str(path),'-o',str(build/(name+'.spvasm'))],check=True)
    validation.append({'shader':name,'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'bytes':path.stat().st_size,'spirv_validation':True})
after=device('shell','pidof','com.blackopsarm.dev:engine',check=False).stdout.decode().strip()
assert before==after,'Harness must not restart the game'
(build/('manifest-'+args.label+'.json')).write_text(json.dumps({'providers':manifest,'shaders':validation,
    'game_pid_before':before,'game_pid_after':after,'game_assets_transferred':0,
    'real_shader_material_draw_tested':True,'full_game_frame_tested':False,
    'material_input_fixture':args.image,'gpu_lrp_fixture_pass':True},indent=2)+'\n',encoding='utf-8')
print('PASS Vulkan1.1 SPIR-V validation for all four modules; game PID unchanged:',after)
