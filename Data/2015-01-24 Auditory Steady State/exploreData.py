
# coding: utf-8

## Explore Data from OpenBCI

# EEG Hacker, 2015-01-17, MIT License

#### Purpose

# OpenBCI data file.  Do some simple visualizations to make sure that it looks OK.

#### Prepare

# First, we have to import some libraries...though this might be done automagically for IPython Notebooks...I'm not sure.

# In[25]:

get_ipython().magic(u'matplotlib inline')
import matplotlib.pyplot as plt
import numpy as np
import matplotlib.mlab as mlab


#### Load the Data

# In[26]:

# Define Data to Load...be sure to UNZIP the data!!!
pname = 'SavedData/'

if (0): 
    fname = 'OpenBCI-RAW-2015-01-24_12-13-06_eyesOpen_noCaffeine.txt'
    t_lim_sec = [150.0, 290.0]
else:
    fname = 'OpenBCI-RAW-2015-01-24_12-25-42_eyesClosed_noCaffeine.txt'
    t_lim_sec = [60.0, 180.0]
    
# load data into numpy array
fs_Hz = 250.0        # assumed sample rate for the EEG data
data = np.loadtxt(pname + fname,
                  delimiter=',',
                  skiprows=5)


#### Check the Packet Counter

# In[27]:

# check the packet counter for dropped packets
data_indices = data[:, 0]   # the first column is the packet index
d_indices = data_indices[2:]-data_indices[1:-1]
n_jump = np.count_nonzero((d_indices != 1) & (d_indices != -255))
print("Number of discontinuities in the packet counter: " + str(n_jump))


#### Unpack the Data

# In[28]:

# parse the data out of the values read from the text file
eeg_data_uV = data[:, 1:(8+1)] # EEG data, microvolts
#accel_data_counts = data[:, 9:(11+1)]  # note, accel data is NOT in engineering units

# convert units
#unused_bits = 4 #the 4 LSB are unused?
#accel_data_counts = accel_data_counts / (2**unused_bits) # strip off this extra LSB
#scale_G_per_count = 0.002  # for full-scale = +/- 4G, scale is 2 mG per count
#accel_data_G = scale_G_per_count*accel_data_counts # convert to G

# create a vector with the time of each sample
t_sec = np.arange(len(eeg_data_uV[:, 0])) / fs_Hz


#### Plot the EEG Data

# In[29]:

# plot each channel of EEG data...raw
nchan = 1  #normally 8 or 16
ncol = 1; 
nrow = nchan / ncol
plt.figure(figsize=(ncol*10, nrow*5))
for Ichan in range(nchan):      
    plt.subplot(nrow,ncol,Ichan+1)
    plt.plot(t_sec,eeg_data_uV[:, Ichan])
    plt.xlabel("Time (sec)")       
    plt.ylabel("Raw EEG (uV)")
    plt.title("Channel " + str(Ichan+1))
    plt.xlim(t_lim_sec)
    #plt.ylim([-1.5, 1.5])

plt.tight_layout()


#### Spectrograms

# In[30]:

NFFT = 256*2  # pitck the length of the fft
FFTstep = 0.5*fs_Hz  # do a new FFT every half second
overlap = NFFT - FFTstep  # half-second steps
f_lim_Hz = [0, 50]   # frequency limits for plotting

plt.figure(figsize=(ncol*10, nrow*5))
for Ichan in range(nchan):      
    ax = plt.subplot(nrow,ncol,Ichan+1)
    data = np.array(eeg_data_uV[:,Ichan])
    data = data - np.mean(data,0)
    spec_PSDperHz, freqs, t = mlab.specgram(data,
                               NFFT=NFFT,
                               window=mlab.window_hanning,
                               Fs=fs_Hz,
                               noverlap=overlap
                               ) # returns PSD power per Hz
    spec_PSDperBin = spec_PSDperHz * fs_Hz / float(NFFT)  #convert to "per bin"

    plt.pcolor(t, freqs, 10*np.log10(spec_PSDperBin))  # dB re: 1 uV
    plt.clim(20-7.5-3.0+np.array([-30, 0]))
    #plt.xlim(t_sec[0], t_sec[-1])
    plt.xlim(np.array(t_lim_sec)+np.array([-10, 10]))
    #plt.ylim([0, fs_Hz/2.0])  # show the full frequency content of the signal
    plt.ylim(f_lim_Hz)
    plt.xlabel('Time (sec)')
    plt.ylabel('Frequency (Hz)')
    plt.title("Channel " + str(Ichan+1))

    # add annotation for FFT Parameters
    ax.text(0.025, 0.95,
        "NFFT = " + str(NFFT) + "\nfs = " + str(int(fs_Hz)) + " Hz",
        transform=ax.transAxes,
        verticalalignment='top',
        horizontalalignment='left',
        backgroundcolor='w')
    plt.colorbar()

plt.tight_layout()


#### Compute Average Spectrum

# In[31]:

#find spectrum slices within the time period of interest
ind = ((t > t_lim_sec[0]) & (t < t_lim_sec[1]))

#get the mean spectrum in that time
spectrum_PSDperHz = np.mean(spec_PSDperHz[:,ind],1)  #time is horizontal in the 2D array?

#plot
plt.figure(figsize=(ncol*10, nrow*5))
ax = plt.subplot(nrow,ncol,Ichan+1)
plt.plot(freqs, 10*np.log10(spectrum_PSDperHz))  # dB re: 1 uV
#plt.xlim([0, fs_Hz/2.0])  # show the full frequency content of the signal
#plt.xlim(f_lim_Hz)
plt.xlim([30, 50])
plt.ylim([-15.0, 0.0])
plt.plot(38.0*np.array([1, 1]),ax.get_ylim(),'k--',linewidth=2)
plt.plot(40.0*np.array([1, 1]),ax.get_ylim(),'k--',linewidth=2)
plt.plot(42.0*np.array([1, 1]),ax.get_ylim(),'k--',linewidth=2)
plt.xlabel('Frequency (Hz)')
plt.ylabel('PSD per Hz (dB re: 1uV^2/Hz)')
plt.title("Channel " + str(Ichan+1))

# add annotation for FFT Parameters
ax.text(1-0.025, 0.95,
    "NFFT = " + str(NFFT) + "\nfs = " + str(int(fs_Hz)) + " Hz",
    transform=ax.transAxes,
    verticalalignment='top',
    horizontalalignment='right',
    backgroundcolor='w')


# In[32]:

spec_PSDperBin[1,1]

