import serial
import matplotlib.pyplot as plt
import numpy as np
import csv
import sys
from datetime import datetime

# COM Port Parameters
BAUD = 9600                     # Serial port baud rate (should only change if Arduino firmware is changed)
TIMEOUT_S = 2                   # Timeout for serial port read (ideally longer than timeout for pulse measurement)

# Latency calibration
LATENCY_CAL_OFFSET = 0.008901          # Offset for measured latency
LATENCY_CAL_MUL = 1.011429             # Multiplier for measured latency

# Control Flags
PLOT_DATA = True                # Control whether data is plotted
PRINT_TO_CONSOLE = True         # Control whether data is printed to the console
LOG_DATA = True                 # Control whether data is logged to a .csv file

# Plot parameters
HIST_MIN = 0                    # Histogram minimum value for bins
HIST_MAX = 200                  # Histogram maximum value for bins
NUM_BINS = 100                  # Total number of histogram bins
EVENT_DUR_S = 0.01              # Duration to plot a toggle for (in s)
EVENT_PLOT_LEN_S = 10           # Total length of the scrolling event plot (in s)

HIST_COLOR = '#76b900'          # Color for histogram (NV green)
MOUSE_HEIGHT = 1                # Height to plot for mouse click event
MOUSE_COLOR = 'blue'            # Color to plot for mouse click event
PHOTON_HEIGHT = 1               # Height to plot for photon output event
PHOTON_COLOR = 'lightblue'      # Color to plot for photon output event
PIN_HEIGHT = 1                  # Height to plot for pin interrupt event
PIN_COLOR = 'red'               # Color to plot for pin interrupt event

# Logging parameters
LOG_PREFIX = 'Logs/log'         # Prefix for logging directory (can relocate logs using this)

# Create serial port and setup histogram
if(len(sys.argv) > 1): com = sys.argv[1]
else: raise Exception("Need to provide COM port as argument to call!")
ser = serial.Serial(com, BAUD, timeout=TIMEOUT_S)
delays = []
bin_range = np.linspace(HIST_MIN, HIST_MAX, 100)

# Opening print to console
if PRINT_TO_CONSOLE: print("Starting logging (GUI will open when logging data is available)")

# Handle creating log file (and writing header here)
if LOG_DATA:
    fname = LOG_PREFIX + "_" + datetime.now().strftime('%y-%m-%d_%H%M%S') + ".csv"
    csvFile = open(fname, mode='w')
    logger = csv.writer(csvFile, delimiter=',', lineterminator='\n')
    logger.writerow(['Timestamp [s]', 'Latency [ms]', 'Event'])

toggle_time = []
toggle_event = []
mouse_time = []
mouse_event = []
photon_time = []
photon_event = []

# Clean out any data waiting for input
ser.flush()

# Setup the window and add title
fig = plt.figure()
fig.canvas.set_window_title("Latency Logger")

# This is the main loop that handles data aquisition and plotting
while(True):
    line = ser.readline().decode('utf-8').strip()

    # Check that the line is valid (enough for now)
    if not ":" in line: continue

    # Get the timestamp (in s)
    timestamp_s = float(line.split(':')[0])/1000000.0
    
    # Check for timeouts (and remove them) we don't need to log (for now)
    if "Timeout" in line:
        # Update all the time series here
        toggle_time.append(timestamp_s)
        toggle_event.append(0)
        mouse_time.append(timestamp_s)
        mouse_event.append(0)
        photon_time.append(timestamp_s)
        photon_event.append(0)

    # Check for pin toggle event here, and write to log/add to time plot
    elif "Pin Toggle" in line:
        if PRINT_TO_CONSOLE: print("Pin toggle @ {0:0.3f}s".format(timestamp_s))
        if LOG_DATA: logger.writerow([timestamp_s, "", "Pin Toggle"])
        toggle_event.extend([0,PIN_HEIGHT,0])
        toggle_time.extend([timestamp_s-EVENT_DUR_S, timestamp_s, timestamp_s+EVENT_DUR_S])
        continue
    else:
        # Parse delay measurement and perform calibration
        delay_ms = float(line.split(':')[1].replace('ms', ''))
        delay_ms = LATENCY_CAL_MUL*delay_ms + LATENCY_CAL_OFFSET
        if PRINT_TO_CONSOLE: print("Read @ {0:0.3f}s: Latency = {1:.3f}ms".format(timestamp_s, delay_ms))
        delays.append(delay_ms)
        # Add photon events to stream
        photon_event.extend([0,PHOTON_HEIGHT,0])
        photon_time.extend([timestamp_s-EVENT_DUR_S, timestamp_s, timestamp_s+EVENT_DUR_S])
        # Add mouse events to stream
        mtime = timestamp_s - delay_ms/1000
        mouse_event.extend([0,MOUSE_HEIGHT,0])
        mouse_time.extend([mtime-EVENT_DUR_S, mtime, mtime+EVENT_DUR_S])
        # Add nothing to toggle events (nothing occurred)
        toggle_event.append(0)
        toggle_time.append(timestamp_s)

        # Write to the log if desired
        if LOG_DATA: logger.writerow([timestamp_s, delay_ms, "Click to Photon"])

    # Plot latency to the histogram
    if PLOT_DATA:
        # Get rid of the old figure(s)
        plt.clf()

        # Create the histogram
        plt.subplot(2,1,1)
        fig.canvas.set_window_title("Event Logger")
        ax = plt.hist(delays, bins=bin_range, color=HIST_COLOR)
        plt.xlabel('Latency [ms]')
        plt.ylabel('Occurences')
        plt.title('Latency Histogram')
        
        # Create a scrolling time plot for events
        ax = plt.subplot(2,1,2)
        plt.plot(toggle_time, toggle_event, color=PIN_COLOR)
        plt.plot(mouse_time, mouse_event, color=MOUSE_COLOR)
        plt.plot(photon_time, photon_event, color=PHOTON_COLOR)
        plt.title('Event Plot')
        plt.ylim(0,1.5*max([MOUSE_HEIGHT, PHOTON_HEIGHT, PIN_HEIGHT]))
        plt.xlim(timestamp_s-EVENT_PLOT_LEN_S, timestamp_s)
        plt.ylabel('Events')
        plt.xlabel('Time (s)')
        ax.axes.get_yaxis().set_ticklabels([])
        plt.legend(['Toggle', 'Mouse', 'Photon'], loc='upper center', ncol=3)

        # Plot maintainance and update
        plt.tight_layout()
        plt.pause(0.1)
        if not plt.get_fignums(): break

plt.show()






