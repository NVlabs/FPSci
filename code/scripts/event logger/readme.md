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

### Typical Usage
1. Run the [logger (event_logger.py)](./software/event_logger.py)  from the command line, providing the Arduino COM port and output file basename (output log without the "_adc.csv" or "_event.csv") argument. Make sure you provide the full COM port string (i.e. 'COM3' in the window environoment). Example calls are provided below.

    ```python event_logger.py [COM port] [Output basefile] [(Optional) Sync COM Port]```

    Without SYNC port: ```python event_logger.py COM9 ../results/mytest```

    With SYNC port: ```python event_logger.py COM9 ../results/mytest COM1```

2. View the data in realtime (via histogram and time plot if `PLOT_DATA` is set to `True`) and make sure that log files are being created.

## Required Hardware
The firmware here is an Ardiuno sketch intended to work the the [SparkFun Pro Micro](https://www.sparkfun.com/products/12640). This firmware is intended to work with a [custom-designed piece of measurement hardware](https://workspace.circuitmaker.com/Projects/Details/Ben-Boudaoud-2/Latency-Measurement), though this can be replaced with a breadboard-based setup.

## More Information
For more information on this setup you can visit the (NVIDIA internal) [Confluence page](https://confluence.nvidia.com/display/NXP/Click+to+Photon+Latency+Measurement+and+Hardware+Event+Monitor) or contact [Ben Boudaoud](mailto:bboudaoud@nvidia.com).
