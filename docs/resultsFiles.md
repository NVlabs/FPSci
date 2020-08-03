# FPSci Result File Readme
This file documents the function of various tables and fields within the output SQL database provided by FPSci on a per-session basis.

Generally speaking the current FPSci results files are **not** considered broadly merge safe (i.e. multiple, possibly simultaneous sessions across multiple users cannot be generically merged into a single database without issue). However, multiple sequential sessions from a single user should be more or less merge safe.

## Database Format
The FPSci output database is a SQLite database with time strings provided in one of the standard/supported SQL time formats. It should work with most common SQLite tools. For more tips on querying SQLite databases see the [Useful Queries section below](#useful_queries).

## Results Tables
This section outlines the high-level results tables, with more info provided on each below.

* [`Frame_Info`](#frame_info): Timing information about each frame presented to the user during the session
* [`Player_Action`](#player_action): Information about each aim/fire point the player made during the session
* [`Questions`](#questions): Results from questions answered using the in-app questions systems
* [`Target_Trajectory`](#target_trajectory): The position of each target (in cartesian coordiantes) over time
* [`Targets`](#targets): The high-level parameters/randomized ranges used to spawn a particular target
* [`Trials`](#trials): High-level information about each trial and it's completion
* [`Users`](#users): Information about the user(s) who took part in this session

### Frame_Info
The `Frame_Info` table is intended primarily for debugging issues with rendering and display performance in local systems. The table contains just 2 columns:

* `time`: The (wall clock) time at which a frame occurred
* `sdt`: The simulation time delta that matches this frame

Looking for variation in the `sdt` column values can help detect/verify conditions like frame stutter and other timing issues.

### Player_Action
The `Player_Action` table is the primary tool for analyzing player move, aim, and fire actions in more detail. It includes the following columns:

* `time`: The (wall clock) time at which the actions was logged
* `position_az`: The player aim position azimuth
* `position_el`: The player aim position elevation
* `position_x`: The player world position (translation) X coordinate
* `position_y`: The player world position (translation) Y coordinate
* `position_z`: The player world position (translation) Z coordinate
* `event`: The event that occurred at this entry, possible options include:
    * `invalid`: Occurs when a player fires during the weapon cooldown period during a trial
    * `non-task`: Occurs when the player fires outside of the trial (including destroying a reference target)
    * `aim`: This row records a player aim direction without a fire event occurring
    * `miss`: Occurs when a player fires and misses. When using hit scanned weapons this is at the time of the fire, otherwise it is when the (propogated) projectile intersects geometry that is not a target
    * `hit`: Occurs when a player fires and hits a target during a trial
    * `destroy`: Occurs when a player fires, hits a target, and destroys that target with this hit during a trial
* `target_id`: When the event is `hit`, `destroy`, or `non-task` (destroying the reference target) this records the target that was interacted with by the player action. When the reference target is destroyed this field will be `reference`. Otherwise this field is an empty string.

### Questions
The `Questions` table is intended to quickly capture feedback from questions asked of the user in app using the simple dialog system at the end of a session. It includes the following columns:

* `session`: The session id of the session in which the question was asked
* `question`: The text of the question asked of the user
* `response`: The response provided by the user

### Sessions
The `Sessions` table is the highest-level description in the per-session results files. By default the sessions table supports the following columns:

* `sessionID`: The id of this sesion
* `time`: The (wall clock) time at which the session started/the results file was created
* `subjectID`: The subject who took part in this session
* `appendingDescription`: The experiment description appended to the session description (separated by a `/`)

In addition to the default fields provided above, the user can provide additional parameters (by name) in the [`sessParamsToLog` field](general_config.md#logging_controls) which are added to this table. Any session-level configuration parameter should be supported for logging here.

###  Target_Trajectory
The `Target_Trajectory` table describes the motion of targets within the session. Each target trajectory entry includes the following columns:

* `time`: The (wall clock) time at which the target position was logged
* `target_id`: The name of the target being logged (specific to the trial type and target, but not unique to individual trials)
* `position_x`: The target world position (translation) X coordinate
* `position_y`: The target world position (translation) Y coordinate
* `position_z`: The target world position (translation) Z coordinate

### Targets
The `Targets` table is intended to provide additional detailed infomration about each type of target that was spawned within a trial. Both the parametric spawn information and the selected values are logged to this table. Table columns include:

* `name`: The target name, corresponding to the name logged in the [`Target_Trajectory` table](#target_trajectory) and notably, not unique to an individual trial
* `spawn_time`: The time at which this target spawned (assumed unique to an individual trial)
* `type`: The type of the target (can be `waypoint` or `parametrized`)
    * `waypoint` based targets do not use randomized parameters, and instead move between a set of predefined waypoints
    * `parametrized` targets randomize various motion parameters within provided ranges and create new target paths based on these values
* `destSpace`: The destination space used by the target (can be `player` or `world`)
    * `player` space targets move about the player and are typically only supported with `parametrized` types
    * `world` space targets can be specified as either `waypoint` based or `parametrized` with certain world-space bounds they cannot leave
* `size`: This records the actual size of the target (useful when randomized in a range for the `parametrized` type)

#### Parametric Target Info
The following columns are only valid for `parametrized` target types. They can/should be ignored for all `waypoint` targets.

* `spawn_ecc_h`: The (randomized) horizontal spawn eccentricity of the target
* `spawn_ecc_v`: The (randomized) vertical spawn eccentricity of the target
* `min_size`: The minimum spawned target size (low end of randomized range)
* `max_size`: The maximum spawned target size (high end of randomized range)
* `min_ecc_h`: The minimum horizontal spawn eccentricity (low end of randomized range)
* `max_ecc_h`: The maximum horizontal spawn eccentricity (high end of randomized range)
* `min_ecc_v`: The minimum vertical spawn eccentricity (low end of randomized range)
* `max_ecc_v`: The maximum vertical spawn eccentricity (high end of randomized range)
* `min_speed`: The minimum movement speed the target can select on motion change
* `max_speed`: The maximum movement speed the target can select on motion change
* `min_motion_change_period`: The minimum time after which target motion can change
* `max_motion_change_period`: The maximum time after which target motion can change
* `jump_enabled`: Whether the target was allowed to perform "jump" motions
* `model_file`: The name of the model file used for this target

### Trials
The `Trials` table provides more detailed feedback on user performance within each trial. Table columns include:

* `trial_id`: The ID (index) of the trial being performed within the session (i.e. its index within the session specifications `trials` array)
* `session_id`: The ID string of the session the trial was performed within
* `session_mode`: Historically named, the description field from the session the trial was logged from within
* `block_id`: The ID (index) of the current block being performed within the session (repeats trials)
* `start_time`: The (wall clock) start time of the trial (useful for affiliating player/target actions)
* `end_time`: The (wall clock) end time of the trial (useful for affiliating player/target actions)
* `task_execution_time`: The total time spent in the task state within this trial
* `destroyed_targets`: A count of total targets destroyed within this trial
* `total_targets`: A count of the total targets to be presented in this trial (if an unlimited number of targets has been specified this value is `-1`).

### Users
The users table provides user-based information for the logged session. The table is logged to once at the start and once at the end of each session to allow those performing data anaylsis to be aware if user settings changed during the session. The table includes the following columns:

* `subjectID`: The subject ID of the user
* `session`: The ID of the current session
* `time`: Either `start` or ``end` indicating whether this entry was logged at the start or end of the session
* `cmp360`: The "raw" user mouse sensitivity in cm/360°
* `mouseDPI`: The user mouse DPI setting reported
* `reticleIndex`: The index of the reticle the user has selected
* `reticleScaleMin`: The minimum scale of the reticle (long after a shot)
* `reticleScaleMax`: The maximum scale of the reticle (shortly after a shot)
* `reticleColorMinScale`: The reticle color when at the minimum sclae (long after a shot)
* `reticleColorMaxScale`: The reticle color when at the maximum scale (shortly after a shot)
* `reticleChangeTime`: The time it takes for the reticle to scale/change colors following a shot
* `userTurnScaleX`: The user provided X turn scale
* `userTurnScaleY`: The user provided Y turn scale (negative if inverted)
* `sessTurnScaleX`: The session provided X turn scale (stacks with user turn scale)
* `sessTurnScaleY`: The session provided Y turn scale (stacks with user turn scale)
* `sensitivityX`: The composite X sensitivity (`cmp360 * userTurnScaleX * sessTurnScaleX`) in cm/360°
* `sensitivityY`: The composite Y sensitivity (`cmp360 * userTurnScaleY * sessTurnScaleY`) in cm/360°

## Useful Queries
FPSci results files can be queried a variety of ways but some common/useful queries are included below for reference:

### Time-based Selection
Selecting all frame info, player actions, or target motions from within a trial (by time) can be done using a SQL subquery approach as demonstrated below:

```
SELECT [field(s)] FROM [table(s)]
WHERE [table].time BETWEEN [start] AND [end]
```

This is a common approach for segmenting data by trial when not considering trials that could have been run concurrent (i.e. at the same wall clock time).

### Getting Time Differences in SQLite
If you'd like to perform a time difference between dates provided in the results file use of the `julianday()` method helps. For example:

```
SELECT 24*3600*(julianday(end_time) - julianday(start_time)) AS time_s FROM Trials
```