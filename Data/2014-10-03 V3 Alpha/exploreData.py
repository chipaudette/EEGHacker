# ######################
#
# exploreData.py
#
# load and Plot data from OpenBCI text file
# 
# Written for the Anaconda distribution of Python
# Run inside the Spyder IDE (Pyton 2.7)
#
# Chip Audette, 2014
# Distribute under the MIT License
# http://opensource.org/licenses/MIT
#
# ########################
import matplotlib.pyplot as plt
import matplotlib.mlab as mlab
import numpy as np
from scipy import signal

# some program constants
NFFT = 256  # pitck the length of the fft
fs_Hz = 250.0        # assumed sample rate for the EEG data
f_lim_Hz = [0, 40]   # frequency limits for plotting
channel_to_plot = 2  # counting the first channel as one

# define which data to load
case = 4  # choose which case to load
pname = 'SavedData/'
if (case == 1):
    fname = 'openBCI_raw_2014-10-04_18-50-20_RightForehead_countebackby3.txt'
    f_lim_Hz = [0, 100]
elif (case == 2):
    fname = 'openBCI_raw_2014-10-04_18-55-41_O1_Alpha.txt'
elif (case == 3):
    fname = 'openBCI_raw_2014-10-04_19-06-13_O1_Alpha.txt'
elif (case == 4):
    fname = 'openBCI_raw_2014-10-05_17-14-45_O1_Alpha_noCaffeine.txt'

# load data into numpy array
data = np.loadtxt(pname + fname,
                  delimiter=',',
                  skiprows=5)

# parse the data
data_indices = data[:, 0]  # the first column is the packet index
eeg_data_uV = data[:, channel_to_plot]       # ignore the first channel (column 0), so channel 1 is column 1

# filter the data to remove DC
hp_cutoff_Hz = 1.0
print "highpass filtering at: " + str(hp_cutoff_Hz) + " Hz"
b, a = signal.butter(2, hp_cutoff_Hz/(fs_Hz / 2.0), 'highpass')  # define the filter
f_eeg_data_uV = signal.lfilter(b, a, eeg_data_uV, 0) # apply along the zeroeth dimension

# notch filter the data to remove 60 Hz and 120 Hz interference
notch_freq_Hz = np.array([60.0, 120.0])  # these are the center frequencies
for freq_Hz in np.nditer(notch_freq_Hz):  # loop over each center freq
    bp_stop_Hz = freq_Hz + 3.0*np.array([-1, 1])  # set the stop band
    print "notch filtering: " + str(bp_stop_Hz[0]) + "-" + str(bp_stop_Hz[1]) + " Hz"
    b, a = signal.butter(3, bp_stop_Hz/(fs_Hz / 2.0), 'bandstop')  # create the filter
    f_eeg_data_uV = signal.lfilter(b, a, f_eeg_data_uV, 0)  # apply along the zeroeth dimension


bp_Hz = np.zeros(0)
if (0):
    #filter to focus on alpha waves
    bp_Hz = np.array([7.0, 12.0])
    print "bandpass filtering to: " + str(bp_Hz[0]) + "-" + str(bp_Hz[1]) + " Hz"
    b, a = signal.butter(3, bp_Hz/(fs_Hz / 2.0),'bandpass')  # create the filter
    f_eeg_data_uV = signal.lfilter(b, a, f_eeg_data_uV, 0)  # apply along the zeroeth dimension


# %% plots

# make time-domain plot
fig = plt.figure(figsize=(10.0, 7.5))  # make new figure, set size in inches

ax1 = plt.subplot(211)
t_sec = np.array(range(0, f_eeg_data_uV.size)) / fs_Hz
plt.plot(t_sec, f_eeg_data_uV)
plt.ylim(-100, 100)
plt.ylabel('EEG (uV)')
plt.xlabel('Time (sec)')
plt.title(fname)
plt.xlim(t_sec[0], t_sec[-1])

# add annotation for filtering
if (bp_Hz.size > 0):
    ax1.text(0.025, 0.95,
             "BP = [" + str(bp_Hz[0]) + " " + str(bp_Hz[1]) + "] Hz",
             transform=ax1.transAxes,
             verticalalignment='top',
             horizontalalignment='left',
             backgroundcolor='w')


# make spectrogram
ax = plt.subplot(212, sharex=ax1)
FFTstep = 100  # do a new FFT every FFTstep data points
overlap = NFFT - FFTstep  # half-second steps
spec_PSDperHz, freqs, t = mlab.specgram(np.squeeze(f_eeg_data_uV),
                               NFFT=NFFT,
                               window=mlab.window_hanning,
                               Fs=fs_Hz,
                               noverlap=overlap
                               ) # returns PSD power per Hz
spec_PSDperBin = spec_PSDperHz * fs_Hz / float(NFFT)  #convert to "per bin"
del spec_PSDperHz  # remove this variable so that I don't mistakenly use it

plt.pcolor(t, freqs, 10*np.log10(spec_PSDperBin))  # dB re: 1 uV
plt.clim(25-5+np.array([-40, 0]))
plt.xlim(t_sec[0], t_sec[-1])
plt.ylim([0, fs_Hz/2.0])  # show the full frequency content of the signal
plt.ylim(f_lim_Hz)
plt.xlabel('Time (sec)')
plt.ylabel('Frequency (Hz)')

# add annotation for FFT Parameters
ax.text(0.025, 0.95,
        "NFFT = " + str(NFFT) + "\nfs = " + str(int(fs_Hz)) + " Hz",
        transform=ax.transAxes,
        verticalalignment='top',
        horizontalalignment='left',
        backgroundcolor='w')

plt.tight_layout()
