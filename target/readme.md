# Introduction
The `abstract-fps` application is a simple FPS-style tool designed to help study frame rate, latency, and other effects in a controlled environment. The application is implemented in the [G3D Innovation Engine](https://casual-effects.com/g3d/www/index.html) and requires the latest version of G3D to be run. All additional resources required to run the application are included in this git repository.

The application implements a simple mouse-controlled view model with a variety of parameters controllable through various `.Any` files (more on this below). The scene, weapon, target size/behavior, and frame rate/latency controls are all available via this interface.

# Organization and Terminology
Since the `abstract-fps` application was developed for use in scientific studies it adopts somewhat standard experimental terminology from psychophysics for much of its organization. This section describes the various levels of control in the application and provides the terminology by which they will be referred to for the remainder of this documentation.

The `experiment` is the highest level "grouping" of tasks in the application. An experiment is defined by a set of parameters specified in an [`experimentconfig.Any`](./data-files/experimentConfigReadme.md) which include information about included `sessions`, as well as more general scene, weapon, and timing configurations.

A `session` is the next level of task grouping in the application. A session consists of frame-based information (control over `frameDelay` and `frameRate` parameters) as well as list of `trails` along with their counts.

A `trial` is the lowest level grouping of task in the application. From a user-perspective the trial represents a single target being spawned, traveling, and either being shot or missed within a given time period. In specifying trials the developer enters trial groups, which are sets of trials specified using a trial type id and count for one or multiple trial types.

# Config `.Any` Files
As mentioned above a variety of `.Any` files (similar to JSON file format) are used to control the behavior of the application. These files are as follows:

* [`startupconfig.Any`](./data-files/startupConfigReadme.md) is a simple configuration file for setting the `playMode` (debug feature) and pointing to the experiment and user configs
* [`experimentconfig.Any`](./data-files/experimentConfigReadme.md) is the primary configuration for the experiment. It includes scene, target, and weapon information and also establishes the sessions to take place during the experiment
* [`userconfig.Any`](./data-files/userConfigReadme.md) holds user information for (multiple) users including mouse sensitivity and DPI
* [`userstatus.Any`](./data-files/userStatusReadme.md) keeps track of both the session ordering and the completed sessions for any given user
* [`weaponconfig.Any`](./data-files/weapon/weaponConfigReadme.md) can be optionally included (using the `#include("filename")` option in the .Any format) to allow quick swap of weapons across multiple configs
* [`systemconfig.Any`](./data-files/systemConfigReadme.md) optionally configures an attached hardware click-to-photon monitor and provides (as output) specs from the system the application is being run on

## Managing Configurations
One reason for using the modular `.Any` file configuration structure shown above is the ability to track multiple experiments/sessions/trials without a need to perform major edits on the files. For example, an `experimentconfig.Any` file can either be renamed or can make use of the `#include("[filename]")` syntax provided by G3D's `.Any` parsing to include entire `.Any` files as sections in a larger file.

This allows users to create `.Any` files for a wide variety of experiments, weapons, users, and even system configurations, then swap between these at runtime by simply changing filenames or `#include` statements.