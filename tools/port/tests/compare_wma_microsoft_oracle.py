"""Compare actual HONOR decoder summaries with actual Microsoft xWMA SDK decode.
Uses existing scratch WAVs reconstructed from the two validated packet classes.
No new asset tests or decoder substitution; only rounding-tolerant waveform proof.
"""
import pathlib,re,wave,struct,math,sys
out=pathlib.Path('build-android-audit/native_registry_closure');log=pathlib.Path(sys.argv[1]).read_text();reference={}
for ch in(1,2):
 with wave.open(str(out/f'retail-wma-class{ch}-ms.wav')) as w:reference[ch]=struct.unpack('<'+'h'*(w.getnframes()*ch),w.readframes(w.getnframes()))
count=0;worst_mean=worst_rms=0
for match in re.finditer(r'WMA_ORACLE ch(\d+) first(\d+) count(\d+) mean([-\d.]+) rms([-\d.]+)',log):
 ch,first,n=map(int,match.group(1,2,3));values=[v/32768 for v in reference[ch][first:first+n]]
 assert len(values)==n
 mean=sum(values)/n;rms=math.sqrt(sum(v*v for v in values)/n)
 mean_error=abs(mean-float(match[4]));rms_error=abs(rms-float(match[5]));worst_mean=max(worst_mean,mean_error);worst_rms=max(worst_rms,rms_error)
 if mean_error>3e-5 or rms_error>3e-5:raise RuntimeError(f'Actual Microsoft waveform mismatch ch{ch} first{first}: meanError{mean_error} rmsError{rms_error}')
 count+=1
assert count==185, count
print(f'PASS actual Microsoft oracle all{count} 512-frame windows bothpacketclasses including final tails: maxMeanError{worst_mean:.9g} maxRmsError{worst_rms:.9g}; tolerance3e-5 PCM16 rounding')
