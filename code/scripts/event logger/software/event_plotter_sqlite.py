# Use this script to visualize data from a sqlite3 db file
# can connect while another thread is writing logs to that file

import sys
import sqlite3
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import numpy as np

# COM Port Parameters
BAUD = 115200                   # Serial port baud rate (should only change if Arduino firmware is changed)
TIMEOUT_S = 0.3                 # Timeout for serial port read (ideally longer than timeout for pulse measurement)
DEFAULT_LOGFILE = 'Logs/test.db'

time = []
val = []

def update(x):
    c.execute("SELECT * FROM events ORDER BY timestamp DESC limit 500")
    # self.cursor.execute(self.query)
    [time, val] = np.array(c.fetchall()).swapaxes(0,1).astype(np.float)
    ax.clear()
    plt.ylim(0, 1100)
    ax.plot(time, val)

if __name__=='__main__':
    # TODO command line db log file name

    # connect to db
    dbconn = sqlite3.connect(DEFAULT_LOGFILE)
    c = dbconn.cursor()

    # get 50 most recent entries
    c.execute("SELECT * FROM events ORDER BY timestamp DESC limit 50")
    [time, val] = np.array(c.fetchall()).swapaxes(0,1).astype(np.float)
    fig = plt.figure()
    ax = fig.add_subplot(1,1,1)
    ax.plot(time, val)
    plt.ylim(0, 1100)

    # pd = PlotData(c, "SELECT * FROM events ORDER BY timestamp DESC limit 50")

    # fig = plt.figure()
    ani = animation.FuncAnimation(fig, update)

    plt.show()

    dbconn.close()
