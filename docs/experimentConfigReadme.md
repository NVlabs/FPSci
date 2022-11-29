# Introduction
The experiment config is by far the most elaborate of the configuration files. It sets a number of "world" parameters that will be constant across any given experiment. Many high-level parameters are implemented here, though most are optional.

Broad areas of control included in this config file include:
* Scene, field of view, and rendering parameters
* Timing for experimental states
* Universal player parameters like the move rate, HUD control, and walk simulation
* Information on the weapon to use
* Session setup and target movement

For a full description of fields see the descriptions below. Along with each subsection an example configuration is provided. In many cases the example values below are the defaults provided in these field values. Where this is not the case the default will be indicated in the comments.

## File Location
The `experimentconfig.Any` file is located in the [`data-files`](../data-files/) directory at the root of the project. If no `experimentconfig.Any` file is present at startup, a default experiment configuration is written to `experimentconfig.Any`.

## Experiment Heirarchy
FPSci uses a multi-tiered experiment and configuration heirarcy to manage presentation of stimulus to users. Configuration is inherited by each new lower level of the heirarchy from its parents, with only some levels of the heirarcy supporting specification of [general configuration](general_config.md).

This heirarchy is intended to let developers re-use global configuration while tapering configuration towards specific conditions they'd like to change on a per session or trial level.

The heirarchy of FPSci specification is as follows:
- **Experiment**: Provides base-level general config at the highest-level (plus a few specific fields mentioned below)
  - **Session**: Supports different general config (plus a few specific fields mentioned below)
    - **Block**: A repeat of all tasks/trials below this level, *does not support configuration or questions*
      - **Task**: A grouping of trials with a count and specific affiliated questions, *does not support configuration*
        - **Trial**: Lowest-level (general) configuration, affiliates a (set of) target(s) with a task to run


*Note:* Tasks and blocks do not need to be specified in FPSci. If blocks are not specified, by default each session consists of only 1 block (no repeats of the tasks/trials below the session). If tasks are not specified the default behavior is that each trial becomes its own task, with a task count as specified in the trial. For more information on tasks see the description below.

# Experiment Config Field Descriptions

The experment config supports inclusion of any of the configuration parameters documented in the [general configuration parameter guide](general_config.md). In addition to these common parameters, there are a number of unique inputs to experiment config. The following is a description of what each one means, and how it is meant to be used.

* `description` allows the user to annotate this experiment's results with a custom string
* `closeOnComplete` closes the application once all sessions from the sessions array (defined below) are complete 
```
"description": "your description here",    // Description of this file (default = "default")
"closeOnComplete": false,                  // Don't close automatically when all sessions are complete
```

### Session Configuration
Each session can specify any of the [general configuration parameters](general_config.md) used in the experiment config above to create experimental conditions. If both the experiment level and the session level specify a field supported by the general configuration, the session value has priority and will be used for that session. The experiment level configuration will be used for any session that doesn't specify that parameter.

In addition to these general parameters each session also has a few unique parameters documented below.

* `sessions` is a list of all sessions and their affiliated information:
    * `session id` is a short name for the session
    * `description` is used to indicate an additional mode for affiliated sessions (such as `real` vs `training`)
    * `closeOnComplete` signals to close the application whenever this session (in particular) is completed
    * `randomizeTaskOrder` determines whether tasks are presented in the order they are listed or in a randomized order (if specified). `randomizeTrialOrder` is treated as `randomizeTaskOrder` to preserve its historical behavior.
    * `blockCount` is an integer number of (repeated) groups of trials within a session, with the block number printed to the screen between "blocks" (or a single "default" block if not provided)
    * `trials` is a list of trials referencing the `trials` table above:
        * `id` is an (optional) target ID that (if specified) is used for logging purposes. If unspecified the `id` defaults to the (integer) index of the trial in the `trials` array.
        * `ids` is a list of short names for the trial(s) to affiliate with the `targets` or `reactions` table below, if multiple ids are provided multiple target are spawned simultaneously in each trial
        * `count` provides the number of trials in this session (should always be an integer strictly greater than 0)

#### Session Configuration Example
An example session configuration snippet is included below:

```
"sessions" : [
    {
        "id" : "test-session",          // This is a short name for our session
        "description" : "test",         // This is an arbitrary string tag (for now)
        "closeOnComplete": false,       // Don't automatically close the application when the session completes
        "randomizeTaskOrder": true,     // Randomize order of tasks by default
        "frameRate" : 120,              // Example of a generic parameter modified for this session
        "blockCount" : 1,         // Single block design
        "trials" : [
            {
                // Single target example
                "ids" : ["simple_target"],
                "count": 20,
            },
            {
                // Multi-target example
                "ids" : ["simple_target", "world-space paramtetric", "example_target", "example_target"],
                "count" : 5
            }
        ]
    },
    {
        "id" : "minimal-session",       // This session inherits all settings from the experiment above it
        "trials" : [
            "id": "example trial";
            "ids" : ["simple_target", "destination-based"],
            "count" : 10
        ]
    },
],
```

### Task Configuration
FPSci tasks are a way to affiliate trials into meaningful grouping with repeat logic automatically managed.

Tasks are configured using the following parameters:
- `id` an identifier for the task, used for logging
- `trialOrders` an array of valid orderings of trials (referenced by `id`)
- `questions` a question(s) asked after each `trialOrder` is completed
- `count` a number of times to repeat each of the `trialOrders` specified in this task

Note that rather than being a direct `Array<Array<String>>` the `trialOrders` array needs to be an `Array<Object>` in order to parse correctly in the Any format. To do this the `trialOrders` array requires specification of an `order` array within it to accomplish this nesting, as demonstrated in the example below.

For example a task may specify presenting 2 trials (in either order) then asking a question comparing them. An example of this configuration is provided below:

```
tasks = [
    {
        id = "comparison";
        trialOrders = [
            {order = ["trial 1", "trial 2"]},       // Show trial 1/2 in this order
            {order = ["trial 2", "trial 1"]}        // Show trial 1/2 in reverse order
        ];

        // Present a question about which trial was preferred after any pairing of 2 trials
        questions = [
            {
                prompt = "Which trial did you prefer?";
                type = "MultipleChoice";
                options = ["First", "Second"];
            }
        ];
        
        // Repeat each order 10 times (20 total sets of 2 trials)
        count = 10;
    },
]
```
#### Task/Trial Interaction and Ordering
When no `tasks` are specified in a session configuration, FPSci treats trials as tasks creating one task per trial *type* in the [trial configuration](#trial-configuration). Note that this has impacts on the interpretation of `randomizeTrialOrder` when trials are treated as tasks.

As described [above](#session-configuration) the `randomizeTaskOrder` session-level configuration parameter allows the experiment designer to select tasks in either the order they are specified or a random order. When `tasks` are specified in a session `randomizeTrialOrder` randomizes within the `order` specified in each element of the `trialOrders` array. However,  in order to maintain compatibility with older configurations and avoid complicating configuration, when no `tasks` array is specified in a session (i.e., trials are treated as tasks) `randomizeTrialOrder` behaves the same as `randomizeTaskOrder`.


### Trial Configuration
Trials provide the lowest level of [general configuration](general_configuration.md) in FPSci. Trials are specified with the following parameters:
- `id` is a name to refer to this trial by in logging (and in [tasks](#task-configuration))
- `ids` is an array of target ids to present within this trial
- `count` is a count that is applied to this trial (when it is treated as a task)

As mentioned above, if no tasks are specified, each trial's `count` is used to generate a task with a single trial order with just this trial inside of it. This provides a fallback to pre-task FPSci configuration in which trials were run this way directly below blocks.

### Target Configuration
The `targets` array specifies a list of targets each of which can contain any/all of the following parameters. The following sections provide a more detailed breakdown of target parameters by group.

#### Basic Configuration 
The following configuration is universal to all target types.

* `id` a short string to refer to this target information
* `respawnCount` is an integer providing the number of respawns to occur. For non-respawning items use `0` or leave unspecified. A value of `-1` creates a target that respawns infinitely (trial ends when ammo or task time runs out).
* `visualSize` is a vector indicating the minimum ([0]) and maximum ([1]) visual size for the target (in deg)
* `colors` is an array of 2 colors (max and min health) which are interpolated between based on target damage (note this setting overrides the experiment or session-level [`targetHealthColors`](general_config.md#target-rendering) setting). If unspecified the experiment/session level settings are used.
* `gloss` is a `Color4` representing glossyness, the first 3 channels are RGB w/ alpha representing minimum reflection (F0). Set all channels to 0 or do not specify to disable glossy reflections (note this setting overrides the experiment or session-level [`targetGloss`](general_config.md#target-rendering) setting). If unspecified the experiment/session level settings are used.
* `emissive` is an array of 2 colors (max and min health) which are interpolated between based on the target damage (note this setting overrides the experiment or session-level [`targetEmissive`](general_config.md#target-rendering) setting). If unspecified the experiment/session level settings are used.
* `destSpace` the space for which the target is rendered (useful for non-destiantion based targets, "player" or "world")
* `hitSound` is a filename for the sound to play when the target is hit but not destroyed (for no sound use an empty string).
* `hitSoundVol` provides the volume (as a float) for the hit sound to be played at (default is `1.0`).
* `destroyedSound` is a filename for the sound to play when the target is both hit and destroyed (for no sound use an empty string).
* `destroyedSoundVol` provides the volume (as a float) for the destroyed sound to be played at (default is `1.0`).
* `destroyDecal` the decal to show when destroyed
* `destroyDecalScale` a scale to apply the the destroy decal (may be decal dependent)
* `destroyDecalDuration` is the duration to
* `modelSpec` is an `Any` that constructs an `ArticulatedModel` similar to that used in the [the weapon config readme](weaponConfigReadme.md). For now this spec needs to point to an `obj` file with a model named `core/icosahedron_default`.

#### Player-space (Parametric) Targets
The following configuration only applies to player-bound parametric targets.

* `speed` is a vector indictating the minimum ([0]) and maximum ([1]) speeds in angular velocity (in deg/s)
* `distance` is the distance to this target (in meters)
* `symmetricEccH/V` When `True` the eccH/V are assumed symmetric about their respective axes and should always be >0. When `False` the eccentricities can be signed, with positive eccentricities indicate values to the left (azim) and up (elev) of the central view direction.
* `eccH/V` are controls for min ([0])/max([1]) horizontal/vertical eccentricity for target initial position (in deg)
* `motionChangePeriod` is a vector indicating the minimum ([0]) and maximum ([1]) motion change period allowed (in s)
* `upperHemisphereOnly` is a boolean flag indicating whether target flies only on the upper hemisphere of player-centric sphere. Only applicable to `FlyingEntity` defined in the "player" space.
* `logTargetTrajectory` is a boolean flag indicating whether or not this (individual) target's position should be logged for trials it is displayed for
* `jumpEnabled` determines whether the target can "jump" or not
* `jumpPeriod` is a vector indicating the minimum ([0]) and maximum ([1]) period to wait between jumps (in seconds)
* `jumpSpeed` is a vector indicating the minimum ([0]) and maximum([1]) angular speed with which to jump (in deg/s)
* `accelGravity` is the min ([0])/max ([1]) acceleration due to gravity during the jump (in m/s^2)

#### World-space Target Specific Configuration
The following configuration parameters are specific to world-space targets:

* `destinations` is an array of `Destination` types each of which contains:
    * `t` the time (in seconds) for this point in the path
    * `xyz` the position for this point in the path
* `spawnBounds` specifies an axis-aligned bounding box (`G3D::AABox`) to specify the bounds for the target's spawn location in cases where `destSpace="world"` and the target is not destination-based. For more information see the [section below on serializing bounding boxes](##-Bounding-Boxes-(`G3D::AABox`-Serialization)).
* `moveBounds` specifies an axis-aligned bounding box (`G3D::AABox`) to specify the bounds for target motion in cases where `destSpace="world"` and the target is not destination-based. For more information see the [section below on serializing bounding boxes](##-Bounding-Boxes-(`G3D::AABox`-Serialization)).
* `axisLocked` is a boolean array specifying which (if any) axes of motion are "locked" (i.e. disallowed) for this target's motion in [X,Y,Z] order. This only applies for world-space, parametric targets.


#### Target Configuration Example
An example target configuration snippet is provided below:

```
targets = [
    {
        "id": "simple_target",
        "destSpace" : "player",                 // This is a player-centered-spherical-space target
        "visualSize" : [0.5, 0.5],              // 0.5m size
        "colors": [Color3(0,1,0), Color3(1,0,0)];   // Green at max health, red at min health
        "respawnCount" : 0,                     // Don't respawn
        "speed": [1.0, 3.0],                    // 1-3m/s speed
        "symmetricEccH" : true;                 // Target will only spawn randomly on any quadrant within specified eccH (between (-)15 to (-)5 or 5 to 15)
        "symmetricEccV" : true;                 // Target will only spawn randomly on any quadrant within specified eccH (between (-)5 to 0 or 0 to 5)
        "eccH" : [5.0, 15.0],                   // 5-15° initial spawn location (horizontal)
        "eccV" : [0.0, 5.0],                    // 0-5° intitial spawn location (vertical)
        "hitSound" : "sound/fpsci_ding_100ms.wav",      // Sound to play when target hit
        "hitSoundVol" : 1.0,                    // Volume to play the hit sound at
        "destroyedSound" : "sound/fpsci_destroy_150ms.wav", // Sound to play when target destroyed (explosion)
        "destroyedSoundVol" : 1.0f,             // Volume to play destroyed sound at
        "destroyDecal" : "explosion_01.png",    // Use default explosion decal
        "destroyDecalScale" : 1.0,              // Use default sizing
        "destroyDecalDuration" : 0.1,           // Show the decal for 0.1s (default)
    },
    {
        "id": "world-space paramtetric",
        "destSpace" : "world",                  // This is a world-space target
        "bounds" : AABox {
                Point3(-8.5, 0.5, -11.5),       // It is important these are specified in "increasing order"
                Point3(-6.5, 1.5, -7.5)         // All x,y,z coordinates must be greater than those above
        },
        "axisLocked": [true, false, false],
        "visualSize" : [0.3, 1.0],              // Visual size between 0.3-1m
        "respawnCount" : -1,                    // Respawn forever
    },
    {
        "id" : "destination-based",
        "destSpace" : "world",                  // Important this is specified here
        "destinations" : {
            {"t": 0.0, "xyz": Vector3(0.00, 0.00, 0.00)},
            {"t": 0.1, "xyz": Vector3(0.00, 1.00, 0.00)},
            ...
            {"t": 10.2, "xyz": Vector3(10.1, 1.01, -100.3)}
        },
        modelSpec = ArticulatedModel::Specification{
            filename = "model/target/sphere.obj";
        },
    },
    #include("example_target.Any"),             // Example of including an external .Any file
],
```

#### Target model notes

Target models can get quite large in file size when they're highly detailed. As a result, we only include a handful of different model files with the FPSci repo and binary distributions. We have made a high quality sphere model available that you can [download from google drive here](https://drive.google.com/file/d/1LvJaJUD3k7DR0taZYZ_9Y0PNVQDMdShM/view?usp=sharing). Once you get that file (named `high_poly_sphere.obj`), you can place it in the `data-files/model/target/` directory or the `FPSci/model/target/` directory if using a binary distribution. We recommend scripting the download of this type of file if you are building an automated experiment build on FPSci. These files can then be used by targets as follows:

```
targets = ( 
    { 
        id = "ico"; 
        destSpace = "player"; 
        speed = ( 0, 0 ); 
        visualSize = ( 0.05, 0.05 ); 
        modelSpec = ArticulatedModel::Specification{
            filename = "model/target/target.obj";
        };
    }, 
    { 
        id = "low"; 
        destSpace = "player"; 
        speed = ( 0, 0 ); 
        visualSize = ( 0.05, 0.05 ); 
        modelSpec = ArticulatedModel::Specification{
            filename = "model/target/low_poly_sphere.obj";
        };
    }, 
    { 
        id = "mid"; 
        destSpace = "player"; 
        speed = ( 0, 0 ); 
        visualSize = ( 0.05, 0.05 ); 
        modelSpec = ArticulatedModel::Specification{
            filename = "model/target/mid_poly_sphere.obj";
        };
    }, 
    { 
        // This one only works if you download the file from https://drive.google.com/file/d/1LvJaJUD3k7DR0taZYZ_9Y0PNVQDMdShM/view?usp=sharing
        id = "high"; 
        destSpace = "player"; 
        speed = ( 0, 0 ); 
        visualSize = ( 0.05, 0.05 ); 
        modelSpec = ArticulatedModel::Specification{
            filename = "model/target/high_poly_sphere.obj";
        };
    }, 
);
```

The above examples are borrowed from the Spheres experiment sample that is provided with FPSci. Feel free to try that out if you want to compare the target shapes in game.

## Target Paths (Using Destinations)
The `destinations` array within the target object overrides much of the default motion behavior in the target motion controls. Once a destinations array (including more than 2 destiantions) is specified all other motion parameters are considered unused. Once a `destinations` array is specified only the following fields from the [target configuration](#target-configuration) apply:

* `id`
* `visualSize`
* `respawnCount`
* `modelSpec`

When specifying a `destinations` array there are several key assumptions worth noting:
* All interpolation between points is linear w/ time. This means that velocity can be controlled using either timing or point location, points do not need to be uniformly sampled (i.e. any two destinations may have arbitrary time between them)
* The default behavior is to "loop" paths once they are complete to avoid requiring paths to match trial times, this will include a discontinuity (jump) in the target motion if the path is not a closed loop. If you want to avoid this behavior we suggest creating closed loop paths and including a duplicate beginning/end sample to guarantee smooth motion
* Time values can be specified at any precision, but the `oneFrame()` loop rate (ideally the frame rate) sets the "resampling" rate for this path, destinations whose time values are spaced by less than a frame time are not recommended

Currently the destination time values are specified as an increasing time base (i.e. 0.0 on the first destination up to the total time); however, in the future we could move towards/also include time deltas to allow for faster editing of files.

## Bounding Boxes (`G3D::AABox` Serialization)
The `G3D::AABox` is a 3D, axis-aligned bounding box useful for specifying regions of the scene in which a player/target can move.

### (De)serializing `G3D::AABox`es from `.Any` files
Like many G3D native objects the `G3D::AABox` supports direct (de)serialization from `.Any` using a simple definition. Any table value in the `.Any` which starts with an `AABox {}` specification will deserialize to a `G3D::AABox` object.

Within the `AABox{}` definition, only 2 points need be specified (a lower and upper corner, in that order) to produce a `G3D:AABox`.

### Note on Order of Specification
The `G3D::AABox` implementation seems to work bets when then `AABox` any specification lists the "lower" corner before the "upper". That is that all 3 coordinates of the first provided corner are _less than or equal to_ the 3 coordinates of the second provided corner.

Thus we always recommend specifying the `TargetConfig`'s `bounds` field as follows:

```
...
    "bounds" : AABox {
        Point3(x1, y1, z1),
        Point3(x2, y2, z2)
    }
...
```

Where `x1 < x2`, `y1 < y2`, and `z1 < z2`. Negative coordinates are not treated any differently here (do not use the magnitude of the points, just their values).
