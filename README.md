# `abstract-fps` Application
The `abstract-fps` application is a tool for conducting user studies on first person shooter (FPS) style tasks. The name comes from our desire to use an abstract art style for the graphics, and the target set of tasks.

## G3D dependence
We depend on the [G3D innovation engine](https://casual-effects.com/g3d) for much of our functionality. [Installation instructions](https://casual-effects.com/g3d/www/index.html#install).

When you check out G3D, you'll need some data files from the `research` and `game` subdirectories so you can't just checkout the `common` subdirectory.

## Project Location and Tools
The most current/up-to-date version of the `abstract-fps` experiment solution can be found at [`target/abstract-fps.sln`](./target/abstract-fps.sln). 

This solution file can be built/run using [Visual Studio](https://visualstudio.microsoft.com/vs/). If you install G3D using its `.hta` installer, then this will automatically install Visual Studio Community Edition for you.

## Instructions for running the experiment
For more information on how to run the experiment and configure its `config.Any` files refer to the [target experiment readme](./target/readme.md).

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
