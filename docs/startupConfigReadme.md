# Introduction
The startup config is the highest level configuration file provided to `FirstPersonScience`. This file specifies the experiment configuration path, as well as the user configuration path and play mode.

## File Location
The `startupconfig.Any` file is located in the [`data-files` directory](../data-files/) in the root of the project. If no `startupconfig.Any` is present at launch the application will create one.

# Fields
The following fields are valid for a startupconfig.Any file:

* `developerMode` controls whether the application is run in experiment mode, or [developer mode](./developermode.md). When `false` the application is in experiment/play mode and is ready to collect data. In developer mode (`developerMode`=`true`) the application is run with additional windows available for user mouse interaction.
* `waypointEditorMode` controls whether the application is run including additional support for [target path creation/waypoint editing](./patheditor.md). Waypoint editor mode **only applies when `developerMode`=`true`**! In waypoint editor mode an additional menu is made available and the user can drop waypoints or start/stop record their motion using key bound functions.
* `fullscreen` controls whether the application is run in fullscreen mode or windowed (independent of other options)
* `windowSize` controls the (default) window size when running in windowed mode (`fullscreen`=`false`) as a `Vector2`. The window can then be resized from this default while running.
* `experimentConfigFilename` sets the file path to an [experiment config file](./experimentConfigReadme.md) for futher configuration of an experiment.
* `userConfigFilename` sets the file path to a user config file for per user setup.
* `userStatusFilename` sets the file path to a user status file for user tracking.
* `latencyLoggerConfigFilename` sets the file path to a latency logger config file for controlling the latency logger hardware.
* `resultsDirPath` sets the path to the results directory. If this directory does not exists the application will create it at runtime.
* `audioEnable` turns on or off audio

## Sample/Default Values
The default `startup.Any` file is included below (as an example):
```
developerMode = false;              // Set this to true to enable developer mode (extra windows)
waypointEditorMode = false;         // Set this to true to enable waypoint editor mode (target path creation)
fullscreen = true;                  // Set this to false to run in windowed mode
windowSize = Vector2(1920, 980);    // This sets the default window size (when running with fullscreen = false)
experimentConfigFilename = "";      // Leave this empty for default "experimentconfig.Any"
userConfigFilename = "";            // Leave this empty for default "userconfig.Any"
resultsDirPath = "./results/";      // Change to save results somewhere else
audioEnable = true;                 // Set false to turn off audio
```