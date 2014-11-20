
import matplotlib.pyplot as plt
import numpy as np
from scipy import signal

# assumed sample rate of OpenBCI
fs_Hz = 250.0

# create the 60 Hz filter
bp_stop_Hz = np.array([59.0, 61.0])
b, a = signal.butter(2,bp_stop_Hz/(fs_Hz / 2.0), 'bandstop')
    
# create the 50 Hz filter
bp2_stop_Hz = np.array([49, 51.0]) 
b2, a2 = signal.butter(2,bp2_stop_Hz/(fs_Hz / 2.0), 'bandstop')

# compute the frequency response
w, h = signal.freqz(b,a,1000)
w, h2 = signal.freqz(b2,a2,1000)
f = w * fs_Hz / (2*np.pi)  # convert from rad/sample to Hz

# plot
fig = plt.figure()
ax1 = fig.add_subplot(1,1,1)
plt.plot(f,10.0*np.log10(h * np.conj(h)),'b.-', \
         f,10.0*np.log10(h2 * np.conj(h2)), 'r.-')
plt.xlabel("Frequency (Hz)")
plt.ylabel("Response (dB)")
plt.xlim([40.0, 70.0])
plt.ylim([-60, 0])
plt.legend(['60Hz Notch','50Hz Notch'])

