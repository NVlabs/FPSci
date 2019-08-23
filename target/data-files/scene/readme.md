# Scene Files
G3D scene files are specified using the [`.Any`](../../documentation/AnyFile.md) format. This readme doesn't fully document the `scene.Any` file, but does provide some useful tips for experiment developers interested in editing the scene.

Scene files specify the following aspects of the in-game experience:
* Scene geometry (including player geometry), skybox, and lighting
* Initial player postion and heading
* Simple world-space physics (`gravity` and `minHeight` for respawn)
* Camera settings and rendering parameters

## Player Position and Heading
One set of parameters controlled from within the `scene.Any` file is the initial spawn location and heading of the player. These are specified using the `frame` and `heading` fields of the `PlayerEntity`.

* `frame` can be specified either as a full `CFrame` or as a position using a `Point3` type. If the frame is specified as a `CFrame` the rotation will be ignored in favor of `heading` as specified below
* `heading` is a float specifying the distance to rotate from the "initial" position (in degrees, counter clockwise)
* `model` is a pointer to a 3D model type for the player specified elsewhere in the `scene.Any` file
* `collisionSphere` is a `Sphere` specified as a radius (in meters) for the geometry to use to detect collisions

In addition the size of the sphere used for collision detection can also be set using the `collisionSphere` parameter.

An example player declaration (from the top-level of a `scene.Any` file) is included below for reference:

```
player = PlayerEntity { 
            frame = Point3(47.523, -2.3398, -0.35916); 
            heading = 86.4005; 
            model = "playerModel";
            collisionSphere = Sphere(1.5); 
        }; 
```

## Scene Physics
There are currently only 2 "physics" parameters available for control in the scene file. These are specified under a `Physics` parameter at the top-level of the file.

* `minHeight` specifies the height (in meters) below which the player is respawned at the initial position
* `gravity` specifies the acceleration (in meters/second^2) experienced by the player in the scene

An example specification is provided below:

```
Physics = {
    minHeight = -10,
    gravity = 9.8
}
```

## Naming Conventions
The default scene filenaming convention is `[scene name].scene.Any`. 

A scene's `name` in G3D is not (necessarily) it's filename. Instead the `name` field from the top-level of the scene specifies what it will be referred to as in G3D.

Scene files are heirachically organized based upon matches in the start of their `name` strings. For example a scene where `name = "Test Scene #1"`
 and a scene where `name = "Test Scene #2"` will (in G3D supported menus) be automatically grouped together under the title `Test Scene` as sub-items `#1` and `#2`.

## File Organization
Our project adopts the G3D convention of a [`scene`](../scene) directory. Convention is to store only the `[filename].scene.Any` file(s) in the `scene` directory while storing any affiliated models in the nearby [`model`](../model) directory.

The `scene.Any` file typical refers to its models using a path of `model/[modelname]/` from the data directory.
