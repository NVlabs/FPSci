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

There are a number of inputs to experiment config. The following is a description of what each one means, and how it is meant to be used.

* `settingsVersion` refers to the version of the contents of this file, unless you are aware of changes that need to be made, don't change this number.
```
"settingsVersion": 1,     // Used for file parsing (do not change unless you are introducing a new any parser)
```

### Global Settings
* `appendingDescription` allows the user to annotate these results with a custom string
* `sceneName` provides the name of the (virtual) scene in which this experiment takes place

```
"appendingDescription": "your description here",    // Description of this file (default = "default")
"sceneName": "eSports Simple Hallway",              // Default scene
```

### Duration Settings
* `feedbackDuration` describes the duration of the feedback window between experiments
* `readyDuration` sets the time before the start of each trial
* `taskDuration` sets the maximum time over which the task can occur

```
"feedbackDuration": 1.0,    // Time allocated for providing user feedback
"readyDuration": 0.5,       // Time allocated for preparing for trial
"taskDuration": 100000.0,   // Maximum duration allowed for completion of the task
```

### Camera/View Settings
* `horizontalFieldOfView` sets the (horizontal) field of view for the user's display (in degrees), to get the vertical FoV multiply this by `1 / your display's aspect ratio` (9/16 for common FHD, or 1920x1080)
* `moveRate` sets the rate of player motion in m/s, set this parameter to `0` to disable player motion
* `playerHeight` sets the height of the player above the ground when "standing"
* `crouchHeight` set the height of the player when crouched (set equal to `playerHeight` for no crouch)
* `jumpVelocity` sets the magnitude of the upward impulse introduced by a jump (set to 0 for no jump)
* `jumpInterval` sets the minimum time between player jumps in seconds (set to 0 for no limiting)
* `jumpTouch` sets whether or not the player needs to be in contact w/ a surface to jump
* `playerGravity` set the graivty vector that impacts the player
* `shader` provides the (relative) path of an (optional) shader to run

```
"horizontalFieldOfView":  103.0,            // Field of view (horizontal) for the user in degrees
"moveRate": 0.0,                            // Player move rate (0 for no motion)
"jumpVelocity": 40.0,                       // Jump velocity
"jumpInterval": 0.5,                        // Minimum jump interval
"jumpTouch": true,                          // Require touch for jump
"playerHeight":  1.5,                       // Normal player height
"crouchHeight": 0.8,                        // Crouch height
"playerGravity": Vector3(0.0, -5.0, 0.0),   // Player gravity
"shader": "[your shader].pix",              // Default is "" or no shader
```

### HUD settings
* `showHUD` is the master control for whether or not HUD elements are displayed (score banner, player health bar, and ammo indicator)
* `showBanner` controls whether or not the score banner is displayed (currently w/ time remaining, percent of session complete, and score)
* `bannerLargeFontSize` sets the "large" font for the percent complete in the banner
* `bannerSmallFontSize` sets the "small" font for the time remaining and score
* `hudFont` sets the font to use (as a `.fnt` file) for the HUD (for available fonts check `%g3d%/data10/common/font` or `%g3d%/G3D10/data-files/font`). We suggest using a fixed width font (such as `console.fnt`) for HUD elements

```
"showHUD":  true,               // Show the player HUD (banner, ammo, health bar)
"showBanner":  true,            // Control the banner at the top of the screen (shows time, score, and session % complete)
"bannerLargeFontSize": 30.0,    // Large font size to use in the banner (% complete)
"bannerSmallFontSize": 14.0,    // Small font size to use in the banner (time remaining and score)
"hudFont": "console.fnt",       // Font to use for the HUD (fixed with highly suggested!)
```

#### Player Health Bar
* `showPlayerHealthBar` determines whether or not a player health bar is drawn to the HUD
* `playerHealthBarSize` sets the size of the player health bar (in pixels)
* `playerHealthBarPosition` sets the position of the player health bar (in pixels from the top right of the screen)
* `playerHealthBarBorderSize` sets the width of the player health bar border
* `playerHealthBarBorderColor` sets the `Color4` of the player health bar border
* `playerHealthBarColors` sets the max/min health colors for the player health bar as an array of [`max color`, `min color`] as `Color4`. If you are using low alpha values with this field, make sure you consider the alpha value for `playerHealthBarBorderColor` as well.

```
"showPlayerHealthBar":  true,                               // Show the player health bar (default is false)      
"playerHealthBarSize": Point2(200.0, 20.0),                 // Size of the health bar       
"playerHealthBarPosition": Point2(74.0, 74.0),              // Position of the bar      
"playerHealthBarBorderSize": Point2(2.0, 2.0),              // Size of the bar border/background
"playerHealthBarBorderColor": Color4(0.0,0.0,0.0,1.0),      // Background color w/ alpha
"playerHealthBarColors": [                                  // Transition player health bar from green --> red
    Color4(0.0, 1.0, 0.0, 1.0),
    Color4(1.0, 0.0, 0.0, 1.0),
],
```

#### Ammo Indicator
* `showAmmo` controls whether the ammo indicator is drawn in the HUD
* `ammoPostion` controls the position of the ammo indicator as an offset from the bottom right of the display
* `ammoSize` sets the font size for the ammo indicator
* `ammoColor` sets the (foreground) color for the ammo indicator as a `Color4`. If applying low alpha values here, consider also applying these to `ammoOutlineColor` to create a true transparency to the text
* `ammoOutlineColor` sets the outline color for the ammo indicator as a `Color4`

```
"showAmmo":  true,                                  // Show the ammo indicator (default is false)
"ammoPosition": Point2(74.0, 74.0),                 // Position the ammo indicator (from the bottom right)
"ammoSize": 24.0,                                   // Use 24pt font for the ammo indicator
"ammoColor": Color4(1.0,1.0,1.0,1.0),               // Set the ammo indicator to white
"ammoOutlineColor": Color4(0.0,0.0,0.0,1.0),        // Set the outline/background color for the ammo indicator
```

#### Weapon Cooldown
* `renderWeaponStatus` controls whether or not the weapon cooldown is rendered on the left side of the screen
* `cooldownMode` controls the type of display used for weapon cooldown ("ring" or "box")
* `weaponStatusSide` controls which side of the display the weapon status is drawn on (can be `right` or `left`), this only applies in "box" mode
* `cooldownInnerRadius` controls the inner radius of the cooldown ring in "ring" mode (in pixels)
* `cooldownThickness` controls the thickness of the cooldown ring in "ring" mode (in pixels)
* `cooldownSubdivisions` allows the user to set the number of subdivisions/faces in the draw ring
* `cooldownColor` sets the (active) `Color4` of the ring segments, by default they are not drawn when inactive

```
"renderWeaponStatus": true,                 // Show the cooldown indicator
"cooldownMode": "ring",                     // Use a ring indicator (other option is "box")
"weaponStatusSide: "left",                  // Place the weapon status on the left
"cooldownInnerRadius": 40.0,                // 40 pixel ring radius
"cooldownThickness": 10.0,                  // 10 pixel ring thickness
"cooldownSubdivisions": 64,                 // 64 subdivisions (good enough to look like a circle)
"cooldownColor": Color4(1.0,1.0,1.0,0.75),  // White w/ 75% alpha
```

#### Click to Photon Monitoring
* `renderClickPhoton` controls whether or not the click-to-photon indicator box is drawn to the screen
* `clickPhotonSide` controls which side of the display (`left` vs `right`) the click-to-photon indicator box is drawn on
* `clickPhotonVertPos` controls the vertical position of the click-to-photon output box on the `clickPhotonSide` of the display
* `clickPhotonSize` controls the size of the click-to-photon box as a ratio of total screen size
* `clickPhotonColors` provides the mouse up/down colors for the click-to-photon indicator box as an array of `Color3`. The order is [mouse down color, mouse up color]

```
"renderClickPhoton": true,              // Draw the click to photon box
"clickPhotonSide": "right",             // Draw the box on the right side (opposite weapon status)
"clickPhotonVertPos": 0.5,              // Draw the click to photon box in the middle of the display
"clickPhotonSize" : Point2(0.05,0.035), // Size of the box (as a ratio of screen space)
"clickPhotonColors": [                  // Array of mouse up/down colors
    Color3(0.2,0.2,0.2),                // Avoid using black-->white for better gray-to-gray timing
    Color3(0.8,0.8,0.8)
],
```

### Target Rendering
* `targetHealthColors` sets the max/min health colors for the target as an array of [`max color`, `min color`] as `Color3`s, if you do not want the target to change color as its health drops, set these values both to the same color
* `dummyTargetColor` sets the color of the "dummy" targets spawned between trials
* `dummyTargetSize` set the size of the "dummy" targets spawned between trials
* `explosionSound` sets the sound to play when a target is destroyed
* `explosionSoundVol` sets the volume of the explosion sound to play

```
"targetHealthColors": [                     // Array of two colors to interpolate between for health
    Color3(0.0, 1.0, 0.0),
    Color3(1.0, 0.0, 0.0)
],
"dummyTargetColor": Color3(1.0,1.0,1.0),    // Draw the dummy targets (return to "0" view direction)
"dummyTargetSize": 0.01,                    // This is a scale factor for the model
"explosionSound": "sound/32882__Alcove_Audio__BobKessler_Metal_Bangs-1.wav",
"explosionSoundVol": 10.0f,

```

#### Health Bars
* `showTargetHealthBars` determines whether or not target health bars are drawn for each target
* `targetHealthBarSize` sets the size of the health bar (in pixels)
* `targetHealthBarOffset` sets the offset of the health bar relative to the target (in pixels)
* `targetHealthBarBorderSize` sets the size of the border around the target (see `targetHealthBarBorderColor` to set the color as well)
* `targetHealthBarBorderColor` sets the `Color4` of the target health bar border (can set alpha = 0 for no border)
* `targetHealthBarColors` sets the max/min health colors for the target health bar as an array of [`max color`, `min color`] as `Color4`. If you are using low alpha values with this field, make sure you also set a low alpha for `targetHealthBarBorderColor` as otherwise this will show through

```
"showTargetHealthBars": true,                               // Turn on target health bars (default is false)
"targetHealthBarSize": Point2(100.0, 10.0),                 // Target health bar (x,y) size
"targetHealthBarOffset": Point3(0.0, -50.0, 0.0),           // Position the health bar 50px above the target
"targetHealthBarBorderSize": Point2(2.0, 2.0),              // Target health bar border/background size
"targetHealthBarBorderColor": Color4(0.0,0.0,0.0,1.0)       // Black background by default
"targetHealthBarColors": [                                  // Use a green --> red transition
    Color4(0.0, 1.0, 0.0, 1.0),
    Color4(1.0, 0.0, 0.0, 1.0)
],
```

#### Floating Combat Text
* `showFloatingCombatText` determines whether or not combat text is drawn for hit targets
* `floatingCombatTextSize` controls the size of the font in point
* `floatingCombatTextFont` controls the font used for the floating combat text (as a `Color4`)
* `floatingCombatTextColor` controls the (foreground) combat text color (as a `Color4`)
* `floatingCombatTextOutlineColor` controls the color of the combat text outline
* `floatingCombatTextOffset` controls the initial offset of the text from the center of the target as a `Point3`
* `floatingCombatTextVelocity` controls the velocity vector for combat text (once it is spawned) as a `Point3`
* `floatingCombatTextFade` provides a (compounded) alpha fade for the text and it's outline
* `floatingCombatTextTimeout` controls the timeout for the combat text in seconds

```
"showFloatingCombatText": true,                             // Show floating combat text (default is false)
"floatingCombatTextSize": 16.0,                             // Use 16pt font for the floating text
"floatingCombatTextFont": "dominant.fnt",                   // Use this font for combat text (see %g3d%/data10/common/font for more options)    
"floatingCombatTextColor": Color4(1.0,0.0,0.0,1.0),         // Red combat text
"floatingCombatTextOutlineColor": Color4(0.0,0.0,0.0,1.0),  // Black background/outline for combat text
"floatingCombatTextOffset": Point3(0.0,-10.0,0.0),          // Start the combat text 10 pixels above the hit point
"floatingCombatTextVelocity": Point3(0.0,-100.0,0.0),       // Move the combat text up at 100px/s                    
"floatingCombatTextFade": 0.98,                             // Fade rate for the combat text
"floatingCombatTextTimeout": 0.5,                           // Fade out the combat text in 0.5s
```

### Weapon Configuration
* `weapon` provides a configuration for the weapon used in the experiment (for more info see [the weapon config readme](./weapon/weaponConfigReadme.md))

### Session Configuration
* `sessions` is a list of all sessions and their affiliated information:
    * `session id` is a short name for the session
    * `frameDelay` is an (integer) count of frames to delay to control latency
    * `frameRate` is the frame rate of the display (constant for a given session) for more info see the [Frame Rate Modes section](#Frame-Rate-Modes) below.
    * `expMode` is used to indicate an additional mode for affiliated sessions (such as `real` vs `training`)
    * `trials` is a list of trials referencing the `trials` table above:
        * `ids` is a list of short names for the trial(s) to affiliate with the `targets` or `reactions` table below, if multiple ids are provided multiple target are spawned simultaneously in each trial
        * `count` provides the number of trials in this session

#### Session Configuration Example
An example session configuration snippet is included below:

```
"sessions" : [
    {
        "id" : "test-session",
        "frameDelay" : 3,           // Frame delay (in frames)
        "frameRate" : 60,           // Frame/update rate (in Hz)
        "expMode" : "test",         // This is an arbitrary string tag (for now)
        "trials" : [
            {
                "ids" : ["simple_target"],                                  // Single target example
                "count": 20,
            },
            {
                "ids" : ["simple_target", "world-space paramtetric", "example_target", "example_target"],
                "count" : 5
            }
        ]
    },
    {
        "id" : "minimal-session",       // This session uses default 0 frameDelay and unlocked frame rate
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
    * `destSpace` the space for which the target is rendered (useful for non-destiantion based targets)
    * `bounds` specifies an axis-aligned bounding box (`G3D::AABox`) to specify the bounds for cases where `destSpace="world"` and the target is not destination-based. For more information see the [section below on serializing bounding boxes](##-Bounding-Boxes-(`G3D::AABox`-Serialization)).

#### Target Configuration Example
An example target configuration snippet is provided below:

```
targets = [
    {
        "id": "simple_target",
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