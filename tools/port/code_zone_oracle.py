"""Independent bounded retail decode and all36 code RawFile extents/hashes."""
from pathlib import Path
import hashlib,struct,zlib
from rawfile_header_oracle import CODE_SHA
def inspect_code_zone(reference,pointer_bytes):
 path=Path(reference['original_path'])
 if path.stat().st_size>1024*1024:raise RuntimeError('Code retail read exceeds bound')
 raw=path.read_bytes()
 if len(raw)>1024*1024 or hashlib.sha256(raw).hexdigest()!=CODE_SHA or raw[:12]!=b'IWffu100'+struct.pack('<I',473):raise RuntimeError('Genuine code provenance differs')
 decoder=zlib.decompressobj();data=decoder.decompress(raw[12:],65536)
 if len(data)!=49593 or not decoder.eof or decoder.unconsumed_tail or any(decoder.unused_data) or hashlib.sha256(data).hexdigest()!=reference['decoded_sha256']:raise RuntimeError('Complete bounded code decode differs')
 # Table extent is independently established by the existing strict table oracle.
 cursor=10407;disk=reference['stringtable_indices']['disk_end'];native=reference['stringtable_indices']['native64_end'] if pointer_bytes==8 else disk
 # Original Win32 mask0 byte children have no padding; native64 adds mask7.
 rows=[];names=set();mask=7 if pointer_bytes==8 else 0
 for index in range(1,37):
  start=cursor;tag,length,buffer=struct.unpack_from('<IiI',data,cursor);cursor+=12
  if tag!=0xffffffff or buffer!=0xffffffff or length<0 or length>len(data):raise RuntimeError('Unexpected real RawFile domain')
  end=data.index(0,cursor)+1;name=data[cursor:end]
  if len(name)<=1 or name[0]==44 or any(c>=128 for c in name) or name.lower() in names:raise RuntimeError('Unexpected default/duplicate name')
  names.add(name.lower());cursor=end;count=length+1
  if cursor+count>len(data):raise RuntimeError('Raw buffer exceeds decoded source')
  payload=data[cursor:cursor+count];cursor+=count;native=(native+mask)&~mask;native+=len(name);native=(native+mask)&~mask;native+=count;disk+=len(name)+count
  rows.append({'directory_index':index,'source_start':start,'source_end':cursor,'length':length,'name_bytes':len(name),'buffer_bytes':count,'name_adler32':zlib.adler32(name),'buffer_adler32':zlib.adler32(payload),'disk_end':disk,'native_end':native})
 if cursor!=49593:raise RuntimeError('Whole code zone leaves decoded residue')
 return {'raw_sha256':CODE_SHA,'decoded_sha256':hashlib.sha256(data).hexdigest(),'decoded_adler32':zlib.adler32(data),'decoded_bytes':len(data),'rawfiles':rows,'residue':0,'pointer_bytes':pointer_bytes,'production_functions_executed':False}
