# Use this script if you want to log directly to a sqlite3 db file

import sys
import sqlite3
# event logger interfaces
from event_logger_interface import EventLoggerInterface, SerialSynchronizer

# COM Port Parameters
BAUD = 115200                   # Serial port baud rate (should only change if Arduino firmware is changed)
TIMEOUT_S = 0.3                 # Timeout for serial port read (ideally longer than timeout for pulse measurement)
DEFAULT_LOGFILE = 'Logs/test.db'

if __name__=='__main__':
    if len(sys.argv) > 1: com = sys.argv[1]
    else: raise Exception('Please provide COM port as argument to call!')

    # TODO command line db log file name or similar
    if len(sys.argv) > 2: pass
    else: pass

    # initialize serial port
    hwInterface = EventLoggerInterface(com, BAUD, TIMEOUT_S)
    hwInterface.flush()
    # TODO: handle synchronization if it matters

    # open database
    dbconn = sqlite3.connect(DEFAULT_LOGFILE)

    # create table - TODO: check if it exists first
    c = dbconn.cursor()
    c.execute('''CREATE TABLE IF NOT EXISTS events
                 (timestamp float, data int)''')
    dbconn.commit()

    # TODO - set up loop end correctly instead of hard coded number of events
    for i in range(5000):
        # grab new set of samples
        vals = hwInterface.parseLines()
        if vals is None: continue
        # iterate through samples and add to DB
        for val in vals:
            [timestamp_s, data] = val
            # log to database
            c.execute("INSERT INTO events VALUES (?, ?)", (timestamp_s, data))
        dbconn.commit()        

    # close the database when done
    dbconn.close()
