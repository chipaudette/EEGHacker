
import matplotlib.pyplot as plt
import numpy as np
from helperFunctions import loadAndFilterData, convertToFreqDomain, assessAlphaAndGuard, findTrueAndFalseDetections
        

# some program constants
fs_Hz = 250.0   # assumed sample rate for the EEG data
f_lim_Hz = [0, 30]      # frequency limits for plotting

# frequency-based processing parameters
NFFT = 256      # pick the length of the fft
overlap = NFFT - 50  # fixed step of 50 points
alpha_band_Hz = np.array([7.5, 11.5])   # where to look for the alpha peak
noise_band_Hz = np.array([14.0, 20.0])  # was 20-40 Hz
guard_band_Hz = np.array([[3.0, 6.5], [13.0, 18.0]])

# detection rule sets to use
all_use_rule = np.array([1, 2, 3, 4])  # detection rules

# prepare for scanning across all detection thresholds
#thresh1 = np.arange(0.0,6.0,0.05)  #threshold for alpha amplitude
#thresh2 = np.arange(0.0,6.0,0.05)  #threshold for guard amplitude, or alpha_guard_ratio
thresh1 = np.arange(0.0,10.0,0.1)  #threshold for alpha amplitude
thresh2 = np.arange(0.0,6.0,0.05)  #threshold for guard amplitude, or alpha_guard_ratio
N_true = np.zeros([thresh1.size, thresh2.size, all_use_rule.size])
N_false = np.zeros([thresh1.size, thresh2.size, all_use_rule.size])
N_possible = np.zeros([all_use_rule.size, 1])

# loop over each data file
filesToProcess = np.array([1, 2, 3, 4, 5, 6])
for Ifile in range(filesToProcess.size):
    print "Processing " + str(Ifile+1) + " of " + str(filesToProcess.size)    
    
    # define some default values that will get overwritten in a moment
    t_lim_sec = [0, 0]      # default plot time limits [0,0] will be ignored
    alpha_lim_sec = [[0, 0]]  # default
    t_other_sec = [0, 0]    # default
    
    # define which data to load
    case = filesToProcess[Ifile]  # choose which case to load
    pname = 'SavedData/'
    if (case == 1):
        fname = 'openBCI_raw_2014-10-04_18-50-20_RightForehead_countebackby3.txt'
        t_lim_sec = [0, 138]
        alpha_lim_sec = [[37.5, 51.7],[107.8, 110.0]]
    elif (case == 2):
        fname = 'openBCI_raw_2014-10-04_18-55-41_O1_Alpha.txt'
        #t_lim_sec = [0, 85]
        t_lim_sec = [2, 91]
        alpha_lim_sec = [[11.5, 30.8], [53, 80.8]]
    elif (case == 3):
        fname = 'openBCI_raw_2014-10-04_19-06-13_O1_Alpha.txt'
        t_lim_sec = [2, 83]
        #alpha_lim_sec = [58-2, 76+2]  
        alpha_lim_sec = [[45, 76+2]]
    elif (case == 4):
        fname = 'openBCI_raw_2014-10-05_17-14-45_O1_Alpha_noCaffeine.txt'
        t_lim_sec = [50, 125]  # could go [10, 125]
        alpha_lim_sec = [[87.75, 118.5]]
    elif (case == 5):
        pname = "../2014-05-31 RobotControl/SavedData/"
        fname = "openBCI_raw_2014-05-31_20-57-51_Robot05.txt"
        t_lim_sec = [100, 210]
        alpha_lim_sec = [[116, 123]]
    elif (case == 6):
        pname = "../2014-05-08 Multi-Rate Visual Evoked Potentials/SavedData/"
        fname = "openBCI_raw_2014-05-08_20-26-47_EyesClosedSeperates_Left_Right_Left_Right_Both.txt"    
        t_lim_sec = [133, 307]
        alpha_lim_sec = [[152, 167], [199, 212], [241, 264], [273.5, 295.2]]
    
    
    # %% load and process data
    
    # load and filter
    f_eeg_data_uV = loadAndFilterData(pname+fname, fs_Hz)
    
    # convert to frequency domain
    full_spec_PSDperBin, full_t_spec, freqs = convertToFreqDomain(f_eeg_data_uV, fs_Hz, NFFT, overlap)
    
    # focus on Alpha and guard bands
    alpha_max_uVperSqrtBin, guard_mean_uVperSqrtBin, alpha_guard_ratio = assessAlphaAndGuard(full_t_spec, freqs, full_spec_PSDperBin, alpha_band_Hz, guard_band_Hz)
    
    # process using the pre-defined detection rules
    for Irule in range(all_use_rule.size):
        use_rule = all_use_rule[Irule]
        print "Using rule " + str(Irule)
    
        # loop over different detection thresholds
        N_true_foo = np.zeros([thresh1.size, thresh2.size])
        N_false_foo = np.zeros([thresh1.size, thresh2.size])
        bool_inTime = (full_t_spec >= t_lim_sec[0]) & (full_t_spec <= t_lim_sec[1])
        bool_inTrueTime = np.zeros(full_t_spec.shape,dtype='bool')
        for lim_sec in alpha_lim_sec:
            bool_inTrueTime = bool_inTrueTime | ((full_t_spec >= lim_sec[0]) & (full_t_spec <= lim_sec[1]))    
        bool_inTrueTime =bool_inTrueTime[bool_inTime]
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
                
                #if (Irule==1) & (I1 == 9) & (I2==89):
                #    print "I1, I2 = " + str(I1) + " " + str(I2) + ", N_true_foo = " + str(N_true_foo[I1,I2])
                
        # accumulate results for each rule, summed across files
        #I1 = 9
        #I2 = 89
        #print "Repeat: I1, I2 = " + str(I1) + " " + str(I2) + ", N_true_foo = " + str(N_true_foo[I1,I2])
        N_true[:, :, Irule] += N_true_foo
        N_false[:, :, Irule] += N_false_foo
        N_possible[Irule] += N_possible_foo
        #print "Again: I1, I2, Irule = " + str(I1) + " " + str(I2) + " " + str(Irule) + ", N_true = " + str(N_true[I1,I2,Irule])
        


# %% find best N_true for each value of N_false for each rule
plot_N_false = np.arange(0,200,1)
plot_best_N_true = np.zeros([plot_N_false.size,all_use_rule.size])
plot_best_N_frac = np.zeros(plot_best_N_true.shape)
plot_best_thresh1 = np.zeros(plot_best_N_true.shape)
plot_best_thresh2 = np.zeros(plot_best_N_true.shape)
for Irule in range(all_use_rule.size):
    use_rule = all_use_rule[Irule]
    N_true_foo = N_true[:, :, Irule] 
    N_false_foo = N_false[:, :, Irule]
    
    for I_N_false in range(plot_N_false.size):
        bool = (N_false_foo == plot_N_false[I_N_false]);
        if np.any(bool):
            
            plot_best_N_true[I_N_false, Irule] = np.max(N_true_foo[bool])
            
            foo = np.copy(N_true_foo)
            foo[~bool] = 0.0  # some small value to all values at a different N_false
            inds = np.unravel_index(np.argmax(foo), foo.shape)
            plot_best_thresh1[I_N_false, Irule] = thresh1[inds[0]]
            plot_best_thresh2[I_N_false, Irule] = thresh2[inds[1]]
            
        # never be smaller than the previous value
        if (I_N_false > 0):
            if (plot_best_N_true[I_N_false-1,Irule] > plot_best_N_true[I_N_false,Irule]):
                plot_best_N_true[I_N_false,Irule] = plot_best_N_true[I_N_false-1,Irule]
                plot_best_thresh1[I_N_false, Irule] = plot_best_thresh1[I_N_false-1, Irule]
                plot_best_thresh2[I_N_false, Irule] = plot_best_thresh2[I_N_false-1, Irule]
        
    plot_best_N_frac[:, Irule] = (plot_best_N_true[:, Irule]) / (N_possible[Irule])
 
fig = plt.figure(figsize=(6.5,9))  # make new figure, set size in inches
plt.subplot(311)
plt.plot(plot_N_false, plot_best_N_frac*100, linewidth=3)
if (all_use_rule.size < 4): 
    plt.legend(('Rule 1','Rule 2','Rule 3'),loc=4,fontsize='medium')
else:
    plt.legend(('Rule 1','Rule 2','Rule 3','Rule 4'),loc=4,fontsize='medium')

if (filesToProcess.size == 1):
    plt.title(fname[12:])
else:
    plt.title("Number of EEG Recordings = " + str(filesToProcess.size))
plt.xlabel('N_False')
plt.ylabel('Fraction of Eyes-Closed Data\nCorrectly Detected (%)')
max_N_false = 200
plt.xlim([0, max_N_false])
plt.ylim([0, 100.5])

plt.subplot(312)
plt.plot(plot_N_false, plot_best_thresh1, linewidth=3)
plt.xlabel('N_False')
plt.ylabel('Best Alpha Threshold\n(uVrms)')
plt.xlim([0, max_N_false])
plt.ylim([0, 5])

plt.subplot(313)
plt.plot(plot_N_false, plot_best_thresh2, linewidth=3)
plt.xlabel('N_False')
plt.ylabel('Best Thresh Rule 2\n(uVrms or Ratio)')
plt.xlim([0, max_N_false])
plt.ylim([0, 5])


plt.tight_layout()
