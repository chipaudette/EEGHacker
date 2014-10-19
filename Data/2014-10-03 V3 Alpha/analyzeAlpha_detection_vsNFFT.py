
import matplotlib.pyplot as plt
import matplotlib.mlab as mlab
import numpy as np
from scipy import signal

def loadAndFilterData(full_fname, fs_Hz, t_lim_sec, alpha_lim_sec):
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
    
    
    return f_eeg_data_uV

def convertToFreqDomain(f_eeg_data_uV, fs_Hz, NFFT, overlap):
    
    # make sine wave to test the scaling
    #f_eeg_data_uV = np.sin(2.0*np.pi*(fs_Hz / (NFFT-1))*10.0*1.0*t_sec)
    #f_eeg_data_uV = np.sqrt(2)*f_eeg_data_uV * np.sqrt(2.0)
    
    # compute spectrogram
    #fig = plt.figure(figsize=(7.5, 9.25))  # make new figure, set size in inches
    #ax1 = plt.subplot(311)
    spec_PSDperHz, freqs, t_spec = mlab.specgram(np.squeeze(f_eeg_data_uV),
                                   NFFT=NFFT,
                                   window=mlab.window_hanning,
                                   Fs=fs_Hz,
                                   noverlap=overlap
                                   ) # returns PSD power per Hz
                                   
    # convert the units of the spectral data
    spec_PSDperBin = spec_PSDperHz * fs_Hz / float(NFFT)  # convert to "per bin"
    del spec_PSDperHz  # remove this variable so that I don't mistakenly use it
    
    
    return spec_PSDperBin, t_spec, freqs

def assessAlphaAndGuard(full_t_spec, full_spec_PSDperBin, alpha_band_Hz, guard_band_Hz):
    # compute alpha vs time
    bool_inds = (freqs > alpha_band_Hz[0]) & (freqs < alpha_band_Hz[1])
    alpha_max_uVperSqrtBin = np.sqrt(np.amax(full_spec_PSDperBin[bool_inds, :], 0))
    # alpha_sum_uVrms = np.sqrt(np.sum(full_spec_PSDperBin[bool_inds, :],0))
    
    bool_inds = ((freqs > guard_band_Hz[0][0]) & (freqs < guard_band_Hz[0][1]) |
                 (freqs > guard_band_Hz[1][0]) & (freqs < guard_band_Hz[1][1]))
    guard_mean_uVperSqrtBin = np.sqrt(np.mean(full_spec_PSDperBin[bool_inds, :], 0))
    alpha_guard_ratio = alpha_max_uVperSqrtBin / guard_mean_uVperSqrtBin
    
    return alpha_max_uVperSqrtBin, guard_mean_uVperSqrtBin, alpha_guard_ratio

def findTrueAndFalseDetections(full_t_spec,
                               alpha_max_uVperSqrtBin,
                               guard_mean_uVperSqrtBin,
                               alpha_guard_ratio,
                               t_lim_sec,
                               alpha_lim_sec,
                               detection_rule_set,
                               thresh1,
                               thresh2):
                               
    bool_inTime = (full_t_spec >= t_lim_sec[0]) & (full_t_spec <= t_lim_sec[1])
    bool_inTrueTime = (full_t_spec[bool_inTime] >= alpha_lim_sec[0]) & (full_t_spec[bool_inTime] <= alpha_lim_sec[1])
 
    #all three rule sets test the alpha amplitude
    bool_alpha_thresh = (alpha_max_uVperSqrtBin > thresh1)
    
    #the second test changes depending upon the rule set
    if (detection_rule_set == 1):
        bool_detect = bool_alpha_thresh[bool_inTime]
    elif (detection_rule_set == 2):
        bool_detect = bool_alpha_thresh[bool_inTime] & (guard_mean_uVperSqrtBin[bool_inTime] < thresh2)
    elif (detection_rule_set == 3):
        bool_detect = bool_alpha_thresh[bool_inTime] & (alpha_guard_ratio[bool_inTime] > thresh2)
    elif (detection_rule_set == 4):
        bool_alpha_thresh[2:-1] = bool_alpha_thresh[1:-2] | bool_alpha_thresh[2:-1]  #copy "true" to next time as well
        bool_guard = guard_mean_uVperSqrtBin < thresh2
        bool_guard[2:-1] = bool_guard[1:-2] & bool_guard[2:-1]  #copy "false" to next time as well
        bool_detect = bool_alpha_thresh[bool_inTime] & bool_guard[bool_inTime]
            
        
    #count true or false detections
    bool_true = bool_detect & bool_inTrueTime
    N_true = np.count_nonzero(bool_true)
    N_possible = np.count_nonzero(bool_inTrueTime)  #number of potential True detections
    bool_false = bool_detect & ~bool_inTrueTime
    N_false = np.count_nonzero(bool_false)
    
    
    return N_true, N_false, N_possible, bool_true, bool_false, bool_inTrueTime
                
                

# some program constants
fs_Hz = 250.0   # assumed sample rate for the EEG data
f_lim_Hz = [0, 30]      # frequency limits for plotting

# frequency-based processing parameters
# all_NFFT = np.array([128, 256, 512, 1024])    # pick the length of the fft
all_NFFT = np.array([256])    # pick the length of the fft

FFT_step = 50  # fixed step of 50 points
alpha_band_Hz = np.array([7.5, 11.5])   # where to look for the alpha peak
noise_band_Hz = np.array([14.0, 20.0])  # was 20-40 Hz
guard_band_Hz = np.array([[3.0, 6.5], [13.0, 18.0]])

# detection rule sets to use
all_use_rule = np.array([2])  # detection rule

# prepare for scanning across all detection thresholds
if 0:
    thresh1 = np.arange(0.0,10.0,0.1)  #threshold for alpha amplitude
    thresh2 = np.arange(0.0,10.0,0.1)  #threshold for guard amplitude, or alpha_guard_ratio
else:
    thresh1 = np.arange(0.0,6.0,0.05)  #threshold for alpha amplitude
    thresh2 = np.arange(0.0,5.0,0.05)  #threshold for guard amplitude, or alpha_guard_ratio
N_true = np.zeros([thresh1.size, thresh2.size, all_NFFT.size])
N_false = np.zeros([thresh1.size, thresh2.size, all_NFFT.size])
N_possible = np.zeros([all_NFFT.size, 1])

# loop over each data file
filesToProcess = np.array([4])
for Ifile in range(filesToProcess.size):
    print "Processing " + str(Ifile+1) + " of " + str(filesToProcess.size)    
    
    # define some default values that will get overwritten in a moment
    t_lim_sec = [0, 0]      # default plot time limits [0,0] will be ignored
    alpha_lim_sec = [0, 0]  # default
    t_other_sec = [0, 0]    # default
    
    # define which data to load
    case = filesToProcess[Ifile]  # choose which case to load
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
    
    # %% load and process data
    
    # load and filter
    f_eeg_data_uV = loadAndFilterData(pname+fname, fs_Hz, t_lim_sec, alpha_lim_sec)
    
    # process using the pre-defined detection rules
    for I_NFFT in range(all_NFFT.size):
        NFFT = all_NFFT[I_NFFT]
        overlap = NFFT - FFT_step  # fixed step of 50 points
        print "Using NFFT = " + str(NFFT)
        
        # convert to frequency domain
        full_spec_PSDperBin, full_t_spec, freqs = convertToFreqDomain(f_eeg_data_uV, fs_Hz, NFFT, overlap)
    
        # focus on Alpha and guard bands
        alpha_max_uVperSqrtBin, guard_mean_uVperSqrtBin, alpha_guard_ratio = assessAlphaAndGuard(full_t_spec, full_spec_PSDperBin, alpha_band_Hz, guard_band_Hz)
        
        #apply detection rules
        use_rule = all_use_rule[0]
        #print "Using rule " + str(I_NFFT)
    
        # loop over different detection thresholds
        N_true_foo = np.zeros([thresh1.size, thresh2.size])
        N_false_foo = np.zeros([thresh1.size, thresh2.size])
        bool_inTime = (full_t_spec >= t_lim_sec[0]) & (full_t_spec <= t_lim_sec[1])
        bool_inTrueTime = (full_t_spec[bool_inTime] >= alpha_lim_sec[0]) & (full_t_spec[bool_inTime] <= alpha_lim_sec[1])
        for I1 in range(thresh1.size):
            for I2 in range(thresh2.size):
                #count detections
                N_true_foo[I1,I2] , N_false_foo[I1,I2], N_possible_foo, bool_true, bool_false, bool_inTrueTime = findTrueAndFalseDetections(
                    full_t_spec,
                    alpha_max_uVperSqrtBin,
                    guard_mean_uVperSqrtBin,
                    alpha_guard_ratio,
                    t_lim_sec,
                    alpha_lim_sec,
                    use_rule,
                    thresh1[I1],
                    thresh2[I2])
                
                #if (I_NFFT==1) & (I1 == 9) & (I2==89):
                #    print "I1, I2 = " + str(I1) + " " + str(I2) + ", N_true_foo = " + str(N_true_foo[I1,I2])
                
        # accumulate results for each rule, summed across files
        #I1 = 9
        #I2 = 89
        #print "Repeat: I1, I2 = " + str(I1) + " " + str(I2) + ", N_true_foo = " + str(N_true_foo[I1,I2])
        N_true[:, :, I_NFFT] += N_true_foo
        N_false[:, :, I_NFFT] += N_false_foo
        N_possible[I_NFFT] += N_possible_foo
        #print "Again: I1, I2, I_NFFT = " + str(I1) + " " + str(I2) + " " + str(I_NFFT) + ", N_true = " + str(N_true[I1,I2,I_NFFT])
        


# %% find best N_true for each value of N_false for each rule
plot_N_false = np.arange(0,200,1)
plot_best_N_true = np.zeros([plot_N_false.size,all_NFFT.size])
plot_best_N_frac = np.zeros(plot_best_N_true.shape)
plot_best_thresh1 = np.zeros(plot_best_N_true.shape)
plot_best_thresh2 = np.zeros(plot_best_N_true.shape)
for I_NFFT in range(all_NFFT.size):
    N_true_foo = N_true[:, :, I_NFFT] 
    N_false_foo = N_false[:, :, I_NFFT]
    
    for I_N_false in range(plot_N_false.size):
        bool = (N_false_foo == plot_N_false[I_N_false]);
        if np.any(bool):
            
            plot_best_N_true[I_N_false, I_NFFT] = np.max(N_true_foo[bool])
            
            foo = np.copy(N_true_foo)
            foo[~bool] = 0.0  # some small value to all values at a different N_false
            inds = np.unravel_index(np.argmax(foo), foo.shape)
            plot_best_thresh1[I_N_false, I_NFFT] = thresh1[inds[0]]
            plot_best_thresh2[I_N_false, I_NFFT] = thresh2[inds[1]]
            
        # never be smaller than the previous value
        if (I_N_false > 0):
            if (plot_best_N_true[I_N_false-1,I_NFFT] > plot_best_N_true[I_N_false,I_NFFT]):
                plot_best_N_true[I_N_false,I_NFFT] = plot_best_N_true[I_N_false-1,I_NFFT]
                plot_best_thresh1[I_N_false, I_NFFT] = plot_best_thresh1[I_N_false-1, I_NFFT]
                plot_best_thresh2[I_N_false, I_NFFT] = plot_best_thresh2[I_N_false-1, I_NFFT]
        
    plot_best_N_frac[:, I_NFFT] = (plot_best_N_true[:, I_NFFT]) / (N_possible[I_NFFT])
 
fig = plt.figure(figsize=(6.5,9))  # make new figure, set size in inches
ax=plt.subplot(311)
plt.plot(plot_N_false, plot_best_N_frac*100, linewidth=3)
if (all_NFFT.size > 2):
    plt.legend(('NFFT ' + str(all_NFFT[0]),
                'NFFT ' + str(all_NFFT[1]),
                'NFFT ' + str(all_NFFT[2])),
                loc=4,fontsize='medium')
    if (all_NFFT.size > 3):
                plt.legend(('NFFT ' + str(all_NFFT[0]),
                        'NFFT ' + str(all_NFFT[1]),
                        'NFFT ' + str(all_NFFT[2]),
                        'NFFT ' + str(all_NFFT[3])),
                        loc=4,fontsize='medium')

if (filesToProcess.size == 1):
    plt.title(fname[12:])
else:
    plt.title("Number of EEG Recordings = " + str(filesToProcess.size))
plt.xlabel('N_False')
plt.ylabel('Fraction of Eyes-Closed Data\nCorrectly Detected (%)')
plt.xlim([0, 50])
plt.ylim([0, 100.5])
ax.text(0.025,0.95,
       'Detect Rule = ' + str(use_rule),
       transform=ax.transAxes,
       verticalalignment='top',
       horizontalalignment='left')

ax=plt.subplot(312)
plt.plot(plot_N_false, plot_best_thresh1, linewidth=3)
plt.xlabel('N_False')
plt.ylabel('Best Alpha Threshold\n(uVrms)')
plt.xlim([0, 50])
plt.ylim([0, 6])
ax.text(1-0.025,0.95,
       'Detect Rule = ' + str(use_rule),
       transform=ax.transAxes,
       verticalalignment='top',
       horizontalalignment='right')

ax=plt.subplot(313)
plt.plot(plot_N_false, plot_best_thresh2, linewidth=3)
plt.xlabel('N_False')
plt.ylabel('Best Thresh Rule 2\n(uVrms or Ratio)')
plt.xlim([0, 50])
ax.text(1-0.025,0.95,
       'Detect Rule = ' + str(use_rule),
       transform=ax.transAxes,
       verticalalignment='top',
       horizontalalignment='right')
plt.ylim([0, 5])


plt.tight_layout()
