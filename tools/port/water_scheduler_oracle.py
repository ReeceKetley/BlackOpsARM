"""Local retail water scheduling reference; no retail bytes are redistributed.

Config/view/model/worker calls are intercepted at their original entry points.
The scheduler's timing, row selection, expiry/free lists, mesh FIFO, metadata,
allocation and queue payload instructions execute unchanged in Unicorn.
Debug drawing is disabled. This does not validate TrackView or GPU submission.
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
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EAX, UC_X86_REG_EIP

parser = argparse.ArgumentParser()
parser.add_argument('--exe', type=Path, default=Path(r'D:\Games\Call of Duty - Black Ops\BlackOps.exe'))
parser.add_argument('--output', type=Path, default=ROOT / 'build/run/water_scheduler52.json')
args = parser.parse_args()
raw = args.exe.read_bytes()
sha = hashlib.sha256(raw).hexdigest()
if sha != '2ba4a04e81de2b60582da0e4489efde21e6cb07904a223bae633b6ccaebbf759':
    raise RuntimeError('Unsupported retail executable')
pe = pefile.PE(data=raw)
image = pe.get_memory_mapped_image()
if image[0x2dd640:0x2dd64c] != bytes.fromhex('558bec83e4f081ecb4000000'):
    raise RuntimeError('Unexpected scheduler prologue')
FRAME, ENABLED, DELTA, LOCKED = 0x4638350, 0x4638354, 0x4638358, 0x463839c
TILES, PREV, EXPIRED, FREECOUNT, FREE = 0x46383a0, 0x3e4ddc0, 0x462be30, 0x3b400fc, 0x3e4d138
STOP, DVAR, EYE, VERTICES, STACK = 0x10000800, 0x10001000, 0x10002000, 0x11000000, 0x20000000

cases = [
    dict(name='disabled', enabled=False), dict(name='no_cells', cells=0),
    dict(name='unallocated', allocated=False), dict(name='negative_time', time=-1),
    dict(name='fractional_time', delta=16, time=1),
    dict(name='all_rows', frame=15, time=18),
    dict(name='bank_swap', frame=31, time=18, previous=240),
    dict(name='sparse_previous_seam', frame=15, time=18, sparse=True, previous=240),
    dict(name='wrapped_mesh_fifo', frame=15, time=18, rings=True, free_count=288),
    dict(name='insufficient_blocks', frame=15, time=18, free_count=15),
    dict(name='partial_allocation', frame=15, time=18, free_count=20),
    dict(name='draw_disabled', frame=32, time=36, draw=False),
    dict(name='expiry_wrap', frame=15, time=18, free_count=15, expiry=True),
    dict(name='repeated_steps', frame=15, time=54),
]
manifest = dict(exe_sha256=sha, scheduler_address='0x6dd640',
    intercepted_calls={'config': '0x6dd120', 'track_view': '0x6dc640',
                       'render_model': '0x6dd280', 'worker_enqueue': '0x5317a0'}, cases=[])

for changes in cases:
    inp = dict(enabled=True, cells=256, allocated=True, frame=0, delta=0, time=0,
               previous=0, draw=True, sparse=False, rings=False, free_count=800, expiry=False)
    inp.update(changes)
    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    uc.mem_map(0x400000, (pe.OPTIONAL_HEADER.SizeOfImage + 4095) & ~4095)
    uc.mem_write(0x400000, image)
    uc.mem_map(0x10000000, 0x10000)
    uc.mem_map(STACK, 0x10000)
    write32 = lambda addr, val: uc.mem_write(addr, struct.pack('<I', val & 0xffffffff))
    read32 = lambda addr: struct.unpack('<I', uc.mem_read(addr, 4))[0]
    write32(0x3b400dc, DVAR)
    uc.mem_write(DVAR + 0x18, bytes([inp['enabled']]))
    write32(0x3b1fe6c, DVAR + 0x100)
    uc.mem_write(DVAR + 0x118, bytes([inp['draw']]))
    write32(0x3b400bc, DVAR + 0x200)
    uc.mem_write(DVAR + 0x218, b'\0')
    write32(0x243fb88, inp['cells'])
    uc.mem_write(0x460c0b6, bytes([inp['allocated']]))
    write32(FRAME, inp['frame']); write32(DELTA, inp['delta'])
    uc.mem_write(ENABLED, b'\0'); write32(LOCKED, VERTICES)
    uc.mem_write(PREV, bytes(84)); write32(PREV, inp['previous']); write32(PREV + 8, 1)
    for t in range(256):
        base = TILES + t * 188
        uc.mem_write(base, bytes(188))
        uc.mem_write(base, struct.pack('<5i', t % 16 * 16, t // 16 * 16,
                     t * 3 - 200, t * 5 + 10, t % 9 - 4))
        recent = inp['frame'] if inp['sparse'] and t // 16 != 2 else 0
        write32(base + 48, recent)
        write32(base + 52, 3 if inp['rings'] else 0)
        write32(base + 56, 1 if inp['rings'] else 0)
        for slot in range(4):
            mesh = base + 60 + slot * 32
            old = (t * 2 + (1 if slot == 0 else 0)) % 512
            uc.mem_write(mesh, struct.pack('<6i2I', -10, -20, -30, -40, -50, -1,
                                          old * 289, VERTICES + old * 289 * 16))
    write32(FREECOUNT, inp['free_count'])
    for n in range(inp['free_count']):
        write32(FREE + n * 4, (n + (512 if inp['rings'] else 0)) * 289)
    uc.mem_write(EXPIRED, bytes(8 + 1024 * 8))
    if inp['expiry']:
        write32(EXPIRED, 1022); write32(EXPIRED + 4, 1)
        for pos, frames, vertex in [(1022, 1, 600 * 289), (1023, 0, 601 * 289), (0, 2, 602 * 289)]:
            write32(EXPIRED + 8 + pos * 8, frames); write32(EXPIRED + 12 + pos * 8, vertex)
    calls, commands = [], []

    def pointer_id(pointer):
        offset = pointer - TILES
        tile, within = divmod(offset, 188)
        if not (0 <= tile < 256 and within >= 60 and (within - 60) % 32 == 0 and within < 188):
            raise RuntimeError(f'Unexpected retail mesh pointer {pointer:#x}')
        return [tile, (within - 60) // 32]

    def intercept(machine, address, size, userdata):
        names = {0x6dd120: 'config', 0x6dc640: 'track_view', 0x6dd280: 'render_model', 0x5317a0: 'worker_enqueue'}
        if address not in names:
            return
        sp = machine.reg_read(UC_X86_REG_ESP)
        calls.append(names[address])
        if address == 0x5317a0:
            pointer = read32(sp + 8)
            words = struct.unpack('<21I', machine.mem_read(pointer, 84))
            commands.append(dict(update=list(words[:3]), skin=list(words[3:5]),
                                 meshes=[pointer_id(p) for p in words[5:]]))
        machine.reg_write(UC_X86_REG_EIP, read32(sp))
        machine.reg_write(UC_X86_REG_ESP, sp + 4)

    uc.hook_add(UC_HOOK_CODE, intercept)
    sp = STACK + 0xff00
    uc.mem_write(sp, struct.pack('<3I', STOP, EYE, 7))
    uc.reg_write(UC_X86_REG_ESP, sp); uc.reg_write(UC_X86_REG_EAX, inp['time'] & 0xffffffff)
    uc.emu_start(0x6dd640, STOP, count=1000000)
    if uc.reg_read(UC_X86_REG_EIP) != STOP:
        raise RuntimeError('Scheduler instruction limit exceeded')
    tiles = []
    for t in range(256):
        base = TILES + t * 188
        meshes = []
        for slot in range(4):
            words = list(struct.unpack('<6i2I', uc.mem_read(base + 60 + slot * 32, 32)))
            words[-1] = (words[-1] - VERTICES) // 16
            meshes.append(words)
        tiles.append(dict(last_update=read32(base + 48), head=read32(base + 52),
                          tail=read32(base + 56), meshes=meshes))
    output = dict(frame=read32(FRAME), enabled=bool(uc.mem_read(ENABLED, 1)[0]), delta=read32(DELTA),
        free_count=read32(FREECOUNT), free=[read32(FREE + i * 4) for i in range(read32(FREECOUNT))],
        expired_head=read32(EXPIRED), expired_tail=read32(EXPIRED + 4),
        expired=list(struct.unpack('<2048I', uc.mem_read(EXPIRED + 8, 8192))),
        calls=calls, commands=commands, tiles=tiles)
    manifest['cases'].append(dict(input=inp, output=output))
    print(f"{inp['name']}: {len(commands)} queued commands, frame{output['frame']}, free{output['free_count']}")
args.output.parent.mkdir(parents=True, exist_ok=True)
args.output.write_text(json.dumps(manifest, indent=2) + '\n')
print(args.output)
