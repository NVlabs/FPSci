# Model Files
This directory contains model files for various custom models not contained within the default G3D install.

Most of the models used here are encoded as `.obj` files for geometry with their afilliated `.mtl` files colocated.

Examples of models that can be stored here include:
* Weapons
* Player(s)
* Targets
* Scenes

## Directory Structure
We adopt the convention of storing each model file (together with its afilliated material information) in an appropriately named subdirectory.

## Adding New Models
To add new models, simply create a directory with the model "name" and refer to `model/[dir name]/` from G3D when loading the model (path relative to the `data-files` directory). 