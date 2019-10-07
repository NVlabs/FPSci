# General Configuration Parameters
FPSci offers a number of different [`.Any` file](./AnyFile.md) configurable parameters that can be set at either the "experiment" or "session" level. This document describes these parameters and gives examples of their usage.

## Settings Version
| Parameter Name     |Units| Description                                                        |
|--------------------|-----|--------------------------------------------------------------------|
|`settingsVersion`   |N/A  |The version of the contents of this file, unless you are aware of changes that need to be made, don't change this number. |
```
"settingsVersion": 1,     // Used for file parsing (do not change unless you are introducing a new any parser)
```

## Scene Control
The `sceneName` parameter allows the user to control the scene in which the application is run. If unspecified, the `sceneName` comes from:

1. An inherited experiment-level `sceneName` parameter
2. The last specified session-level `sceneName` parameter (in time)
3. The `App::m_defaultScene` field within the source (currently set to `"FPSci Simple Hallway"` which is distributed with the repository)

If a scene name is specified at the experiment level it will be applied to all scenes that do not have a `sceneName` specified. If you do not specify a `sceneName` in the experiment config, and do not specify `sceneName` for every session, then session ordering may have an impact on which scene pairs with which session.


| Parameter Name     |Units| Description                                                            |
|--------------------|-----|------------------------------------------------------------------------|
|`sceneName`         |name |The `name` of the (virtual) scene (not necessarily the filename!)       |

```
"sceneName": "eSports Simple Hallway",              // Default scene
```

## Weapon Configuration
* `weapon` provides a configuration for the weapon used in the experiment (for more info see [the weapon config readme](../data-files/weapon/weaponConfigReadme.md))

The `weapon` config should be thought of as an atomic type (just like an `int` or `float`). Even though it is a (more complex) data structure, it does not use the experiment-->session level inheritance appraoch offered elsewhere in the configuration format (i.e. any `weapon` specification should be complete). For this reason we recommend storing weapon configurations in independent `.weapon.Any` files and including them using the `.Any` `#include()` directive.

## Duration Settings
The following settings allow the user to control various timings/durations around the per trial state machine.

| Parameter Name     |Units| Description                                                        |
|--------------------|-----|--------------------------------------------------------------------|
|`feedbackDuration`  |s    |The duration of the feedback window between experiments             |
|`readyDuration`     |s    |The time before the start of each trial                             |
|`taskDuration`      |s    |The maximum time over which the task can occur                      |

```
"feedbackDuration": 1.0,    // Time allocated for providing user feedback
"readyDuration": 0.5,       // Time allocated for preparing for trial
"taskDuration": 100000.0,   // Maximum duration allowed for completion of the task
```

## Rendering Settings
| Parameter Name            |Units  | Description                                                        |
|---------------------------|-------|--------------------------------------------------------------------|
|`horizontalFieldOfView`    |°      |The (horizontal) field of view for the user's display, to get the vertical FoV multiply this by `1 / your display's aspect ratio` (9/16 for common FHD, or 1920x1080)|
|`frameDelay`               |frames | An (integer) count of frames to delay to control latency           |
|`frameRate`                |fps/Hz | The (target) frame rate of the display (constant for a given session) for more info see the [Frame Rate Modes section](#Frame-Rate-Modes) below.|
|`shader`                   |file    | The (relative) path/filename of an (optional) shader to run (as a `.pix`) |

```
"horizontalFieldOfView":  103.0,            // Field of view (horizontal) for the user in degrees
"frameDelay" : 3,                           // Frame delay (in frames)
"frameRate" : 60,                           // Frame/update rate (in Hz)
"shader": "[your shader].pix",              // Default is "" or no shader
```

## Player Controls
| Parameter Name     |Units| Description                                                                        |
|--------------------|-----|------------------------------------------------------------------------------------|
|`moveRate`          |m/s  | The rate of player motion, set this parameter to `0` to disable player motion      |
|`playerHeight`      |m    | The height of the player above the ground when "standing"                          |
|`crouchHeight`      |m    | The height of the player when crouched (set equal to `playerHeight` for no crouch) |
|`jumpVelocity`      |m/s  | The magnitude of the upward impulse introduced by a jump (set to 0 for no jump)    |
|`jumpInterval`      |s    | The minimum time between player jumps in seconds (set to 0 for no limiting)        |
|`jumpTouch`         |bool | Whether or not the player needs to be in contact w/ a surface to jump              |
|`playerGravity`     |m/s^2| The graivty vector that impacts the player

```
"moveRate": 0.0,                            // Player move rate (0 for no motion)
"jumpVelocity": 40.0,                       // Jump velocity
"jumpInterval": 0.5,                        // Minimum jump interval
"jumpTouch": true,                          // Require touch for jump
"playerHeight":  1.5,                       // Normal player height
"crouchHeight": 0.8,                        // Crouch height
"playerGravity": Vector3(0.0, -5.0, 0.0),   // Player gravity
```

## Feedback Questions
In addition to supporting in-app performance-based reporting the application also includes `.Any` configurable prompts that can be configured from the experiment or session level. Currently `MultipleChoice` and (text) `Entry` questions are supported, though more support could be added for other question types.

Questions are configured on a per experiment/session basis using the `questions` array within the general config parameters. Each element of the `questions` array specifies the following:

| Parameter Name        | Units | Description                                                                      |
|-----------------------|-------|----------------------------------------------------------------------------------|
|`type`                 |N/A    | The question type (required), can be `"MultipleChoice"` or (text) `"Entry"`      |
|`prompt`               |N/A    | The question prompt (required), a string to present the user with                |
|`title`                |N/A    | The title for the feedback prompt                                                |
|`options`              |Array<String>| An array of `String` options for `MultipleChoice` questions only           |

The user can specify one or more questions using the `questions` array, as demonstrated below.

```
"questions" : [
    {
        "type": "Entry",
        "prompt": "Write some text!",
        "title": "Example text entry"
    },
    {
        "type": "MultipleChoice",
        "prompt": "Choose an option!",
        "options": ["A", "B", "C"]
    }
]
```

Each question in the array is then asked of the user (via an independent time-sequenced dialog box) before being recorded to the output log.

## HUD settings
| Parameter Name        |Units| Description                                                                        |
|-----------------------|-----|------------------------------------------------------------------------------------|
|`showHUD`              |bool | The master control for whether or not HUD elements are displayed (score banner, player health bar, and ammo indicator) |
|`showBanner`           |bool | Whether or not the score banner is displayed (currently w/ time remaining, percent of session complete, and score)     |
|`bannerLargeFontSize`  |pt   | The "large" font for the percent complete in the banner
|`bannerSmallFontSize`  |pt   | The "small" font for the time remaining and score
|`hudFont`              |file | The font to use (as a `.fnt` file) for the HUD (for available fonts check `%g3d%/data10/common/font` or `%g3d%/G3D10/  data-files/font`). We suggest using a fixed width font (such as `console.fnt`) for HUD elements|

```
"showHUD":  true,               // Show the player HUD (banner, ammo, health bar)
"showBanner":  true,            // Control the banner at the top of the screen (shows time, score, and session % complete)
"bannerLargeFontSize": 30.0,    // Large font size to use in the banner (% complete)
"bannerSmallFontSize": 14.0,    // Small font size to use in the banner (time remaining and score)
"hudFont": "console.fnt",       // Font to use for the HUD (fixed with highly suggested!)
```

### Player Health Bar
| Parameter Name                |Units          | Description                                                                        |
|-------------------------------|---------------|------------------------------------------------------------------------------------|
|`showPlayerHealthBar`          |bool           | Whether or not a player health bar is drawn to the HUD                             |
|`playerHealthBarSize`          |`Point2`(px)   | The size of the player health bar                                                  |
|`playerHealthBarPosition`      |`Point2`(px)   | The position of the player health bar (from the top right of the screen)           |
|`playerHealthBarBorderSize`    |`Point2`(px)   | The width of the player health bar border                                          |
|`playerHealthBarBorderColor`   |`Color4`       | The color of the player health bar border                                          |
|`playerHealthBarColors`        |[`Color4`, `Color4`] | The max/min health colors for the player health bar as an array of [`max color`, `min color`]. If you are using low alpha values with this field, make sure you consider the alpha value for `playerHealthBarBorderColor` as well.|

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

### Ammo Indicator
| Parameter Name     |Units         | Description                                                                            |
|--------------------|--------------|----------------------------------------------------------------------------------------|
|`showAmmo`          |bool          | Whether the ammo indicator is drawn in the HUD                                         |
|`ammoPostion`       |`Point2`(px)  | The position of the ammo indicator (as an offset from the bottom right of the display) |
|`ammoSize`          |pt            | The font size for the ammo indicator                                                   |
|`ammoColor`         |`Color4`      | The (foreground) color for the ammo indicator. If applying low alpha values here, consider also applying these to `ammoOutlineColor` to create a true transparency to the text. |
|`ammoOutlineColor`  |`Color4`      | The outline color for the ammo indicator.                                              |

```
"showAmmo":  true,                                  // Show the ammo indicator (default is false)
"ammoPosition": Point2(74.0, 74.0),                 // Position the ammo indicator (from the bottom right)
"ammoSize": 24.0,                                   // Use 24pt font for the ammo indicator
"ammoColor": Color4(1.0,1.0,1.0,1.0),               // Set the ammo indicator to white
"ammoOutlineColor": Color4(0.0,0.0,0.0,1.0),        // Set the outline/background color for the ammo indicator
```

### Weapon Cooldown
| Parameter Name        |Units| Description                                                                             |
|-----------------------|-----|-----------------------------------------------------------------------------------------|
|`renderWeaponStatus`   |bool | Whether or not the weapon cooldown is rendered                                          |
|`cooldownMode`         |`"ring"` or `"box"` | The type of display used for weapon cooldown                             |
|`weaponStatusSide`     |`"right"` or `"left"`| Which side of the display the weapon status is drawn on in "box" mode   |
|`cooldownInnerRadius`  |px   | The inner radius of the cooldown ring in "ring" mode                                    |
|`cooldownThickness`    |px   | The thickness of the cooldown ring in "ring" mode                                       |
|`cooldownSubdivisions` |int  | The number of subdivisions/faces in the draw ring in "ring" mode                        |
|`cooldownColor`        |`Color4`| The (active) `Color4` of the ring segments, by default they are not drawn when inactive|

```
"renderWeaponStatus": true,                 // Show the cooldown indicator
"cooldownMode": "ring",                     // Use a ring indicator (other option is "box")
"weaponStatusSide: "left",                  // Place the weapon status on the left
"cooldownInnerRadius": 40.0,                // 40 pixel ring radius
"cooldownThickness": 10.0,                  // 10 pixel ring thickness
"cooldownSubdivisions": 64,                 // 64 subdivisions (good enough to look like a circle)
"cooldownColor": Color4(1.0,1.0,1.0,0.75),  // White w/ 75% alpha
```

## Click to Photon Monitoring
| Parameter Name     |Units                 | Description                                                                        |
|--------------------|----------------------|------------------------------------------------------------------------------------|
|`renderClickPhoton` |bool                  | Whether or not the click-to-photon indicator box is drawn to the screen            |
|`clickPhotonSide`   |`"right"` or `"left"` | Which side of the display (`left` vs `right`) the click-to-photon indicator box is drawn on |
|`clickPhotonSize`   |`Point2`(ratio)       | The size of the click-to-photon box as a ratio of total screen size               |
|`clickPhotonVertPos`|ratio                 | The vertical position of the click-to-photon output box on the `clickPhotonSide` of the display |
|`clickPhotonColors` |[`Color3`, `Color3`]  | The mouse up/down colors for the click-to-photon indicator box, order is [mouse down color, mouse up color] |

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

## Target Rendering
| Parameter Name        |Units                  | Description                                                                        |
|-----------------------|-----------------------|------------------------------------------------------------------------------------|
|`targetHealthColors`   |[`Color3`, `Color3`]   | The max/min health colors for the target as an array of [`max color`, `min color`], if you do not want the target to change color as its health drops, set these values both to the same color                                                  |
|`referenceTargetColor` |`Color3`               | The color of the "reference" targets spawned between trials                        |
|`referenceTargetSize`  |m                      | The size of the "reference" targets spawned between trials                         |
|`explosionSound`       |file                   | The sound to play when a target is destroyed                                       |
|`explosionSoundVol`    |ratio                  | The volume of the explosion sound to play                                          |

```
"targetHealthColors": [                         // Array of two colors to interpolate between for target health
    Color3(0.0, 1.0, 0.0),
    Color3(1.0, 0.0, 0.0)
],
"referenceTargetColor": Color3(1.0,1.0,1.0),    // Reference target color (return to "0" view direction)
"referenceTargetSize": 0.01,                    // This is a size in meters
"explosionSound": "sound/32882__Alcove_Audio__BobKessler_Metal_Bangs-1.wav",
"explosionSoundVol": 10.0f,

```

### Target Health Bars
| Parameter Name                |Units          | Description                                                                           |
|-------------------------------|---------------|---------------------------------------------------------------------------------------|
|`showTargetHealthBars`         |bool           | Whether or not target health bars are drawn for each target                           |
|`targetHealthBarSize`          |`Point2`(px)   | The size of the health bar                                                            |
|`targetHealthBarOffset`        |`Point3`(px)   | The offset of the health bar relative to the target                                   |
|`targetHealthBarBorderSize`    |`Point2`(px)   | The size of the border around the target (see `targetHealthBarBorderColor` to set the color as well) |
|`targetHealthBarBorderColor`   |`Color4`       | The target health bar border (can set alpha = 0 for no border)                        |
|`targetHealthBarColors`        |[`Color4`, `Color4`] | The max/min health colors for the target health bar as an array of [`max color`, `min color`] . If you are using low alpha values with this field, make sure you also set a low alpha for `targetHealthBarBorderColor` as otherwise this will show through |

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

### Floating Combat Text
| Parameter Name                    |Units          | Description                                                                           |
|-----------------------------------|---------------|---------------------------------------------------------------------------------------|
|`showFloatingCombatText`           |bool           | Whether or not combat text is drawn for hit targets                                   |
|`floatingCombatTextSize`           |pt             | The size of the combat text font                                                      |
|`floatingCombatTextFont`           |font           | The font used for the floating combat text (as an `.fnt` file)                        |   
|`floatingCombatTextColor`          |`Color4`       | The (foreground) combat text color                                                    |
|`floatingCombatTextOutlineColor`   |`Color4`       | The color of the combat text outline                                                  |
|`floatingCombatTextOffset`         |`Point3`(px)   | The initial offset of the text from the center of the target                          |
|`floatingCombatTextVelocity`       |`Point3`(px/s) | The velocity vector for combat text (once it is spawned)                              |
|`floatingCombatTextFade`           |ratio          | A (compounded) alpha fade for the text and it's outline (1 will stay disable)         |
|`floatingCombatTextTimeout`        |s              | The timeout for the combat text in seconds                                            |

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

# Frame Rate Modes
The `frameRate` parameter in any given session config can be used in 3 different modes:

|Set frame rate         |Resulting mode |
|-----------------------|---------------|
|>> Refresh rate        |Unlocked mode  |
|Close to refresh rate  |Fixed mode     |
|0                      |Default mode   |

* If the `frameRate` parameter is set to a value >> refresh rate of the display (we suggest `8192fps`), then the program runs in "unlocked" mode wherein as many frames as can be drawn are rendered per displayed frame. This is the common mode of operation in many modern games.
* If the `frameRate` parameter is set close to the refresh rate of the display then the programs runs in "fixed" frame rate mode, wherein the drawn frames are limited to the rate provided
* If `frameRate = 0` then this indicates "default" mode, wherein the default frame rate settings for the window are applied. This should be equivalent to the "unlocked" mode for most systems. This is the default setting if you do not specify a frame rate in the file.