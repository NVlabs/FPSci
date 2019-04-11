from event_logger_interface import EventLoggerInterface
from datetime import datetime
import matplotlib
import matplotlib.pyplot as plt
import numpy as np
import sys
import time
import csv

# Operating parameters
BIT_DEPTH = 8                   # This sets the grayscale bit depth to use
TEST_STRIDE = 32                # Number of values to "step" per test
IM_SIZE = 100                   # Sets the size of the square image (in pixels)
FIG_SIZE_IN = 3                 # Set the figure size in inches
DPI = 100                       # Set figure resolution in DPI
MEASURE_TIME_S = 0.5            # Time to measure each level
COUNT_DOWN = 5                  # Time to count down before starting test

DO_COUNTDOWN = True             # Whether or not to perform the countdown
USE_MA = True                   # Use an MA filter on the data?

LOG_TO_CSV = True               # Log to a csv file
CSV_FILE_PREFIX = 'Logs/gtglog' # Common prefix for all logs

RESAMPLE_RATE_HZ = 10000        # This is the (interpolation) resampling rate in Hz
FILT_WINDOW_SAMPS = 20          # This is the lentgh of the filter window in samples

PAUSE_TIME = 0.001              # Argument to plt.pause() throughout
RESET_SLEEP_TIME_S = 0.1        # Time to sleep after new image is shown

# Hardware parameters
BAUD = 115200                   # Communication baud rate
MAX_ADC_VALUE = 1023            # Maximum value reported by ADC

# Derived parameters
NUM_LEVELS = 2**BIT_DEPTH       # Number of binary levels
LEVEL_COUNT = int(round(NUM_LEVELS/TEST_STRIDE)) # Number of colors to test

# Storage for transition time matrix
tmatrix = np.zeros([LEVEL_COUNT, LEVEL_COUNT])
initialized = False                     # Initialization flag

def makeImage(color):
    return np.ones(np.array([IM_SIZE, IM_SIZE, 3]))*color/NUM_LEVELS

def simple_ma(data, window):
        out = []
        for i in range(len(data)-window):
                out.append(np.mean(data[i:i+window-1]))
        out.extend(np.repeat(out[i], window))
        return out

def get_transition_time(data, low_level=None, high_level=None, avgWindow=None):
        t = data[0]
        data = np.array(data[1])
        total_range = max(data)-min(data)
        if low_level is None: low_level = min(data) + 0.1*total_range
        if high_level is None: high_level = max(data)-0.1*total_range
        if avgWindow is not None: avgWindow = int(avgWindow)
        tstart = None
        inInterval = False
        for i in range(len(data)-1):
                # Check we are not in the transition interval
                if not inInterval:
                        if avgWindow is not None:
                                if (np.mean(data[i-avgWindow:i]) < low_level and np.mean(data[i+1:i+avgWindow]) >= low_level) or (np.mean(data[i-avgWindow:i]) > high_level and np.mean(data[i+1:i+avgWindow]) <= high_level):
                                        tstart = t[i+1]
                                        idxStart = i+1
                                        inInterval = True
                        else:
                                if (data[i] < low_level and data[i+1] >= low_level) or (data[i] > high_level and data[i+1] <= high_level):
                                        tstart = t[i+1]
                                        idxStart = i+1
                                        inInterval = True
                else:
                        if(data[i] > low_level and data[i+1] <= low_level) or (data[i] < high_level and data[i+1] >= high_level):
                                return t[i+1] - tstart, idxStart, i+1, low_level, high_level
        return None, None, None, low_level, high_level

def move_figure(f, x, y):
    """Move figure's upper left corner to pixel (x, y)"""
    backend = matplotlib.get_backend()
    if backend == 'TkAgg':
        f.canvas.manager.window.wm_geometry("+%d+%d" % (x, y))
    elif backend == 'WXAgg':
        f.canvas.manager.window.SetPosition((x, y))
    else:
        f.canvas.manager.window.move(x, y)

# Create hardware interface
if(len(sys.argv) > 1): com = sys.argv[1]
else: raise Exception("Need to provide COM port as argument to call!")
hwInterface = EventLoggerInterface(com, BAUD)
print("Testing transitions over", NUM_LEVELS, "different grayscale levels")

# Create the figures/axes for plotting
f1 = plt.figure(figsize=(FIG_SIZE_IN, FIG_SIZE_IN), dpi=DPI)
move_figure(f1, 0, 0)
ax1 = f1.add_subplot(1,1,1)
ax1.set_title('GtG Image')

f2 = plt.figure(figsize=(2*FIG_SIZE_IN, FIG_SIZE_IN), dpi=DPI)
move_figure(f2, 1.5*FIG_SIZE_IN*DPI, 0)
ax21 = f2.add_subplot(2,1,1)
ax22 = f2.add_subplot(2,1,2)
plt.tight_layout()

f3 = plt.figure(figsize=(FIG_SIZE_IN, FIG_SIZE_IN), dpi=DPI)
move_figure(f3, 4*FIG_SIZE_IN*DPI, 0)
ax3 = f3.add_subplot(1,1,1)
ax3.set_title('Transition Matrix')
plt.pause(PAUSE_TIME)

# Get an array of test values
testVals = np.linspace(0, NUM_LEVELS-TEST_STRIDE, LEVEL_COUNT)

# Optionally create the csv log file
if LOG_TO_CSV:
        fname = CSV_FILE_PREFIX+"_"+datetime.now().strftime('%y-%m-%d_%H%M%S')+'.csv'
        csvFile = open(fname, mode='w')
        logger = csv.writer(csvFile, delimiter=',', lineterminator='\n')
        logger.writerow(np.append(np.array([' ']), testVals))

# Print the count-down to the console
if DO_COUNTDOWN:
        print("Test starting in:")
        for i in range(COUNT_DOWN):
                print(COUNT_DOWN-i)
                time.sleep(1)
print("STARTING")    

hwInterface.flush()  # Flush the HW before we start

# Setup 2 loops to cover the space
for fromColor in testVals:
    fromim = makeImage(fromColor)               # Create "from" image
    toColors = np.linspace(fromColor, NUM_LEVELS-TEST_STRIDE, LEVEL_COUNT-int(round(fromColor/TEST_STRIDE)))
    for toColor in toColors:
        if fromColor == toColor: continue       # Skip color to same color "transition"
        toim = makeImage(toColor)               # Create "to" image  
        
        ############################################
        # Core Measurement ("Tightly" Timed)
        ############################################

        hwInterface.flush()                                     # Flush the hardware here for capture (unused first run)

        # Show from image and get average value
        ax1.imshow(fromim)                                              # Show the "from image"
        ax1.set_axis_off()                                              # Clear the axis
        f1.canvas.draw()
        plt.pause(PAUSE_TIME)                                           # Update the plot
        if initialized: trans1 = hwInterface.get_analog_values(MEASURE_TIME_S/2, flush=False) # Get the "double" measurement here
        time.sleep(RESET_SLEEP_TIME_S)                                  # Wait a bit
        fromData = hwInterface.get_analog_values(MEASURE_TIME_S)        # Record MEASURE_TIME_S worth of data
        fromVals = fromData[1]; fromVal = np.mean(fromVals)             # Compute the "fromVals" and their mean

        # Transition measurement
        hwInterface.flush()                     # Clear the buffer
        ax1.imshow(toim)                        # Show the new image
        f1.canvas.draw()
        plt.pause(PAUSE_TIME)                   # Update the plot
        trans2 = hwInterface.get_analog_values(time_window_s=MEASURE_TIME_S/2, flush=False) # Capture the transition here

        # Get average value over "to values"
        toData = hwInterface.get_analog_values(MEASURE_TIME_S)          # Record another MEASURE_TIM_S worth of data
        toVals = toData[1]; toVal = np.mean(toVals)                     # Compute the "toVals" and their mean

        ############################################
        # End Core Measurement
        ############################################

        # Interpolate and smooth, then calculate transition time
        trans2[0] = np.array(trans2[0])-min(trans2[0])                                  # Offset to get time to start at 0
        dur2 = max(trans2[0])-min(trans2[0])                                            # Get "clip" duration
        t2 = np.linspace(0, dur2, num=int(round(dur2*RESAMPLE_RATE_HZ)))                # Create a new (regular) time base
        y2 = np.interp(t2, trans2[0], trans2[1])                                        # (Linearly) interpolate the data onto the new timebase
        if USE_MA: yf2 = simple_ma(y2, FILT_WINDOW_SAMPS)                               # Filter (if desired)
        else: yf2 = y2
        r2 = abs(fromVal - toVal)                                                       # Get "average" range
        ll2 = min(fromVal+0.1*r2, toVal+0.1*r2)                                         # Get 10% level
        hl2 = max(fromVal-0.1*r2, toVal-0.1*r2)                                         # Get 90% level
        ttime2,istart2,iend2,ll2,hl2 = get_transition_time([t2,yf2], ll2, hl2)            # Get transition time and start/end index        
        
        # Create transition plot data
        if ttime2 is not None:                           # Update markers if found transition
                m2 = np.zeros(len(t2))-1
                m2[istart2] = yf2[istart2]
                m2[iend2] = yf2[iend2]
        ylim2 = 1.1*min(MAX_ADC_VALUE, max(y2))
        s2 = np.zeros(len(t2))-1                    # Update "sample" markers (i.e. gray background)
        for trans in trans2[0]: s2[int(trans*RESAMPLE_RATE_HZ)-1] = ylim2
        
        if initialized:                                                                 # Repeat it all again for 1st transition (if initialized)
                trans1[0] = np.array(trans1[0])-min(trans1[0])
                dur1 = max(trans1[0])-min(trans1[0])
                t1 = np.linspace(0, dur1, num=int(round(dur1*RESAMPLE_RATE_HZ)))
                y1 = np.interp(t1, trans1[0], trans1[1])
                if USE_MA: yf1 = simple_ma(y1, FILT_WINDOW_SAMPS)
                else: yf1 = y1
                r1 = abs(lastVal-fromVal)
                ll1 = min(lastVal+0.1*r1, fromVal+0.1*r1)
                hl1 = max(lastVal-0.1*r1, fromVal-0.1*r1)
                ttime1, istart1, iend1, ll1, hl1 = get_transition_time([t1,yf1], ll1, hl1)

                        # Create transition plot data
                if ttime1 is not None:                           # Update markers if found transition
                        m1 = np.zeros(len(t1))-1
                        m1[istart1] = yf1[istart1]
                        m1[iend1] = yf1[iend1]
                ylim1 = 1.1*min(MAX_ADC_VALUE, max(y1))
                s1 = np.zeros(len(t1))-1                    # Update "sample" markers (i.e. gray background)
                for trans in trans1[0]: s1[int(trans*RESAMPLE_RATE_HZ)-1] = ylim1

                print("\"Back Transitioned\" from {0:0.2f} ({1} --> {2:0.2f} ({3})".format(lastVal, lastColor, fromVal, fromColor))
                if ttime1 is not None: print("\tTransition time = ", ttime1*1000.0, "ms")
        
         # Print statistics to the console
        print("Transitioned from {0:0.2f} ({1}) --> {2:0.2f} ({3})".format(fromVal, fromColor, toVal, toColor))
        print("\tFrom vals:", min(fromVals), "-", max(fromVals))
        print("\tTo vals:", min(toVals), "-", max(toVals))
        if ttime2 is not None: print("\tTransition time = ", ttime2*1000.0, "ms")
        
        # Do the time-domain plotting here
        ax22.clear()
        ax22.set_title('Transition Plot')
        ax22.plot(t2, s2, color='lightgray')
        ax22.plot(t2, np.repeat(ll2,len(t2)), color='red', linestyle='dashed')
        ax22.plot(t2, np.repeat(hl2,len(t2)), color='red', linestyle='dashed')
        ax22.plot(t2, y2, color='yellow')
        if ttime2 is not None: ax22.plot(t2, m2, color='green')
        ax22.plot(t2, yf2, color='black')
        ax22.set_xlim(0, dur2)
        ax22.set_ylim(0, ylim2)
        plt.tight_layout()
        # Repeat all the plots above if we are measuring double transition
        if initialized:
                ax21.clear()
                ax21.set_title('Back Transition Plot')
                ax21.plot(t1, s1, color='lightgray')
                ax21.plot(t1, np.repeat(ll1,len(t1)), color='red', linestyle='dashed')
                ax21.plot(t1, np.repeat(hl1,len(t1)), color='red', linestyle='dashed')
                ax21.plot(t1, y1, color='yellow')
                if ttime1 is not None: ax21.plot(t1, m1, color='green')
                ax21.plot(t1, yf1, color='black')
                ax21.set_xlim(0, dur1)
                ax21.set_ylim(0, ylim1)
        f2.canvas.draw()

        # Log the ttime into matrix and update display
        if initialized: tmatrix[int(lastColor/TEST_STRIDE)][int(fromColor/TEST_STRIDE)] = ttime1   
        tmatrix[int(fromColor/TEST_STRIDE)][int(toColor/TEST_STRIDE)] = ttime2
        ax3.imshow(tmatrix, cmap='hot')
        f3.canvas.draw()

        # Manage tracking stuff from last trial here
        lastVal = toVal
        lastColor = toColor
        initialized = True

        plt.pause(PAUSE_TIME)

    if LOG_TO_CSV: logger.writerow(np.append(np.array([fromColor]),tmatrix[int(fromColor/TEST_STRIDE)]))

print("TESTING COMPLETE!")