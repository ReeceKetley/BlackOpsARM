from pathlib import Path
import sys,hashlib
upstream=Path(sys.argv[1]);output=Path(sys.argv[2]);output.mkdir(parents=True,exist_ok=True)
name="vp8/encoder/encodeframe.c"
original=(upstream/name).read_bytes()
assert hashlib.sha256(original).hexdigest()=="4689ef8bd642c7923740bde8baebaafbf65a75c621cc168fcc0117a1e011988a"
old=b"volatile int *current_mb_col = &cpi->mt_current_mb_col[mb_row];"
new=b"volatile int *current_mb_col = cpi->b_multi_threaded ? &cpi->mt_current_mb_col[mb_row] : NULL;"
assert original.count(old)==1
file=output/name;file.parent.mkdir(parents=True,exist_ok=True);file.write_bytes(original.replace(old,new))
