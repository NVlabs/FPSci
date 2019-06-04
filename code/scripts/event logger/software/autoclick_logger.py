import serial
import matplotlib.pyplot as plt
from matplotlib.ticker import MultipleLocator
from matplotlib.widgets import CheckButtons
import numpy as np
import csv
import sys
import subprocess
import psutil
import datetime
from datetime import datetime
from event_logger_interface import EventLoggerInterface, SerialSynchronizer

# COM Port Parameters
BAUD = 115200                   # Serial port baud rate (should only change if Arduino firmware is changed)
TIMEOUT_S = 0.3                 # Timeout for serial port read (ideally longer than timeout for pulse measurement)

# Control Flags
LOG_C2P_DATA = True             # Control whether click to photon data is logged to a .csv file (seperate from above)
LOG_EVENT_DATA = False          # Control whether event data is logged to a .csv file (seperate from ADC data)
LOG_ADC_DATA = False            # Control whether analog data is logged to a .csv file (seperate from event data)
PLOT_DATA = False               # Control whether data is plotted
PRINT_TO_CONSOLE = True         # Control whether data is printed to the console
CLICK_TO_START = True           # Control whether a single M1 click starts the autoclicking

CLICK_TO_PHOTON_THRESH_S = 0.3  # Maximum delay expected between click and photon

# Autoclick parameters
AUTOCLICK_C2P_COUNT_TOTAL = 20 # Number of autoclick events to perform once autoclick is enable
AUTOCLICK_TARGET_PERIOD_S = 0.5 # Approximate target period
AUTOCLICK_JITTER_MS = 10        # Jitter range for the autoclick interval
AUTOCLICK_DURATION_MS = 100     # Duration of the autoclick

MIN_EVENT_SPACING_S = AUTOCLICK_TARGET_PERIOD_S * 0.4      # Minimum allowable amount of time between 2 similar events

# Logging parameters
IN_LOG_TIME_FORMAT = '%Y-%m-%d %H:%M:%S.%f'
LOG_NAME_TIME_FORMAT = '%y-%m-%d_%H%M%S'

# Create storage and lookups
lastM1Time = 0; lastM2Time = 0; lastPdTime = 0; lastSwTime = 0
timeLookup = {"M1": lastM1Time, "M2": lastM2Time, "PD": lastPdTime, "SW": lastSwTime}

# Create lookup for event (proper names)
nameLookup = {
    "M1": "Left mouse button",
    "M2": "Right mouse button",
    "PD": "Photodetector",
    "SW": "Software interrupt"
}

# Create serial port and setup histogram
if(len(sys.argv) > 2): 
    com = sys.argv[1]
    logbasename = sys.argv[2]
    emulate = False
    if com == 'EMU': 
        emulate = True
        print("Running in emulation mode.")
    else:
        print("Opening logger on {0}...".format(com))
else: raise Exception("Need to provide COM port and output filename as argument to call!")

if(len(sys.argv) > 3): serCard = sys.argv[3]
else: serCard = None

# Check test mode (for device emulation)
emuParams = EventLoggerInterface.EmulationParams(
    events=['M1', 'M2', 'PD'],
    probability_dict = {'M1':0.1,
                        'M2':0.001,
                        'PD':0.1}
)
hwInterface = EventLoggerInterface(com, BAUD, TIMEOUT_S, emulate=emulate, emuParams=emuParams)
if serCard is not None: syncer = SerialSynchronizer(serCard)
else: syncer = None

# Opening print to console
if PRINT_TO_CONSOLE and PLOT_DATA: print("Starting logging (GUI will open when logging data is available)")

# Handle creating log file (and writing header here)
if LOG_EVENT_DATA:
    eventFname = logbasename + "_event.csv"
    eventFile = open(eventFname, mode='w')
    eventLogger = csv.writer(eventFile, delimiter=',', lineterminator='\n')
    eventLogger.writerow(['Timestamp [s]', 'Event'])
    eventFile.flush()

if LOG_ADC_DATA:
    adcFname = logbasename + "_adc.csv"
    adcFile = open(adcFname, mode='w')
    adcLogger = csv.writer(adcFile, delimiter=',', lineterminator='\n')
    adcLogger.writerow(['Timestamps [s]', 'Value'])
    adcFile.flush()

if LOG_C2P_DATA:
    c2pFname = logbasename + "_c2p.csv"
    c2pFile = open(c2pFname, mode='w')
    c2pLogger = csv.writer(c2pFile, delimiter=',', lineterminator='\n')
    c2pLogger.writerow(['Timestamp [s]', 'Latency [ms]'])

# Clean out any data waiting for input
hwInterface.flush()

# If plotting open the plotter tool in another thread here
if PLOT_DATA and LOG_ADC_DATA: proc = subprocess.Popen('python event_plotter.py \"{0}\" \"{1}\"'.format(eventFname, adcFname))
elif PLOT_DATA and LOG_EVENT_DATA: proc = subprocess.Popen('python event_plotter.py \"{0}\"'.format(eventFname))
elif PLOT_DATA: raise Exception('Need to set LOG_EVENT_DATA = True for plotting!')

# Create intro sync here (used to align data to wallclock later)
synced  = False
if syncer is not None:
    time = syncer.sync()
    if LOG_EVENT_DATA: 
        eventLogger.writerow([time.strftime(IN_LOG_TIME_FORMAT), "SW sync"])
        eventFile.flush()
    if LOG_ADC_DATA: 
        adcLogger.writerow([time.strftime(IN_LOG_TIME_FORMAT), "SW sync"])
        adcFile.flush()
    synced = True

# This is the main loop that handles data aquisition and plotting
c2ps = 0
started = not CLICK_TO_START
last_click_time = datetime.now()
hwInterface.flush()
while(c2ps < AUTOCLICK_C2P_COUNT_TOTAL):
    # Shutdown if the plot is closed
    if PLOT_DATA and not psutil.pid_exists(proc.pid): break  

    # Periodic autoclicking is handled here...
    if started:
        period_s = AUTOCLICK_TARGET_PERIOD_S + AUTOCLICK_JITTER_MS/1000 * (np.random.random()-0.5)
        if (datetime.now() - last_click_time).total_seconds() > period_s: 
            last_click_time = datetime.now()
            hwInterface.click(AUTOCLICK_DURATION_MS)

    # Read the values from the HW interface
    vals = hwInterface.parseLines()             # Get all lines waiting on read from the serial port
    for val in vals:                            # Iterate through the lines to get data from each
        if val is None: continue                # If no value returned skip this
        [timestamp_s, data] = val               # Otherwise split the timestamp/data from the message

        # Do "soft sync" here (just grab timestamp and add a virtual "sync" event) this only happens once!
        if not synced:                         
            time = datetime.now()
            if LOG_EVENT_DATA:
                eventLogger.writerow([time.strftime(IN_LOG_TIME_FORMAT), "SW sync"])
                eventLogger.writerow([timestamp_s, "SW"])
                eventFile.flush()
            if LOG_ADC_DATA:
                adcLogger.writerow([time.strftime(IN_LOG_TIME_FORMAT), "SW sync"])
                adcLogger.writerow([timestamp_s, "SW"])
                adcFile.flush()
            synced = True

        if type(data) is int:                   # Check for ADC data in the message
            if LOG_ADC_DATA: adcLogger.writerow([timestamp_s, data]); adcFile.flush()
        else:                                   # Otherwise this is an event timestamp
            event_type = data                   # Data is just an event string
            # Check that this event is far enough away from last event of this type (debounce)
            if(timeLookup[event_type] != 0 and (timestamp_s - timeLookup[event_type]) < MIN_EVENT_SPACING_S): continue

            # Print and log event
            if PRINT_TO_CONSOLE: print("{0} at {1:0.3f}s ({2} from last)".format(nameLookup[event_type], timestamp_s, timestamp_s - timeLookup[event_type]))
            if LOG_EVENT_DATA: eventLogger.writerow([timestamp_s, event_type]); eventFile.flush()
            # Allow SW events to be written to the ADC log to allow time sync
            if LOG_ADC_DATA and event_type == 'SW': adcLogger.writerow([timestamp_s, event_type]); adcFile.flush()

            timeLookup[event_type] = timestamp_s   # Otherwise make this the last event of its type


            # Try making a mouse-to-photon measurement here
            if(event_type == 'M1'): 
                if CLICK_TO_START and not started: 
                    started = True       # Start autoclicking on the first M1 if in CLICK_TO_START
                    if PRINT_TO_CONSOLE: print("Starting autoclicking...")
                else: lastM1Time = timestamp_s                          # Otherwise just grab the M1 time event for M1-->PD measurement
            if(event_type == 'PD' and timestamp_s-lastM1Time < CLICK_TO_PHOTON_THRESH_S):
                    c2ps += 1
                    c2p = 1000*(timestamp_s-lastM1Time)
                    if PRINT_TO_CONSOLE: print("\t\tMouse-to-photon #{0} = {1:0.3f}ms".format(c2ps, c2p)); sys.stdout.flush()
                    if LOG_C2P_DATA: c2pLogger.writerow([lastM1Time, c2p]); c2pFile.flush()
    # if not PRINT_TO_CONSOLE: print('%d clicks'%c2ps)