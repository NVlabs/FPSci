# How experiment configs work

There are a number of inputs to experiment config. The following is a description of what each one means, and how it is meant to be used.

* `settingsVersion` refers to the version of the contents of this file, unless you are aware of changes that need to be made, don't change this number.
* `playMode` controls whether or not you are in the experimental play mode (`true`) or developer mode (`false`)
* `appendingDescription` allows the user to annotate these results with a custom string
* `sceneName` provides the name of the (virtual) scene in which this experiment takes place
* `taskType` refers to the task at hand (either `target` or `reaction` for now)
* `feedbackDuration` describes the duration of the feedback window between experiments?
* `readyDuration` sets the time before the start of each trial?
* `sessionOrder?` may be used to do some automatic session ordering within the experiment
* `sessions` is a list of all sessions and their affiliated information:
    * `session id` is a short name for the session
    * `FrameDelay` is an (integer) count of frames to delay to control latency
    * `FrameRate` is the frame rate of the display (constant for a given session)
    * `SelectionOrder` describes the order of trial selection from the trial set
    * `trials` is a list of trials referencing the `trials` table above:
        * `trial id` is a short name for the trial to affiliate with the parent table
        * `trainingCount` provides the number of training trials in this session
        * `realCount` provides the number of measured trials in this session
* `trials` this trials table contains more detailed constraints for path generation within that trial:
    * `trial id` a short string to refer to this trial
    * `minSpeed` minimum velocity (your interpretation) for the trial
    * `maxSpeed` maximum velocity (your interpretation) for the trial
    * `visualSize` the visual size of the target for this trial
    * `min/maxEccH/V` are controls for min/max horizontal/vertical eccentricity for target paths
    * `motionChangePeriod` describes how often the motion path should change