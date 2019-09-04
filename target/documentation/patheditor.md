# Waypoint-based Path Editor
The waypoint-based path editor is a user-friendly way to create reproducible target paths from either realtime interactive sessions or recorded player motion. It is intended to be as simple, yet flexible as possible to allow for extension by the user.

![](./content/waypoint_editor.jpg)

## Features
The currently supported, in-app path editor features include:

* Manually drop waypoints at positions and pre-set timing (adjustable per point)
* Record your own motion
    * Distance based (constant speed replay)
    * Time based (your actual speed w/ an optional scale factor)
* Save/load existing paths
* Preview a loaded path using a dummy target
* Window-based manager for path editing/tweaking
    * Selection of an arbitrary point from a path list via list
    * Mouse-based selection of points
    * Remove the last dropped point
    * Remove an arbitrary dropped point
* Keyboard-based path position editing

## Keyboard-based Path Editing
In addition to path recording and text-based editing for paths we now support a keyboard-based waypoint editing mode. The editor use the following keys for (axis-aligned) manipulation.


| Key       | Description           |
|-----------|-----------------------|
|`Insert`   | Move in +X direction  |
|`Delete`   | Move in -X direction  |
|`Page Up`  | Move in +Y direction  |
|`Page Down`| Move in -Y direction  |
|`Home`     | Move in +Z direction  |
|`End`      | Move in -Z direction  |

Remaining TODO Items:
---
* Click and hold/drag based interface for position points
* The position field should be extended to include rotation