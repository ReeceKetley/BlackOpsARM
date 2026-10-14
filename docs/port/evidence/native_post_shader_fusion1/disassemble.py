from pathlib import Path
import ctypes as c,json,hashlib
root=Path.cwd();out=Path(__file__).parent;src=root/'build-android-audit/native_registry_closure/sdk476-post-shaders3'
dll=c.WinDLL('d3dcompiler_47.dll');fn=dll.D3DDisassemble
fn.argtypes=[c.c_void_p,c.c_size_t,c.c_uint,c.c_char_p,c.POINTER(c.c_void_p)];fn.restype=c.c_long
manifest={}
for name in ['bloomApplyLDR-pass0.ps.bin','postFxColorMaterial-pass0.ps.bin']:
 b=(src/name).read_bytes();buffer=c.create_string_buffer(b);blob=c.c_void_p()
 hr=fn(buffer,len(b),0,None,c.byref(blob));assert hr==0,(name,hr)
 vtbl=c.cast(blob,c.POINTER(c.POINTER(c.c_void_p))).contents
 getptr=c.WINFUNCTYPE(c.c_void_p,c.c_void_p)(vtbl[3]);getsize=c.WINFUNCTYPE(c.c_size_t,c.c_void_p)(vtbl[4]);release=c.WINFUNCTYPE(c.c_ulong,c.c_void_p)(vtbl[2])
 data=c.string_at(getptr(blob),getsize(blob));release(blob)
 text=data.rstrip(b'\x00').decode('utf8');(out/(name+'.asm')).write_text(text,encoding='utf8')
 manifest[name]={'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest(),'disassemble_hresult':hr,'assembly':name+'.asm'}
 print(name+'\n'+text)
(out/'manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf8')
