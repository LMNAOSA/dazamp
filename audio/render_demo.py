#!/usr/bin/env python3
"""Reproducible ORIGINAL instrument-like synthesized riff through our DAZAMP DSP reference.
This is NOT a real guitar recording, nor a neural amp model, nor a licensed song.
"""
from pathlib import Path
import numpy as np
import soundfile as sf
from scipy import signal
from scipy.ndimage import maximum_filter1d

HERE=Path(__file__).parent
SR=48000
BPM=156
STEP=60/BPM/4
BARS=4
N=int((BARS*16*STEP+.95)*SR)
SEED=3708
rng=np.random.default_rng(SEED)

# Unique, composed, syncopated low-string riff. This is not transcribed from any record.
# Each tuple is 16th-note step, semitone offset from low B1, length in steps, energy, mute.
ROOT=61.7354
NOTES=[
(0,0,1.0,1.0,1),(2,0,.75,.72,1),(3,0,.72,.77,1),
(5,3,2.0,.92,0),(8,0,.85,1,1),(9,0,.75,.77,1),(10,0,.75,.79,1),
(12,5,2.0,.9,0),(15,0,.78,.82,1),
(16,0,.7,1,1),(17,0,.7,.73,1),(19,0,.65,.85,1),
(21,2,2.0,.92,0),(24,0,.8,1,1),(26,0,.76,.78,1),
(28,7,1.3,.93,0),(30,5,1.8,.94,0),
(32,0,.85,1,1),(33,0,.8,.8,1),(35,0,.75,.84,1),
(37,3,1.5,.91,0),(40,0,.9,1,1),(42,0,.75,.78,1),(43,0,.75,.78,1),
(45,5,2.2,.96,0),(48,0,.75,1,1),(49,0,.73,.8,1),
(51,0,.75,.8,1),(53,2,1.5,.95,0),(56,0,.7,1,1),(57,0,.65,.7,1),
(59,7,1.8,.97,0),(61,5,2.5,1.0,0),
]

def synth_string(freq, length, mute, velocity, seed):
    """Detuned, dispersive modal string pluck with pick-position filtering and fret clack."""
    rr=np.random.default_rng(seed)
    size=int(SR*length)
    t=np.arange(size,dtype=np.float64)/SR
    sig=np.zeros(size,dtype=np.float64)
    # Frequencies of guitar string harmonics, with slightly stretched upper modes.
    for h in range(1,25):
        harmonic_freq=freq*h*(1+0.000012*h*h)
        if harmonic_freq>SR*.43: break
        pick_node=abs(np.sin(np.pi*h*.17))
        mag=pick_node/(h**1.22)
        phase=float(rr.uniform(0,2*np.pi))
        # High harmonics die faster, palm muting much faster than ringing notes.
        decay=(5.4 if mute else 1.85)+(h**.73)*(.44 if mute else .18)
        sig+=mag*np.sin(2*np.pi*harmonic_freq*t+phase)*np.exp(-decay*t)
    attacknoise=rr.normal(size=size)
    # Frequency-shaped pick noise at onset, front-loaded.
    sos=signal.butter(2,[800,10000],btype='bandpass',fs=SR,output='sos')
    pick=signal.sosfilt(sos,attacknoise)
    sig+=.07*pick*np.exp(-180*t)
    sig*=np.minimum(1,t/.002)*np.exp(-.18*t)
    sig*=velocity*.16
    return sig.astype(np.float32)

def build_di(take):
    mono=np.zeros(N,dtype=np.float32)
    rr=np.random.default_rng(SEED+take*100)
    for idx,(step,semi,dur,velocity,mute) in enumerate(NOTES):
        # Tiny manually-played variation, but never random timing wandering too far from beat.
        pos=step*STEP+float(rr.normal(0,.0037))+(take*.0025)
        length=min((dur+(.12 if mute else 1.1))*STEP,N/SR-pos-.015)
        if length<=0: continue
        sig=synth_string(ROOT*2**(semi/12),length,mute,velocity,SEED+1000*take+idx)
        # On chord accents, add a fifth and octave with slightly staggered attacks.
        start=max(0,int(pos*SR)); end=min(N,start+len(sig))
        mono[start:end]+=sig[:end-start]
        if not mute:
            for j,interval in enumerate((7,12)):
                add=synth_string(ROOT*2**((semi+interval)/12),length*.95,False,velocity*.64,SEED+8000+1000*take+idx*10+j)
                st=start+int((.006+j*.003)*SR); en=min(N,st+len(add)); mono[st:en]+=add[:en-st]
    # Preserve transparent dynamic range, avoid clipping while mixing chords.
    return np.tanh(mono*1.3).astype(np.float32)

# Approximate guitar amp voicing; ALL DSP ORIGINAL and procedural, not NAM.
def eq(x,lo,hi,mid,boost):
    sos=signal.butter(2,[lo,hi],btype='bandpass',fs=SR,output='sos')
    v=signal.sosfilt(sos,x)
    # asymmetrical gentle midrange tilt
    s=signal.butter(1,mid,btype='highpass',fs=SR,output='sos')
    return v+boost*signal.sosfilt(s,v)

def cab_fir():
    # Finite impulse response corresponding to idealized close-miked 4x12-style rolloff.
    freqs=np.array([0,35,55,70,90,130,220,410,800,1350,1900,2600,3300,4050,4700,5500,6500,8500,11000,24000],dtype=float)
    db=np.array([-85,-65,-33,-20,-7,-1,1,-2,-5,-1,2,4,1,-3,-7,-16,-28,-58,-85,-90],dtype=float)
    return signal.firwin2(1025,freqs,10**(db/20),fs=SR).astype(np.float32)

FIR=cab_fir()

def amp(x,grind=.76,tight=.78,body=.64):
    # Emulates boosted high-gain circuit response via 4x oversampling and 2 nonlinear stages.
    v=eq(x,67+55*tight,12500,790, .15+tight*.27)
    v=np.tanh(v*(2.8+2.9*tight))*0.58
    high=signal.resample_poly(v,4,1)
    drive=3.6+grind*11.5
    high=np.tanh(high*drive+0.04)-np.tanh(.04)
    # Saturation asymmetry and soft clipping.
    high=.92*np.tanh(high*.91)
    v=signal.resample_poly(high,1,4).astype(np.float32)
    # Realistic speaker low/mid coloration and sharp high end rolloff.
    v=signal.fftconvolve(v,FIR,'full')[:len(v)]
    s=signal.butter(2,[67,7400],btype='bandpass',fs=SR,output='sos')
    v=signal.sosfilt(s,v)
    return np.asarray(v,dtype=np.float32)

def stereo_mix(a,b):
    # Double tracking = separate performances, not a Haas copy or fake side enhancer.
    channels=np.vstack((a,b)).T
    # 1st reflection after the attack; kept quiet to preserve tightness.
    shift1=int(.087*SR);shift2=int(.119*SR)
    channels[shift1:,0] += a[:-shift1]*.075
    channels[shift2:,1] += b[:-shift2]*.067
    return channels

if __name__=='__main__':
    di_a=build_di(0);di_b=build_di(1)
    amp_a=amp(di_a);amp_b=amp(di_b)
    dry=di_a/ max(1e-7,np.max(np.abs(di_a)))*.78
    mono=amp_a/max(1e-7,np.max(np.abs(amp_a)))*.78
    stereo=stereo_mix(amp_a,amp_b)
    stereo/=max(1e-7,np.max(np.abs(stereo)))/.84
    # Short fade tails for no clicks.
    fade=int(SR*.05)
    for ar in (dry,mono,stereo):
        ar[-fade:]*=np.linspace(1,0,fade)[:,None] if ar.ndim==2 else np.linspace(1,0,fade)
    sf.write(HERE/'DAZAMP_DRY_DI.wav',dry,SR,subtype='PCM_24')
    sf.write(HERE/'DAZAMP_DISTORTED_MONO.wav',mono,SR,subtype='PCM_24')
    sf.write(HERE/'DAZAMP_DISTORTED_STEREO.wav',stereo,SR,subtype='PCM_24')
    print('Rendered',round(N/SR,2),'secs. dry rms',round(float(np.sqrt(np.mean(dry**2))),4),'wet rms',round(float(np.sqrt(np.mean(mono**2))),4),'stereo peak',round(float(np.max(np.abs(stereo))),4))
