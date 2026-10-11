"""Execute local original water lookup, tile interpolation and base-height sampling.

No engine or math call is intercepted. Only the installed SHA-pinned executable
is read; fixtures are stored in ignored build/run, never original executable bytes.
"""
import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'build/deps/water-oracle'))
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EAX, UC_X86_REG_ESI, UC_X86_REG_XMM0, UC_X86_REG_FPCW, UC_X86_REG_MXCSR
parser=argparse.ArgumentParser()
parser.add_argument('--exe',type=Path,default=Path(r'D:\Games\Call of Duty - Black Ops\BlackOps.exe'))
args=parser.parse_args()
raw=args.exe.read_bytes(); sha=hashlib.sha256(raw).hexdigest()
assert sha=='2ba4a04e81de2b60582da0e4489efde21e6cb07904a223bae633b6ccaebbf759','Unsupported original executable'
image=pefile.PE(data=raw).get_memory_mapped_image()
STOP,STACK,CELLS,TILE,QUERY=0x10000000,0x20000000,0x10001000,0x10002000,0x10003000
channels=[(0x462df10,0x10100000,16),(0x4631ff0,0x10200000,16),
          (0x46360d0,0x10300000,2),(0x4636970,0x10320000,1),
          (0x4636e10,0x10330000,1),(0x46372b0,0x10340000,4)]
initial=[struct.pack('<4f',.5,0.,-1.25,4.)*65536,struct.pack('<4f',2.,2.,3.,-4.)*65536,
         b''.join(struct.pack('<h',i%60001-30000) for i in range(65536)),b'\x5a'*65536,b'\x69'*65536,b'\x37'*262144]
cell_values=[(173,127,0,[0,17,254,255]),(-601,-128,1,[255,32,16,0]),(32767,25,255,[64,128,192,240]),
             (-32768,50,0,[254,253,252,251]),(2711,100,3,[7,13,29,61]),(-995,0,2,[91,87,83,79]),
             (31000,-1,0,[44,55,66,77]),(-31000,1,1,[88,99,111,122]),(0,126,255,[222,233,244,255])]
cell_bytes=b''.join(struct.pack('<hbB4B',h,f,s,*c) for h,f,s,c in cell_values)

def machine(header,count):
    uc=Uc(UC_ARCH_X86,UC_MODE_32)
    uc.mem_map(0x400000,(len(image)+4095)&~4095);uc.mem_write(0x400000,image)
    uc.mem_map(0x10000000,0x400000);uc.mem_map(STACK,0x10000)
    uc.mem_write(0x243fb78,struct.pack('<4iII',*header,count,CELLS));uc.mem_write(CELLS,cell_bytes)
    uc.mem_write(0x3e5b614,struct.pack('<f',25.))
    for (pointer,address,width),content in zip(channels,initial):
        uc.mem_write(pointer,struct.pack('<I',address));uc.mem_write(address,content)
    uc.reg_write(UC_X86_REG_FPCW,0x027f);uc.reg_write(UC_X86_REG_MXCSR,0x1f80)
    return uc

def execute(uc,address,argument=None):
    sp=STACK+0xff00;uc.mem_write(sp,struct.pack('<I',STOP))
    if argument is not None:uc.mem_write(sp+4,struct.pack('<I',argument))
    uc.reg_write(UC_X86_REG_ESP,sp);uc.emu_start(address,STOP,count=2000000)
    assert uc.reg_read(UC_X86_REG_EIP)==STOP,'Instruction limit'

cases=[('interior',0,0,0,0,0,9),('row1',0,0,16,16,16,9),
       ('edge_x',0,0,240,0,32,9),('edge_y',0,0,0,240,0,9),
       ('corner',0,0,240,240,32,9),('negative_header',-2,-3,16,240,-32,9),
       ('outside_negative',0,0,240,16,-1,9),('outside_upper',0,0,0,0,48,9),
       ('no_cells',0,0,240,240,0,0)]
manifest=dict(exe_sha256=sha,functions=dict(fill='0x6da4f0',lookup='0x6da420',base='0x6dfba0',convert='0x6ddf90',global_height='0x6e0280'),intercepted_calls=[],fill_cases=[],height_cases=[])
output=ROOT/'build/run/water_sampling55.bin'
with output.open('wb') as out:
    out.write(struct.pack('<8sII',b'BO1SMPL1',len(cases),22));out.write(cell_bytes)
    for name,minx,miny,u,v,x,count in cases:
        header=[minx,miny,minx+3,miny+3];y=16*miny
        if name=='row1':y=16
        if name=='edge_y':y=32
        if name=='corner':y=32
        tile=[u,v,x,y,-93]
        uc=machine(header,count);uc.mem_write(TILE,struct.pack('<5i',*tile)+bytes(188-20))
        execute(uc,0x6da4f0,TILE);valid=uc.reg_read(UC_X86_REG_EAX)&255
        bounds=bytes(uc.mem_read(TILE+20,28))
        worldz=bytes(uc.mem_read(TILE+16,4))
        out.write(name.encode().ljust(32,b'\0'));out.write(struct.pack('<4iI5iI',*header,count,*tile,valid));out.write(worldz+bounds)
        hashes=[]
        for _,address,width in channels:
            content=bytes(uc.mem_read(address,width*65536));out.write(content);hashes.append(hashlib.sha256(content).hexdigest())
        manifest['fill_cases'].append(dict(name=name,valid=valid,channel_sha256=hashes));print('Original fill',name,valid)
    queries=[('origin',[0.,0.,0.]),('inside',[12.49,37.51,8.]),('half_positive',[12.5,12.5,0.]),
             ('half_negative',[-12.5,-12.5,0.]),('just_negative',[-12.500001,-.0001,0.]),
             ('grid_edge',[3199.999755859375,0.,0.]),('grid_outside',[3200.,0.,0.]),
             ('world_edge',[1199.9998779296875,0.,0.]),('world_outside',[1200.,0.,0.]),
             ('reciprocal_positive',[224.99998474121094,0.,0.]),('reciprocal_negative',[-224.99998474121094,0.,0.])]
    for enabled in [0,1]:
        for label,query in queries:
            name=f'{label}_{enabled}';uc=machine([0,0,3,3],9)
            uc.mem_write(0x4638354,bytes([enabled]));uc.mem_write(0x4638360,struct.pack('<4i',0,0,248,248));uc.mem_write(QUERY,struct.pack('<3f',*query))
            uc.reg_write(UC_X86_REG_ESI,QUERY);execute(uc,0x6dfba0)
            bits=uc.reg_read(UC_X86_REG_XMM0)&0xffffffff
            out.write(name.encode().ljust(32,b'\0'));out.write(struct.pack('<I3fI',enabled,*query,bits))
            manifest['height_cases'].append(dict(name=name,query=query,enabled=enabled,expected_bits=f'{bits:08x}'));print('Original height',name,f'{bits:08x}')
(ROOT/'build/run/water_sampling55.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(output)
