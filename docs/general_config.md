# General Configuration Parameters
FPSci offers a number of different [`.Any` file](./AnyFile.md) configurable parameters that can be set at either the "experiment" or "session" level. This document describes these parameters and gives examples of their usage. Note that any value specified at both the "experiment" and "session" level will use the value specified by the session level.

## Settings Version
| Parameter Name     |Units| Description                                                        |
|--------------------|-----|--------------------------------------------------------------------|
|`settingsVersion`   |`int`|The version of the contents of this file, unless you are aware of changes that need to be made, don't change this number. |
```
"settingsVersion": 1,     // Used for file parsing (do not change unless you are introducing a new any parser)
```

## Scene Control
The `sceneName` parameter allows the user to control the scene in which the application is run. If unspecified, the `sceneName` comes from:

1. An inherited experiment-level `sceneName` parameter
2. The last specified session-level `sceneName` parameter (in time)
3. The `App::m_defaultScene` field within the source (currently set to `"FPSci Simple Hallway"` which is distributed with the repository)

If a scene name is specified at the experiment level it will be applied to all scenes that do not have a `sceneName` specified. If you do not specify a `sceneName` in the experiment config, and do not specify `sceneName` for every session, then session ordering may have an impact on which scene pairs with which session.


| Parameter Name     |Units   | Description                                                            |
|--------------------|--------|------------------------------------------------------------------------|
|`sceneName`         |`String`|The `name` of the (virtual) scene (not necessarily the filename!)       |

```
"sceneName": "eSports Simple Hallway",              // Default scene
```

## Weapon Configuration
* `weapon` provides a configuration for the weapon used in the experiment (for more info see [the weapon config readme](weaponConfigReadme.md))

The `weapon` config should be thought of as an atomic type (just like an `int` or `float`). Even though it is a (more complex) data structure, it does not use the experiment-->session level inheritance appraoch offered elsewhere in the configuration format (i.e. any `weapon` specification should be complete). For this reason we recommend storing weapon configurations in independent `.weapon.Any` files and including them using the `.Any` `#include()` directive.

## Duration Settings
The following settings allow the user to control various timings/durations around the per trial state machine.

| Parameter Name            |Units  | Description                                                        |
|---------------------------|-------|--------------------------------------------------------------------|
|`readyDuration`            |s      |The time before the start of each trial                             |
|`taskDuration`             |s      |The maximum time over which the task can occur                      |
|`feedbackDuration`         |s      |The duration of the feedback window between trials                  |
|`scoreboardDuration`       |s      |The duration of the feedback window between sessions                |
|`scoreboardRequireClick`   |`bool` |Require the user to click to move past the scoreboard (in addition to waiting the `scoreboardDuration`)|

```
"readyDuration": 0.5,         // Time allocated for preparing for trial
"taskDuration": 100000.0,     // Maximum duration allowed for completion of the task
"feedbackDuration": 1.0,      // Time for user feedback between trials
"scoreboardDuration": 5.0,    // Time for user feedback between sessions
"scoreboardRequireClick" : false,      // Don't require a click to move past the scoreboard
```

## Rendering Settings
| Parameter Name            |Units  | Description                                                        |
|---------------------------|-------|--------------------------------------------------------------------|
|`horizontalFieldOfView`    |°      |The (horizontal) field of view for the user's display, to get the vertical FoV multiply this by `1 / your display's aspect ratio` (9/16 for common FHD, or 1920x1080)|
|`frameDelay`               |frames | An (integer) count of frames to delay to control latency           |
|`frameRate`                |fps/Hz | The (target) frame rate of the display (constant for a given session) for more info see the [Frame Rate Modes section](#frame-rate-modes) below.|
|`shader`                   |file    | The (relative) path/filename of an (optional) shader to run (as a `.pix`) |

```
"horizontalFieldOfView":  103.0,            // Field of view (horizontal) for the user in degrees
"frameDelay" : 3,                           // Frame delay (in frames)
"frameRate" : 60,                           // Frame/update rate (in Hz)
"shader": "[your shader].pix",              // Default is "" or no shader
```

## Audio Settings
| Parameter Name        |Units  | Description                                                           |
|-----------------------|-------|-----------------------------------------------------------------------|
|`sceneHitSound`        |file   | The sound to play when the scene (not the target) is hit by a weapon  |
|`sceneHitSoundVol`     |ratio  | The volume of the scene hit sound to play                             |

```
"sceneHitSound": "sound/18382__inferno__hvylas.wav",
"sceneHitSoundVol": 1.0f,
```

## Player Controls
| Parameter Name     |Units | Description                                                                        |
|--------------------|------|------------------------------------------------------------------------------------|
|`moveRate`          |m/s  | The rate of player motion, set this parameter to `0` to disable player motion      |
|`moveScale`         |`Vector2`| A scaler for X/Y player-space motion (set to 0 to lock forward/back, strafe motion)|
|`playerAxisLock`    |`Array<bool>`| Axis aligned motion lock for player                                        |
|`turnScale`         |`Vector2`| A scaler for horizontal/vertical player mouse motion (set to 0 to disable)     |
|`playerHeight`      |m    | The height of the player above the ground when "standing"                          |
|`crouchHeight`      |m    | The height of the player when crouched (set equal to `playerHeight` for no crouch) |
|`jumpVelocity`      |m/s  | The magnitude of the upward impulse introduced by a jump (set to 0 for no jump)    |
|`jumpInterval`      |s    | The minimum time between player jumps in seconds (set to 0 for no limiting)        |
|`jumpTouch`         |`bool` | Whether or not the player needs to be in contact w/ a surface to jump            |
|`playerGravity`     |m/s^2| The graivty vector that impacts the player                                         |
|`disablePlayerMotionBetweenTrials`|`bool`|Don't allow the player to move when not in a trial?                  |
|`resetPlayerPositionBetweenTrials`|`bool`|Respawn the player to their original position between trials?        |

```
"moveRate": 0.0,                            // Player move rate (0 for no motion)
"moveScale" : Vector2(1.0, 1.0),            // Movement scaling
"playerAxisLock": [false, false, false],    // Don't lock player motion in any axis
"turnScale": Vector2(1.0, 1.0),             // Turn rate scaling
"playerHeight":  1.5,                       // Normal player height
"crouchHeight": 0.8,                        // Crouch height
"jumpVelocity": 3.5,                        // Jump velocity
"jumpInterval": 0.5,                        // Minimum jump interval
"jumpTouch": true,                          // Require touch for jump
"playerGravity": Vector3(0.0, -10.0, 0.0),  // Player gravity
"disablePlayerMotionBetweenTrials": false,  // Don't allow the player to move in between trials
"resetPlayerPositionBetweenTrials": false,  // Respawn the player in the starting location between trials
```

## Logging Controls
As part of the general configuration parameters several controls over reporting of data via the output SQL database are provided. These flags and their functions are described below.

| Parameter Name        | Units | Description                                                                      |
|-----------------------|-------|----------------------------------------------------------------------------------|
|`logEnable`            |`bool` | Enables the logger and creation of an output database                            |
|`logTargetTrajectories`|`bool` | Whether or not to log target position to the `Target_Trajectory` table           |
|`logFrameInfo`         |`bool` | Whether or not to log frame info into the `Frame_Info` table                     |
|`logPlayerActions`     |`bool` | Whether or not to log player actions into the `Player_Action` table              |
|`logTrialResponse`     |`bool` | Whether or not to log trial responses into the `Trials` table                    |
|`sessParamsToLog`      |`Array<String>`| A list of additional parameter names (from the config) to log            |

```
"logEnable" : true,
"logTargetTrajectories": true,
"logFrameInfo": true,
"logPlayerActions": true,
"logTrialResponse": true,
"sessParamsToLog" : [],
```

### Logging Session Parameters
The `sessParamsToLog` parameter allows the user to provide an additional list of parameter names to log into the `Sessions` table in the output database. This allows users to control their reporting of conditions on a per-session basis. These logging control can (of course) also be specified at the experiment level. For example, if we had a series of sessions over which the player's `moveRate` or the HUD's `showAmmo` value was changing we could add these to the `sessParamsToLog` array by specifying:
```
"sessParamsToLog" = ["moveRate", "showAmmo"],
```
In the top-level of the experiment config file. This allows the experiment designer to tag their sessions w/ relevant/changing parameters as needed for ease of reference later on from the database output file(s).

## Feedback Questions
In addition to supporting in-app performance-based reporting the application also includes `.Any` configurable prompts that can be configured from the experiment or session level. Currently `MultipleChoice` and (text) `Entry` questions are supported, though more support could be added for other question types.

Questions are configured on a per experiment/session basis using the `questions` array within the general config parameters. Each element of the `questions` array specifies the following:

| Parameter Name        | Units  | Description                                                                      |
|-----------------------|--------|----------------------------------------------------------------------------------|
|`type`                 |`String`| The question type (required), can be `"MultipleChoice"` or (text) `"Entry"`      |
|`prompt`               |`String`| The question prompt (required), a string to present the user with                |
|`title`                |`String`| The title for the feedback prompt                                                |
|`options`              |`Array<String>`| An array of `String` options for `MultipleChoice` questions only           |

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
| Parameter Name        |Units  | Description                                                                        |
|-----------------------|-------|------------------------------------------------------------------------------------|
|`showHUD`              |`bool` | The master control for whether or not HUD elements are displayed (score banner, player health bar, and ammo indicator) |
|`showBanner`           |`bool` | Whether or not the score banner is displayed (currently w/ time remaining, percent of session complete, and score)     |
|`bannerLargeFontSize`  |pt     | The "large" font for the percent complete in the banner
|`bannerSmallFontSize`  |pt     | The "small" font for the time remaining and score
|`hudFont`              |file   | The font to use (as a `.fnt` file) for the HUD (for available fonts check `%g3d%/data10/common/font` or `%g3d%/G3D10/  data-files/font`). We suggest using a fixed width font (such as `console.fnt`) for HUD elements|

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
|`showPlayerHealthBar`          |`bool`         | Whether or not a player health bar is drawn to the HUD                             |
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
|`showAmmo`          |`bool`        | Whether the ammo indicator is drawn in the HUD                                         |
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
|`renderWeaponStatus`   |`bool`| Whether or not the weapon cooldown is rendered                                          |
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

### Static HUD Elements
In addition to the (dynamic) HUD elements listed above, arbitrary lists of static HUD elements can be provided to draw in the UI using the `staticHUDElements` parameter in a general config. The `staticHUDElements` parameter value is an array of elements, each of which specifies the following sub-parameters:

| Parameter Name    | Type      | Description                                                                               |
|-------------------|-----------|-------------------------------------------------------------------------------------------|
|`filename`         |`String`   | A filename to find for the image to draw (`.png` files are suggested)                     |
|`position`         |`Vector2`  | The position to draw the element centered at, as a ratio of screen space (i.e. `Vector2(0.5, 0.5) for an element in the middle of the screen)  |
|`scale`            |`Vector2`  | An additional scale to apply to the drawn element (as a fraction of it's original size)   | 

The `position` parameter specifies the offset to the center of the image with `Vector2(0,0)` indicating the top-left corner and `Vector2(1,1)` indicating the bottom-right corner of the window).

No static HUD elements are drawn by default. An example snippet including 2 (non-existant) HUD elements is provided below for reference:

```
"staticHUDElements" : [
    // Element 1 (centered and scaled)
    {
        "filename": "centerImage.png",              // Use this file to draw an image (should be within data-files directory)
        "position": Vector2(0.5, 0.5),              // Center the image (draw it's center at 1/2 the screen size horizontal/vertical)
        "scale": Vector2(0.25, 0.25)                // Scale the image by 1/4 it's original resolution
    },
    // Element 2 (unscaled)
    {
        "filename" : "hud/unscaled.png",            // Use this filename (can add relative paths to directories that won't be searched implicitly)
        "position": Vector2(0,0)                    // Draw this element at the top-left of the screen
        // No scale specification implies Vector2(1,1) scaling
    }
]
```

## Click to Photon Monitoring
These flags help control the behavior of click-to-photon monitoring in application:

| Parameter Name     |Units                 | Description                                                                        |
|--------------------|----------------------|------------------------------------------------------------------------------------|
|`renderClickPhoton` |`bool`                | Whether or not the click-to-photon indicator box is drawn to the screen            |
|`clickPhotonSide`   |`"right"` or `"left"` | Which side of the display (`left` vs `right`) the click-to-photon indicator box is drawn on |
|`clickPhotonMode`   |`"minimum"` or `"total"`| Which click to photon time is recorded, `minimum` does not include added frame delay, while `total` does |
|`clickPhotonSize`   |`Point2`(ratio)       | The size of the click-to-photon box as a ratio of total screen size               |
|`clickPhotonVertPos`|ratio                 | The vertical position of the click-to-photon output box on the `clickPhotonSide` of the display |
|`clickPhotonColors` |[`Color3`, `Color3`]  | The mouse up/down colors for the click-to-photon indicator box, order is [mouse down color, mouse up color] |

```
"renderClickPhoton": true,              // Draw the click to photon box
"clickPhotonSide": "right",             // Draw the box on the right side (opposite weapon status)
"clickPhotonMode": "total",             // Use the total click-to-photon time
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
|`targetHealthColors`   |[`Color3`, `Color3`]   | The max/min health colors for the target as an array of [`max color`, `min color`], if you do not want the target to change color as its health drops, set these values both to the same color                                              |
|`showReferenceTarget`   |`bool`                | Show a reference target to re-center the view between trials/sessions?             |
|`referenceTargetColor` |`Color3`               | The color of the "reference" targets spawned between trials                        |
|`referenceTargetSize`  |m                      | The size of the "reference" targets spawned between trials                         |


```
"targetHealthColors": [                         // Array of two colors to interpolate between for target health
    Color3(0.0, 1.0, 0.0),
    Color3(1.0, 0.0, 0.0)
],
"showReferenceTarget": true,                    // Show a reference target between trials
"referenceTargetColor": Color3(1.0,1.0,1.0),    // Reference target color (return to "0" view direction)
"referenceTargetSize": 0.01,                    // This is a size in meters
```

### Target Health Bars
| Parameter Name                |Units          | Description                                                                           |
|-------------------------------|---------------|---------------------------------------------------------------------------------------|
|`showTargetHealthBars`         |`bool`         | Whether or not target health bars are drawn for each target                           |
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
|`showFloatingCombatText`           |`bool`         | Whether or not combat text is drawn for hit targets                                   |
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


## Menu Config
These flags control the display of the in-game user menu:

| Parameter Name                    | Type      | Description                                                           |
|-----------------------------------|-----------|-----------------------------------------------------------------------|
|`showMenuLogo`                     |`bool`     |Show a logo at the top of the menu (currently `materials/FPSciLogo.png`) |
|`showExperimentSettings`           |`bool`     |Show the options to select user/session                                |
|`showUserSettings`                 |`bool`     |Show the per-user customization (sensitivity, reticle, etc) options    |
|`allowUserSettingsSave`            |`bool`     |Allow the user to save their settings from the menu                    |
|`allowSensitivityChange`           |`bool`     |Allow the user to change their (cm/360) sensitivity value from the menu|
|`allowTurnScaleChange`             |`bool`     |Allow the user to change their turn scale from the menu                |
|`xTurnScaleAdjustMode`             |`String`   |Mode for adjusting the X turn scale (when allowed), can be `"None"` (i.e. do not allow) or `"Slider"` |
|`yTurnScaleAdjustMode`             |`String`   |Mode for adjusting the Y turn scale (when allowed), can be `"None"` (i.e. do not allow), `"Slider"`, or `"Invert"` (i.e. an "Invert Y" checkbox)|
|`allowReticleChange`               |`bool`     |Allow the user to edit their reticle from the user menu                |
|`allowReticleIdxChange`            |`bool`     |Allow the user to change the "index" of their reticle (i.e. reticle style) |
|`allowReticleSizeChange`           |`bool`     |Allow the user to change the size of their reticle (pre/post shot)     |
|`allowReticleColorChange`          |`bool`     |Allow the user to change the color of their reticle (pre/post shot)    |
|`allowReticleChangeTimeChange`     |`bool`     |Allow the user to change the time it takes to change the color and size of the reticle following a shot |
|`showReticlePreview`               |`bool`     |Show the user a preview of their (pre-shot) reticle (size is not applied) | 
|`showMenuOnStartup`                |`bool`     |Controls whether the user menu is shown at startup (should only be set at the experiment level)|
|`showMenuBetweenSessions`          |`bool`     |Controls whether the user menu is shown between sessions (can be controlled on a per-session basis)|

```
"showMenuLogo": true,                   // Show the logo
"showExperimentSettings" : true,        // Allow user/session seleciton
"showUserSettings": true,               // Show the user settings
"allowUserSettingsSave": true,          // Allow the user to save their settings changes
"allowSensitivityChange": true,         // Allow the user to change the cm/360 sensitivity
"allowTurnScaleChange": true,           // Allow the user to change their turn scale (see below)
"xTurnScaleAdjustMode": "None",         // Don't allow X-turn scale adjustment (use sensitivity)
"yTurnScaleAdjustMode": "Invert",       // Only allow simple "invert" behavior for Y turn scale
"allowReticleChange": false,            // Don't allow the user to change the reticle (ignore below)
"allowReticleIdxChange": true,          // If reticle changes are enabled, allow index (reticle style) changes
"allowReticleSizeChange": true,         // If reticle changes are enabled, allow size changes
"allowReticleColorChange": true,        // If reticle changes are enabled, allow color changes
"allowReticleTimeChange": false,        // Even if reticle change is enabled, don't allow "shrink time" to change
"showReticlePreview": true,             // If reticle changes are enabled show the preview
"showMenuOnStartup" : true,             // Show the user menu when the application starts
"showMenuBetweenSessions": true         // Show the user menu between each session
```

## Logger Config
These flags control whether various information is written to the output database file:

| Parameter Name                    | Type  | Description                                                           |
|-----------------------------------|-------|-----------------------------------------------------------------------|
|`logEnable`                        |`bool` | Enable/disable for all output (SQL) database logging                  |
|`logTargetTrajectories`            |`bool` | Enable/disable for logging target position to database (per frame)    |
|`logFrameInfo`                     |`bool` | Enable/disable for logging frame info to database (per frame)         |
|`logPlayerActions`                 |`bool` | Enable/disable for logging player position, aim , and actions to database (per frame) |
|`logTrialResponse`                 |`bool` | Enable/disable for logging trial responses to database (per trial)    |
|`logUsers`                         |`bool` | Enable/disable for logging users to database (per session)            |

```
"logEnable" = true,
"logTargetTrajectories" = true,
"logFrameInfo" = true,
"logPlayerActions" = true,
"logTrialResponse" = true,
"logUsers" = true,
```

## Command Config
In addition to the programmable behavior above the general config also supports running of arbitrary commands around the FPSci runtime. Note that the "end" commands keep running and there's the potential for orphaned processes if you specify commands that are long running or infinite. The command options include:

| Parameter Name                    | Type              | Description                                                                                  |
|-----------------------------------|-------------------|----------------------------------------------------------------------------------------------|
|`commandsOnSessionStart`           |`Array<String>`    | Command(s) to run at the start of a new session. Command(s) quit on session end              |
|`commandsOnSessionEnd`             |`Array<String>`    | Command(s) to run at the end of a new session. Command(s) not forced to quit                 |
|`commandsOnTrialStart`             |`Array<String>`    | Command(s) to run at the start of a new trial within a session. Command(s) quit on trial end |
|`commandsOnTrialEnd`               |`Array<String>`    | Command(s) to run at the end of a new trial within a session. Command(s) not forced to quit  |

Note that the `Array` of commands provided for each of the parameters above is ordered, but the commands are launched (nearly) simultaneously in a non-blocking manner. This means that run order within a set of commands cannot be strictly guaranteed. If you have serial dependencies within a list of commands consider using a script to sequence them.

For example, the following will cause session start, session end, trial start and trial end strings to be written to a `commandLog.txt` file.

```
commandsOnSessionStart = ( "cmd /c echo Session start>> commandLog.txt", "cmd /c echo Session start second command>> commandLog.txt" );
commandsOnSessionEnd = ( "cmd /c echo Session end>> commandLog.txt", "cmd /c echo Session end second command>> commandLog.txt" );
commandsOnTrialStart = ( "cmd /c echo Trial start>> commandLog.txt" );
commandsOnTrialEnd = ( "cmd /c echo Trial end>> commandLog.txt" );
```

Another common use would be to run a python script/code at the start or end of a session. For example:

```
commandsOnSessionStart = ( "python \"../scripts/event logger/event_logger.py\"" );
commandsOnSessionEnd = ( "python -c \"f = open('texttest.txt', 'w'); f.write('Hello world!'); f.close()\"" );
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