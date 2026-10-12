"""Read-only retail index diagnostics and source lookup model; no game text."""
from pathlib import Path
import argparse,hashlib,json,struct,sys
sys.dont_write_bytecode=True
from stringtable_payload_oracle import reference,_stream_prefix,sha
ROOT=Path(__file__).resolve().parents[2]
def signed(value):
    value&=0xffffffff
    return value if value<0x80000000 else value-0x100000000
def inspect(source,oracle):
    ref=reference(source,oracle);prefix=_stream_prefix(source,ref,ref['prefix_bytes'])
    entries=ref['stringtable_cells'];count=len(entries);columns=ref['stringtable_header']['column_count']
    hashes=[e['signed_hash'] for e in entries];strings=[];non_ascii=0;agree=0
    for e in entries:
        info=e['string']
        if info['kind']=='offset':info=entries[info['target_cell_index']]['string']
        if info['kind']=='null':text=None
        else:text=bytes(prefix[info['source_start']:info['source_end']-1])
        strings.append(text)
        if text is not None:
            non_ascii+=int(any(c>=128 for c in text))
            value=5381
            for c in text.lower():value=(33*value+c)&0xffffffff
            agree+=int(signed(value)==e['signed_hash'])
    index=ref['stringtable_indices'];indices=struct.unpack_from('<'+'h'*count,prefix,index['source_start'])
    signed_inv=[];unsigned_inv=[];wrap_inv=[];column_inv=[]
    for position,(a,b) in enumerate(zip(indices,indices[1:]),1):
        if hashes[a]>hashes[b]:signed_inv.append({'position':position,'before_cell':a,'before_hash':hashes[a],'before_column':a%columns,'after_cell':b,'after_hash':hashes[b],'after_column':b%columns})
        if (hashes[a]&0xffffffff)>(hashes[b]&0xffffffff):unsigned_inv.append(position)
        difference=signed(hashes[a]-hashes[b])
        if difference>0 or (difference==0 and a%columns>b%columns):wrap_inv.append(position)
        if hashes[a]==hashes[b] and a%columns>b%columns:column_inv.append(position)
    def lookup(query,column,linear):
        if linear:positions=range(count)
        else:
            begin,end=0,count
            while begin<end:
                mid=(begin+end)//2
                if hashes[indices[mid]]>=hashes[query]:end=mid
                else:begin=mid+1
            positions=range(begin,count)
        for position in positions:
            cell=indices[position]
            if hashes[cell]!=hashes[query]:
                if not linear:return -1
                continue
            if cell%columns==column and strings[cell] is not None and strings[cell].lower()==strings[query].lower():return cell//columns
        return -1
    misses=0;other_rows=0
    for cell in range(count):
        if strings[cell] is None:continue
        actual=lookup(cell,cell%columns,False);expected=lookup(cell,cell%columns,True)
        if actual==-1 and expected>=0:misses+=1
        elif actual!=expected:other_rows+=1
    return {'file':Path(source).stem,'raw_sha256':ref['raw_sha256'],'decoded_sha256':ref['decoded_sha256'],
            'index_sha256':index['sha256'],'cells':count,'columns':columns,'permutation':sorted(indices)==list(range(count)),
            'hash_agreements':agree,'non_ascii_cells':non_ascii,'signed_inversions':signed_inv,
            'unsigned_inversion_positions':unsigned_inv,'wrapped_adjacent_inversions':len(wrap_inv),
            'equal_hash_column_inversions':len(column_inv),'modeled_existing_query_misses':misses,
            'modeled_other_row_differences':other_rows,'queries':count}
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--retail-root',type=Path,default=Path('D:/Games/Call of Duty - Black Ops'));args=ap.parse_args()
    output=args.output.resolve()
    if not output.is_relative_to(ROOT) or output.exists():raise RuntimeError('Use fresh output inside BlackOpsARM')
    result={'schema':'BlackOpsARM.stringtable-index-readonly-audit.v2','native_function_executed':False,
            'comparator_inference_only':True,'locale_domain':'Actual genuine strings are ASCII; independent lowercase model',
            'action':'Preserve retail indices. Source models are diagnostics, not native lookup certification or resorting permission.',
            'source_sha256':{n:sha(ROOT/n) for n in ['src/universal/com_stringtable.cpp','src/universal/com_stringtable_obj.cpp','src/server_mp/sv_bot_mp.cpp']},'records':[]}
    for asset,leaf in [('code_pre_gfx','retained.inflate-genuine-oracle.json'),('zombie_pentagon','retained.inflate-zombie-oracle.json')]:
        oracle=json.loads((ROOT/'docs/port/evidence/arm64_inflate'/leaf).read_text(encoding='utf-8'))
        result['records'].append(inspect(args.retail_root/'zone/common'/(asset+'.ff'),oracle))
    output.parent.mkdir(parents=True,exist_ok=True);output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    for row in result['records']:print(row['file'],row['permutation'],row['hash_agreements'],'signed inversions',len(row['signed_inversions']),'modeled misses',row['modeled_existing_query_misses'])
if __name__=='__main__':main()
