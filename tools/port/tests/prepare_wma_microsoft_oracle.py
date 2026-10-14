"""Reconstruct the two validated canonical packet classes for the real SDK oracle.
Reads protected originals only; all decoder output goes to ignored audit scratch.
"""
import hashlib,pathlib,struct,subprocess,wave,zlib
out=pathlib.Path('build-android-audit/native_registry_closure');out.mkdir(parents=True,exist_ok=True)
raw=zlib.decompress(pathlib.Path('D:/Games/Call of Duty - Black Ops/zone/Common/code_post_gfx.ff').read_bytes()[12:])
def chunk(name,data):return name+struct.pack('<I',len(data))+data+(b'\0' if len(data)%2 else b'')
for channel,offset,digest in [(2,5292715,'0450a70e738a6dd402b94462adebfdb8179415b786c2d270a3b4a1e057ffb6b6'),(1,5326506,'cb4c00ad6449555adc0fa200dca9151b452a4fc7184d4f8d6d244d0122c57180')]:
 h=struct.unpack_from('<15I',raw,offset);assert h[0]==h[12]==h[14]==0xffffffff and h[1]==1 and h[8]==7 and h[4]==channel
 name=raw.index(b'\0',offset+60)+1;seek=raw[name:name+h[11]*4];payload=raw[name+h[11]*4:name+h[11]*4+h[13]]
 assert hashlib.sha256(payload).hexdigest()==digest and h[13]//h[11]==(2230 if channel==1 else 4096)
 fmt=struct.pack('<HHIIHHH',0x161,h[4],h[3],6000*h[4],h[13]//h[11],16,0)
 riff=b'XWMA'+chunk(b'fmt ',fmt)+chunk(b'dpds',seek)+chunk(b'data',payload)
 source=out/f'retail-wma-class{channel}.xwma';source.write_bytes(b'RIFF'+struct.pack('<I',len(riff))+riff)
 target=out/f'retail-wma-class{channel}-ms.wav'
 run=subprocess.run(['C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)/Utilities/bin/x86/xWMAEncode.exe',str(source.resolve()),str(target.resolve())],capture_output=True,text=True)
 (out/f'wma-ms-decode-class{channel}.log').write_text(run.stdout+run.stderr)
 if run.returncode:raise RuntimeError(f'Microsoft SDK decode failed {run.returncode}: {run.stdout} {run.stderr}')
 with wave.open(str(target)) as w:assert w.getnchannels()==channel and w.getframerate()==h[3] and w.getnframes()==h[2] and w.getsampwidth()==2
 print(f'PASS actual Microsoft xWMA oracle ch{channel} rate{h[3]} frames{h[2]} sourcePayloadSHA256{digest}')
