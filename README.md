# `abstract-fps` Application
The `abstract-fps` application is a tool for conducting user studies on first person shooter (FPS) style tasks. The name comes from our desire to use an abstract art style for the graphics, and the target set of tasks.

## G3D dependence
We depend on the [G3D innovation engine](https://casual-effects.com/g3d) for much of our functionality. [Installation instructions](https://casual-effects.com/g3d/www/index.html#install).

When you check out G3D, you'll need some data files from the `research` and `game` subdirectories so you can't just checkout the `common` subdirectory.

## Project Location and Tools
The most current/up-to-date version of the `abstract-fps` experiment solution can be found at [`target/abstract-fps.sln`](./target/abstract-fps.sln). 

This solution file can be built/run using [Visual Studio](https://visualstudio.microsoft.com/vs/). If you install G3D using its `.hta` installer, then this will automatically install Visual Studio Community Edition for you.

## Instructions for running the experiment

There are two files that control the experiment settings, `userconfig.Any` and `experimentconfig.Any`. Samples of these files are provide in the `data-files` directory, so you can copy the `SAMPLEuserconfig.Any` to `userconfig.Any` and `SAMPLEexperimentconfig.Any` to `experimentconfig.Any` prior to running the application. The standard location during development is the `data-files` directory. If the application fails to find `userconfig.Any` and `experimentconfig.Any` when it loads, it will silently copy the two sample files and put the config files in the `data-files` directory.

`userconfig.Any` contains settings that pertain to the user, such as the `subjectID`, `mouseDPI` and `cmp360` settings. One way to set this up effectively is to have a settings file per user and switch between them as you switch users.

`experimentconfig.Any` allows you to set up the various experiment settings, including `targetFrameRate`, `expMode`, `taskType` and `appendingDescription`. More details can be found in [experimentConfigReadme.md](/code/data-files/experimentConfigReadme.md). It is recommended to set up a list of experimental settings you would like to run ahead of time, and switch between them when executing.

## File Organization
This repository is used as the working development enivronment for the NVR NXP team so there are a variety of folders available at the top-level. For most causual users the [`target`](./target) directory has the copy of the `abstract-fps` application with the most recent fixes and cleanest code structure.

A quick guide is provided below to this directory structure:

* [`code`](./code) is the working directory for old development code (unused for most users)
* [`newGfx`](./newGfx) is a working directory for an optimized example project setup by Morgan McGuire (unused for most users)
* [`reaction`](./reaction) contains a project affiliated with measuring reaction time (currently unsupported)
* [`scripts`](./scripts) contains some useful scripts for developers
* [`target`](./target) has the primary `abstract-fps` application code cleaned up for users

## Progress tracking document (Nvidia employees only)
https://docs.google.com/spreadsheets/d/1rxkRC-GVi-nCIIElz8XpO-7lhCwv9De2x2wF_ZM9Vnk/edit?usp=sharing
