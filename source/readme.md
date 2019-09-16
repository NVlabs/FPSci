# `source` Directory
This directory contains all source files for the `abstract-fps` application.

## Source File Descriptions
Below are some short descriptions for each file in this directory. Refer to the code/comments for more details.

* [`App.cpp/h`](./App.h) contains the core application code, binding to G3D's callback functions and handling user input/video output
* [`Experiment.cpp/h`](./Experiment.h) contains the experiment-specific controls and the 
* [`ExperimentConfig.h`](./ExperimentConfig.h) contains classes for serializing all configuration structures from `.Any` files
* [`Logger.cpp/h`](./Logger.h) contains the logger class used for managing/writing structured experiment data to the output SQL database
* [`SingleThresholdMeasurement.cpp/h`](./SingleThresholdMeasurement.h) serves as a helper for `Experiment.cpp/h` containing most of the low-level psychophysics primitives plus a simple dictionary-style `param` class
* [`TargetEntity.cpp/h`](./TargetEntity.h) contains functions for creating, displaying, and animating targets
* [`sqlHelpers.cpp/h`](./sqlHelpers.h) contains a simple wrapper for low-level SQLite3 operations that make interacting with the output database convenient