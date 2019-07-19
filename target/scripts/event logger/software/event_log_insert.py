import sys
import os
from shutil import copyfile
import csv
import sqlite3
from event_log_syncer import sync_log_to_wallclock
import pdb

VERBOSE = True      # Flag to control extra print statements
CREATE_NEW = True   # Controls whether or not to create a new db file w/ merged results

# Get click to photon timings from a dataset
def get_click_to_photon(eventLog, maxClick2Photon=0.3):
    lastM1Time = None
    foundM1 = False
    delays = []    
    for line in eventLog:
        time = line[0]; event = line[1]
        if event == 'M1': 
            lastM1Time = time
            foundM1 = True
        elif event == 'PD' and foundM1 and (time-lastM1Time).total_seconds() < maxClick2Photon:
            delays.append([lastM1Time,1000.0*(time - lastM1Time).total_seconds()])
            foundM1 = False
    return delays

# Insert into the click_latencies table in the database
def insert_in_db(dbName, tableName, info):
    dbconn = sqlite3.connect(dbName)
    c = dbconn.cursor()
    if('event' in tableName): c.execute('''CREATE TABLE IF NOT EXISTS {0} (time float, event string, UNIQUE(time, event))'''.format(tableName))
    else: c.execute('''CREATE TABLE IF NOT EXISTS {0} (time float, latency float, UNIQUE(time, latency))'''.format(tableName))
    # Write each entry into the new table
    for i in info:
        time = i[0]
        c.execute("INSERT OR IGNORE INTO {0} VALUES (?, ?)".format(tableName), (time, i[1]))
    # Commit and close the database
    dbconn.commit()
    dbconn.close()

# Get the input ADC/event log file
if len(sys.argv) < 3: raise Exception("Provide input event log and output database name!")
eventLogName = sys.argv[1]
inputDbName = sys.argv[2]

# Create a new db file here if we need to
if CREATE_NEW: 
    outputDbName = os.path.dirname(inputDbName) + "\\merged_" + os.path.basename(inputDbName)
    copyfile(inputDbName, outputDbName)
else: outputDbName = inputDbName

# Get input reader and get wall-clock synced data from event log
if VERBOSE: print("Reading events from CSV logfile and time synchronizing...")
eventLog = csv.reader(open(eventLogName, mode='r'), delimiter=',', lineterminator='\n')
eventData = sync_log_to_wallclock(eventLog)

# Measure click-to-photon latency
if VERBOSE: print("Measuring click-to-photon latencies...")
c2p = get_click_to_photon(eventData)
if VERBOSE: print("Found {0} click to photon events...".format(len(c2p)))

# Insert this new data into the SQL database
if VERBOSE: print("Writing table to {0}...".format(outputDbName))
insert_in_db(outputDbName, "click_latencies", c2p)
insert_in_db(outputDbName, "event_log", eventData)

if VERBOSE: print("Done.")

