# System Configuration
The system configuration file supports details specific a given computer. This file's (historical) outputs have been redirected to `log.txt`.

# Latency Logger Support
These flags support the reseach hardware latency logger:
| Parameter Name     |Units                 | Description                                                                        |
|--------------------|----------------------|------------------------------------------------------------------------------------|
|`hasLatencyLogger`  |`bool`    | Whether this system has a click-to-photon logger connected, when set to `false` this parameter disables all calls to hardware logging scripts |
|`loggerComPort`     |`String`  | The port on which the logger is connected when `hasLogger` is set to `true`. Generally speaking this is a string (i.e. on windows `COM[X]`) |
|`hasLatencyLoggerSync` |`bool` | Whether the system has an additional serial card where the DTR signal will be used for timebase syncing the logger to the PC (if `hasLatencyLogger` is `true` and `hasLatencyLoggerSync` is false, the first USB packet exchanged through the system is used to create the timestamp at a lower precision). |
|`loggerSyncComPort` |`String`  | The port on which the sync card is connected if `hasLatencyLoggerSync` is set to `true`. Generally speaking these ports tend to be enumerated at lower port numbers (i.e. `COM0` or `COM1`) than the Virtual COM Ports (VCPs) produced by USB. |

An example of this structure's usage is provided below:

```
"hasLatencyLogger" :  true,
"loggerComPort" : "COM3",
"hasLatencyLoggerSync": true,
"loggerSyncComPort" : "COM1",
```

The `loggerComPort` and `loggerSyncComPort` fields can be used in [commands](general_config.md#supported-substrings-for-commands) via their affiliaied `%loggerComPort` and `%loggerSyncComPort` replacement substrings. 

# (Historical) Output Fields
The following fields were (historically) written by the application as output from the `systemconfig.Any` file, but are no longer:

* `CPU` is the reported name of the CPU being run on
* `CoreCount` represents the core count of the CPU being run on
* `GPU` is the reported name of the GPU being run on
* `MemoryCapacityMB` indicates the system RAM capacity in MB
* `DisplayName` provides the friendly name of the display
* `DisplayResXpx` provides the horizontal resolution of the display (in pixels)
* `DisplayResYpx` provides the vertical resolution of the display (in pixels)
* `DisplaySizeXmm` provides the horizontal size of the display (in mm)
* `DisplaySizeYmm` provides the vertical size of the display (in mm)

These parameters are now written to `log.txt` at startup to avoid confusion and make `startupconfig.Any` a "pure" configuration (i.e. input) file.