"""Execute installed retail view tracking, including its sqrt/pow/expiry/teleport.

Only base-height sampling and tile filling are intercepted. This isolates
tracking math/relocation without pretending to certify the map samplers.
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
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EAX, UC_X86_REG_EIP, UC_X86_REG_XMM0, UC_X86_REG_FPCW, UC_X86_REG_MXCSR
parser = argparse.ArgumentParser()
parser.add_argument('--exe', type=Path, default=Path(r'D:\Games\Call of Duty - Black Ops\BlackOps.exe'))
parser.add_argument('--output', type=Path, default=ROOT / 'build/run/water_track54.json')
args = parser.parse_args()
raw = args.exe.read_bytes(); sha = hashlib.sha256(raw).hexdigest()
if sha != '2ba4a04e81de2b60582da0e4489efde21e6cb07904a223bae633b6ccaebbf759':
    raise RuntimeError('Unsupported retail executable')
image = pefile.PE(data=raw).get_memory_mapped_image()
FRAME, GRID, CENTER, LOCAL, OLD = 0x4638350, 0x4638360, 0x4638368, 0x4638370, 0x463837c
TILES, EXPIRED, STOP, EYE, STACK = 0x46383a0, 0x462be30, 0x10000800, 0x10002000, 0x20000000
cases = [dict(name='stationary'), dict(name='small_motion', eye=[1.,0.,0.]),
         dict(name='half_motion', eye=[12.5,0.,0.]), dict(name='diagonal', eye=[3.,4.,12.]),
         dict(name='clamped_motion', eye=[30.,20.,-10.]),
         dict(name='height_rounding', height=17389.125, local=-15431.75, eye=[0.,0.,.03125]),
         dict(name='precise_distance', eye=[24.,.004,.00425], speed=0.),
         dict(name='precise_fraction', eye=[7.3125,3.34375,2.28125], speed=0.),
         dict(name='shift_x', eye=[425.,0.,0.], rings=True),
         dict(name='shift_y', eye=[0.,-425.,0.], rings=True),
         dict(name='shift_diagonal', eye=[825.,-825.,0.], rings=True),
         dict(name='negative_grid', grid=[-16,-16], eye=[-425.,-425.,0.], old=[-425.,-425.,0.]),
         dict(name='positive_seam', center=[248,248], eye=[425.,425.,0.], rings=True),
         dict(name='negative_seam', center=[0,0], eye=[-825.,-825.,0.], rings=True),
         dict(name='teleport_boundary', eye=[6400.,0.,0.], rings=True),
         dict(name='teleport_negative', eye=[-7000.,6500.,24.], rings=True),
         dict(name='short_of_teleport', eye=[6399.99951171875,0.,0.], rings=True)]
manifest = dict(exe_sha256=sha, track_address='0x6dc640', fpcw='0x027f', mxcsr='0x1f80',
                intercepted_calls={'base_height':'0x6dfba0','fill_tile':'0x6da4f0'}, cases=[])
for changes in cases:
    inp = dict(grid=[0,0], center=[128,128], eye=[0.,0.,0.], old=[0.,0.,0.],
               height=17.25, local=-31.5, speed=.75, scale=25., rings=False)
    inp.update(changes)
    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    uc.mem_map(0x400000, (len(image)+4095)&~4095); uc.mem_write(0x400000,image)
    uc.mem_map(0x10000000,0x10000); uc.mem_map(STACK,0x10000)
    write32=lambda addr,val:uc.mem_write(addr,struct.pack('<I',val&0xffffffff))
    read32=lambda addr:struct.unpack('<I',uc.mem_read(addr,4))[0]
    write32(FRAME,31)
    for i,v in enumerate(inp['grid']): write32(GRID+i*4,v)
    for i,v in enumerate(inp['center']): write32(CENTER+i*4,v)
    uc.mem_write(LOCAL,struct.pack('<3f',-99.,inp['local'],inp['speed']))
    uc.mem_write(OLD,struct.pack('<3f',*inp['old'])); uc.mem_write(EYE,struct.pack('<3f',*inp['eye']))
    uc.mem_write(0x3e5b614,struct.pack('<f',inp['scale']))
    uc.mem_write(EXPIRED,bytes(8+8192))
    for t in range(256):
        base=TILES+t*188; x=t%16*16; y=t//16*16
        uc.mem_write(base,bytes(188))
        uc.mem_write(base,struct.pack('<5i',x,y,x+inp['grid'][0]-128,y+inp['grid'][1]-128,t%9-4))
        uc.mem_write(base+20,b'\1'); write32(base+48,t//16)
        write32(base+52,3 if inp['rings'] else 0); write32(base+56,1 if inp['rings'] else 0)
        for s in range(4):
            old=(t*2+(s==0))%512
            uc.mem_write(base+60+s*32,struct.pack('<6i2I',-10,-20,-30,-40,-50,-1,old*289,0))
    fills=[]; base_calls=[0]
    def intercept(machine,address,size,userdata):
        if address not in [0x6dfba0,0x6da4f0]: return
        sp=machine.reg_read(UC_X86_REG_ESP)
        if address==0x6dfba0:
            base_calls[0]+=1
            machine.reg_write(UC_X86_REG_XMM0,struct.unpack('<I',struct.pack('<f',inp['height']))[0])
        else:
            pointer=read32(sp+4); tile=(pointer-TILES)//188
            if pointer!=TILES+tile*188 or not 0<=tile<256: raise RuntimeError('Invalid retail fill pointer')
            coords=list(struct.unpack('<5i',machine.mem_read(pointer,20)))
            valid=int((coords[2]+coords[3])%3!=0)
            fills.append([tile,*coords,valid]); machine.reg_write(UC_X86_REG_EAX,valid)
        machine.reg_write(UC_X86_REG_EIP,read32(sp)); machine.reg_write(UC_X86_REG_ESP,sp+4)
    uc.hook_add(UC_HOOK_CODE,intercept)
    sp=STACK+0xff00;write32(sp,STOP);uc.reg_write(UC_X86_REG_ESP,sp);uc.reg_write(UC_X86_REG_EAX,EYE)
    uc.reg_write(UC_X86_REG_FPCW,0x027f);uc.reg_write(UC_X86_REG_MXCSR,0x1f80)
    uc.emu_start(0x6dc640,STOP,count=2000000)
    if uc.reg_read(UC_X86_REG_EIP)!=STOP: raise RuntimeError('Tracking instruction limit exceeded')
    tiles=[]
    for t in range(256):
        base=TILES+t*188
        tiles.append(list(struct.unpack('<5I',uc.mem_read(base,20)))+
                     [uc.mem_read(base+20,1)[0],read32(base+48),read32(base+52),read32(base+56)])
    out=dict(frame=read32(FRAME), grid=[read32(GRID+i*4) for i in range(2)],
             center=[read32(CENTER+i*4) for i in range(2)], local_bits=[read32(LOCAL+i*4) for i in range(3)],
             old_bits=[read32(OLD+i*4) for i in range(3)], base_calls=base_calls[0], fills=fills, tiles=tiles,
             expired_head=read32(EXPIRED),expired_tail=read32(EXPIRED+4),
             expired=list(struct.unpack('<2048I',uc.mem_read(EXPIRED+8,8192))))
    manifest['cases'].append(dict(input=inp,output=out))
    print(f"{inp['name']}: fills{len(fills)}, speedBits{out['local_bits'][2]:08x}")
args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(manifest,indent=2)+'\n')
print(args.output)
