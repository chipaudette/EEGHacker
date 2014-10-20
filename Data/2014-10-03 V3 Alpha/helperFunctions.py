import matplotlib.mlab as mlab
import numpy as np
from scipy import signal

def loadAndFilterData(full_fname, fs_Hz):
    # load data into numpy array
    data = np.loadtxt(full_fname,
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

def assessAlphaAndGuard(full_t_spec, freqs, full_spec_PSDperBin, alpha_band_Hz, guard_band_Hz):
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
    bool_inTrueTime = np.zeros(full_t_spec.shape,dtype='bool')
    for lim_sec in alpha_lim_sec:
        bool_inTrueTime = bool_inTrueTime | ((full_t_spec >= lim_sec[0]) & (full_t_spec <= lim_sec[1]))    
    bool_inTrueTime =bool_inTrueTime[bool_inTime]
 
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
