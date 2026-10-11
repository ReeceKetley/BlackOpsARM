"""Local retail reference for the complete wave update and cache flush.

Original update/cache/normalization instructions execute in Unicorn. No game
or executable data is redistributed. See water_retail_oracle.py for setup.
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
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EAX, UC_X86_REG_EDI, UC_X86_REG_FPCW, UC_X86_REG_MXCSR

parser = argparse.ArgumentParser()
parser.add_argument('--exe', type=Path, default=Path(r'D:\Games\Call of Duty - Black Ops\BlackOps.exe'))
parser.add_argument('--output', type=Path, default=ROOT / 'build/run/water_update50.bin')
args = parser.parse_args()
raw = args.exe.read_bytes()
assert hashlib.sha256(raw).hexdigest() == '2ba4a04e81de2b60582da0e4489efde21e6cb07904a223bae633b6ccaebbf759'
pe = pefile.PE(data=raw)
image = pe.get_memory_mapped_image()
COUNT = 65536
BANKS = (0x10010000, 0x10120000)
HEIGHT, FLOOR, SHORE = 0x10240000, 0x10270000, 0x10290000
CMD, DVAR, STOP, STACK = 0x10001000, 0x10002000, 0x10000800, 0x20000000
CHANNELS = (0x462df10, 0x4631ff0, 0x46360d0, 0x4636970, 0x4636e10)

def inputs(mode):
    banks = [bytearray(), bytearray()]
    height, floor, shore = bytearray(), bytearray(), bytearray()
    for v in range(256):
        for u in range(256):
            for bank in range(2):
                if mode == 0:
                    value = (0., 0., 0., 0.)
                elif mode == 1:
                    value = ((u % 13 - 6) / 8., (u % 17 + v % 11 - 13 + bank) / 16.,
                             (v % 19 - 9) / 8., (u % 3 + v % 5) / 16.)
                else:
                    seed = (u * 1664525 + v * 1013904223 + bank * 374761393) & 0xffffffff
                    value = tuple((((seed >> shift) & 255) - 128) / 32. for shift in (0, 8, 16))
                    value += (((seed >> 24) & 255) / 256.,)
                banks[bank] += struct.pack('<4f', *value)
            height += struct.pack('<h', (u % 13 - v % 11) * 25)
            floor += struct.pack('<b', (u + v) % 256 - 128)
            shore += struct.pack('<b', (u * 17 + v * 3) % 256 - 128)
    return [bytes(x) for x in (*banks, height, floor, shore)]

# Config is the production84byte layout. Vary one branch at a time, then combine.
def config(**changes):
    values = dict(framerate=20, waveDamping=20., waveSeedHeight=1., waveSeedRadius=2.,
        waveSeedDelay=5., gridScale=25., curlAmount=0., curlMax=100000., curlReduce=.75,
        minShoreHeight=0., foamAppear=.25, foamDisappear=.02, windAmount=0., windMax=2.,
        windX=1., windY=0., windZ=0., particleGravity=1., particleLimit=1., particleLength=1., particleWidth=1.)
    values.update(changes)
    return struct.pack('<i20f', *values.values())

cases = [('flat', 0, config(), 0, 0, 1),
         ('history', 1, config(), 32, 0, 1),
         ('curl', 2, config(curlAmount=.65, curlMax=.1, curlReduce=.45), 128, 0, 1),
         ('wind', 2, config(windAmount=.3, windX=.6, windZ=.8), 252, 0, 1),
         ('shore', 1, config(minShoreHeight=12.), 240, 0, 1),
         ('flatten', 2, config(curlAmount=.4, windAmount=.15), 0, 1, 1),
         ('combined', 2, config(curlAmount=.6, curlMax=.2, curlReduce=.55,
             windAmount=.2, windX=.6, windZ=.8, foamAppear=2., foamDisappear=.15), 248, 0, 3)]
args.output.parent.mkdir(parents=True, exist_ok=True)
manifest = {'exe_sha256': hashlib.sha256(raw).hexdigest(), 'update_address': '0x6daa30',
            'flush_address': '0x6e03c0', 'cases': []}
with args.output.open('wb') as out:
    out.write(struct.pack('<8sI', b'BO1UPDT1', len(cases)))
    for name, mode, cfg, start, flatten, steps in cases:
        uc = Uc(UC_ARCH_X86, UC_MODE_32)
        uc.mem_map(0x400000, (pe.OPTIONAL_HEADER.SizeOfImage + 4095) & ~4095)
        uc.mem_write(0x400000, image)
        uc.mem_map(0x10000000, 0x400000)
        uc.mem_map(STACK, 0x10000)
        write32 = lambda a, v: uc.mem_write(a, struct.pack('<I', v))
        for address, pointer, element, stride, meta in zip(CHANNELS,
                (*BANKS, HEIGHT, FLOOR, SHORE), (16,16,2,1,1), (0x1030,0x1030,0x220,0x120,0x120),
                (0x1020,0x1020,0x214,0x114,0x114)):
            write32(address, pointer)
            write32(address + 4, COUNT * element + 128)
            for i in range(4): write32(address + 16 + i * stride + meta, 0xffffffff)
        write32(0x3e4ddb8, DVAR)
        uc.mem_write(DVAR + 0x18, bytes([flatten]))
        uc.mem_write(0x3e5b600, cfg)
        write32(0x4638368, 37)
        write32(0x463836c, 213)
        values = inputs(mode)
        for address, value in zip((*BANKS, HEIGHT, FLOOR, SHORE), values): uc.mem_write(address, value)
        for step in range(steps):
            src = step & 1
            uc.mem_write(CMD, struct.pack('<3I', start, src, src ^ 1))
            sp = STACK + 0xf000
            uc.mem_write(sp, struct.pack('<I', STOP))
            uc.reg_write(UC_X86_REG_ESP, sp)
            uc.reg_write(UC_X86_REG_EAX, CMD)
            uc.reg_write(UC_X86_REG_FPCW, 0x027f)
            uc.reg_write(UC_X86_REG_MXCSR, 0x1f80)
            uc.emu_start(0x6daa30, STOP, timeout=30_000_000, count=50_000_000)
            assert uc.reg_read(UC_X86_REG_ESP) == sp + 4, 'Incomplete retail update'
            for channel in CHANNELS[:2]:
                uc.mem_write(sp, struct.pack('<I', STOP))
                uc.reg_write(UC_X86_REG_ESP, sp)
                uc.reg_write(UC_X86_REG_EDI, channel)
                uc.emu_start(0x6e03c0, STOP, timeout=30_000_000, count=1_000_000)
        expected = [bytes(uc.mem_read(address, COUNT * 16)) for address in BANKS]
        out.write(struct.pack('<32s', name.encode()) + cfg + struct.pack('<5I', start, 37, 213, flatten, steps))
        for value in (*values, *expected): out.write(value)
        item = {'name': name, 'steps': steps, 'updated_cells': steps * 4096,
                'output_sha256': [hashlib.sha256(value).hexdigest() for value in expected]}
        print(json.dumps(item), flush=True)
        manifest['cases'].append(item)
manifest['fixture_sha256'] = hashlib.sha256(args.output.read_bytes()).hexdigest()
args.output.with_suffix('.json').write_text(json.dumps(manifest, indent=2) + '\n')
