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
* `defaultExperiment` controls the default set of files to use for the experiment, these values are inherited by any unspecified values from experiments in the `experimentList` (see below)
* `experimentList` optionally specifies a list of experiments that can be selected from in developer mode, if none is provided a single experiment that matches the `defaultExperiment` specification is used
* `audioEnable` turns on or off audio

## Experiment Specification
The following fields are specified on a per-experiment basis:

* `experimentConfigFilename` sets the path and file name for an [experiment config file](./experimentConfigReadme.md).
* `userConfigFilename` sets the path and file name for a [user config file](./userConfigReadme.md).
* `userStatusFilename` sets the path and file name for a [user status file](./userStatusReadme.md).
* `keymapConfigFilename` sets the path and file name for a [keymap config file](./keymap.md).
* `systemConfigFilename` sets the path and file name for a [system config file](systemConfigReadme.md) for controlling latency logger hardware.
* `resultsDirPath` sets the path to the results directory. If this directory does not exists the application will create it at runtime.
* `name` is the name identifier for each entry in the `experimentList`.

## Sample/Default Values
The default `startup.Any` file is included below (as an example):
```
developerMode = false;                              // Set this to true to enable developer mode (extra windows)
waypointEditorMode = false;                         // Set this to true to enable waypoint editor mode (target path creation)
fullscreen = true;                                  // Set this to false to run in windowed mode
windowSize = Vector2(1920, 980);                    // This sets the default window size (when running with fullscreen = false)
audioEnable = true;                                 // Set false to turn off audio

defaultExperiment = {
    name = "default";
    experimentConfigFilename = "experimentconfig.Any";  // Can include a path
    userConfigFilename = "userconfig.Any";              // Can include a path
    userStatusFilename = "userstatus.Any";              // Can include a path
    keymapConfigFilename = "keymap.Any";                // Can include a path
    systemConfigFilename = "systemconfig.Any";          // Can include a path
    resultsDirPath = "./results/";                      // Change to save results somewhere else
};
```

As an example of how to use the `experimentList`, let's say we have 2 experiments that use the same userConfig, keymap, and results, but have their own experimentConfig and userStatus to track progress. The `experimentList` for this example might look something like the following:

```
{
    audioEnable = true; 
    developerMode = true; 
    fullscreen = false; 
    waypointEditorMode = false; 
    defaultExperiment = {
        name = "default";                                   // Note that `name` is required and "default" is useful if using an `experimentList`
        experimentConfigFilename = "experimentconfig.Any"; 
        userStatusFilename = "userstatus.Any"; 
        userConfigFilename = "userconfig.Any"; 
        resultsDirPath = "./results/"; 
        systemConfigFilename = "systemconfig.Any"; 
        keymapConfigFilename = "keymap.Any"; 
    };
    experimentList = [
        { name = "default"; }, // include this if you want the default experiment in the list
        {
            name = "experiment1";
            experimentConfigFilename = "greatExperiment.Any"; 
            userStatusFilename = "greatUserstatus.Any"; 
        },
        {
            name = "experiment2";
            experimentConfigFilename = "betterExperiment.Any"; 
            userStatusFilename = "betterUserstatus.Any"; 
        },
    ];
}
```