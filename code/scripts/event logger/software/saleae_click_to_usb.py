from matplotlib import pyplot as plt
from datetime import datetime
import numpy as np
import saleae
import csv
import time
import sys
from event_logger_interface import EventLoggerInterface

# Colors for print
HEADER = '\033[95m'
OKBLUE = '\033[94m'
OKGREEN = '\033[92m'
WARNING = '\033[93m'
FAIL = '\033[91m'
ENDC = '\033[0m'
BOLD = '\033[1m'
UNDERLINE = '\033[4m'

ELOG_COM_PORT = 'COM3'

CAPTURE_S = 0.2             # Time to capture in seconds (max delay between click and USB)
MOUSE_CHANNEL   = 2         # Channel mouse is setup on
MAX_REASONABLE_LATENCY_MS = 50  # This is the maximum latency (used for sanity checking)

HIST_MIN_MS = 0             # Histogram minimum
HIST_MAX_MS = 300           # Histogram maximum 
HIST_NUM_BINS = 50          # Histogram number of bins
HIST_COLOR = 'green'        # Histogram plotting color

ANALYZER_NAME = "Mouse Analyzer"                   # Name to look for for the (pre-setup mouse analyzer)
ANALYZER_DATA_PATH = "C:/temp/saleae_log"          # You can set this to wherever exists/makes sense for your system

LOG_TO_CSV = True
CSV_PATH = "./Logs/click_to_usb"

# Setup the saleae logic to capture
s = saleae.Saleae()

# Check for a valid USB analyzer
haveAnalyzer = False
aIndex = None
for a in s.get_analyzers():
    if "" in a[0]: 
        haveAnalyzer = True
        aIndex = a[1]
if not haveAnalyzer: raise Exception('No USB analyzer found!')

# Setup the capture
#s.set_trigger_one_channel(MOUSE_CHANNEL, saleae.Trigger.Low)
s.set_capture_seconds(CAPTURE_S)

# Get event logger object to produce clicks
elogger = EventLoggerInterface(ELOG_COM_PORT)

m2usbDist = []
bin_range = np.linspace(HIST_MIN_MS, HIST_MAX_MS, HIST_NUM_BINS)

fig = plt.figure()
trialNum = 1

if LOG_TO_CSV:
    logFname = CSV_PATH + "_" + datetime.now().strftime('%y-%m-%d_%H%M%S') + ".csv"
    logFile = open(logFname, mode='w')
    logger = csv.writer(logFile, delimiter=',', lineterminator='\n')
    logger.writerow(['Click to USB Latency [ms]'])

while(True):
    # Print the start of trial statement then create the analyzer filename and increment the trial count
    print("Starting trial #{0}".format(trialNum))
    trialNum += 1
    analyzerExportPath = ANALYZER_DATA_PATH+ "_" + datetime.now().strftime('%y-%m-%d_%H%M%S') + ".csv"
    #print("\tPRESS AND HOLD the mouse")
    
    # Wait for capture and save
    s.capture_start()
    time.sleep(0.01)

    # Click and wait for capture to complete
    elogger.flush()
    elogger.mouseDown()
    while not s.is_processing_complete(): continue
    
    #elogger.autoclick_off()

    try: analyzer_data = s.export_analyzer(aIndex, analyzerExportPath)
    except: 
        print("\tFailed to export analyzer data")
        continue

    #print("\tRELEASE the mouse")
    print("\tAnalyzing CSV for transition...")

    # Wait for log file to finish writing
    time.sleep(1)

    # Load the CSV file
    try: analyzerFile = open(analyzerExportPath, mode='r')
    except: 
        print("\tFailed to open CSV file")
        continue
    reader = csv.reader(analyzerFile)
    m2usb = None
    for line in reader:
        if "DATA" in line[1]:
            m2usb = float(line[0])*1000.0
            if(m2usb < 0 or m2usb > MAX_REASONABLE_LATENCY_MS):
                m2usb = None
                continue
            print("\tMouse to USB time = {0:0.2f}ms".format(m2usb))
            break

    # Stop the capture
    s.capture_stop()

    # If a mouse event occurred here, then go ahead and log/update the histogram
    if m2usb is not None:
        m2usbDist.append(m2usb)
        plt.hist(m2usbDist, bins=bin_range, color=HIST_COLOR)
        plt.pause(0.01)
        print("\tHave now found {0} valid measurements.".format(len(m2usbDist)))
        if LOG_TO_CSV: logger.writerow([m2usb])
        continue
    else: print("\tDid not find a mouse to USB transition in the data")

print("Done, took a total of {0} measurements out of {1} trials.".format(len(m2usbDist), trialNum-1))