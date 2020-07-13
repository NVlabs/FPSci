# `data-files` Directory
This directory contains data files affiliated with the `abstract-fps` application, including all of the `.Any` config files affiliated with controlling experiments. The project built by [`abstract-fps.vcxproj`](../abstract-fps.vcxproj) is configured to run from this directory.

Not all of these files are currently used in the application, but they have been maintained to provide useful resources to future developers.

## File Structure
Brief outlines of the sub-directories to this directory are included below:

* [`gui`](./gui) contains some `.png` files used for drawing 2D GUI elements
* [`material`](./material) contains `.png` and `.Any` files affiliated with materials used in the game (explosion, bullet decals)
* [`model`](./model) contains `.mtl` and `.obj` files for [weapon](./model/sniper) and [target](./model/target) models used in the application
* [`scene`](./scene) contains `.Any` files specifying G3D scenes
* [`shader`](./shader) contains any `.pix` shaders affiliated with the application
* [`weapon`](./weapon) contains `.Any` file weapon configs for a variety of weapons, as well as the [readme.md](./weapon/weaponConfigReadme.md) documenting these `.Any` files

In addition the following files are included in this directory:

* `icon.png` and `icon.ico` are files affiliated with the application's icon
* `log.txt` the output log from a run of the application (contains some descriptive)
* `reticle.png` is an unused reticle

## Included Documentation
The following documentation used to be included here (it has now been moved to the [`docs](../docs) directory.

### Configuration Files
* [`experimentConfigReadme.md`](../docs/experimentConfigReadme.md) - A guide to the `experimentconfig.Any` file
* [`startupConfigReadme.md`](../docs/startupConfigReadme.md) - A guide to the `startupconfig.Any` file
* [`userConfigReadme.md`](../docs/userConfigReadme.md) - A guide to the `userconfig.Any` file
* [`userStatusReadme.md`](../docs/userStatusReadme.md) - A guide to the `userstatus.Any` file
* [`weaponConfigReadme.md`](../docs/weaponConfigReadme.md) - A guide to weapon configurations in `experimentconfig.Any`
* [`keymap.md](../docs/keymap.md) - A guide to the `keymap.Any` file

### Scene Files
More information on scene files is available in the [scene readme](scene/readme.md).

### Model Files
More information on model files is available in the [model readme](model/readme.md).