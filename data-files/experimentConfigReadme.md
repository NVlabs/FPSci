# Introduction
The experiment config is by far the most elaborate of the configuration files. It sets a number of "world" parameters that will be constant across any given experiment. Many high-level parameters are implemented here, though most are optional.

Broad areas of control included in this config file include:
* Scene, field of view, and rendering parameters
* Timing for experimental states
* Universal player parameters like the move rate, HUD control, and walk simulation
* Information on the weapon to use
* Session setup and target movement

For a full description of fields see the descriptions below. Along with each subsection an example configuration is provided. In many cases the example values below are the defaults provided in these field values. Where this is not the case the default will be indicated in the comments.

# Experiment Config Field Descriptions

The experment config supports inclusion of any of the configuration parameters documented in the [general configuration parameter guide](../docs/general_config.md). In addition to these common parameters, there are a number of unique inputs to experiment config. The following is a description of what each one means, and how it is meant to be used.

* `description` allows the user to annotate this experiment's results with a custom string
```
"description": "your description here",    // Description of this file (default = "default")
```

### Session Configuration
Each session can specify any of the [general configuration parameters](../docs/general_config.md) used in the experiment config above to create experimental conditions. In addition to these general parameters each session also has a few unique parameters documented below.

* `sessions` is a list of all sessions and their affiliated information:
    * `session id` is a short name for the session
    * `description` is used to indicate an additional mode for affiliated sessions (such as `real` vs `training`)
    * `trials` is a list of trials referencing the `trials` table above:
        * `ids` is a list of short names for the trial(s) to affiliate with the `targets` or `reactions` table below, if multiple ids are provided multiple target are spawned simultaneously in each trial
        * `count` provides the number of trials in this session

#### Session Configuration Example
An example session configuration snippet is included below:

```
"sessions" : [
    {
        "id" : "test-session",          // This is a short name for our session
        "description" : "test",         // This is an arbitrary string tag (for now)
        "frameRate" : 120,              // Example of a generic parameter modified for this session
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
* `targets` this target config table contains more detailed constraints for path generation for targets:
    * `id` a short string to refer to this target information
    * `respawnCount` is an integer providing the number of respawns to occur. For non-respawning items use `0` or leave unspecified.
    * `visualSize` is a vector indicating the minimum ([0]) and maximum ([1]) visual size for the target (in deg)
    * `speed` is a vector indictating the minimum ([0]) and maximum ([1]) speeds in angular velocity (in deg/s)
    * `distance` is the distance to this target (in meters)
    * `elevationLocked` indicates whether or not the target is locked to its initial elevation
    * `eccH/V` are controls for min ([0])/max([1]) horizontal/vertical eccentricity for target initial position (in deg)
    * `motionChangePeriod` is a vector indicating the minimum ([0]) and maximum ([1]) motion change period allowed (in s)
    * `jumpEnabled` determines whether the target can "jump" or not
    * `jumpPeriod` is a vector indicating the minimum ([0]) and maximum ([1]) period to wait between jumps (in seconds)
    * `jumpSpeed` is a vector indicating the minimum ([0]) and maximum([1]) angular speed with which to jump (in deg/s)
    * `accelGravity` is the min ([0])/max ([1]) acceleration due to gravity during the jump (in m/s^2)
    * `modelSpec` is an `Any` that constructs an `ArticulatedModel` similar to that used in the [the weapon config readme](./weapon/weaponConfigReadme.md). For now this spec needs to point to an `obj` file with a model named `core/icosahedron_default`.
    * `destinations` is an array of `Destination` types each of which contains:
      * `t` the time (in seconds) for this point in the path
      * `xyz` the position for this point in the path
    * `destSpace` the space for which the target is rendered (useful for non-destiantion based targets, "player" or "world")
    * `bounds` specifies an axis-aligned bounding box (`G3D::AABox`) to specify the bounds for cases where `destSpace="world"` and the target is not destination-based. For more information see the [section below on serializing bounding boxes](##-Bounding-Boxes-(`G3D::AABox`-Serialization)).

#### Target Configuration Example
An example target configuration snippet is provided below:

```
targets = [
    {
        "id": "simple_target",
        "destSpace" : "player",             // This is a player-centered-spherical-space target
        "visualSize" : [0.5, 0.5],          // 0.5m size
        "respawnCount" : 0,                 // Don't respawn
        "speed": [1.0, 3.0],                // 1-3m/s speed
        "eccH" : [5.0, 15.0],               // 5-15° initial spawn location (horizontal)
        "eccV" : [0.0, 5.0],                // 0-5° intitial spawn location (vertical)
    },
    {
        "id": "world-space paramtetric",
        "destSpace" : "world",              // This is a world-space target
        "bounds" : AABox {
                Point3(-8.5, 0.5, -11.5),   // It is important these are specified in "increasing order"
                Point3(-6.5, 1.5, -7.5)     // All x,y,z coordinates must be greater than those above
        },
        "visualSize" : [0.3, 1.0],          // Visual size between 0.3-1m
        "respawnCount" : -1,                // Respawn forever
    },
    {
        "id" : "destination-based",
        "destSpace" : "world",              // Important this is specified here
        "destinations" : {
            {"t": 0.0, "xyz": Vector3(0.00, 0.00, 0.00)},
            {"t": 0.1, "xyz": Vector3(0.00, 1.00, 0.00)},
            ...
            {"t": 10.2, "xyz": Vector3(10.1, 1.01, -100.3)}
        },
    },
    #include("example_target.Any"),         // Example of including an external .Any file
],
```

## Frame Rate Modes
The `frameRate` parameter in any given session config can be used in 3 different modes:
* If the `frameRate` parameter is set to a value >> refresh rate of the display (we suggest `8192fps`), then the program runs in "unlocked" mode wherein as many frames as can be drawn are rendered per displayed frame. This is the common mode of operation in many modern games.
* If the `frameRate` parameter is set close to the refresh rate of the display then the programs runs in "fixed" frame rate mode, wherein the drawn frames are limited to the rate provided
* If `frameRate = 0` then this indicates "default" mode, wherein the default frame rate settings for the window are applied. This should be equivalent to the "unlocked" mode for most systems. This is the default setting if you do not specify a frame rate in the file.

## Target Paths (Using Destinations)
The `destinations` array within the target object overrides much of the default motion behavior in the target motion controls. Once a destinations array (including more than 2 destiantions) is specified all other motion parameters are considered unused. Once a `destinations` array is specified only the following fields from the [target configuration](###-Target-Configuration) apply:

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
