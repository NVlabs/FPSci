# System Configuration
The system configuration file plays a dual role in the abstract-fps application. First it provides configuration information related to the hardware setup of the machine (namely whether a hardware click-to-photon logger is present and if so which COM port it uses). Second it records the system parameters from the experiment for later analysis.

## Input Fields
The following fields are input to the `systemconfig.Any` file:

* `HasLogger` indicates whether this system will perform any click-to-photon logging, when set to `false` this parameter disables all calls to hardware logging scripts.
* `LoggerComPort` indicates the port on which the logger is connected when `HasLogger` is set to `true`. Generally speaking this is a string (i.e. on windows `COM[X]`)
* `HasSync` indicates whether the system has an additional serial card where the DTR signal will be used for timebase syncing the logger to the PC (if `HasLogger` is `true` and `HasSync` is false, the first USB packet exchanged through the system is used to create the timestamp at a lower precision).
* `SyncComPort` indicates the port on which the sync card is connected if `HasSync` is set to `true`. Generally speaking these ports tend to be enumerated at lower port numbers (i.e. `COM0` or `COM1`) than the Virtual COM Ports (VCPs) produced by USB.

Refer to the [SAMPLEsystemconifg.Any file](SAMPLEsystemconfig.Any) for an example of this.

## Output Fields
The following fields are written by the application as output from the `systemconfig.Any` file:

* `CPU` is the reported name of the CPU being run on
* `CoreCount` represents the core count of the CPU being run on
* `GPU` is the reported name of the GPU being run on
* `MemoryCapacityMB` indicates the system RAM capacity in MB
* `DisplayName` provides the friendly name of the display
* `DisplayResXpx` provides the horizontal resolution of the display (in pixels)
* `DisplayResYpx` provides the vertical resolution of the display (in pixels)
* `DisplaySizeXmm` provides the horizontal size of the display (in mm)
* `DisplaySizeYmm` provides the vertical size of the display (in mm)

These parameters (as input) have no effect on the application.