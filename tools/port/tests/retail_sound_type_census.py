import zipfile,pathlib,collections,json,struct,zlib,re,hashlib
out=pathlib.Path('build-android-audit/native_registry_closure'); sc=collections.Counter(); invalid=[]
for p in pathlib.Path('D:/Games/Call of Duty - Black Ops/main').glob('*.iwd'):
 with zipfile.ZipFile(p) as z:
  for entry in z.infolist():
   if not entry.filename.lower().endswith('.wav'):continue
   with z.open(entry) as f:head=f.read(56)
   if len(head)!=56:invalid.append([p.name,entry.filename,'short header']);continue
   h=struct.unpack('<14I',head);sc[(h[0],h[3],h[4],h[5],h[7],bool(h[10]))]+=1
stream={'count':sum(sc.values()),'invalid':invalid}
result={'streams':{'count':stream['count'],'classes':[{'version_channels_header_block_format_seek':k,'count':v} for k,v in sorted(sc.items())],'invalid':stream['invalid']},'resident_scan':{'scope':'all86 zone/Common/*.ff; strict inline LoadedSound candidates, not full canonical loader traversal','zones':[],'formats':{},'wma_classes':[],'errors':[]}}
formats=collections.Counter();wc=collections.Counter();selected={}
root=pathlib.Path('D:/Games/Call of Duty - Black Ops/zone/Common')
for p in sorted(root.glob('*.ff')):
 raw=p.read_bytes()
 if raw[:8]!=b'IWffu100':result['resident_scan']['errors'].append([p.name,'unsupported magic']);continue
 try:data=zlib.decompress(raw[12:])
 except zlib.error as e:result['resident_scan']['errors'].append([p.name,str(e)]);continue
 count=collections.Counter()
 for m in re.finditer(rb'sound[\\/][a-zA-Z0-9_./\\ -]+\.wav\x00',data,re.I):
  if m.start()<60:continue
  h=struct.unpack_from('<15I',data,m.start()-60)
  if h[0]!=0xffffffff or h[1]!=1 or not 1000<=h[3]<=100000 or h[4] not in(1,2):continue
  if h[14]!=0xffffffff or h[8] not in(0,6,7) or not 0<h[13]<10000000 or h[11]>4096 or (h[11] and h[12]!=0xffffffff):continue
  end=m.end()+4*h[11]+h[13]
  if end>len(data):continue
  seek=data[m.end():m.end()+4*h[11]]
  if h[8]==7:
   if not h[11] or h[13]%h[11]:continue
   sizes=struct.unpack('<'+'I'*h[11],seek)
   if list(sizes)!=sorted(sizes) or sizes[-1]!=h[2]*h[4]*2:continue
   packet=h[13]//h[11];wc[(h[3],h[4],packet,h[5],h[6])]+=1
   if h[4] not in selected:
    payload=data[m.end()+4*h[11]:end];selected[h[4]]={'zone':p.name,'offset':m.start()-60,'name':m.group()[:-1].decode(),'header':h,'seek':list(sizes),'payload_sha256':hashlib.sha256(payload).hexdigest()}
  count[h[8]]+=1;formats[h[8]]+=1
 result['resident_scan']['zones'].append({'zone':p.name,'records':dict(count)})
result['resident_scan']['formats']=dict(formats);result['resident_scan']['wma_classes']=[{'rate_channels_packet_header_block':k,'count':v} for k,v in sorted(wc.items())];result['resident_scan']['selected']=selected
(out/'retail-sound-type-census.json').write_text(json.dumps(result,indent=2)); print('Stream classes:',dict(sc));print('Resident formats:',dict(formats));print('WMA classes:',dict(wc));print('Errors:',result['resident_scan']['errors']);print('Selected:',[(k,v['zone'],v['offset'],v['name']) for k,v in selected.items()])
