# Key Mapping
This file describes both the standard and custom key mapping/bindings for the application.

## Default Mapping
The default key map for the application is split into 2 modes, "normal" mode (`developerMode=false`) and "developer" mode (`developerMode=true`). We outline commands for each mode below.

### Normal Mode
When `developerMode=false` the following keys are used:

|Key Name       |Action                         |
|---------------|-------------------------------|
|`W`            |Move forward                   |   
|`A`            |Strafe left                    |   
|`S`            |Strafe right                   |   
|`D`            |Move backward                  |   
|`L Ctrl`       |Crouch                         |
|`Space`        |Jump                           |
|`Esc` or `Tab` |Open in-game menu              |
|`L Mouse`      |Fire the weapon during trial   |
|`R Shift`      |Fire the weapon pre-trial      |
|`-`            |Quit the application           |

### Developer Mode
When `developerMode=true` the following keys/actions are available in addition to the "normal" key map above.

| Key Name  |Action                             |
|-----------|-----------------------------------|
|`Q`        |Drop a waypoint                    |
|`R`        |Record player motion as waypoints  |
|`1`        |Open the render control window     |
|`2`        |Open the player control window     |
|`3`        |Open the weapon control window     |
|`4`        |Open the waypoint control window   |
|`Page Up`  |Move waypoint up in space          |
|`Page Down`|Move waypoint down in space        |
|`Home`     |Move waypoint in in space          |
|`End`      |Move waypoint out in space         |
|`Insert`   |Move waypoint right in space       |
|`Delete`   |Move waypoint left in space        |

## Custom Key Mappings
As an alternative to the standard key mappings provided above the user can change/add buttons to any of these actions using a `keymap.Any` file. If no `keymap.Any` file exists when the application starts it will write out the default.

This file associates each of the map names outlined below to an array of `GKey` strings indicating the key(s) that should be mapped to this action.


|Action                             |Map Name               |Default Key(s)         |
|-----------------------------------|-----------------------|-----------------------|
|Move forward                       |`moveForward`          |`["W", "Up"]`          |
|Strafe left                        |`strafeLeft`           |`["A", "Left"]`        |
|Move backward                      |`moveBackward`         |`["S", "Down"]`        |
|Strafe right                       |`strafeRight`          |`["D", "Right"]`       |
|Crouch                             |`crouch`               |`["L Ctrl"]`           |
|Jump                               |`jump`                 |`["Spc"]`              |
|Open in-game menu                  |`openMenu`             |`["Esc"]`              |
|Fire the weapon during trial       |`shoot`                |`["L Mouse"]`          |
|Use a scope                        |`scope`                |`["R Mouse"]`          |
|Fire the weapon pre-trial          |`dummyShoot`           |`["R Shift"]`          |
|Quit the application               |`quit`                 |`["Keypad -", "Pause"]`|
|Drop a waypoint                    |`dropWaypoint`         |`["Q"]`                |
|Record player motion as waypoints  |`toggleRecording`      |`["R"]`                |
|Open the render control window     |`toggleRenderWindow`   |`["1"]`                |
|Open the player control window     |`togglePlayerWindow`   |`["2"]`                |
|Open the weapon control window     |`toggleWeaponWindow`   |`["3"]`                |
|Open the waypoint control window   |`toggleWaypointWindow` |`["4"]`                |
|Move waypoint up in space          |`moveWaypointUp`       |`["Pg Up"]`            |
|Move waypoint down in space        |`moveWaypointDown`     |`["Pg Dn"]`            |
|Move waypoint in in space          |`moveWaypointIn`       |`["Home"]`             |
|Move waypoint out in space         |`moveWaypointOut`      |`["End"]`              |
|Move waypoint right in space       |`moveWaypointRight`    |`["Ins"]`              |
|Move waypoint left in space        |`moveWaypointLeft`     |`["Del"]`              |

### GKey Strings
The table below provides some useful macros for mapping `String` --> `GKey`. Whenever you are referring to a "normal" character key, be sure to use upper case letters!

|Key                |String                         |
|-------------------|-------------------------------|
|Letter             |Captial letter (i.e. `"A"`)    |
|Number             |Number string (i.e. `"1"`)     |
|Symbol             |Symbol for key (i.e. `"-"`)    |
|Space bar          |`"Spc"`                        |
|Left/Right Shift   |`"L Shift"`/`"R Shift"`        |
|Left/Right Control |`"L Ctrl"`/`"R Ctrl"`          |
|Left/Right Mouse   |`"L Mouse"`/`"R Mouse"`        |
|Page Up/Down       |`"Pg Up"`/`"Pg Dn"`            |
|Home               |`"Home"`                       |
|End                |`"End"`                        |
|Insert             |`"Ins"`                        |
|Delete             |`"Del"`                        |
|Escape             |`"Esc"`                        |
|Tab                |`"Tab"`                        |
|Up/Down (arrow)    |`"Up"`/`"Down"`                |
|Left/Right (arrow) |`"Left"`/`"Right"`             |

