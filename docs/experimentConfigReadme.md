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
    * `blockCount` is an integer number of (repeated) groups of trials within a session, with the block number printed to the screen between "blocks" (or a single "default" block if not provided).
    * `trials` is a list of trials referencing the `trials` table above:
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
            "ids" : ["simple_target", "destination-based"],
            "count" : 10
        ]
    },
],
```

### Target Configuration
The `targets` array specifies a list of targets each of which can contain any/all of the following parameters. The following sections provide a more detailed breakdown of target parameters by group.

#### Basic Configuration 
The following configuration is universal to all target types.

* `id` a short string to refer to this target information
* `respawnCount` is an integer providing the number of respawns to occur. For non-respawning items use `0` or leave unspecified. A value of `-1` creates a target that respawns infinitely (trial ends when ammo or task time runs out).
* `visualSize` is a vector indicating the minimum ([0]) and maximum ([1]) visual size for the target (in deg)
* `colors` is an array of 2 colors (max and min health) which are interpolated between based on target damage (note this setting overrides the experiment or session-level [`targetHealthColors`](general_config.md#target-rendering) setting). If unspecified the experiment/session level settings are used.
* `gloss` is a `Color4` representing glossyness, the first 3 channels are RGB w/ alpha representing minimum reflection (F0). Set all channels to 0 or do not specify to disable glossy reflections (note this setting overrides the experiment or session-level [`targetGloss`](general_config.md#target-rendering) setting). If unspecified the experiment/session level settings are used.
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
