# How experiment configs work

There are a number of inputs to experiment config. The following is a description of what each one means, and how it is meant to be used.

* `targetFrameRate` : sets a target frame rate for the experiment. The application will slow down to hit that rate, but cannot exceed what it is capable of (based on CPU/GPU/RAM/etc.)
* `settingsVersion` : set to "1" for now. May be used to support multiple file versions in the future
* `expMode` : one of either `"training"` or `"real"`. `"training"` indicates that the trials will not be used for final study results while `"real"` are the real results. Training may also include some instruction for the user to learn the task.
* `taskType` : this describes which type of task the user will be running. The list of tasks is later in this document.
* `expVersion` : this appears to set the type of motion for `"targeting"` tasks only. Valid values are `"SimpleMotion"` and `"ComplexMotion"`.
* `appendingDescription` : a string to append to the results logged in the database

## taskType

Valid task types are the following:

* `"reaction"` : a simple reaction test. The screen starts red, then when it turns green the user should click and the reaction time is recorded.
* `"targeting"` : the user is placed in a room and a series of targets appears which the user must click on. TODO: how many trials, how many targets, etc.?
