
import matplotlib.pyplot as plt
import matplotlib.mlab as mlab
import numpy as np
from scipy import signal


# some program constants
NFFT = 400  # pitck the length of the fft
fs_Hz = 250.0       # assumed sample rate for the EEG data
f_lim_Hz = [0, 40]  # frequency limits for plotting
t_lim_sec = [0, 0]   # default plot time limits [0,0] will be ignored
alpha_lim_sec = [0, 0]

# define which data to load
case = 2  # choose which case to load
pname = 'SavedData/'
if (case == 1):
    fname = 'openBCI_raw_2014-10-04_18-50-20_RightForehead_countebackby3.txt'
    f_lim_Hz = [0, 100]
elif (case == 2):
    fname = 'openBCI_raw_2014-10-04_18-55-41_O1_Alpha.txt'
    t_lim_sec = [35, 85]
    alpha_lim_sec = [54, 77]
elif (case == 3):
    fname = 'openBCI_raw_2014-10-04_19-06-13_O1_Alpha.txt'
    t_lim_sec = [40, 83]
    alpha_lim_sec = [58, 76]
elif (case == 4):
    fname = 'openBCI_raw_2014-10-05_17-14-45_O1_Alpha_noCaffeine.txt'
    t_lim_sec = [70, 122]
    alpha_lim_sec = [90, 118]


# load data into numpy array
data = np.loadtxt(pname + fname,
                  delimiter=',',
                  skiprows=5)


# parse the data
data_indices = data[:, [0]]  # the first column is the packet index
eeg_data_uV = data[:, [2]]       # the 3rd column is EEG channel 2

# filter the data to remove DC
hp_cutoff_Hz = 1.0
b, a = signal.butter(2, hp_cutoff_Hz/(fs_Hz / 2.0), 'highpass')  # define the filter
f_eeg_data_uV = signal.lfilter(b, a, eeg_data_uV, 0) # apply along the zeroeth dimension

# notch filter the data to remove 60 Hz and 120 Hz
notch_freq_Hz = np.array([60.0, 120.0])  # these are the center frequencies
for freq_Hz in np.nditer(notch_freq_Hz):  # loop over each center freq
    bp_stop_Hz = freq_Hz + 3.0*np.array([-1, 1])  # set the stop band
    b, a = signal.butter(3, bp_stop_Hz/(fs_Hz / 2.0), 'bandstop')  # create the filter
    f_eeg_data_uV = signal.lfilter(b, a, f_eeg_data_uV, 0)  # apply along the zeroeth dimension


# make sine wave to test the scaling
#f_eeg_data_uV = np.sin(2.0*np.pi*(fs_Hz / (NFFT-1))*10.0*1.0*t_sec)
#f_eeg_data_uV = np.sqrt(2)*f_eeg_data_uV * np.sqrt(2.0)

# make time-domain plot
fig = plt.figure(figsize=(7.5, 9.25))  # make new figure, set size in inches

ax1 = plt.subplot(311)
t_sec = np.array(range(0, f_eeg_data_uV.size)) / fs_Hz
plt.plot(t_sec, f_eeg_data_uV)
plt.ylim(-100, 100)
plt.ylabel('EEG (uV)')
plt.xlabel('Time (sec)')
plt.title(fname)
plt.xlim(t_sec[0], t_sec[-1])


# make spectrogram
ax = plt.subplot(312, sharex=ax1)
overlap = NFFT - int(0.5 * fs_Hz)  # half-second steps
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
if (t_lim_sec[2-1] != 0):
    plt.xlim(t_lim_sec)
plt.ylim(f_lim_Hz)
plt.xlabel('Time (sec)')
plt.ylabel('Frequency (Hz)')


# add annotation for FFT Parameters
ax.text(1.0-0.025, 0.95,
        "NFFT = " + str(NFFT) + "\nfs = " + str(int(fs_Hz)) + " Hz",
        transform=ax.transAxes,
        verticalalignment='top',
        horizontalalignment='right',
        backgroundcolor='w')

# find spectra that are in our time span
foo_spec = spec_PSDperBin
if (alpha_lim_sec[2-1] != 0):
    foo_spec = foo_spec[:, ((t >= alpha_lim_sec[0]) & (t <= alpha_lim_sec[1]))]
    
    # add markers on the figure
    yl = ax.get_ylim()
    plt.plot(alpha_lim_sec[0]*np.array([1, 1]), yl, 'k--', linewidth=2);
    plt.plot(alpha_lim_sec[1]*np.array([1, 1]), yl, 'k--', linewidth=2);

# get the mean spectra and convert from PSD to uVrms
mean_spectra_PSDperBin = np.mean(foo_spec, 1);
mean_spectra_uVrmsPerSqrtBin = np.sqrt(mean_spectra_PSDperBin)

# plot the mean spectra
ax = plt.subplot(313)
plt.plot(freqs, mean_spectra_uVrmsPerSqrtBin, '.-')
plt.xlim(f_lim_Hz)
plt.ylim([0, 6])
plt.xlabel('Frequency (Hz)')
plt.ylabel('RMS Amplitude\n(uV per sqrt(Bin))')


# find maximum value and put it on the chart
bool_inds = (freqs > 7) & (freqs < 12)
foo_uVrmsPerSqrtBin = mean_spectra_uVrmsPerSqrtBin[bool_inds]
foo_Hz = freqs[bool_inds]
signal_rms = np.max(foo_uVrmsPerSqrtBin)
max_ind = np.argmax(foo_uVrmsPerSqrtBin)
signal_freq_Hz = foo_Hz[max_ind]
units_txt = 'uVrms per sqrt(bin)'
df_Hz = freqs[2]-freqs[1]
alpha_Hz = np.array([foo_Hz[max_ind],  foo_Hz[max_ind]])
if (NFFT == int(fs_Hz)):
    units_txt = 'uVrms'
if 0:
    # refine by computing the amplitude based on the total power around the peak
    #alpha_Hz = np.array([7.5, 11])
    #bool_alpha = (freqs > alpha_Hz[0]) & (freqs < alpha_Hz[1])
    #alpha_spectrum_PSDperBin = mean_spectra_PSDperBin[bool_alpha]
    inds = np.where(bool_inds) # find the non-zero indices
    inds = inds[0]  # get rid of tuple...why, I don't know I need to do this
    inds = inds[max_ind] + np.array([-1, 0, 1]) # include one index to each side
    alpha_spectrum_PSDperBin = mean_spectra_PSDperBin[inds] # ge the values
    alpha_PSD = np.sum(alpha_spectrum_PSDperBin) # sum the power in each bin
    signal_rms = np.sqrt(alpha_PSD) # convert from power to voltage
    units_txt = "uVrms"  # clarify the units
    
    # compute power-weighted average frequency in the alpha
    weight_fac = alpha_spectrum_PSDperBin / np.sum(alpha_spectrum_PSDperBin)
    #signal_freq_Hz = np.sum(freqs[bool_alpha] * weight_fac)
    signal_freq_Hz = np.sum(freqs[inds] * weight_fac)
    alpha_Hz = freqs[[inds[0], inds[-1]]] + df_Hz*np.array([-0.5, 0.5])

# assess noise
noiseband_Hz = np.array([20.0, 40.0])
foo_PSDperBin = mean_spectra_PSDperBin[(freqs > noiseband_Hz[0]) & (freqs < noiseband_Hz[1])]
mean_noise_uVrmsPerSqrtBin = np.sqrt(np.mean(foo_PSDperBin))

print 'Alpha Amplitude ' + str(round(signal_rms, 4)) + " " + units_txt
print 'Alpha Frequency ' + str(round(signal_freq_Hz, 4)) + ' Hz'
print 'Mean Noise ' + str(round(mean_noise_uVrmsPerSqrtBin, 4)) + ' ' + units_txt
print 'SNR ' + str(round(20*np.log10(signal_rms / mean_noise_uVrmsPerSqrtBin), 4)) + ' dB'

# add markings showing the "alpha" that was assessed
if (alpha_Hz[0] != alpha_Hz[1]):
    plt.plot(alpha_Hz[0]*np.array([1, 1]), np.array([0, signal_rms]), 'k--')
    plt.plot(alpha_Hz[1]*np.array([1, 1]), np.array([0, signal_rms]), 'k--')

# add annotation for the alpha amplitude
plt.plot(signal_freq_Hz, signal_rms, 'bo', linewidth=2)
ax.text(signal_freq_Hz, signal_rms+0.2,
        str(round(signal_rms, 2)) + " " + units_txt + '\n' +
        'at ' + str(round(signal_freq_Hz, 2)) + " Hz",
        verticalalignment='bottom',
        horizontalalignment='center')
        
# add marking and annotation for noise
plt.plot(noiseband_Hz,mean_noise_uVrmsPerSqrtBin*np.array([1, 1]),'k--', linewidth=2);
ax.text(np.mean(noiseband_Hz), mean_noise_uVrmsPerSqrtBin+0.5,
        "Mean Noise\n" + str(round(mean_noise_uVrmsPerSqrtBin, 2)) + " " + units_txt,
        verticalalignment='bottom',
        horizontalalignment='center')
        
# add generic annotation about the FFT processing
ax.text(1.0-0.025, 0.95,
        "NFFT = " + str(NFFT) + "\nfs = " + str(int(fs_Hz)) + " Hz",
        transform=ax.transAxes,
        verticalalignment='top',
        horizontalalignment='right',
        backgroundcolor='w')        

plt.tight_layout()
