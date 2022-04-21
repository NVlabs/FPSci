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
from datetime import datetime, timedelta
from event_logger_interface import EventLoggerInterface, SerialSynchronizer

# COM Port Parameters
BAUD = 115200                   # Serial port baud rate (should only change if Arduino firmware is changed)
TIMEOUT_S = 0.3                 # Timeout for serial port read (ideally longer than timeout for pulse measurement)

# Control Flags
LOG_EVENT_DATA = True           # Control whether event data is logged to a .csv file (seperate from ADC data)
LOG_ADC_DATA = False            # Control whether analog data is logged to a .csv file (seperate from event data)
PLOT_DATA = False               # Control whether data is plotted
PRINT_TO_CONSOLE = True         # Control whether data is printed to the console

CLICK_TO_PHOTON_THRESH_S = 0.3  # Maximum delay expected between click and photon
MIN_EVENT_SPACING_S = 0.1       # Minimum allowable amount of time between 2 similar events

# Logging parameters
IN_LOG_TIME_FORMAT = '%Y-%m-%d %H:%M:%S.%f'
LOG_NAME_TIME_FORMAT = '%y-%m-%d_%H%M%S'

# Create storage and lookups
lastM1Time = 0; lastM2Time = 0; lastPdTime = 0; lastSwTime = 0
timeLookup = {"M1": lastM1Time, "M2": lastM2Time, "PD": lastPdTime, "SW": lastSwTime}
t_offset_s = (pow(2,32)-1) * 1e-6 # Arduino `micros()` function wraps around after 2^32 microseconds (~71 minutes)
num_offsets = 0 # The number of times the Arduino has wrapped around

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
else: raise Exception("Need to provide COM port and output filename as argument to call!")

if(len(sys.argv) > 3): serCard = sys.argv[3]
else: serCard = None

# Check test mode (for device emulation)
hwInterface = EventLoggerInterface(com, BAUD, TIMEOUT_S)
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

# Clean out any data waiting for input
hwInterface.flush()

# If plotting open the plotter tool in another thread here
if PLOT_DATA and LOG_ADC_DATA: proc = subprocess.Popen('python event_plotter.py \"{0}\" \"{1}\"'.format(eventFname, adcFname))
elif PLOT_DATA: proc = subprocess.Popen('python event_plotter.py \"{0}\"'.format(eventFname))
hwInterface.set_adc_report(LOG_ADC_DATA)
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

# Stuff for tracking the autoclicker
NUM_CLICKS = 1050
ctime = datetime.now()
# Wait before starting
while ctime + timedelta(seconds=4) > datetime.now():
    pass
ctime = datetime.now()
count = 0

# This is the main loop that handles data aquisition and plotting
while(True):
    # Shutdown if the plot is closed
    if PLOT_DATA and not psutil.pid_exists(proc.pid): break
    
    # click for 50 ms
    if NUM_CLICKS > 0 and ctime + timedelta(milliseconds=330) < datetime.now():
        ctime = datetime.now()
        print(f'{NUM_CLICKS} remaining at {ctime}, {count} events since last')
        hwInterface.click(50)
        NUM_CLICKS-=1
        count = 0

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

        # Update arduino-reported time stamp for tracked wrap around count
        timestamp_s += num_offsets * t_offset_s

        if type(data) is int:                   # Check for ADC data in the message
            # Handle timestamp wrap around case for ADC events
            if timestamp_s < lastADCTime - 100:
                num_offsets += 1
                timestamp_s += t_offset_s
            lastADCTime = timestamp_s
            if LOG_ADC_DATA: 
                adcLogger.writerow([timestamp_s, data])
                adcFile.flush()
        else:                                   # Otherwise this is an event timestamp
            event_type = data                   # Data is just an event string
            
            # Handle timestamp wrap around case for non-ADC events
            if timestamp_s < timeLookup[event_type] - 100:
                num_offsets += 1
                timestamp_s += t_offset_s

            # Check that this event is far enough away from last event of this type (debounce)
            if(timeLookup[event_type] != 0 and (timestamp_s - timeLookup[event_type]) < MIN_EVENT_SPACING_S): continue

            # Otherwise make this the last event of its type
            timeLookup[event_type] = timestamp_s

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

    # Counting events
    if NUM_CLICKS > 0:
        count += len(vals)