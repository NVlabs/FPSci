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
LOG_EVENT_DATA = True           # Control whether event data is logged to a .csv file (seperate from ADC data)
LOG_ADC_DATA = True             # Control whether analog data is logged to a .csv file (seperate from event data)
PLOT_DATA = True                # Control whether data is plotted
PRINT_TO_CONSOLE = True         # Control whether data is printed to the console

CLICK_TO_PHOTON_THRESH_S = 0.3  # Maximum delay expected between click and photon
MIN_EVENT_SPACING_S = 0.1       # Minimum allowable amount of time between 2 similar events
AUTOCLICK_COUNT_TOTAL = 500     # Number of autoclick events to perform once autoclick is enable

# Logging parameters
LOG_PREFIX = 'Logs/log'         # Prefix for logging directory (can relocate logs using this)
IN_LOG_TIME_FORMAT = '%S.%f'
LOG_NAME_TIME_FORMAT = '%y-%m-%d_%H%M%S'

# Create storage and lookups
m1Time = []; m1Event = [];
m2Time = []; m2Event = [];
pdTime = []; pdEvent = [];
swTime = []; swEvent = [];
lastM1Time = 0
timeLookup = {"M1": m1Time, "M2": m2Time, "PD": pdTime, "SW": swTime}
eventLookup = {"M1": m1Event, "M2": m2Event, "PD": pdEvent, "SW": swEvent}

# Create lookup for event (proper names)
nameLookup = {
    "M1": "Left mouse button",
    "M2": "Right mouse button",
    "PD": "Photodetector",
    "SW": "Software interrupt"
}

# Create serial port and setup histogram
if(len(sys.argv) > 1): com = sys.argv[1]
else: raise Exception("Need to provide COM port as argument to call!")

if(len(sys.argv) > 2): serCard = sys.argv[2]
else: serCard = None

# Check test mode (for device emulation)
hwInterface = EventLoggerInterface(com, BAUD, TIMEOUT_S)
if serCard is not None: syncer = SerialSynchronizer(serCard)
else: syncer = None

autoclick = False

# Opening print to console
if PRINT_TO_CONSOLE: print("Starting logging (GUI will open when logging data is available)")

# Handle creating log file (and writing header here)
if LOG_EVENT_DATA:
    eventFname = LOG_PREFIX + "_event_" + datetime.now().strftime(LOG_NAME_TIME_FORMAT) + ".csv"
    eventFile = open(eventFname, mode='w')
    eventLogger = csv.writer(eventFile, delimiter=',', lineterminator='\n')
    eventLogger.writerow(['Timestamp [s]', 'Event'])
    eventFile.flush()

if LOG_ADC_DATA:
    adcFname = LOG_PREFIX + "_adc_"+ datetime.now().strftime(LOG_NAME_TIME_FORMAT) + ".csv"
    adcFile = open(adcFname, mode='w')
    adcLogger = csv.writer(adcFile, delimiter=',', lineterminator='\n')
    adcLogger.writerow(['Timestamps [s]', 'Value'])
    adcFile.flush()

# Clean out any data waiting for input
hwInterface.flush()

# If plotting open the plotter tool in another thread here
if PLOT_DATA: proc = subprocess.Popen('python event_plotter.py \"{0}\" \"{1}\"'.format(eventFname, adcFname))

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
while(True):
    # Shutdown if the plot is closed
    if PLOT_DATA and not psutil.pid_exists(proc.pid): break
    
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
            if LOG_ADC_DATA: 
                adcLogger.writerow([timestamp_s, data])
                adcFile.flush()
        else:                                   # Otherwise this is an event timestamp
            event_type = data                   # Data is just an event string
            
            # Check that this event is far enough away from last event of this type (debounce)
            if(len(timeLookup[event_type]) != 0 and timestamp_s - timeLookup[event_type][-1] < MIN_EVENT_SPACING_S): continue

            # Print/log the event if requested
            if PRINT_TO_CONSOLE: print("{0} at {1:0.3f}s".format(nameLookup[event_type], timestamp_s))
            
            # Write the event to a the log file
            if LOG_EVENT_DATA: 
                eventLogger.writerow([timestamp_s, event_type])
                eventFile.flush()

            # Allow SW events to be written to the ADC log to allow time sync
            if LOG_ADC_DATA and event_type == 'SW': 
                adcLogger.writerow([timestamp_s, event_type])
                adcFile.flush()

            # MOVED TO event_plotter.py FOR NOW!!!
            # # Try making a mouse-to-photon measurement here
            # if(event_type == 'M1'): lastM1Time = timestamp_s; m2pd = False
            # if(event_type == 'PD' and timestamp_s-lastM1Time < CLICK_TO_PHOTON_THRESH_S):
            #     m2photon_ms = 1000*(timestamp_s-lastM1Time)
            #     if PRINT_TO_CONSOLE: print("\tMouse-to-photon = {0:0.3f}ms".format(m2photon_ms))
                
            #     if autoclick:
            #         # Count the autoclicks (mouse-to-photons and turn off auto clicker here)
            #         autoclick_count += 1
            #         if autoclick_count > autoclick_count_max:
            #             hwInterface.write("off\n".encode('utf-8'))
            #             autoclick = False