# `abstract-fps`
`abstract-fps` is an tool for conducting user studies on first person shooter (FPS) style tasks. The name comes from our desire to use an abstract art style for the graphics, and the target set of tasks.

## G3D dependence
We depend on the [G3D innovation engine](https://casual-effects.com/g3d) for much of our functionality. [Installation instructions](https://casual-effects.com/g3d/www/index.html#install).

When you check out G3D, you'll need some data files from the `research` and `game` subdirectories so you can't just checkout the `common` subdirectory.

## Dependency setting for Psychophysics library
We currently depend on an unreleased psychophysics library. If you have that library, you should set up an environment variable as follows:
- name: PSYCHOPHYSICS
- value: the root folder of psychophysics-lib

## Instructions for running the experiment

There are two files that control the experiment settings, `userconfig.Any` and `experimentconfig.Any`. When you start the application, files with these names must be present in the data path. The standard location during development is the `data-files` directory.

`userconfig.Any` contains settings that pertain to the user, such as the `subjectID`, `mouseDPI` and `cmp360` settings. One way to set this up effectively is to have a settings file per user and switch between them as you switch users.

`experimentconfig.Any` allows you to set up the various experiment settings, including `targetFrameRate`, `expMode`, `taskType` and `appendingDescription`. More details can be found in [experimentConfigReadme.md](/code/data-files/experimentConfigReadme.md). It is recommended to set up a list of experimental settings you would like to run ahead of time, and switch between them when executing.
