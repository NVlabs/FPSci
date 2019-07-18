# Introduction
The experiment config is by far the most elaborate of the configuration files. It sets a number of "world" parameters that will be constant across any given experiment. Many high-level parameters are implemented here, though most are optional.

Broad areas of control included in this config file include:
* Scene, field of view, and rendering parameters
* Timing for experimental states
* Universal player parameters like the move rate
* Information on the weapon to use
* Session setup and target movement

For a full description of fields see the descriptions below.

# Experiment Config Field Descriptions

There are a number of inputs to experiment config. The following is a description of what each one means, and how it is meant to be used.

* `settingsVersion` refers to the version of the contents of this file, unless you are aware of changes that need to be made, don't change this number.

### Global Settings
* `appendingDescription` allows the user to annotate these results with a custom string
* `sceneName` provides the name of the (virtual) scene in which this experiment takes place

### Duration Settings
* `feedbackDuration` describes the duration of the feedback window between experiments
* `readyDuration` sets the time before the start of each trial
* `taskDuration` sets the maximum time over which the task can occur

### Camera/View Settings
* `fieldOfView` sets the (horizontal) field of view for the user's display
* `moveRate` sets the rate of player motion in m/s, set this parameter to `0` to display player motion
* `walkMode` sets whether or not we are in "walking" mode that includes graivty and jump vs flying camera mode
* `playerHeight` sets the height of the player above the ground (for `walkMode=True` only for now)
* `jumpVelocity` sets the magnitude of the upward impulse introduced by a jump when `walkMode=True`
* `playerGravity` set the graivty vector that impacts the player when `walkMode=True`
* `renderWeaponStatus` controls whether or not the weapon cooldown is rendered on the left side of the screen
* `weaponStatusSide` controls which side of the display the weapon status is drawn on (can be `right` or `left`)
* `clickPhotonSide` controls which side of the display the click-to-photon region is drawn on (can be `right` or `left`)
* `shader` provides the (relative) path of an (optional) shader to run

#### HUD settings
* `showHUD` is the master control for whether or not HUD elements are displayed (score banner, player health bar, and ammo indicator)
* `showBanner` controls whether or not the score banner is displayed (currently w/ time remaining, percent of session complete, and score)
* `hudFont` sets the font to use (as a `.fnt` file) for the HUD (for available fonts check `%g3d%/data10/common/font` or `%g3d%/G3D10/data-files/font`)
* `showPlayerHealthBar` determines whether or not a player health bar is drawn to the HUD
* `playerHealthBarSize` sets the size of the player health bar (in pixels)
* `playerHealthBarPosition` sets the position of the player health bar (in pixels from the top right of the screen)
* `playerHealthBarBorderSize` sets the width of the player health bar border
* `playerHealthBarBorderColor` sets the `Color4` of the player health bar border
* `playerHealthBarColors` sets the max/min health colors for the player health bar as an array of [`max color`, `min color`] as `Color4`. If you are using low alpha values with this field, make sure you consider the alpha value for `playerHealthBarBorderColor` as well.

### Target Rendering
* `targetHealthColors` sets the max/min health colors for the target as an array of [`max color`, `min color`] as `Color3`s, if you do not want the target to change color as its health drops, set these values both to the same color
* `dummyTargetColor` sets the color of the "dummy" targets spawned between trial

#### Health Bars
* `showTargetHealthBars` determines whether or not target health bars are drawn for each target
* `targetHealthBarSize` sets the size of the health bar (in pixels)
* `targetHealthBarOffset` sets the (world-space) offset of the health bar relative to the target
* `targetHealthBarBorderSize` sets the size of the border around the target (see `targetHealthBarBorderColor` to set the color as well)
* `targetHealthBarBorderColor` sets the `Color4` of the target health bar border (can set alpha = 0 for no border)
* `targetHealthBarColors` sets the max/min health colors for the target health bar as an array of [`max color`, `min color`] as `Color4`. If you are using low alpha values with this field, make sure you also set a low alpha for `targetHealthBarBorderColor` as otherwise this will show through

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

### Target Configuration
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
    * `modelSpec` is an `Any` that constructs an `ArticulatedModel` similar to that used in the [the weapon config readme](./weapon/weaponConfigReadme.md). For now this spec needs to point to an `obj` file with a model named `core/icosahedron_default`.
## Frame Rate Modes
The `frameRate` parameter in any given session config can be used in 3 different modes:
* If the `frameRate` parameter is set to a value >> refresh rate of the display (we suggest `8192fps`), then the program runs in "unlocked" mode wherein as many frames as can be drawn are rendered per displayed frame. This is the common mode of operation in many modern games.
* If the `frameRate` parameter is set close to the refresh rate of the display then the programs runs in "fixed" frame rate mode, wherein the drawn frames are limited to the rate provided
* If `frameRate = 0` then this indicates "default" mode, wherein the default frame rate settings for the window are applied. This should be equivalent to the "unlocked" mode for most systems. This is the default setting if you do not specify a frame rate in the file.