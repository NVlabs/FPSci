# Experiment Config Field Descriptions

There are a number of inputs to experiment config. The following is a description of what each one means, and how it is meant to be used.

* `settingsVersion` refers to the version of the contents of this file, unless you are aware of changes that need to be made, don't change this number.
* `appendingDescription` allows the user to annotate these results with a custom string
* `sceneName` provides the name of the (virtual) scene in which this experiment takes place
* `feedbackDuration` describes the duration of the feedback window between experiments
* `readyDuration` sets the time before the start of each trial
* `taskDuration` sets the maximum time over which the task can occur
* `fieldOfView` sets the (horizontal) field of view for the user's display
* `renderWeaponStatus` controls whether or not the weapon cooldown is rendered on the left side of the screen
* `weapon` provides a configuration for the weapon used in the experiment (for more info see [the weapon config readme](./weapon/weaponConfigReadme.md))
* `sessions` is a list of all sessions and their affiliated information:
    * `session id` is a short name for the session
    * `frameDelay` is an (integer) count of frames to delay to control latency
    * `frameRate` is the frame rate of the display (constant for a given session)
    * `expMode` is used to indicate an additional mode for affiliated sessions (such as `real` vs `training`)
    * `trials` is a list of trials referencing the `trials` table above:
        * `id` is a short name for the trial to affiliate with the `targets` or `reactions` table below
        * `count` provides the number of trials in this session
* `targets` this target config table contains more detailed constraints for path generation for targets:
    * `id` a short string to refer to this target information
    * `speed` is a vector indictating the minimum ([0]) and maximum ([1]) speeds in angular velocity (in deg/s)
    * `visualSize` is a vector indicating the minimum ([0]) and maximum ([1]) visual size for the target (in deg)
    * `distance` is the distance to this target (in meters)
    * `elevationLocked` indicates whether or not the target is locked to its initial elevation
    * `eccH/V` are controls for min ([0])/max([1]) horizontal/vertical eccentricity for target initial position (in deg)
    * `motionChangePeriod` is a vector indicating the minimum ([0]) and maximum ([1]) motion change period allowed (in s)
    * `jumpEnabled` determines whether the target can "jump" or not
    * `jumpPeriod` is a vector indicating the minimum ([0]) and maximum ([1]) period to wait between jumps (in seconds)
    * `jumpSpeed` is a vector indicating the minimum ([0]) and maximum([1]) angular speed with which to jump (in deg/s)
    * `accelGravity` is the min ([0])/max ([1]) acceleration due to gravity during the jump (in m/s^2)