# Scene Files Requirements
First Person Science uses a modified version of the G3D `scene.any` file specification modified to include a few required fields.

## Additional Fields
In addition to the base `.scene.any` file, the following fields must/can be provided for a scene file to be considered valid for FPSci:

### Physics
Two "physics-based" parameters are also provided by the scene file in their own `Physics` header, these are:

* `minHeight` - A height at which the player is respawned (at their spawn location) if the fall below
* `gravity` - Currently unused...

An example of this configuration (from within the top-level of a `scene.Any` file) is provided below:

```
Physics = {
	minHeight = -10.0;
},
```

### Player Entity
The `PlayerEntity` is specified by the following sub-fields:

* `model` - A model to display for the player (currently not drawn)
* `frame` - An initial frame for the player (in the scene geometry coordinate units)
* `heading` - An (independent) initial heading for the player (in radians)
* `collisionSphere` - A `Sphere` to be used as a collision proxy for the player (normally used to specify the radius of the proxy in scene units)

The parameters above allow the scene file to completely contain all scene-dependent information related to player motion. An example from the top-level of a `scene.any` file is provided below:

```
player = PlayerEntity {
    model = "playerModel";              // Use the model (specifed below as "playerModel") for the player
    frame = Point3(0, 1.5, 0);          // Initialize the player 1.5m above the origin
	heading = 0;                        // Player initial rotation of 0 radians
    collisionSphere = Sphere(1.0);      // Use a 1m sphere as the collision proxy
};
```

### Player Camera
The player camera may be specified as with any other camera in a `scene.any` file, but is required to have the name `playerCamera` as it's name to specify it as the camera used for the player view. If no player camera is provided in the scene file, but a `PlayerEntity` is provided, the application creates the player camera from the player entity.

The player camera is a way to modify camera properties for the player view, excluding the following properties which are overriden in the application:

* Anti-aliasing
* Bloom strength
* Position/Rotation
* Field of View