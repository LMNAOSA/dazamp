from pathlib import Path
import numpy as np
import soundfile as sf
from scipy import signal
ROOT=Path(__file__).resolve().parents[1]/'audio'
a,sra=sf.read(ROOT/'DAZAMP_DRY_DI.wav')
b,srb=sf.read(ROOT/'DAZAMP_DISTORTED_MONO.wav')
c,src=sf.read(ROOT/'DAZAMP_DISTORTED_STEREO.wav')
assert sra==srb==src==48000
assert len(a)==len(b)==len(c) and 6.5<len(a)/sra<8.0
assert a.ndim==b.ndim==1 and c.ndim==2 and c.shape[1]==2
assert all(np.max(np.abs(arr))<0.95 for arr in (a,b,c))
assert np.isfinite(a).all() and np.isfinite(b).all() and np.isfinite(c).all()
assert np.corrcoef(c[:,0],c[:,1])[0,1]<0.8 # genuinely separate takes
f,spA=signal.welch(a,sra,nperseg=8192)
_,spB=signal.welch(b,srb,nperseg=8192)
high=f>1800
low=(f>80)&(f<500)
ratioA=spA[high].sum()/spA[low].sum()
ratioB=spB[high].sum()/spB[low].sum()
assert ratioB>ratioA*4, 'Distortion should generate audibly different upper harmonics'
print(f'PASS: audio 48 kHz, {len(a)/sra:.2f}s, no clipped samples, stereo independent tracks, harmonic ratio boost {ratioB/ratioA:.1f}x')
