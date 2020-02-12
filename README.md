# `FirstPersonScience` Application
The `FirstPersonScience` application (`FPSci` for short) is a tool for conducting user studies on first person shooter (FPS) style tasks. 

## G3D dependence
FPSci depends on the [G3D innovation engine](https://casual-effects.com/g3d) which is a prerequisite for building FPSci. [Installation instructions](https://casual-effects.com/g3d/www/index.html#install).

When you check out G3D, you'll need some data files from the `research` and `game` subdirectories so you can't just checkout the `common` subdirectory.

If you install G3D using its `.hta` installer, then this will automatically install Visual Studio Community Edition for you.

### G3D with VS2017
If you are using this repo with VS2017 you will want to use Revision [R6939](https://sourceforge.net/p/g3d/code/6939/) as this is the last version of G3D without any VS2019 updates.

## Building instructions
The solution file (`FirstPersonScience.sln`) can build and run using [Visual Studio](https://visualstudio.microsoft.com/vs/). 

## Instructions for configuring and running an experiment
`FirstPersonScience` implements a simple mouse-controlled view model with a variety of parameters controllable through various `.Any` files (more on this below). The scene, weapon, target size/behavior, and frame rate/latency controls are all available via this interface.

When running, the keypad `-` button can be pressed to exit the application, or you can press `TAB` or `ESC` to toggle the user configuration menu, which has a "quit" button in it.

### Organization and Terminology
Since `FirstPersonScience` was developed for use in scientific studies it adopts somewhat standard experimental terminology from psychophysics for much of its organization. This section describes the various levels of control in the application and provides the terminology by which they will be referred to for the remainder of this documentation.

The `experiment` is the highest level "grouping" of tasks in the application. An experiment is defined by a set of parameters specified in an [`experimentconfig.Any`](docs/experimentConfigReadme.md) which include information about included `sessions`, as well as more general scene, weapon, and timing configurations.

A `session` is the next level of task grouping in the application. A session can reconfigure many of the (global default) properties setup in the experiment (it is assumed some property of interest is typically varied between sessions) as well as list of `trials` to include in this session along with their counts.

A `trial` is the lowest level grouping of task in the application. From a user-perspective the trial represents one or more targets being spawned, traveling, and either being shot or missed within a given time period. In specifying trials the developer enters trial groups, which are sets of trials specified using a target type id and count for one or multiple target types.

For more general information related to `FirstPersonScience` check out the [documentation directory](docs).

### Config `.Any` Files
As mentioned above a variety of `.Any` files (similar to JSON file format) are used to control the behavior of the application. These files are as follows:

* [`startupconfig.Any`](docs/startupConfigReadme.md) is a simple configuration file for setting the `playMode` (debug feature) and pointing to the experiment and user configs
* [`experimentconfig.Any`](docs/experimentConfigReadme.md) is the primary configuration for the experiment. It includes scene, target, and weapon information and also establishes the sessions to take place during the experiment
* [`userconfig.Any`](docs/userConfigReadme.md) holds user information for (multiple) users including mouse sensitivity and DPI
* [`userstatus.Any`](docs/userStatusReadme.md) keeps track of both the session ordering and the completed sessions for any given user
* [`weaponconfig.Any`](docs/weaponConfigReadme.md) can be optionally included (using the `#include("filename")` option in the .Any format) to allow quick swap of weapons across multiple configs
* [`systemconfig.Any`](docs/systemConfigReadme.md) optionally configures an attached hardware click-to-photon monitor and provides (as output) specs from the system the application is being run on

All configuration files referenced above can be found within the [`data-files`](data-files) directory.

#### Managing Configurations
One reason for using the modular `.Any` file configuration structure shown above is the ability to track multiple experiments/sessions/trials without a need to perform major edits on the files. For example, an `experimentconfig.Any` file can either be renamed or can make use of the `#include("[filename]")` syntax provided by G3D's `.Any` parsing to include entire `.Any` files as sections in a larger file.

This allows users to create `.Any` files for a wide variety of experiments, weapons, users, and even system configurations, then swap between these at runtime by simply changing filenames or `#include` statements.

## Repository Organization
A quick guide is provided below to this repository structure:

* [`source`](source) holds the `FirstPersonScience` c++ source files
* [`data-files`](data-files) holds scene and model files as well as the `.Any` files to configure the experiment
* [`docs`](docs) contains documentation to aid experiment designers
* [`scripts`](scripts) contains some useful scripts for developers including some basic data analysis
* `results` is created when executing `FirstPersonScience` and holds the results `.db` files from each experiment

## Source Organization
The source files (located in the [`source`](source) directory) implement a fairly typical G3D application structure with additional classes/files for many peripheral features. We outline the purpose of each of these source files below.

### Application Code
All graphics and scene-related routines are in [`App.cpp`](source/App.cpp) and its [affiliated header file](source/App.h). The `App` class inherits `GApp` from G3D and thus controls all rendering. Unlike many other applications this `App` class overrides the `oneFrame()` method from within `GApp` to allow us to reorder the render loop to reduce latency.

Several helper classes/files have been created to assist with application-level tasks. 

* The `PhysicsScene` class/[file](source/PhysicsScene.cpp) is borrowed (w/ some modifications) from the G3D `simpleGame` example. This class extends the typical `Scene` by adding support for a tree based collision model, using a sphere as a proxy for the player.
* `PlayerEntity` class/[file](source/PlayerEntity.cpp) is also borrowed from the G3D `simpleGame` example. The class provides a controllable player-entity (rather than no-clip camera) that works well with the `PhysicsScene`
* `TargetEntity` class/[file](source/TargetEntity.cpp) is a custom class implementing different target types that inherit a common, generic `TargetEntity` (which in turn inherits G3D's `VisibleEntity`)
* The `GuiElements` class/[file](source/GuiElements.cpp) contains some custom GUI menus designed to work specifically with this application
* The [`Dialogs.h` file](source/Dialogs.h) contains some generic dialog box classes that are used for asking questions of the user following a session.
* The [`PyLogger.h` file](source/PyLogger.h) encapsulates the calls used to launch the LDAT-R python logging tools from within the application when running FPSci with an LDAT-R device present.

### Experiment/Session Code
The majority of experiment/session level management occurs within [`Session.cpp`](source/Session.cpp) and it's [affiliated header file](source/Session.h). This includes configuring sessions, controlling the per-trial/session state, and logging results.

Experiment/session-level helpers include:
* [`Logger.cpp`](source/Logger.cpp) / [`Logger.h`](source/Logger.h) which manage logging for the session
* [`ExperimentConfig.h`](source/ExperimentConfig.h) which contains the `.Any` serializable classes for all session/experiment control
* [`Param.h`](source/Param.h) which is used as a sort of dynamically typed dictionary to help consolidate parameter passing
