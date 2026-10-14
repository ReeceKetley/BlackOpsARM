from pathlib import Path
old=Path('docs/port/evidence/native_post_fusion_trial1');out=Path(__file__).parent
(out/'compile.py').write_bytes((old/'compile.py').read_bytes())
s=(old/'ownership_check.py').read_text().replace("s.index('bool DrawTrial()')","s.index('bool DrawQuad(')")
(out/'ownership_check.py').write_text(s,encoding='utf8')
s=(old/'pixel_check.py').read_text().replace("s.index('bool Compare()')","s.index('void ReportDiff(')").replace('r.maxRgb==1&&r.alphaMismatch==1','r.maxRgb==1&&r.alphaMismatch==1&&r.maxAlpha==1')
(out/'pixel_check.py').write_text(s,encoding='utf8')
