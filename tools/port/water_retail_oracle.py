"""Run local retail BO1's water skinner in Unicorn; never redistribute executable bytes.

Install pefile and unicorn in an ignored dependency directory before running this.
The renderer's event/COM calls are stubbed; sampling, derivatives, normalization,
packing, loop bounds and source-bank selection execute unchanged retail code.
"""
import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'build/deps/water-oracle'))
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_FPCW, UC_X86_REG_MXCSR

parser = argparse.ArgumentParser()
parser.add_argument('--exe', type=Path, default=Path(r'D:\Games\Call of Duty - Black Ops\BlackOps.exe'))
parser.add_argument('--output', type=Path, default=ROOT / 'build/run/water_retail49.bin')
args = parser.parse_args()
raw = args.exe.read_bytes()
assert hashlib.sha256(raw).hexdigest() == '2ba4a04e81de2b60582da0e4489efde21e6cb07904a223bae633b6ccaebbf759', 'Unsupported retail executable'
pe = pefile.PE(data=raw)
assert pe.OPTIONAL_HEADER.ImageBase == 0x400000 and pe.FILE_HEADER.Machine == 0x14c
image = pe.get_memory_mapped_image()
# Fail closed on another executable/version; these are math/prologue instructions.
assert image[0x2de870:0x2de87c] == bytes.fromhex('558bec83e4f081ecc4020000')
assert image[0x1cc830:0x1cc83f] == bytes.fromhex('558bec83e4f8d94508d9fa8be55dc3')

COUNT = 256 * 256
BUFFER0, BUFFER1 = 0x10010000, 0x10110000
HEIGHT, SHORE, COLOR = 0x10210000, 0x10230000, 0x10240000
CMD, MESH, VB, VTABLE, STOP = 0x10001000, 0x10002000, 0x10003000, 0x10004000, 0x10000800
OUTPUT, STACK = 0x11000000, 0x20000000
VERTICES = 16 * (17 * 17 + 7) + 8

def channels(mode):
    banks = [bytearray(), bytearray()]
    heights, shores, colors = bytearray(), bytearray(), bytearray()
    for v in range(256):
        for u in range(256):
            for bank in range(2):
                if mode == 0:
                    value = (0., 0., 0., 0.)
                elif mode == 1:
                    # Binary fractions provide stable sloped derivatives and foam.
                    value = ((u % 23 - 11) / 16., (u % 19 + v % 17 - 18) / 8.,
                             (v % 29 - 14) / 16., (u % 7 + v % 11 + bank) / 16.)
                elif mode == 2:
                    value = ((u % 5 - 2) * 12., (v % 7 - 3) * 11.,
                             (u % 11 - 5) * 10., (u % 5 - 2) * 2.)
                elif mode == 3:
                    seed = (u * 1664525 + v * 1013904223 + bank * 374761393) & 0xffffffff
                    value = tuple((((seed >> shift) & 255) - 128) / 64. for shift in (0, 8, 16, 24))
                else:
                    # Interior derivatives exactly cancel the +4 grid basis.
                    value = (float(-(u % 64)), 0., float(-(v % 64)), 0.)
                banks[bank] += struct.pack('<4f', *value)
            heights += struct.pack('<h', (u % 13 - v % 11) * 25 if mode else 0)
            shores += struct.pack('<b', (u * 17 + v * 3) % 256 - 128)
            colors += struct.pack('<I', 0xa0000000 | (u << 16) | (v << 8) | ((u ^ v) & 255))
    return [bytes(x) for x in (*banks, heights, shores, colors)]

cases = [
    ('flat', 0, 25., 0, [(32, 48, 32, 48, 0)]),
    ('sloped', 1, 25., 1, [(32, 48, 32, 48, -3)]),
    ('clamped', 2, 25., 0, [(240, 240, -16, -16, 17)]),
    ('negative_seam', 3, 25., 1, [(0, -1, -272, -272, -4)]),
    ('positive_seam', 3, 7.5, 0, [(248, 248, 248, 248, 3)]),
    ('all_slots', 3, 25., 1, [(i * 16, (i * 31) - 16, i * 16, i * 32, i - 8) for i in range(16)]),
    ('zero_derivatives', 4, 25., 0, [(32, 32, 32, 32, 0)]),
]
args.output.parent.mkdir(parents=True, exist_ok=True)
manifest = {'exe_sha256': hashlib.sha256(raw).hexdigest(), 'skin_address': '0x6de870',
            'read_address': '0x6de6e0', 'cases': [], 'stubbed': ['device event', 'VB lock/unlock']}
with args.output.open('wb') as out:
    out.write(struct.pack('<8sII', b'BO1WATR1', len(cases), VERTICES))
    for name, mode, scale, bank, mesh_params in cases:
        uc = Uc(UC_ARCH_X86, UC_MODE_32)
        uc.mem_map(0x400000, (pe.OPTIONAL_HEADER.SizeOfImage + 4095) & ~4095)
        uc.mem_write(0x400000, image)
        uc.mem_map(0x10000000, 0x300000)
        uc.mem_map(OUTPUT, 0x20000)
        uc.mem_map(STACK, 0x10000)
        write32 = lambda address, value: uc.mem_write(address, struct.pack('<I', value))
        write32(0x462df10, BUFFER0)
        write32(0x462df10 + 0x40e0, BUFFER1)
        write32(0x46360d0, HEIGHT)
        write32(0x4636e10, SHORE)
        write32(0x46372b0, COLOR)
        uc.mem_write(0x3e5b614, struct.pack('<f', scale))
        write32(0x4638398, VB)
        write32(VB, VTABLE)
        write32(VTABLE + 0x2c, 0x10000600)
        write32(VTABLE + 0x30, 0x10000700)
        # lock: *ppData=OUTPUT; return S_OK; stdcall 5 arguments.
        uc.mem_write(0x10000600, bytes.fromhex('8b442410c700') + struct.pack('<I', OUTPUT) + bytes.fromhex('31c0c21400'))
        uc.mem_write(0x10000700, bytes.fromhex('31c0c20400'))
        uc.mem_write(0x4c9a10, bytes.fromhex('b801000000c3'))
        command = [0] * 21
        command[4] = bank
        meshes = []
        for i, (su, sv, wx, wy, wz) in enumerate(mesh_params):
            # Gaps and leading/trailing sentinels also check output pointer strides.
            fields = (su, sv, wx, wy, wz, 0, 4 + i * 296)
            meshes.append(fields)
            addr = MESH + i * 32
            command[5 + i] = addr
            uc.mem_write(addr, struct.pack('<7iI', *fields, 0))
        uc.mem_write(CMD, struct.pack('<21I', *command))
        inputs = channels(mode)
        for addr, value in zip((BUFFER0, BUFFER1, HEIGHT, SHORE, COLOR), inputs):
            uc.mem_write(addr, value)
        uc.mem_write(OUTPUT, b'\xcd' * (VERTICES * 16))
        sp = STACK + 0xf000
        uc.mem_write(sp, struct.pack('<II', STOP, CMD))
        uc.reg_write(UC_X86_REG_ESP, sp)
        uc.reg_write(UC_X86_REG_FPCW, 0x027f)
        uc.reg_write(UC_X86_REG_MXCSR, 0x1f80)
        uc.emu_start(0x6de870, STOP, timeout=30_000_000, count=50_000_000)
        assert uc.reg_read(UC_X86_REG_ESP) == sp + 4, 'Incomplete retail call'
        expected = bytes(uc.mem_read(OUTPUT, VERTICES * 16))
        out.write(struct.pack('<32sfII', name.encode(), scale, bank, len(meshes)))
        for fields in meshes:
            out.write(struct.pack('<7i', *fields))
        for value in inputs:
            out.write(value)
        out.write(expected)
        item = {'name': name, 'tiles': len(meshes), 'vertices': len(meshes) * 289,
                'output_sha256': hashlib.sha256(expected).hexdigest()}
        manifest['cases'].append(item)
        print(json.dumps(item), flush=True)
manifest['fixture_sha256'] = hashlib.sha256(args.output.read_bytes()).hexdigest()
args.output.with_suffix('.json').write_text(json.dumps(manifest, indent=2) + '\n')
