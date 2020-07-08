# System Configuration
The system configuration file is now depricated. It's input (config fields) have been moved to the [Latency Logger Config within the general config](general_config.md#latency-logger) and it's (historical) outputs have been redirected to `log.txt`.

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