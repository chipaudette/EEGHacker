# -*- coding: utf-8 -*-
"""
Created on Tue Jul 01 12:20:20 2014

@author: wea
"""

import matplotlib
import matplotlib.pyplot as plt
import numpy as np 
from scipy import signal
import math
from c2cb import c2cb

pname = "SavedData\\"
fname = "openBCI_raw_2014-05-31_20-57-51_Robot05.txt"
t_plot_sec = [0.0, 135.0]
fs_Hz = 250.0  #sample rate

#load data
fullfname = pname+fname
print("loading from: " + fullfname)
data = np.loadtxt(fullfname,delimiter=',',comments='%')


#parse the data
(r,c) = data.shape
print 'Size of data = %i, %i' % (r, c)
data_counter = data[:,0] # get just first column
data = data[:,1:]  #remove first column
(r,c) = data.shape
print 'Size of data = %i, %i' % (r, c)
if (c < 16):
    naux = c-8
else:
    naux = c-16
data_aux = data[:,-naux:]
data = data[:,:-naux]    
(r,c) = data.shape
print 'Size of data = %i, %i' % (r, c)


# High-pass filter
Ichan = 2-1
N = 2
hp_Hz = 1.0
(b, a) = signal.butter(N, hp_Hz / (fs_Hz / 2.0), 'high')
filter_axis = 0
data_filt = signal.lfilter(b, a, data, filter_axis)

bp_Hz =np.array([8.0, 14.0])
(b, a) = signal.butter(N, bp_Hz / (fs_Hz / 2.0), 'bandpass')
filter_axis = 0
data_filt_bp = signal.lfilter(b, a, data, filter_axis)

# compute the spectrogram
Nfft = 512
novrlap = Nfft-50
(Pxx,freqs,t) = plt.mlab.specgram(data_filt[:,Ichan],Nfft,fs_Hz,noverlap=novrlap)
Pxx = np.array(Pxx)
Pxx = Pxx

#smooth the spectrogram in log space
smooth_fac = 0.9;
b = np.array([1.0-smooth_fac])
a = np.array([1.0, -smooth_fac])
Pxx_dB = np.log10(Pxx)
Pxx_dB = signal.lfilter(b,a,Pxx_dB)
Pxx = np.power(10.0,Pxx_dB)


#plot the data
t_sec = (data_counter-data_counter[0])/fs_Hz

plt.figure(figsize=(14,12))

plt.subplot(3,1,1)
plt.tight_layout()   #add space between plots
plt.plot(t_sec,data[:,Ichan])
plt.title("Channel " + str(Ichan) + '\n' + fname)
plt.xlabel("Time (sec)")
plt.ylabel('Microvolts')
#plt.xlim(0,t_sec[-1])
plt.xlim(t_plot_sec)
xl = plt.xlim()

plt.subplot(3,1,2)
plt.tight_layout()   #add space between plots
plt.plot(t_sec,data_filt[:,Ichan],t_sec,data_filt_bp[:,Ichan])
plt.ylim(-100,100)
plt.title("Filtered, Channel " + str(Ichan))
plt.xlabel("Time (sec)")
plt.ylabel('Microvolts')
plt.xlim(xl)
plt.legend(['HP','BP'])

plt.subplot(3,1,3)
plt.tight_layout()
plt.pcolormesh(t,freqs,10.0*np.log10(Pxx))
cl = np.array([-30.0, 0.0])
plt.clim(cl+18.0)
plt.xlim(xl)
plt.ylim(0,22)
plt.xlabel("Time (sec)")
plt.ylabel("Frequency (Hz)")
plt.title("Power Spectral Density")
plt.ion()
plt.show()

