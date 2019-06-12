# Experiment Config Field Descriptions

There are a number of inputs to experiment config. The following is a description of what each one means, and how it is meant to be used.

* `settingsVersion` refers to the version of the contents of this file, unless you are aware of changes that need to be made, don't change this number.
* `appendingDescription` allows the user to annotate these results with a custom string
* `taskType` set to  `reaction`
* `feedbackDuration` describes the duration of the feedback window between experiments
* `readyDuration` sets the time before the start of each trial
* `taskDuration` sets the maximum time over which the task can occur
* `fireSound` is the filename/location of the audio clip to use for the weapon firing
* `sessions` is a list of all sessions and their affiliated information:
    * `session id` is a short name for the session
    * `FrameDelay` is an (integer) count of frames to delay to control latency
    * `FrameRate` is the frame rate of the display (constant for a given session)
    * `SelectionOrder` describes the order of trial selection from the trial set
    * `trials` is a list of trials referencing the `trials` table above:
        * `id` is a short name for the trial to affiliate with the `targets` or `reactions` table below
        * `count` provides the number of trials in this session
* `reactions` this is a configuration table for reaction experiments:
    * `id` is a short string to refer to this reaction information
    * `minimumForeperiod` is the minimum amount of time to wait before performing a transition (in seconds)
    * `intensities` is an array of intensities (1 normalized) to use for various trial runs

## Startup Config
* `playMode` controls whether or not you are in the experimental play mode (`true`) or developer mode (`false`)
