# Introduction
The user config is the mechanism by which users are registered and provide their input sensitivity in `FirstPersonScience`.

The high-level `userconfig.Any` file provides the `currentUser` (the default user when launching the application), together with a `users` table that contains per subject mouse DPI and sensitiviy (in cm/360°).

The user config is setup to work across multiple experiments (i.e. it does not have any information specific to an `experimentconfig.Any` file contained within in). For per user session ordering see the [`userStatus.Any`](./userStatusReadme.md).

## File Location
The [`userconfig.Any` file](../data-files/userconfig.Any) is located in the [`data-files` directory](../data-files/) at the root of the project. If no `userconfig.Any` file is present the application copies [`SAMPLEuserconfig.Any`](../data-files/SAMPLEuserconfig.Any) to `userconfig.Any`.

# User Table
Each entry in the user table contains the following fields:

* `id` a quick ID used to identify the user
* `mouseDPI` the mouse DPI used for the player
* `cmp360` the mouse sensitivity for the user (measured in cm/360°)

Refer to the [SAMPLEuserconfig.Any file](SAMPLEuserconfig.Any) for an example of these settings.

### Sensitivity Measure (cm/360°)
Many games use arbitrary mouse sensitivity measures (i.e. 0-10 or low, mid, high scale) to set mouse sensitivity based on iterative player testing (i.e. choose a setting, test whether it meets your needs, adjust the setting accordingly). Unfortunately this means that mouse sensitivity settings can often not be easily translated between applications without a tool to convert between the settings values.

The unit of mouse sensitivity measure selected for abstract-fps is `cm/360°`. This measure reports the (linear) distance the mouse needs to travel in order to produce a full 360° rotation of the player view in game (typically in the horizontal direction). The idea being that a given player could measure this distance for their game of choice and then quickly transfer the setting into the abstract-fps application.

As an added complexity this sensitivity measure requires the user also report the mouse Dots Per Inch (DPI) setting so that signaling from the mouse can be correctly converted to centimeters traveled. This also means that while mice with dynamic DPI setting (i.e. a DPI button) will work fine; however, only a single DPI setting can be affiliated with the cm/360° sensitivity recorded for the mouse.