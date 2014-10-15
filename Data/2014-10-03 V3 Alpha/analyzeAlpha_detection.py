
import matplotlib.pyplot as plt
import matplotlib.mlab as mlab
import numpy as np
from scipy import signal


# some program constants
fs_Hz = 250.0   # assumed sample rate for the EEG data
f_lim_Hz = [0, 30]      # frequency limits for plotting

# frequency-based processing parameters
NFFT = 256      # pick the length of the fft
alpha_band_Hz = np.array([7.5, 11.5])   # where to look for the alpha peak
noise_band_Hz = np.array([14.0, 20.0])  # was 20-40 Hz
guard_band_Hz = np.array([[3.0, 6.5], [13.0, 18.0]])

# detectection parameters
use_detect_rules = 1    # 1 = Alpha only, 2 = Alpha and Guard Thres, 3 = Alpha and Ratio
det_thresh_uV = 3.5     # 3.5 for NFFT = 256, 2.5 for NFFT = 512
guard_thresh_uV = 2.5
det_thresh_ratio = 3.5

# define some default values that will get overwritten in a moment
t_lim_sec = [0, 0]      # default plot time limits [0,0] will be ignored
alpha_lim_sec = [0, 0]  # default
t_other_sec = [0, 0]    # default

# define which data to load
case = 4  # choose which case to load
pname = 'SavedData/'
if (case == 1):
    fname = 'openBCI_raw_2014-10-04_18-50-20_RightForehead_countebackby3.txt'
    t_lim_sec = [0, 138]
    alpha_lim_sec = [37.5, 51.7]
elif (case == 2):
    fname = 'openBCI_raw_2014-10-04_18-55-41_O1_Alpha.txt'
    #t_lim_sec = [0, 85]
    t_lim_sec = [30, 91]
    alpha_lim_sec = [53, 80]
elif (case == 3):
    fname = 'openBCI_raw_2014-10-04_19-06-13_O1_Alpha.txt'
    t_lim_sec = [15, 83]
    #alpha_lim_sec = [58-2, 76+2]  
    alpha_lim_sec = [45, 76+2]
elif (case == 4):
    fname = 'openBCI_raw_2014-10-05_17-14-45_O1_Alpha_noCaffeine.txt'
    t_lim_sec = [50, 125]
    if 1:
        #normal analysis
        alpha_lim_sec = [87.75, 118.5]
    else:
        #compare eyes-closed to interference
        alpha_lim_sec = [108.0, 112.0]
        t_other_sec = [120.0, 124.0]  # burst of interference


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

# %% MAIN PLOTS
t_sec = np.array(range(0, f_eeg_data_uV.size)) / fs_Hz

# compute spectrogram
fig = plt.figure(figsize=(7.5, 9.25))  # make new figure, set size in inches
ax1 = plt.subplot(311)
overlap = NFFT - 50  # fixed step of 50 points
spec_PSDperHz, freqs, t_spec = mlab.specgram(np.squeeze(f_eeg_data_uV),
                               NFFT=NFFT,
                               window=mlab.window_hanning,
                               Fs=fs_Hz,
                               noverlap=overlap
                               ) # returns PSD power per Hz
                               
# convert the units of the spectral data
spec_PSDperBin = spec_PSDperHz * fs_Hz / float(NFFT)  # convert to "per bin"
del spec_PSDperHz  # remove this variable so that I don't mistakenly use it

# reduce size of spectrogram data to speed plotting,
# but keep high-resolution data for later analysis
full_spec_PSDperBin = spec_PSDperBin
full_t_spec = t_spec
spec_PSDperBin = full_spec_PSDperBin[:, 1:-1:2]  # get every other time slice
t_spec = full_t_spec[1:-1:2]  # get every other time slice

# make the spectrogram plot
plt.pcolor(t_spec, freqs, 10*np.log10(spec_PSDperBin))  # dB re: 1 uV
plt.clim(25-5+np.array([-40, 0]))
plt.xlim(t_sec[0], t_sec[-1])
if (t_lim_sec[2-1] != 0):
    plt.xlim(t_lim_sec)
plt.ylim(f_lim_Hz)
plt.xlabel('Time (sec)')
plt.ylabel('Frequency (Hz)')
plt.title(fname[12:])



# add annotation for FFT Parameters
cl=plt.gci().get_clim();
ax1.text(0.025, 0.95,
        "NFFT = " + str(NFFT) + "\nfs = " + str(int(fs_Hz)) + " Hz\nClim = [" + str(cl[0]) + ", " + str(cl[1]) + "]",
        transform=ax1.transAxes,
        verticalalignment='top',
        horizontalalignment='left',
        backgroundcolor='w',
        size='smaller')



# find spectra that are in our time span
foo_spec = spec_PSDperBin
bool_ind = ((t_spec >= alpha_lim_sec[0]) & (t_spec <= alpha_lim_sec[1]))
foo_spec = foo_spec[:, bool_ind]
   
# get the mean spectra and convert from PSD to uVrms
mean_spectra_PSDperBin = np.mean(foo_spec, 1);
mean_spectra_uVrmsPerSqrtBin = np.sqrt(mean_spectra_PSDperBin)

# show the eyes-closed alpha period
yl = ax1.get_ylim()
plt.plot(alpha_lim_sec[0]*np.array([1, 1]), yl, 'w--', linewidth=3)
plt.plot(alpha_lim_sec[1]*np.array([1, 1]), yl, 'w--', linewidth=3)
ax1.text(np.mean(alpha_lim_sec), yl[1]-0.05*(yl[1]-yl[0]), 'Eyes Closed',
         verticalalignment='top',
         horizontalalignment='center',
         backgroundcolor='w')
  
# show the "other" period
if (t_other_sec[1] != 0):
    yl = ax1.get_ylim()
    plt.plot(t_other_sec[0]*np.array([1.0, 1.0]), yl, 'w--', linewidth=3)
    plt.plot(t_other_sec[1]*np.array([1.0, 1.0]), yl, 'w--', linewidth=3)
    ax1.text(np.mean(t_other_sec), yl[1]-0.05*(yl[1]-yl[0]), 'Other',
             verticalalignment='top',
             horizontalalignment='center',
             backgroundcolor='w')

# compute the mean spectra for not-Alpha period
foo_spec = spec_PSDperBin
if (t_other_sec[1] != 0):
    bool_ind = ((t_spec >= t_other_sec[0]) & (t_spec <= t_other_sec[1]))
else:
    bool_ind = ((t_spec >= t_lim_sec[0]) & (t_spec <= t_lim_sec[1]) & ~bool_ind)
foo_spec = foo_spec[:, bool_ind]
mean_spectra_noise_PSDperBin = np.mean(foo_spec, 1);
mean_spectra_noise_uVrmsPerSqrtBin = np.sqrt(mean_spectra_noise_PSDperBin)


# plot the mean spectra...make two similar plots
for Iplot in [0, 1]:
    ax = plt.subplot(312+Iplot)

    #plot all frequencies
    plt.plot(freqs, mean_spectra_uVrmsPerSqrtBin, 'k.-',linewidth=2)
    plt.xlim(f_lim_Hz)
    plt.ylim([0, 6])
    plt.xlabel('Frequency (Hz)')
    plt.ylabel('RMS Amplitude\n(uV per sqrt(Bin))')    
    plt.title('Mean Spectrum for "Eyes Closed" EEG Data')

    if (t_other_sec[1] > 0):
        #change title
        plt.title('Comparing Mean Spectrum for Different Activities')        
        
        #add the "other" signal
        plt.plot(freqs,mean_spectra_noise_uVrmsPerSqrtBin,'r.-',linewidth=2) 
        
        
        # add legend
        plt.legend(('Eyes Closed', 'Other'), loc=1, fontsize='medium')
                
        
#        # highlight the guard
#        bool_inds = ((freqs > guard_band_Hz[0, 0]) & (freqs < guard_band_Hz[0, 1]))
#        plt.plot(freqs[bool_inds], mean_spectra_uVrmsPerSqrtBin[bool_inds], 'go-',linewidth=4)    
#        bool_inds = ((freqs > guard_band_Hz[1, 0]) & (freqs < guard_band_Hz[1, 1]))
#        plt.plot(freqs[bool_inds], mean_spectra_uVrmsPerSqrtBin[bool_inds], 'go-',linewidth=4)
               
    else:
        #highlight the alpha
        bool_inds = ((freqs > alpha_band_Hz[0]) & (freqs < alpha_band_Hz[1]))
        plt.plot(freqs[bool_inds], mean_spectra_uVrmsPerSqrtBin[bool_inds], 'bo-', linewidth=3)    
    
    # add markings showing the "alpha" that was assessed
    plt.plot(alpha_band_Hz[0]*np.array([1, 1]), ax.get_ylim(), 'k--', linewidth=2)
    plt.plot(alpha_band_Hz[1]*np.array([1, 1]), ax.get_ylim(), 'k--', linewidth=2)
    yl = ax.get_ylim();
    ax.text(np.mean(alpha_band_Hz),yl[1]-0.05*(yl[1]-yl[0]),'Alpha\nBand',
            verticalalignment = 'top',
            horizontalalignment = 'center',
            color='b',
            weight='bold');
            

         
    if (Iplot==1):                    
        
        #add markings showing the guard
        plt.plot(guard_band_Hz[0, 0]*np.array([1, 1]), ax.get_ylim(), 'k:', linewidth=2)
        plt.plot(guard_band_Hz[0, 1]*np.array([1, 1]), ax.get_ylim(), 'k:', linewidth=2)
        plt.plot(guard_band_Hz[1, 0]*np.array([1, 1]), ax.get_ylim(), 'k:', linewidth=2)
        plt.plot(guard_band_Hz[1, 1]*np.array([1, 1]), ax.get_ylim(), 'k:', linewidth=2)
        yl = ax.get_ylim();
        ax.text(np.mean(guard_band_Hz[0, :]),yl[1]-0.05*(yl[1]-yl[0]),'Guard\nBand',
                verticalalignment = 'top',
                horizontalalignment = 'center',
                backgroundcolor='w', color = 'g',weight='bold');   
        ax.text(np.mean(guard_band_Hz[1, :]),yl[1]-0.05*(yl[1]-yl[0]),'Guard\nBand',
                verticalalignment = 'top',
                horizontalalignment = 'center',
                backgroundcolor='w', color='g',weight='bold');   
       
    # add generic annotation about the FFT processing
#    ax.text(1.0-0.025, 0.95,
#            "NFFT = " + str(NFFT) + "\nfs = " + str(int(fs_Hz)) + " Hz",
#            transform=ax.transAxes,
#            verticalalignment='top',
#            horizontalalignment='right',
#            backgroundcolor='w')  
       

plt.tight_layout()


# %% Plot the amplitude of alpha vs time

# compute alpha vs time
bool_inds = (freqs > alpha_band_Hz[0]) & (freqs < alpha_band_Hz[1])
alpha_max_uVperSqrtBin = np.sqrt(np.amax(full_spec_PSDperBin[bool_inds, :], 0))
# alpha_sum_uVrms = np.sqrt(np.sum(full_spec_PSDperBin[bool_inds, :],0))

bool_inds = ((freqs > guard_band_Hz[0][0]) & (freqs < guard_band_Hz[0][1]) |
             (freqs > guard_band_Hz[1][0]) & (freqs < guard_band_Hz[1][1]))
guard_mean_uVperSqrtBin = np.sqrt(np.mean(full_spec_PSDperBin[bool_inds, :], 0))
ratio = alpha_max_uVperSqrtBin / guard_mean_uVperSqrtBin

# make figure window
fig = plt.figure(figsize=(7.5, 9.25))  # make new figure, set size in inches

# make spectrogram (again)
ax1 = plt.subplot(311)
plt.pcolor(t_spec, freqs, 10*np.log10(spec_PSDperBin))  # dB re: 1 uV
#plt.pcolor(t_spec, freqs, 10*np.log10(spec_PSDperBin), cmap='CMRmap')  # dB re: 1 uV
#plt.clim(25-5+1.5+np.array([-40, 0]))
plt.clim(25-5+np.array([-40, 0]))
plt.xlim([t_sec[0], t_sec[-1]])
if (t_lim_sec[2-1] != 0):
    plt.xlim(t_lim_sec)
plt.ylim(f_lim_Hz)
plt.xlabel('Time (sec)')
plt.ylabel('Frequency (Hz)')
plt.title(fname[12:])

 
# add lines showing alpha time
if (alpha_lim_sec[1] != 0):
    yl = ax1.get_ylim()
    plt.plot(alpha_lim_sec[0]*np.array([1, 1]), yl, 'k--', linewidth=3)
    plt.plot(alpha_lim_sec[1]*np.array([1, 1]), yl, 'k--', linewidth=3)
    ax1.text(np.mean(alpha_lim_sec), yl[1]-0.05*(yl[1]-yl[0]), 'Eyes Closed',
             verticalalignment='top',
             horizontalalignment='center',
             backgroundcolor='w')
             
# add alpha and guard bands
#xl = ax1.get_xlim()
#plt.plot(xl, alpha_band_Hz[0]*np.array([1, 1]), 'k--', linewidth=2)
#plt.plot(xl, alpha_band_Hz[1]*np.array([1, 1]), 'k--', linewidth=2)
#plt.plot(xl, guard_band_Hz[0, 0]*np.array([1, 1]), 'k:', linewidth=2)
#plt.plot(xl, guard_band_Hz[0, 1]*np.array([1, 1]), 'k:', linewidth=2)
#plt.plot(xl, guard_band_Hz[1, 0]*np.array([1, 1]), 'k:', linewidth=2)
#plt.plot(xl, guard_band_Hz[1, 1]*np.array([1, 1]), 'k:', linewidth=2)


# add annotation for FFT Parameters
cl=plt.gci().get_clim();
ax1.text(0.025, 0.95,
        "NFFT = " + str(NFFT) + "\nfs = " + str(int(fs_Hz)) + " Hz\nClim = [" + str(cl[0]) + ", " + str(cl[1]) + "]",
        transform=ax1.transAxes,
        verticalalignment='top',
        horizontalalignment='left',
        backgroundcolor='w',
        size='smaller')


# make time-domain plot of alpha amplitude
if (use_detect_rules != 2):
    ax = plt.subplot(312, sharex=ax1)
else:
    ax = plt.subplot(313, sharex=ax1)
plt.plot(full_t_spec, alpha_max_uVperSqrtBin, '.-')
plt.ylim([0, 12])
plt.xlim([t_sec[0], t_sec[-1]])
if (t_lim_sec[2-1] != 0):
    plt.xlim(t_lim_sec)
plt.xlabel('Time (sec)')
plt.ylabel('Alpha Band\nEEG Amplitude (uVrms)')
plt.title('Alpha Band = [' + str(alpha_band_Hz[0]) + ' to ' + str(alpha_band_Hz[1]) + '] Hz')


# add detection
detect_txt = ''
if (det_thresh_uV > 0.0):
    #add detection threshold
    plt.plot(ax.get_xlim(),det_thresh_uV * np.array([1, 1]),'r--',linewidth=2)
    
    if (use_detect_rules == 1):
        # simply based on Alpha amplitude
        detect_txt = "Detect If > " + str(det_thresh_uV) + " uVrms"
        bool_ind = ((alpha_max_uVperSqrtBin >= det_thresh_uV) & ((full_t_spec >= t_lim_sec[0]) & (full_t_spec <= t_lim_sec[1])))
        plt.plot(full_t_spec[bool_ind],
                 alpha_max_uVperSqrtBin[bool_ind],
                 'ro', linewidth=3)
    elif (use_detect_rules==2):
        # Alpha amplitude with guard rejection
        detect_txt = ("Detect If Alpha > " + str(det_thresh_uV) + " uVrms\n" +
                      "and Guard < "+ str(guard_thresh_uV) + " uVrms")
        bool_ind = ( (alpha_max_uVperSqrtBin >= det_thresh_uV) &
                     (guard_mean_uVperSqrtBin > guard_thresh_uV) &
                     ((full_t_spec >= t_lim_sec[0]) & (full_t_spec <= t_lim_sec[1])) )
        plt.plot(full_t_spec[bool_ind],
                 alpha_max_uVperSqrtBin[bool_ind],
                 'gx', markeredgewidth=2)
        bool_ind = ( (alpha_max_uVperSqrtBin >= det_thresh_uV) & 
                     (guard_mean_uVperSqrtBin < guard_thresh_uV) &
                     ((full_t_spec >= t_lim_sec[0]) & (full_t_spec <= t_lim_sec[1])) )
        plt.plot(full_t_spec[bool_ind],
                 alpha_max_uVperSqrtBin[bool_ind],
                 'ro', linewidth=2)
    elif (use_detect_rules==3):
        # Alpha amplitude with alpha/guard ratio requirement
        detect_txt = ("Detect If Alpha > " + str(det_thresh_uV) + " uVrms\n" +
                      "and Alpha/Guard Ratio > "+ str(det_thresh_ratio))
        bool_ind = ( (alpha_max_uVperSqrtBin >= det_thresh_uV) &
                     (ratio < det_thresh_ratio) &
                     ((full_t_spec >= t_lim_sec[0]) & (full_t_spec <= t_lim_sec[1])) )
        plt.plot(full_t_spec[bool_ind],
                 alpha_max_uVperSqrtBin[bool_ind],
                 'gx', markeredgewidth=2)
        bool_ind = ( (alpha_max_uVperSqrtBin >= det_thresh_uV) & 
                     (ratio > det_thresh_ratio) &
                     ((full_t_spec >= t_lim_sec[0]) & (full_t_spec <= t_lim_sec[1])) )
        plt.plot(full_t_spec[bool_ind],
                 alpha_max_uVperSqrtBin[bool_ind],
                 'ro', linewidth=2)               
    
    # describe type of detection logic
    ax.text(0.025, 0.95, detect_txt,
            transform=ax.transAxes,
            verticalalignment='top',
            horizontalalignment='left',
            backgroundcolor='w')
    
    # declare sensitivity and false alarms
    n_good = sum(bool_ind & ((full_t_spec >= alpha_lim_sec[0]) & (full_t_spec <= alpha_lim_sec[1])))
    n_eyes_closed = sum((full_t_spec >= alpha_lim_sec[0]) & (full_t_spec <= alpha_lim_sec[1]))
    n_bad = sum(bool_ind & ~((full_t_spec >= alpha_lim_sec[0]) & (full_t_spec <= alpha_lim_sec[1])))
    n_eyes_open = sum((full_t_spec >= t_lim_sec[0]) & (full_t_spec <= t_lim_sec[1]) & ~(full_t_spec >= alpha_lim_sec[0]) & (full_t_spec <= alpha_lim_sec[1]))
    print "N_true = " + str(n_good)
    print "N_eyes_closed = " + str(n_eyes_closed)
    print "N_false = " + str(n_bad)
    print "N_eyes_open = " + str(n_eyes_open)
            
# add lines showing alpha time
if (alpha_lim_sec[1] != 0):
    yl = ax.get_ylim()
    plt.plot(alpha_lim_sec[0]*np.array([1, 1]), yl, 'k--', linewidth=3)
    plt.plot(alpha_lim_sec[1]*np.array([1, 1]), yl, 'k--', linewidth=3)
    ax.text(np.mean(alpha_lim_sec), yl[1]-0.05*(yl[1]-yl[0]), 'Eyes Closed',
            verticalalignment='top',
            horizontalalignment='center',
            backgroundcolor='w')

# plt.legend(('Sum In-Band', 'Max In-Band'), loc=2, fontsize='medium')
# plt.legend(['Alpha Band'], loc=2, fontsize='medium')


# make time-domain plot of alpha and guard amplitude
if (use_detect_rules != 2):
    ax = plt.subplot(313, sharex=ax1)
else:
    ax = plt.subplot(312, sharex=ax1)
#plt.plot(full_t_spec, alpha_sum_uVrms, '.-',
#         full_t_spec, alpha_max_uVperSqrtBin, '.-')
if (use_detect_rules == 3):
    y = ratio
    yt = "Alpha / Guard Ratio\n(uVrms/uVrms)"
    tt = 'Ratio of EEG Amplitude: Alpha Band to Guard Band'
    ythresh = det_thresh_ratio
else:
    y = guard_mean_uVperSqrtBin
    yt = "Guard Band\nEEG Amplitude (uVrms)"
    tt = ('Guard Band = [' +
          str(guard_band_Hz[0, 0]) + ' to ' + str(guard_band_Hz[0, 1]) + '] ' +
          'and [' + str(guard_band_Hz[1, 0]) + ' to ' +
          str(guard_band_Hz[1, 1]) + '] Hz')
    ythresh = guard_thresh_uV
     
     
plt.plot(full_t_spec, y, 'g.-')
if (use_detect_rules == 2):
    bool_ind = (y > ythresh)
    plt.plot(full_t_spec[bool_ind], y[bool_ind], 'gx', markeredgewidth=2)
    ax.text(0.025, 0.95, "Reject if Guard > " + str(ythresh) + " uVrms",
        transform=ax.transAxes,
        verticalalignment='top',
        horizontalalignment='left',
        backgroundcolor='w')
plt.ylim([0, 12])
plt.xlim([t_sec[0], t_sec[-1]])
if (t_lim_sec[2-1] != 0):
    plt.xlim(t_lim_sec)
plt.xlabel('Time (sec)')
plt.ylabel(yt)
plt.title(tt)

# add threshold
if (ythresh > 0.0):
    plt.plot(ax.get_xlim(),ythresh * np.array([1, 1]),'r--',linewidth=2)
    


# add lines showing alpha time
if (alpha_lim_sec[1] != 0):
    yl = ax.get_ylim()
    plt.plot(alpha_lim_sec[0]*np.array([1, 1]), yl, 'k--', linewidth=3)
    plt.plot(alpha_lim_sec[1]*np.array([1, 1]), yl, 'k--', linewidth=3)
    ax.text(np.mean(alpha_lim_sec), yl[1]-0.05*(yl[1]-yl[0]), 'Eyes Closed',
            verticalalignment='top',
            horizontalalignment='center',
            backgroundcolor='w')



# plt.legend(['Guard Band'], loc=2, fontsize='medium')


plt.tight_layout()




# %% discriminator
fig = plt.figure(figsize=(15.25, 8.25))  # make new figure, set size in inches
ax1 = plt.subplot(221)
alpha_bool_inds = (full_t_spec > alpha_lim_sec[0]) & (full_t_spec < alpha_lim_sec[1])
noise_bool_inds = (full_t_spec > t_lim_sec[0]) & (full_t_spec < t_lim_sec[1]) & ~alpha_bool_inds
if 0:
    plt.plot(alpha_max_uVperSqrtBin[alpha_bool_inds], guard_mean_uVperSqrtBin[alpha_bool_inds],'bo', linewidth=2);
    plt.plot(alpha_max_uVperSqrtBin[noise_bool_inds], guard_mean_uVperSqrtBin[noise_bool_inds],'rs', linewidth=2);
    plt.xlabel('Alpha Band (uVrms)')
    plt.ylabel('Guard Band (uVrms)')
else:
    plt.plot(guard_mean_uVperSqrtBin[alpha_bool_inds],alpha_max_uVperSqrtBin[alpha_bool_inds], 'bo', linewidth=2);
    plt.plot(guard_mean_uVperSqrtBin[noise_bool_inds],alpha_max_uVperSqrtBin[noise_bool_inds], 'rs', linewidth=2);
    plt.ylabel('Alpha Band (uVrms)')
    plt.xlabel('Guard Band (uVrms)')

plt.title(fname[12:])
if 0:
    # log
    ax1.set_yscale('log')
    ax1.set_xscale('log')
    plt.xlim([0.5,20])
    plt.ylim([1,20])
else:
    #plt.xlim([0, 12])
    #plt.ylim([0, 8])
    plt.ylim([0, 10])
    plt.xlim([0, 6])

# add original threshold
#plt.plot(det_thresh_uV * np.array([1, 1]),ax.get_ylim(), 'k--',linewidth=2)
plt.plot(ax1.get_xlim(), det_thresh_uV * np.array([1, 1]), 'k--',linewidth=2)

#add threshold for ratio
yl=ax1.get_ylim()
if (use_detect_rules == 2):
    plt.plot(guard_thresh_uV * np.array([1, 1]), yl,'r--',linewidth=2)
elif (use_detect_rules == 3):
    plt.plot(np.array(yl) / np.array(det_thresh_ratio), yl,'r--',linewidth=2)

#ax1.text(0.025,1-0.05, detect_txt,
#        transform=ax1.transAxes,
#        verticalalignment='top',
#        horizontalalignment='left',
#        backgroundcolor='w')

# add legend
plt.legend(['Eyes Closed', 'Other'], loc=1, fontsize='medium')

# plot ratio
ax1 = plt.subplot(222)
plt.plot(alpha_max_uVperSqrtBin[alpha_bool_inds], ratio[alpha_bool_inds],'bo', linewidth=2);
plt.plot(alpha_max_uVperSqrtBin[noise_bool_inds], ratio[noise_bool_inds],'rs', linewidth=2);
plt.xlabel('Alpha Band (uVrms)')
plt.ylabel('Ratio Alpha / Guard  (uVrms/uVrms)')
plt.title(fname[12:])
if 0:
    # log
    ax1.set_yscale('log')
    ax1.set_xscale('log')
    plt.xlim([0.5, 100])
    plt.ylim([0.5, 100])
else:
    plt.xlim([0, 10])
    plt.ylim([0, 8])

# add original threshold
plt.plot(det_thresh_uV * np.array([1, 1]),ax1.get_ylim(), 'k--',linewidth=2)

if (det_thresh_ratio > 0.0):
    plt.plot(ax1.get_xlim(),det_thresh_ratio * np.array([1, 1]),'r--',linewidth=2)



# add legend
plt.legend(['Eyes Closed', 'Other'], loc=2, fontsize='medium')


# plot ratio again
ax1 = plt.subplot(223)
plt.plot(guard_mean_uVperSqrtBin[alpha_bool_inds], ratio[alpha_bool_inds],'bo', linewidth=2);
plt.plot(guard_mean_uVperSqrtBin[noise_bool_inds], ratio[noise_bool_inds],'rs', linewidth=2);
plt.xlabel('Guard Band (uVrms)')
plt.ylabel('Ratio Alpha / Guard  (uVrms/uVrms)')
plt.title(fname[12:])
if 0:
    # log
    ax1.set_yscale('log')
    ax1.set_xscale('log')
    plt.xlim([0.5, 30])
    plt.ylim([0.5, 12])
else:
    plt.xlim([0, 6])
    plt.ylim([0, 8])

# add original threshold
# plt.plot(det_thresh_uV * np.array([1, 1]),ax.get_ylim(), 'k--',linewidth=2)

if (det_thresh_ratio > 0.0):
    plt.plot(ax1.get_xlim(),det_thresh_ratio * np.array([1, 1]),'r--',linewidth=2)


# add legend
plt.legend(['Eyes Closed', 'Other'], loc=1, fontsize='medium')




# plot ratio of alpha to guard
ax = plt.subplot(224)
plt.plot(full_t_spec,ratio,'.-')
plt.ylabel('Ratio Alpha / Guard (uVrms/uVrms)')
plt.xlabel('Time (sec)')
plt.title(fname[12:])
plt.xlim([t_sec[0], t_sec[-1]])
plt.ylim([0, 14])
if (t_lim_sec[2-1] != 0):
    plt.xlim(t_lim_sec)

#add detected points
bool_ind = (alpha_max_uVperSqrtBin > det_thresh_uV) & (ratio > det_thresh_ratio)
plt.plot(full_t_spec[bool_ind],ratio[bool_ind],'ro',linewidth=2)

# add lines showing alpha time
if (alpha_lim_sec[1] != 0):
    plt.plot(alpha_lim_sec[0]*np.array([1, 1]), ax.get_ylim(), 'k--', linewidth=3);
    plt.plot(alpha_lim_sec[1]*np.array([1, 1]), ax.get_ylim(), 'k--', linewidth=3);

if (det_thresh_ratio > 0.0):
    plt.plot(ax.get_xlim(),det_thresh_ratio * np.array([1, 1]),'r--',linewidth=2)


plt.tight_layout()
