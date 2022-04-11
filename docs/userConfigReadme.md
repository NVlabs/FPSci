# Introduction
The user config is the mechanism by which users are registered and provide their input sensitivity in `FirstPersonScience`.

The high-level `userconfig.Any` file provides the `currentUser` (the default user when launching the application), together with a `users` table that contains per subject mouse DPI and sensitivity (in cm/360°). It also provides a [`defaultUser`](#default-user-configuration) to use as a base for new users added in app.

The user config is setup to work across multiple experiments (i.e. it does not have any information specific to an `experimentconfig.Any` file contained within in). For per user session ordering see the [`userStatus.Any`](./userStatusReadme.md).

## File Location
The `userconfig.Any` file is located in the [`data-files` directory](../data-files/) at the root of the project. If no `userconfig.Any` file is present the application writes a default to `userconfig.Any`. The default user is named `anon` and corresponds to the default `userstatus.Any` file. This config assumes an 800DPI mouse and a 12.75cm/360° mouse sensitivity without mouse inversion.

# User ID Uniqueness
The `requireUnique` flag is settable at the top of the user configuration file (defaults to `true`). When this flag is set to true, all users will be required to have unique IDs. This is particularly useful when allowing users to enter their own user IDs using the `allowUserAdd` flag in the [user menu configuration](general_config.md#menu-config). Regardless of whether new users can be added in app,  will check for non-duplicate user IDs and throw an exception at startup.

To disable the unique user ID requirement add the following to the top of your user config file:

```
requireUnique: False;
```

# Default User Configuration
The default user is used when adding new users to the application from the in-app user menu. Defaults set up in this user will automatically be applied to any new user that is created (less the `id` field which is unused). Otherwise the `defaultUser` field supports all the same fields as a normal entry in the [user table](#user-table) as documented below.

If unspecified, any field in the `defaultUser` config defaults to the default user config values as provided below.

```
defaultUser = {                       // Configuration to apply to all newly created users
  mouseDPI = 800;
  mouseDegreePerMillimeter = 2.824;
  turnScale = Vector2(1,1);
  invertY = false;
  scopeTurnScale = Vector2(0,0);
  currentSession = 0;
  reticleIndex = 39;
  reticleScale = [1,1];
  reticleColor = [Color4(1,0,0,1), Color4(1,0,0,1)];
  reticleChangeTimeS = 0.3;
};
```

# User Table
The user table is formatted as an array of user entries. Each entry in the table contains the following fields:

|Field name             |Type     |Description                                                                                          |
|-----------------------|---------|-----------------------------------------------------------------------------------------------------|
|`id`                   |`String` |A quick ID used to identify the user                                                                 |
|`mouseDPI`             |`float`  |The mouse DPI used for the player (used for sensitivity adjustment)                                  |
|`mouseDegPerMillimeter`|`float`  |The mouse sensitivity for the user (measured in °/mm)                                                |
|`invertY`              |`bool`   |A quick flag for inverting the Y view (as opposed to setting the y-value for `turnScale` to -1)      |
|`turnScale`            |`Vector2(float)`|Provides a per-player view rotation/mouse sensitivity scale, designed to compound with the experiment/session-level `turnScale`.|
|`scopeTurnScale`       |`Vector2(float)`|Provides an (optional) additional turn scale to apply when scoped. If this value is `Vector2(0,0)` (or unspecified) then a "default" scaling of the ratio of FoV (scoped vs unscoped) is used to scale mouse sensitivity |
|        |  |     |
|`cmp360`               |`float`  | **_Deprecated_** - ignored if `mouseDegPerMillimeter` is found. The mouse sensitivity for the user (measured in cm/360°) |
|`reticleIndex`         |`int`           |Refers to which reticle this user prefers (if not required for the study)                            |
|`reticleScale`         |`Array<float>`  |Provides a range of reticle sizes over which to set the scale as an `Array` w/ 2 elements (min, max) | 
|`reticleColor`         |`Array<Color4>` |Provides a range of colors over which to set the reticle color as an `Array` w/ 2 elements (min, max)|
|`reticleChangeTime`    |`float`         |Provides the time (in seconds) for the reticle to change color and/or size following a shot          |

The full specification for the a user with default values is provided below as an example:

```
users = (
  {
    id = "anon";                    // "anon" is the application-wide default user name
    mouseDPI = 800;                 // 800 DPI mouse
    mouseDegPerMillimeter = 2.824;  // 2.824°/mm mouse sensitivity (12.75 cm for full rotation)
    turnScale = Vector2(1,1);       // Don't apply any additional mouse-based turn scaling
    invertY = false;                // Don't invert Y mouse controls
    scopeTurnScale = (0,0);         // Don't modify turn scale when scoped

    currentSession = 0;             // Select the first session by default

    reticleIndex = 39;        
    reticleScale = {1,1}            // Don't scale the reticle after a shot
    reticleColor = {Color4(1.0, 0.0, 0.0, 1.0), Color4(1.0, 0.0, 0.0, 1.0)};    // Use a green reticle (no color change)
    reticleChangeTimeS = 0.3;       // Doesn't matter since color/size don't chnage
  },
);
```

### Sensitivity Measure (°/mm)
Many games use arbitrary mouse sensitivity measures (i.e. 0-10 or low, mid, high scale) to set mouse sensitivity based on iterative player testing (i.e. choose a setting, test whether it meets your needs, adjust the setting accordingly). Unfortunately this means that mouse sensitivity settings can often not be easily translated between applications without a tool to convert between the settings values.

The unit of mouse sensitivity measure selected for the application is `°/mm`. This measure reports the (linear) distance the mouse needs to travel in order to produce a degree of player view rotation in game (typically in the horizontal direction). We select this unit for 2 reasons:

1. It is (easily) related to the  common `cm/360°` measure
2. It matches the intuition that high value ==> more sensitive (unlike `cmp360`)

This unit is related to `cm/360°` (`cm/360° = 36 / (°/mm)`), since the distance (in mm) required to make one full turn in game is just 360° / `°/mm`. Using this formula, a player can measure the full-turn distance for their game of choice, then transfer the setting into the abstract-fps application's user config.

Though the functionality is deprecated, since previous versions of `FPSci` supported using a `cmp360` variable, you can enter `cmp360` instead of `mouseDegPerMillimeter` in the user config and FPSci will convert it for you. If both `mouseDegPerMillimeter` and `cmp360` are found, then the `mouseDegPerMillimeter` will be used.

As an added complexity this sensitivity measure requires the user also report the mouse Dots Per Inch (DPI) setting so that signaling from the mouse can be correctly converted to centimeters traveled. This also means that while mice with dynamic DPI setting (i.e. a DPI button) will work fine; however, only a single DPI setting can be affiliated with the sensitivity recorded for the mouse.

### Selecting a Reticle
In order to preview reticles we suggest either:

1. Opening the application with `developerMode=true` and using the `Render Controls` menu to setup the correct reticle/crosshair and color/size
2. Preview the individual images (mostly located in `%g3d%\data10\common\gui\reticle`) to select the desired reticle index (appended to the filename)

If you would not like to have a dynamic reticle (i.e. have it stay the same size/color all the time), then set both values in the array for `reticleScale` and `reticleColor` to the same value.

An example of a non-default user configuration is provided below for a single-user case:

```
users = [ 
    { 
        id = "BB"; 
        mouseDegPerMillimeter = 1.2;        // Equivalent to 30 cm/360
        mouseDPI = 800;
        reticleIndex = 30;
        reticleScale = [0.5, 1.0];          // Scale the reticle from 50 --> 100% on fire
        reticleColor = [                    // Turn the reticle from white --> blue on fire
          Color4(1.0, 1.0, 1.0, 0.8),
          Color4(0.0, 0.0, 1.0, 0.8),
        ];
        reticleShrinkTime = 1.0;            // Scale the reticle back to 50% over 1s after fire
        turnScale = Vector2(1.0, 1.0);      // This is the default condition (no turn scale) set to -1 for invert X/Y
        scopeTurnScale = Vector2(0.0, 0.0); // This is the default condition (scoped turn scale based on ratio of scoped vs unscoped field of view)
    }, 
... 
```