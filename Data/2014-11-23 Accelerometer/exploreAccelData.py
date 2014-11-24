# ######################
#
# exploreAccelData.py
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

# define which data to load
case = 1  # choose which case to load
pname = 'SavedData/'
if (case == 1):
    fname = 'OpenBCI-RAW-2014-11-23_18-54-57.txt'

    
# load data into numpy array
fs_Hz = 250.0        # assumed sample rate for the EEG data
data = np.loadtxt(pname + fname,
                  delimiter=',',
                  skiprows=5)

# parse the data
data_indices = data[:, 0]   # the first column is the packet index
eeg_data_uV = data[:, 1:(8+1)]   # ignore the first channel (column 0), so channel 1 is column 1
accel_data_G = data[:, 9:(11+1)]/1000.0/9.806  #acceleromater data...convert mm/s2 to G
mag_accel_G = np.sqrt(accel_data_G[:,0]**2 + accel_data_G[:,1]**2 + accel_data_G[:,2]**2)

# check data indices
d_indices = data_indices[2:]-data_indices[1:-1]
n_jump = np.count_nonzero((d_indices != 1) & (d_indices != -255))
print("Number of Discontinuities in the packet counter: " + str(n_jump))


## # 
fig = plt.figure(figsize=(10.0, 9.5))  # make new figure, set size in inches
n_row = 3+1

t_sec = np.arange(len(accel_data_G[:, 0])) / fs_Hz
for Iplot in range(3):
    if (Iplot)==0:
        ax1 = plt.subplot(n_row,1,Iplot+1)
        ax = ax1
    else:
        ax = plt.subplot(n_row,1,Iplot+1, sharex=ax1)
            
    plt.plot(t_sec,accel_data_G[:, Iplot])
    plt.xlabel("Time (sec)")       
    plt.ylabel("Acceleration (G)")
    plt.title("Channel " + str(Iplot))

ax = plt.subplot(n_row,1,4, sharex=ax1)
plt.plot(t_sec,mag_accel_G)
plt.xlabel("Time (sec)")       
plt.ylabel("Acceleration (G)")
plt.title("Magnitude")
plt.ylim([0, 1.2])


plt.tight_layout()

