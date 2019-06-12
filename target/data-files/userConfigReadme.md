# Introduction
The user config is the mechanism by which users are registered and provide their input sensitivity in the `abstract-fps` application.

The high-level `userconfig.Any` file provides the `currentUser` (the default user when launching the application), together with a `users` table that contains per subject mouse DPI and sensitiviy (in cm/360°). 

## User Table
Each entry in the user table contains the following fields:

* `id` a quick ID used to identify the user
* `mouseDPI` the mouse DPI used for the player
* `cmp360` the mouse sensitivity for the user (measured in cm/360°)