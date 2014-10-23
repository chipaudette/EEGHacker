
import matplotlib.pyplot as plt
import numpy as np
from helperFunctions import loadAndFilterData, convertToFreqDomain, assessAlphaAndGuard, findTrueAndFalseDetections, computeROC
   
def getFileInfo(case):
    t_lim_sec = [0, 0]      # default plot time limits [0,0] will be ignored
    alpha_lim_sec = [[0, 0]]  # default
    #t_other_sec = [0, 0]    # default
    
    # define which data to load
    case = filesToProcess[Ifile]  # choose which case to load
    pname = 'SavedData/'
    if (case == 1):
        fname = 'openBCI_raw_2014-10-04_18-50-20_RightForehead_countebackby3.txt'
        t_lim_sec = [0, 138]
        alpha_lim_sec = [[37.5, 51.7],[107.8, 110.0]]
    elif (case == 2):
        fname = 'openBCI_raw_2014-10-04_18-55-41_O1_Alpha.txt'
        # t_lim_sec = [0, 85]
        t_lim_sec = [2, 91]
        alpha_lim_sec = [[11.5, 30.8], [53, 80.8]]
    elif (case == 3):
        fname = 'openBCI_raw_2014-10-04_19-06-13_O1_Alpha.txt'
        t_lim_sec = [2, 83]
        # alpha_lim_sec = [58-2, 76+2]  
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
    
    return pname, fname, t_lim_sec, alpha_lim_sec
   

# some program constants
fs_Hz = 250.0   # assumed sample rate for the EEG data
f_lim_Hz = [0, 30]      # frequency limits for plotting

# frequency-based processing parameters
NFFT = 256      # pick the length of the fft
FFTstep = 50
overlap = NFFT - FFTstep  # fixed step of 50 points
alpha_band_Hz = np.array([7.5, 11.5])   # where to look for the alpha peak
noise_band_Hz = np.array([14.0, 20.0])  # was 20-40 Hz
guard_band_Hz = np.array([[3.0, 6.5], [13.0, 18.0]])

# detection rule sets to use
# all_use_rule = np.array([1, 2, 3, 4])  # detection rules

# define which files to process
filesToProcess = np.array([1, 2, 3, 4, 5, 6])

# prepare for scanning across all detection thresholds
thresh1 = np.arange(0.0,10.0,0.1)  #threshold for alpha amplitude
thresh2 = np.arange(0.0,6.0,0.1)  #threshold for guard amplitude, or alpha_guard_ratio
N_true_each = np.zeros([thresh1.size, thresh2.size, filesToProcess.size])
N_false_each = np.zeros([thresh1.size, thresh2.size, filesToProcess.size])
N_eyesClosed_each = np.zeros([filesToProcess.size, 1])  # number of data blocks within the "eyes closed" period
N_eyesOpen_each = np.zeros([filesToProcess.size, 1])

# loop over each data file
snames =[];  # use these text labels for the legend
for Ifile in range(filesToProcess.size):
    print "Processing " + str(Ifile+1) + " of " + str(filesToProcess.size)  
    snames.append('File ' + str(Ifile+1))
    
    # Get the info on the desired data file
    pname, fname, t_lim_sec, alpha_lim_sec = getFileInfo(filesToProcess[Ifile])
    
    # load and filter
    f_eeg_data_uV = loadAndFilterData(pname+fname, fs_Hz)
    
    # convert to frequency domain
    full_spec_PSDperBin, full_t_spec, freqs = convertToFreqDomain(f_eeg_data_uV, fs_Hz, NFFT, overlap)
    
    # focus on Alpha and guard bands
    alpha_max_uVperSqrtBin, guard_mean_uVperSqrtBin, alpha_guard_ratio = assessAlphaAndGuard(full_t_spec, freqs, full_spec_PSDperBin, alpha_band_Hz, guard_band_Hz)
    
    # process using the pre-defined detection rules
    use_rule = 2
    
    # loop over different detection thresholds and count the detections
    N_true_foo = np.zeros([thresh1.size, thresh2.size])
    N_false_foo = np.zeros([thresh1.size, thresh2.size])
    for I1 in range(thresh1.size):
        for I2 in range(thresh2.size):
            N_true_foo[I1,I2] , N_false_foo[I1,I2], N_eyesClosed_foo, N_eyesOpen_foo, bool_true, bool_false, bool_inTrueTime = findTrueAndFalseDetections(
                full_t_spec,
                alpha_max_uVperSqrtBin,
                guard_mean_uVperSqrtBin,
                alpha_guard_ratio,
                t_lim_sec,
                alpha_lim_sec,
                use_rule,
                thresh1[I1],
                thresh2[I2])
    N_true_each[:, :, Ifile] = N_true_foo
    N_false_each[:, :, Ifile] = N_false_foo
    N_eyesClosed_each[Ifile] = N_eyesClosed_foo
    N_eyesOpen_each[Ifile] = N_eyesOpen_foo
    #print "Again: I1, I2, Irule = " + str(I1) + " " + str(I2) + " " + str(Irule) + ", N_true = " + str(N_true[I1,I2,Irule])
    

# find best N_true for each value of N_false for each rule
plot_N_false = np.arange(0,200,1)
plot_best_N_true, plot_best_N_true_frac, plot_best_thresh1, plot_best_thresh2 = computeROC(N_true_each,
                                                                                      N_false_each,
                                                                                      N_eyesClosed_each,
                                                                                      thresh1,
                                                                                      thresh2,
                                                                                      plot_N_false)
                                                                                      
# lump together the other results
N_true_sum = np.sum(N_true_each,-1) # sum over the last dimension
N_false_sum = np.sum(N_false_each,-1) # sum over the last dimension
N_eyesClosed_sum = np.sum(N_eyesClosed_each) # sum over the last dimension
N_eyesOpen_sum = np.sum(N_eyesOpen_each) # sum over the last dimension
plot_best_N_sum_true, plot_best_N_true_sum_frac, plot_best_sum_thresh1, plot_best_sum_thresh2 = computeROC(N_true_sum,
                                                                                      N_false_sum,
                                                                                      N_eyesClosed_sum,
                                                                                      thresh1,
                                                                                      thresh2,
                                                                                      plot_N_false)

# %% more calculations on false alarms and such
# get example data at target thresh2
if 0:
    targ_thresh1 = 3.5
    targ_thresh2 = 2.5 # for rule 2 or 4
    I = np.argmin(np.abs(thresh2 - targ_thresh2))
else:
    targ_thresh1 = 3.54
    targ_thresh2 = 1.63 # for rule 2 or 4
I = np.argmin(np.abs(thresh2 - targ_thresh2))
targ_thresh2 = thresh2[I]
N_true_ex = np.squeeze(N_true_each[:,I,:])
N_false_ex = np.squeeze(N_false_each[:,I,:])
N_true_frac_ex = np.zeros(N_true_ex.shape)
for J in range(N_eyesClosed_each.size):
    N_true_frac_ex[:,J] = N_true_ex[:,J]/N_eyesClosed_each[J]
    
blocks_per_minute = fs_Hz / float(FFTstep) * 60.0
dur_eyesOpen_minute = N_eyesOpen_each / blocks_per_minute
N_false_ex_per_minute = N_false_ex  / (np.ones([N_false_ex.shape[0], 1])*np.transpose(dur_eyesOpen_minute))



# compute false alarm rate for full data
N_false_per_minute_each = np.zeros(N_false_each.shape)
for Icol in range(N_false_each.shape[1]):
    N_false_per_minute_each[:,Icol,:] = N_false_each[:,Icol,:] / (np.ones([N_false_each.shape[0], 1])*np.transpose(dur_eyesOpen_minute))
plot_N_false_per_minute = np.zeros([plot_N_false.size, dur_eyesOpen_minute.size])
for Irow in range(plot_N_false.size):
    foo_num = plot_N_false[Irow]*np.ones([1, dur_eyesOpen_minute.size])
    plot_N_false_per_minute[Irow, :] = foo_num / np.transpose(dur_eyesOpen_minute)

N_false_per_minute_sum = N_false_sum / N_eyesOpen_sum
dur_eyesOpen_sum_minute = N_eyesOpen_sum / blocks_per_minute
plot_N_false_sum_per_minute = plot_N_false / dur_eyesOpen_sum_minute

# %% plots
fig = plt.figure(figsize=(10.5,4.25))  # make new figure, set size in inches

ax = plt.subplot(1,2,1)
plt.plot(thresh1,N_true_frac_ex*100,linewidth=3)
plt.xlabel('Alpha Threshold (uVrms)')
plt.ylabel('Fraction of Correct Detections (%)')
plt.title('True Positive')
plt.legend(snames,loc=3,fontsize='medium')
plt.xlim([0, 7])
plt.ylim([0, 100])
plt.plot(targ_thresh1*np.array([1, 1]),ax.get_ylim(),'k--',linewidth=2)

ax.text(1-0.025, 0.95,
         "Detect Rule = " + str(use_rule) + "\nThresh2 = " + str(targ_thresh2),
         transform=ax.transAxes,
         verticalalignment='top',
         horizontalalignment='right',
         backgroundcolor='w')


ax = plt.subplot(1,2,2)
plt.plot(thresh1,N_false_ex_per_minute,linewidth=3)
plt.xlabel('Alpha Threshold (uVrms)')
plt.ylabel('Incorrect Detections per Minute')
plt.title('False Positive')
plt.xlim([0, 7])
plt.ylim([0, 40])
plt.legend(snames,loc=3,fontsize='medium')
targ_thresh1 = 3.5
plt.plot(targ_thresh1*np.array([1, 1]),ax.get_ylim(),'k--',linewidth=2)

ax.text(1-0.025, 0.95,
         "Detect Rule = " + str(use_rule) + "\nThresh2 = " + str(targ_thresh2),
         transform=ax.transAxes,
         verticalalignment='top',
         horizontalalignment='right',
         backgroundcolor='w')

plt.tight_layout()

 
# %% plot ROC         
      
for Iplot in range(2):
    if Iplot==0:
        foo_best_N_true_frac = plot_best_N_true_frac
        foo_N_false_per_minute = plot_N_false_per_minute
        foo_best_thresh1 = plot_best_thresh1
        foo_best_thresh2 = plot_best_thresh2
        lt = snames
    elif Iplot==1:
        foo_best_N_true_frac = plot_best_N_true_sum_frac
        foo_N_false_per_minute = plot_N_false_sum_per_minute
        foo_best_thresh1 = plot_best_sum_thresh1
        foo_best_thresh2 = plot_best_sum_thresh2
        lt = ["All Data"]
         
         
    fig = plt.figure(figsize=(10.5,4.25*1.9))  # make new figure, set size in inches
    
    ax = plt.subplot(2,2,1)
    plt.plot(foo_N_false_per_minute, foo_best_N_true_frac*100, linewidth=3)
    plt.legend(lt,loc=4,fontsize='medium')
    plt.title('ROC Curve for Alpha Detection')
    plt.xlabel('Incorrect Detections per Minute')
    plt.ylabel('Fraction of Correct Detections (%)')
    max_N_false = 20
    plt.xlim([0, max_N_false])
    plt.ylim([0, 100.5])
    
    ax.text(0.025, 0.05,
         "Detect Rule = " + str(use_rule),
         transform=ax.transAxes,
         verticalalignment='bottom',
         horizontalalignment='left',
         backgroundcolor='w')
    
    
    
    plt.subplot(2,2,3)
    plt.plot(foo_N_false_per_minute, foo_best_thresh1, linewidth=3)
    plt.xlabel('Incorrect Detections per Minute')
    plt.ylabel('Threshold (uVrms)')
    plt.title('Best Alpha Threshold')
    plt.xlim([0, max_N_false])
    plt.ylim([0, 5])
    
    plt.subplot(2,2,4)
    plt.plot(foo_N_false_per_minute, foo_best_thresh2, linewidth=3)
    plt.xlabel('Incorrect Detections per Minute')
    plt.ylabel('Threshold (uVrms)')
    plt.title('Best Guard Threshold (uVrms)')
    if (use_rule == 3):
        plt.ylabel('Ratio (uVrms/uVrms)')
        plt.title('Best Alpha/Guard Ratio')
    plt.xlim([0, max_N_false])
    plt.ylim([0, 5])
    
    plt.tight_layout()
