# Scene Files Requirements
First Person Science uses a modified version of the G3D `scene.any` file specification modified to include a few required fields.

## Additional Fields
In addition to the base `.scene.any` file, the following fields must/can be provided for a scene file to be considered valid for FPSci:

### Physics
Two "physics-based" parameters are also provided by the scene file in their own `Physics` header, these are:

* `minHeight` - A height at which the player respawns (at their spawn location) if they fall below
* `gravity` - Deprecated. Currently ignored...

An example of this configuration (from within the top-level of a `scene.Any` file) is provided below:

```
Physics = {
	minHeight = -10.0;
},
```

### Player Entity
The `PlayerEntity` is specified by the following sub-fields:

* `model` - A model to display for the player (currently not drawn)
* `frame` - An initial frame for the player (in the scene geometry coordinate units) can include rotation
* `collisionSphere` - A `Sphere` to be used as a collision proxy for the player (normally used to specify the radius of the proxy in scene units)

The parameters above allow the scene file to completely contain all scene-dependent information related to player motion. An example from the top-level of a `scene.any` file is provided below:

```
player = PlayerEntity {
    model = "playerModel";              // Use the model (specifed below as "playerModel") for the player
    frame = Point3(0, 1.5, 0);          // Initialize the player 1.5m above the origin
    collisionSphere = Sphere(1.0);      // Use a 1m sphere as the collision proxy
};
```

### Player Camera
Any camera specified in the scene can be used as the camera attached to the player. This mapping is done by putting the name of the chosen camera in the [FPSci scene settings](./general_config.md#scene-settings). If no name is specified, the `defaultCamera` will be used.

The player camera is a way to modify camera properties for the player view, excluding the following properties which are overridden in the application:

* Anti-aliasing
* Bloom strength
* Position/Rotation
* Field of View