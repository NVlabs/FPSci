import matplotlib.pyplot as plt
import matplotlib.animation as animation
import numpy as np
import csv
import sys
import urllib.request
import codecs

# Click to photon parameters
CLICK_TO_PHOTON_THRESH_S = 0.3  # Maximum delay expected between click and photon
PRINT_TO_CONSOLE = True         # Optional print to console for mouse to photon events

# Plot parameters
HIST_MIN = 0                    # Histogram minimum value for bins
HIST_MAX = 50                   # Histogram maximum value for bins
NUM_BINS = 100                  # Total number of histogram bins
EVENT_DUR_S = 0.01              # Duration to plot a toggle for (in s)
EVENT_PLOT_LEN_S = 10           # Total length of the scrolling event plot (in s)

HIST_COLOR = '#76b900'          # Color for histogram (NV green)
MOUSE_HEIGHT = 1                # Height to plot for mouse click event
MOUSE1_COLOR = 'blue'           # Color to plot for mouse1 click event
MOUSE2_COLOR = 'lightblue'      # Color to plot for mouse 2 click event
PHOTON_HEIGHT = 1               # Height to plot for photon output event
PHOTON_COLOR = 'lightgreen'     # Color to plot for photon output event
PIN_HEIGHT = 1                  # Height to plot for pin interrupt event
PIN_COLOR = 'red'               # Color to plot for pin interrupt event
ADC_COLOR = 'black'             # Color to use for plotting of ADC values
bin_range = np.linspace(HIST_MIN, HIST_MAX, NUM_BINS)

# Create storage and lookups
m1Time = []; m1Event = []
m2Time = []; m2Event = []
pdTime = []; pdEvent = []
swTime = []; swEvent = []
aTime = []; aValue = []
delays = []

# Lookup tables for storage above
eventLookup = {"M1": m1Event, "M2": m2Event, "PD": pdEvent, "SW": swEvent}
timeLookup = {"M1": m1Time, "M2": m2Time, "PD": pdTime, "SW": swTime}

lastM1Time = 0          # Last time we saw an M1 event

# Simple button handler for autoclick control
# UNUSED, JUST STORED HERE FOR NOW
# def button_handler(event):
#     global hwInterface, autoclick, autoclick_count
#     autoclick_count = 0
#     if(check.get_status()[0] == True): 
#         hwInterface.autoclick_on()
#         autoclick = True
#     else:
#         hwInterface.autoclick_off()
#         autoclick = False

# Plot update routine for FuncAnimation
def update(i):
    global lastM1Time
    timestamp_s = None
    event_type = None

    # Parse lines out of event reader
    for line in eventReader:
        try: 
            timestamp_s = float(line[0])
            event_type = line[1]
        except: continue
        eventLookup[event_type].extend([0,MOUSE_HEIGHT,0])
        timeLookup[event_type].extend([timestamp_s-EVENT_DUR_S, timestamp_s, timestamp_s+EVENT_DUR_S])

        # Try making a mouse-to-photon measurement here
        if(event_type == 'M1'): lastM1Time = timestamp_s
        if(event_type == 'PD' and timestamp_s-lastM1Time < CLICK_TO_PHOTON_THRESH_S):
                m2photon_ms = 1000*(timestamp_s-lastM1Time)
                if PRINT_TO_CONSOLE: print("Mouse-to-photon = {0:0.3f}ms".format(m2photon_ms))
                delays.append(m2photon_ms)
    
    # Parse lines out of adc reader
    for line in adcReader:
            try: 
                timestamp_s = float(line[0])
                aVal = int(line[1])
            except: continue
            aTime.append(timestamp_s)
            aValue.append(aVal)
    if timestamp_s is None: return

    # Clear the plot, not sure this is necessary
    plt.clf()

    # Create the histogram
    plt.subplot(3,1,1)
    fig.canvas.set_window_title("Event Logger")
    plt.hist(delays, bins=bin_range, color=HIST_COLOR)
    plt.xlabel('Latency [ms]')
    plt.ylabel('Occurences')
    plt.title('Latency Histogram')
    plt.subplots_adjust(right=0.8)
    
    # Create a scrolling time plot for events
    plt.subplot(3,1,2)
    plt.tight_layout()
    plt.plot(m1Time, m1Event, color=MOUSE1_COLOR)
    plt.plot(m2Time, m2Event, color=MOUSE2_COLOR)
    plt.plot(pdTime, pdEvent, color=PHOTON_COLOR)
    plt.plot(swTime, swEvent, color=PIN_COLOR)
    plt.ylim(0,1.5*max([MOUSE_HEIGHT, PHOTON_HEIGHT, PIN_HEIGHT]))
    plt.xlim(timestamp_s-EVENT_PLOT_LEN_S, timestamp_s)
    plt.ylabel('Events')
    ax2.axes.get_yaxis().set_ticklabels([])
    plt.legend(['M1', 'M2', 'Photon', 'Pin'], loc='upper center', ncol=4)
    plt.tight_layout()

    # Create the real-time scolling plot of intensity
    plt.subplot(3,1,3)
    plt.plot(aTime, aValue, color=ADC_COLOR)
    plt.xlim(aTime[-1]-EVENT_PLOT_LEN_S, aTime[-1])
    plt.ylim(0, 1100)
    plt.ylabel('ADC Value')
    plt.xlabel('Time (s)')
    plt.tight_layout()

# Open CSV file(s) for reading
if len(sys.argv) < 3: raise Exception("Need to provide log file as input")
eventLogName = sys.argv[1]
if 'http' in eventLogName: 
    eventLogFile = urllib.request.urlopen(eventLogName)
    eventLogFile = codecs.iterdecode(eventLogFile, 'utf-8')
else: eventLogFile = open(eventLogName, mode='r')
eventReader = csv.reader(eventLogFile, delimiter=',', lineterminator='\n')
try: next(eventReader)
except: 0

adcLogName = sys.argv[2]
if 'http' in adcLogName: 
    adcLogFile = urllib.request.urlopen(adcLogName)
    adcLogFile = codecs.iterdecode(adcLogFile, 'utf-8')
else: adcLogFile = open(adcLogName, mode='r')
adcReader = csv.reader(adcLogFile, delimiter=',', lineterminator='\n')
try: next(adcReader)
except: 0   

# Create parent figure and window
fig = plt.figure()
fig.canvas.set_window_title("Event Logger")

# Create the histogram
plt.subplot(3,1,1)
fig.canvas.set_window_title("Event Logger")
ax1 = plt.hist(delays, bins=bin_range, color=HIST_COLOR)
plt.xlabel('Latency [ms]')
plt.ylabel('Occurences')
plt.title('Latency Histogram')
plt.subplots_adjust(right=0.8)

# Create a scrolling time plot for events
ax2 = plt.subplot(3,1,2)
plt.tight_layout()
plt.plot(m1Time, m1Event, color=MOUSE1_COLOR)
plt.plot(m2Time, m2Event, color=MOUSE2_COLOR)
plt.plot(pdTime, pdEvent, color=PHOTON_COLOR)
plt.plot(swTime, swEvent, color=PIN_COLOR)
plt.ylim(0,1.5*max([MOUSE_HEIGHT, PHOTON_HEIGHT, PIN_HEIGHT]))
plt.xlim(0,EVENT_PLOT_LEN_S)
plt.ylabel('Events')
ax2.axes.get_yaxis().set_ticklabels([])
plt.legend(['M1', 'M2', 'Photon', 'Pin'], loc='upper center', ncol=4)
plt.tight_layout()

# Create the real-time scolling plot of intensity
plt.subplot(3,1,3)
plt.plot(aTime, aValue, color=ADC_COLOR)
plt.xlim(0,EVENT_PLOT_LEN_S)
plt.ylim(0, 1100)
plt.ylabel('ADC Value')
plt.xlabel('Time (s)')
plt.tight_layout()

# Animate and show
ani = animation.FuncAnimation(fig, update)
plt.show()