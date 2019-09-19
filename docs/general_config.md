# General Configuration Parameters
FPSci offers a number of different [`.Any` file](./AnyFile.md) configurable parameters that can be set at either the "experiment" or "session" level. This document describes these parameters and gives examples of their usage.

## Setting Version
* `settingsVersion` refers to the version of the contents of this file, unless you are aware of changes that need to be made, don't change this number.
```
"settingsVersion": 1,     // Used for file parsing (do not change unless you are introducing a new any parser)
```

## Scene Control
* `sceneName` provides the name of the (virtual) scene in which this experiment takes place
```
"sceneName": "eSports Simple Hallway",              // Default scene
```

## Weapon Configuration
* `weapon` provides a configuration for the weapon used in the experiment (for more info see [the weapon config readme](./weapon/weaponConfigReadme.md))

## Duration Settings
* `feedbackDuration` describes the duration of the feedback window between experiments
* `readyDuration` sets the time before the start of each trial
* `taskDuration` sets the maximum time over which the task can occur

```
"feedbackDuration": 1.0,    // Time allocated for providing user feedback
"readyDuration": 0.5,       // Time allocated for preparing for trial
"taskDuration": 100000.0,   // Maximum duration allowed for completion of the task
```
## Rendering Settings
* `horizontalFieldOfView` sets the (horizontal) field of view for the user's display (in degrees), to get the vertical FoV multiply this by `1 / your display's aspect ratio` (9/16 for common FHD, or 1920x1080)
* `frameDelay` is an (integer) count of frames to delay to control latency
* `frameRate` is the frame rate of the display (constant for a given session) for more info see the [Frame Rate Modes section](#Frame-Rate-Modes) below.
* `shader` provides the (relative) path of an (optional) shader to run

```
"horizontalFieldOfView":  103.0,            // Field of view (horizontal) for the user in degrees
"frameDelay" : 3,                           // Frame delay (in frames)
"frameRate" : 60,                           // Frame/update rate (in Hz)
"shader": "[your shader].pix",              // Default is "" or no shader
```

## Player Controls
* `moveRate` sets the rate of player motion in m/s, set this parameter to `0` to disable player motion
* `playerHeight` sets the height of the player above the ground when "standing"
* `crouchHeight` set the height of the player when crouched (set equal to `playerHeight` for no crouch)
* `jumpVelocity` sets the magnitude of the upward impulse introduced by a jump (set to 0 for no jump)
* `jumpInterval` sets the minimum time between player jumps in seconds (set to 0 for no limiting)
* `jumpTouch` sets whether or not the player needs to be in contact w/ a surface to jump
* `playerGravity` set the graivty vector that impacts the player

```
"moveRate": 0.0,                            // Player move rate (0 for no motion)
"jumpVelocity": 40.0,                       // Jump velocity
"jumpInterval": 0.5,                        // Minimum jump interval
"jumpTouch": true,                          // Require touch for jump
"playerHeight":  1.5,                       // Normal player height
"crouchHeight": 0.8,                        // Crouch height
"playerGravity": Vector3(0.0, -5.0, 0.0),   // Player gravity
```

## HUD settings
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

### Player Health Bar
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

### Ammo Indicator
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

### Weapon Cooldown
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

## Click to Photon Monitoring
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

## Target Rendering
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

### Target Health Bars
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

### Floating Combat Text
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