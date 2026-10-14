from pathlib import Path
import struct,hashlib,json
root=Path.cwd();out=Path(__file__).parent
sources={'BloomPS':'build-android-audit/native_registry_closure/sdk476-post-shaders4/bloomApplyLDR-pass0.ps.bin','ColorPS':'build-android-audit/native_registry_closure/sdk476-post-shaders4/postFxColorMaterial-pass0.ps.bin','SharedVS':'build-android-audit/native_registry_closure/sdk476-post-shaders4/bloomApplyLDR-pass0.vs.bin','FusedPS':'docs/port/evidence/native_post_fusion_prototype1/bloom_color_fused_unorm8_half_up.ps.bin'}
text='#pragma once\n#include <cstdint>\nnamespace bo1::android::post_fusion_trial::bytecode {\n';manifest={}
for name,path in sources.items():
 b=(root/path).read_bytes();words=struct.unpack('<'+'I'*(len(b)//4),b);text+='inline constexpr std::uint32_t '+name+'[] = {\n'
 for i in range(0,len(words),8):text+='    '+', '.join(f'0x{n:08x}u' for n in words[i:i+8])+',\n'
 text+='};\n';manifest[name]={'source':path,'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest()}
text+='}\n';p=root/'src/platform/android/post_fusion_trial_bytecode.h';p.write_bytes(text.encode('utf8'))
(out/'bytecode-manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf8')
