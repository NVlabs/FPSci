# Software Resources for the Hardware Event Logger
This is a place to document SW we have developed around the NXP hardware event logger.

## Device Interface
The [event_logger_interface.py](./event_logger_interface.py) script provides a low-level device interface class (`EventLoggerInterface`) that handles data capture and command exchange with the device for the user (not needed for non-developers).

The same file also provides a minimal class (`SerialSynchronizer`) for producing "hard" synchronization with the logger using a 0-5V DTR signal toggle.

## Logging and Plotting
The [event_logger.py](./event_logger.py) script is used to log data coming from logger. It is called from the command line as:

    python event_logger.py [Logger COM port] [(Optional) Sync COM port]

The logger COM port is the COM port which the Virtual COM Port (VCP) USB interface is present on. In Windows you can use Device Manager to view available COM ports.

The optional sync COM port is intended to be used with a serial card with 0-5V RTS signal attached to the logger as a source for "hard" synchronization.

The [event_plotter.py](./event_plotter.py) script is used to plot real time data from an incoming log file. When [event_logger.py](./event_logger.py) is called with the `PLOT_DATA` flag set to `True` an event plotter is automatically instantiated along with the logger instance.

If the plotter ever dies or needs to be re-run it can be called using:

    python event_plotter.py [ADC log file] [Event Log File]

Where the ADC and event log files are the paths to those file respectively. These paths are set by the `LOG_PREFIX` control parameter in [event_logger.py](./event_logger.py).

## Time Synchronization
Time synchronization (to wall clock) is handled via real-world timestamps embedded in the log data files (ADC and event) via the logger tool.

The [event_log_syncer.py](./event_log_syncer.py) script provides useful resources for synchronizing all events/ADC data back to wall clock time by post-processing these logs files.

You can run the time synchronization script (in standalone mode) by calling:

    python event_log_syncer.py [in log file] [out log file]

Where the `in log file` is either an event or ADC log and the output file is the name of the (new) file you'd like to write sync results to.

For developers interested in synchronizing to wall clock without a need for file output the `sync_to_wallclock()` and `write_log_to_file()` methods are separated within [event_log_syncer.py](./event_log_syncer.py) to allow them to be imported and called from another python script.

## Inserting Results into an Experiment Database
As part of a feature for supporting the abstract-fps experiment, we have also developed a script ([event_log_insert.py](./event_log_insert.py)) to allow wall clock synchronization *and* insert of a pre-formatted table into a larger experimental result database.

This script can be called with:

    python event_log_insert.py [Event Log] [DB file]

Where `Event Log` is the existing input event log file (pre-wall clock sync) and `DB File` is the existing experiment database file without these events added.

Note that [event_log_insert.py](./event_log_insert.py) can be called multiple times on a single experiment file (i.e. can append multiple event log files into a single experiment database).

## Additional Files
There are a number of additional files in this directory. This is a place to describe them

### test.html
This is a simple HTML-based web page that can be used to test click to photon behavior. While the mouse is down (anywhere on the page) this entire page is white, when mouse if up it is black.

This is a great way to test out a new click-to-photon logging setup by making sure that both click and photon events are being correctly received by the logger.

### SQLite Logging
Josef developed [event_logger_sqlite.py](./event_logger_sqlite.py) and [event_plotter_sqlite.py](./event_plotter_sqlite.py) as demonstrations of a SQLite-based logging format (currently [event_logger.py](./event_logger.py) uses a CSV based intermediate log).

### GtG Testing
The [gtg_tester.py](./gtg_tester.py) script is a demonstration of using the logger hardware purely for photon input (also using the analog values).

Here we attempt to measure the Gray-to-Gray (GtG) transition times for various monitors by creating controlled transitions and measuring them in real time with the logger.

### Click-to-USB
This was part of another effort to characterize the time from mouse button down to USB packet present on the bus. As part of this experiment this script was written to use the logger to produce clicks with known timing relative to the rest of the script.

### Event Log Emulator
Originally this was a development tool that allowed for simulation of event loggers. Currently it is not working/could probably be incorporated into the [event_logger_interface.py](./event_logger_interface.py) script (think `EMULATION_MODE` control flag).
