import sys
import csv
import datetime

IN_LOG_TIME_FORMAT = '%Y-%m-%d %H:%M:%S.%f'

# Method for syncing data from a CSV reader to wallclock times
def sync_log_to_wallclock(log_reader):
    inSync = False      # Have we established sync?
    syncTime = None     # Wall clock time for sync
    syncVal = None      # Arduino timestamp for sync
    output = []         # Synced output storage
    for line in log_reader:
        if line[0] == "Timestamp [s]": continue     # Skip header row
        if line[1] == "SW sync":                    # Check for SW sync event w/ wallclock timestamp as time
            syncTime = datetime.datetime.strptime(line[0], IN_LOG_TIME_FORMAT)
            inSync = False              # Assume we find this first, so it clears the "in sync" status
        elif line[1] == "SW":                       # Check for SW interrupt event (corresponds to above) w/ Arduino time value
            if not inSync:                          # Make sure we dont "resync" to an incorrect SW pulse w/o another "SW Sync" ocurring
                syncVal = float(line[0])
                if syncTime is not None: inSync = True

        # Check for sync and correct samples if needed
        if inSync: time = syncTime + datetime.timedelta(seconds=(float(line[0])-syncVal))     # Compute a new time based on sync info
        else: time = line[0]                                                                  # Append the line as is if we don't have sync
        output.append([time, line[1]])
    return output

# Method for writing data back out to a new (time synced) CSV log
def write_log_to_file(log_writer, log_data, writeHeader=True):
    if writeHeader: log_writer.writerow(['Time', 'Event'])                  # Write a header to file (optionally)
    for [t,val] in log_data:                                                # Write all data to a CSV file
        log_writer.writerow([t, val])

# Only run the code below if this is the "main" routine (i.e. we were called directly)
if __name__ == '__main__':
    # Get input file name
    if len(sys.argv) > 2: inName = sys.argv[1]; outName = sys.argv[2]
    else: raise Exception("Need to pass log filename as input")

    # Open the input file w/ a csv reader
    inFile = open(inName, mode='r')
    inLog = csv.reader(inFile, delimiter=',', lineterminator='\n')

    # Open the output file w/ a csv writer and write the header row
    outFile = open(outName, mode='w')
    outLog = csv.writer(outFile, delimiter=',', lineterminator='\n')
    outLog.writerow(['Time', 'Event'])


    # Read the input file and create timestamped data
    syncData = sync_log_to_wallclock(inLog)

    # Write the output to another log file
    write_log_to_file(outLog, syncData)