# Hardware Event Logger
This readme documents the firmware and software affiliated with the hardware event logger platform.

## Setup
If you are planning to update firmware/develop for the platform make sure you have the [Arduino IDE](https://www.arduino.cc/en/Main/Software) installed, and have followed the [additional setup guide from SparkFun](https://learn.sparkfun.com/tutorials/pro-micro--fio-v3-hookup-guide/all) for the platform of your choice. Once you have completed the setup you can test it by connecting a [SparkFun Pro Micro](https://www.sparkfun.com/products/12640) and making sure you can compile/write an example sketch to the hardware.

You will also need Python 3 along with the following packages:
* [Matplotlib](https://matplotlib.org/)
* [Numpy](http://www.numpy.org/)
* [PySerial](https://pypi.org/project/pyserial/)

If you already have an [Anaconda](https://www.anaconda.com/) distribution installed you will likely already have most (if not all) of these dependencies.

## Usage
To use the setup you will need a complete hardware setup intended for the desired monitoring. For more information contact [Ben Boudaoud](mailto:bboudaoud@nvidia.com) and/or [Josef Spjut](maltio:jspjut@nvidia.com).

### Optional Steps (FW Upload)
1. Connect the Arduino to the computer via USB and open [EventLogger_v1.2.ino](./firmware\Hardware v1.2\edge_capture\EventLogger_v1.2\EventLogger_v1.2.ino) in the Arduino environment.
2. Use the "Upload" button in the top left to upload the sketch to the Arduino (you may need to reset the Arduino if it has previously been used for serial applications).
3. Configure the [logger](./software/event_logger.py) as needed by editing the "Control Flags", "Plot Parameters", and "Logging Parameters".
### Typical Usage
1. Run the [logger (event_logger.py)](./software/event_logger.py)  from the command line, providing the Arduino COM port as the sole argument. Make sure you provide the full COM port string (i.e. 'COM3' in the window environoment).
2. View the data in realtime (via histogram and time plot) and make sure that log files are being created (default to a "Log" directory you will need to create).

### Calibration (V1 Firmware Only)
*This process is not required in the V2 firmware as timestamped events replaced pulse width measurement*

The software/logger backend implements a simple (linear) calibration procedure to correct errors in the collected output measurements of latency from the hardware platform.

The calibration proceedure described below is one example of a possible method for determining calibration parameters.

1. Setup a signal generator to create a pulse of known (controllable) duration, repeating at a (relatively) slow rate of 1-2Hz.
2. Set a range of pulse widths (from say 100µs-200ms) and for each pulse width record the average reported pulse width over 10-20 pulses.
3. Perform a linear regression between the pulse width meaured by the device (as input) and actual pulse width (reported by the signal generator) to obtain a slope and offset for the correction.
4. Update the LATENCY_CAL_OFFSET (offset) and LATENCY_CAL_MUL (slope) values in the [logger](./software/latency_logger.py) with the new values
5. Re-check the output to make sure the signal generator settings and output from the logger now agree.

## Required Hardware
The firmware here is an Ardiuno sketch intended to work the the [SparkFun Pro Micro](https://www.sparkfun.com/products/12640). This firmware is intended to work with a [custom-designed piece of measurement hardware](https://workspace.circuitmaker.com/Projects/Details/Ben-Boudaoud-2/Latency-Measurement), though this can be replaced with a breadboard-based setup.

## More Information
For more information on this setup you can visit the (NVIDIA internal) [Confluence page](https://confluence.nvidia.com/display/NXP/Click+to+Photon+Latency+Measurement+and+Hardware+Event+Monitor) or contact [Ben Boudaoud](mailto:bboudaoud@nvidia.com).
