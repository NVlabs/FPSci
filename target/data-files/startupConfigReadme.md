# Introduction
The startup config is the highest level configuration file provided to the `abstract-fps` application. This file specifies the experiment configuration path, as well as the user configuration path and play mode.

## Fields
The following fields are valid for a startupconfig.Any file:

* `playMode` controls whether the application is run in experiment mode, or debug mode. When `playMode`=`True` the application is in experiment mode. Here the application runs fullscreen as intended for measurements. In debug mode (`playMode`=`False`) the application is drawn as windowed with additional dialogs available for user mouse interaction. This is recommended for developers only.
* `experimentConfigPath` sets the path to an [experiment config file](./experimentConfigReadme.md) for futher configuration of an experiment.
* `userConfigPath` sets the path to a user config file for per user setup.

The default `startup.Any` file is included below:

```
"playMode" = true;              // Set this to false for developer mode
"experimentConfigPath" = "";    // Leave this empty for default "experimentconfig.Any"
"userConfigPath" = "";          // Leave this empty for default "userconfig.Any"
```