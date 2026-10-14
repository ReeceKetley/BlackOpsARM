"""Offline derived SM3 experiment; never modifies assets or engine sources."""
from pathlib import Path
import ctypes as c
import hashlib
import json
import re
import struct

OUT = Path(__file__).resolve().parent
ROOT = OUT.parents[3]
ASM = OUT.parent / 'native_post_shader_fusion1'
CAPTURE = ROOT / 'build-android-audit/native_registry_closure/sdk476-post-shaders3'
DLL = Path('C:/Windows/System32/d3dcompiler_47.dll')
dll = c.WinDLL(str(DLL))
assemble = dll.D3DAssemble
assemble.argtypes = [c.c_void_p, c.c_size_t, c.c_char_p, c.c_void_p,
                     c.c_void_p, c.c_uint, c.POINTER(c.c_void_p), c.POINTER(c.c_void_p)]
assemble.restype = c.c_long
disassemble = dll.D3DDisassemble
disassemble.argtypes = [c.c_void_p, c.c_size_t, c.c_uint, c.c_char_p, c.POINTER(c.c_void_p)]
disassemble.restype = c.c_long


def take_blob(blob):
    if not blob.value:
        return b''
    vtable = c.cast(blob, c.POINTER(c.POINTER(c.c_void_p))).contents
    ptr = c.WINFUNCTYPE(c.c_void_p, c.c_void_p)(vtable[3])
    size = c.WINFUNCTYPE(c.c_size_t, c.c_void_p)(vtable[4])
    release = c.WINFUNCTYPE(c.c_ulong, c.c_void_p)(vtable[2])
    result = c.string_at(ptr(blob), size(blob))
    release(blob)
    return result


def instructions(path):
    return [line.strip() for line in path.read_text(encoding='utf8').splitlines()
            if line.strip() and not line.lstrip().startswith('//')]


bloom = instructions(ASM / 'bloomApplyLDR-pass0.ps.bin.asm')
color = instructions(ASM / 'postFxColorMaterial-pass0.ps.bin.asm')
assert bloom[0] == color[0] == 'ps_3_0'
assert bloom[1] == 'def c0, 64, 8, 0, 0'
assert color[1] == 'def c0, 0.25, 0.5, 1, 0'
assert bloom[2:5] == ['dcl_texcoord v0.xy', 'dcl_2d s0', 'dcl_2d s1']
assert color[2:4] == ['dcl_texcoord v0.xy', 'dcl_2d s0']
assert color[4] == 'texld_pp r0, v0, s0'
assert not any(re.search(r'\bc(?:1|2|64|65|66)\b', row) for row in color[4:])
assert max(int(v) for v in re.findall(r'\br(\d+)\b', '\n'.join(bloom + color))) == 4


def bloom_remap(line):
    constants = {0: 1, 5: 64, 6: 65, 7: 66}
    line = re.sub(r'\bc(\d+)\b', lambda m: 'c' + str(constants.get(int(m[1]), int(m[1]))), line)
    return line.replace('oC0', 'r5')


bloom_ops = [bloom_remap(row) for row in bloom[5:]]
color_ops = ['mov_pp r0, r5'] + color[5:]
quant_ops = ['mov_sat r5, r5', 'mad r6, r5, c2.x, c2.y', 'frc r7, r6',
             'add r6, r6, -r7', 'mul r5, r6, c2.z']
assert len(bloom_ops) == 15 and len(color_ops) == 23

manifest = {
    'status': 'offline assembled candidate; no GPU execution or equivalence claim',
    'assembler_dll': str(DLL),
    'assembler_dll_sha256': hashlib.sha256(DLL.read_bytes()).hexdigest(),
    'prototype_source': 'https://github.com/wine-mirror/wine/blob/master/dlls/d3dcompiler_43/compiler.c',
    'constant_bindings': {'c0': 'original color def', 'c1': 'original bloom def',
                          'c2': 'UNORM8 quantization local def, quantized variant only',
                          'c64': 'bloom original c5 / canonical constant121 before overwrite',
                          'c65': 'bloom original c6 / canonical constant122 before overwrite',
                          'c66': 'bloom original c7 / canonical constant123 before overwrite',
                          'c5..11,c20..23': 'original color bindings unchanged'},
    'samplers': {'s0': 'original bloom input code image35', 's1': 'original scene code image34'},
    'input_uv': 'v0.xy, original common VS unchanged',
    'input_sources': {}, 'variants': {}
}
for name in ['bloomApplyLDR-pass0.ps.bin', 'postFxColorMaterial-pass0.ps.bin',
             'bloomApplyLDR-pass0.vs.bin', 'postFxColorMaterial-pass0.vs.bin']:
    capture = CAPTURE if '.ps.' in name else CAPTURE.with_name('sdk476-post-shaders4')
    data = (capture / name).read_bytes()
    manifest['input_sources'][name] = {'capture': str(capture.relative_to(ROOT)), 'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest()}

for variant in ['raw', 'unorm8_half_up']:
    quantized = variant != 'raw'
    header = ['ps_3_0', color[1], bloom_remap(bloom[1])]
    if quantized:
        header += ['def c2, 255, 0.5, 0.0039215686274509803921568627451, 0']
    header += bloom[2:5]
    ops = bloom_ops + (quant_ops if quantized else []) + color_ops
    source = '\n'.join(header + ops) + '\n'
    regs = [int(v) for v in re.findall(r'\br(\d+)\b', source)]
    constants = [int(v) for v in re.findall(r'\bc(\d+)\b', source)]
    assert max(regs) <= 31 and max(constants) <= 223
    assert sum(row.startswith('texld') for row in ops) == 2
    name = 'bloom_color_fused_' + variant
    (OUT / (name + '.asm')).write_text(source, encoding='utf8')
    data = source.encode('ascii'); buffer = c.create_string_buffer(data)
    shader = c.c_void_p(); errors = c.c_void_p()
    hr = assemble(buffer, len(data), (name + '.asm').encode(), None, None, 0,
                  c.byref(shader), c.byref(errors))
    diagnostics = take_blob(errors).rstrip(b'\0').decode('utf8', errors='replace')
    (OUT / (name + '.diagnostics.txt')).write_text(diagnostics or 'D3DAssemble HRESULT 0; no diagnostics.\n', encoding='utf8')
    binary = take_blob(shader)
    assert hr == 0 and binary, (variant, hex(hr & 0xffffffff), diagnostics)
    assert struct.unpack_from('<I', binary)[0] == 0xffff0300
    assert struct.unpack_from('<I', binary, len(binary) - 4)[0] == 0xffff
    (OUT / (name + '.ps.bin')).write_bytes(binary)
    buffer2 = c.create_string_buffer(binary); blob = c.c_void_p()
    hr2 = disassemble(buffer2, len(binary), 0, None, c.byref(blob))
    text = take_blob(blob).rstrip(b'\0').decode('utf8')
    assert hr2 == 0
    (OUT / (name + '.disassembled.asm')).write_text(text, encoding='utf8')
    actual_ops = instructions(OUT / (name + '.disassembled.asm'))
    expected = header + ops
    # Disassembler normalizes literals and redundant full write masks, but
    # instruction order, opcode modifiers, register references remain checked.
    def canon(row):
        row = row.replace('.xyzw', '')
        return re.sub(r'\s+', ' ', row).strip()
    actual_nondef = [canon(row) for row in actual_ops if not row.startswith('def ')]
    expected_nondef = [canon(row) for row in expected if not row.startswith('def ')]
    assert actual_nondef == expected_nondef, (variant, actual_nondef, expected_nondef)
    manifest['variants'][variant] = {
        'assemble_hresult': hr, 'disassemble_hresult': hr2,
        'bytes': len(binary), 'sha256': hashlib.sha256(binary).hexdigest(),
        'source_sha256': hashlib.sha256(data).hexdigest(),
        'texture_instructions': 2, 'executable_instruction_count': len(ops),
        'highest_temp_register': max(regs), 'highest_constant_register': max(constants),
        'instruction_order_and_modifiers_roundtrip': True,
        'quantization': 'none; diagnostic only' if not quantized else 'saturate RGBA, floor(255*x+0.5)/255; driver rounding and sRGB unproven'
    }
    print(variant, json.dumps(manifest['variants'][variant]))
(OUT / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf8')
