"""Independent full streaming retail provenance; bounded next-asset metadata."""
import hashlib,struct,zlib
from stringtable_payload_oracle import _stream_prefix
def reference(prior):
    p=_stream_prefix(prior['original_path'],prior,66226)
    directory=prior['directory_source_start']
    if struct.unpack_from('<iI',p,directory+8)!=(7,0xffffffff) or struct.unpack_from('<iI',p,directory+16)!=(5,0xffffffff):raise RuntimeError('Actual type7/type5 directory differs')
    h=p[65678:66206];n=p[66206:66226]
    if hashlib.sha256(h).hexdigest()!='533086489d129a1222717dac632c2bda2c68ffce33faefbe4f90c7866f44f614' or struct.unpack('<I',h[:4])[0]!=0xffffffff or any(h[4:]):raise RuntimeError('Genuine all-null528 header differs')
    if hashlib.sha256(n).hexdigest()!='4b5cba3bbecffc1cb6788a25f63ef57c76679644fdd88064bf9aec4670ee4ad9' or n[-1] or 0 in n[:-1]:raise RuntimeError('Genuine inline20 name differs')
    return {'prefix_sha256':hashlib.sha256(p).hexdigest(),'header_sha256':hashlib.sha256(h).hexdigest(),'name_sha256':hashlib.sha256(n).hexdigest(),'name_adler32':zlib.adler32(n),'source_start':65678,'header_end':66206,'source_end':66226,'null_techniques':130,'next_type':5}
