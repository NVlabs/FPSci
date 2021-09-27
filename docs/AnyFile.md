# Any File Specification
The [G3D Innovation Engine](https://casual-effects.com/g3d/www/index.html) uses an object serialization format similar to JSON or XML for control/creation of many objects without a need to edit C++.

Understanding the `.Any` file format and specification can help substantially reduce developer time and improve organization of projects with many moving pieces. This document is intended to serve as supplementary material to the existing [`.Any` documentation](https://casual-effects.com/g3d/G3D10/build/manual/class_g3_d_1_1_any.html#details) already available in the G3D documentation.

The following text, taken from the documentation linked above summarizes the developer's choice to create the `.Any` format ahead of existing solutions and acknowledges support for JSON formatting.

> The custom serialization format was chosen to be terse, easy for humans to read, and easy for machines to parse. It was specifically chosen over formats like XML, YAML, JSON, S-expressions, and Protocol Buffers, although there is no reason you could not write readers and writers for G3D::Any that support those. Any also currently supports the JSON format.



## Basic Syntax
The `.Any` file format is a super-set of JSON with tolerance for various useful constructs from C++ and XML. This section describes syntax for much of the basic capture of information in the `.Any` format.

The format is designed to be line-to-line compatible with JSON import/export tools to serve as an easy interface to other languages/scripts that might provide useful functionality to G3D.

### Line Termination and White Space
The `.Any` format is (except where otherwise noted) white space tolerant (many formatting are acceptable) and can use either `,` or `;` as a line delimiter. This makes it compatible both with JSON/XML and C++ style formatting.

### Value Specification
Similar to JSON or XML the heart of the `.Any` specification is the assignment of a value to a (typically) named field in a "table" (dictionary) style data structure. Typically these assignments look as follows:

```
fieldName = [value]
```

However many other format are also supported. All of the following are also valid assignments in the `.Any` format:

```
fieldName : [value],
fieldName = [value]
"fieldName" = [value],
"fieldName" : [value],
```

### Comments and Documentation
Unlike JSON, the `.Any` specification supports C-style line/block comments specified by either preprending the `//` to a line comment, or using `/*` and `*/` to delimit block comments.

```
/*
This is an example block comment
*/
fieldName = [value],            // This is an example line comment
```

### Including Other Files
In addition to the constructs described above, the `.Any` specification allows "inlining" of other `.Any` file contents into the structure through use of a C-style `#include` statement as demonstrated below.

```
fieldName = #include("[filename].Any")
```

This allows `.Any` files to be modularly constructed from files containing sub-sets of their contents. This construct effectively decouples the `.Any` file structure/organization from the parsed contents of a single file and is _very_ convenient when restructuring `.Any` heirarchy for improved organization/readability.

Note that the target of a `#include` directive need not be a `.Any` file, it can be any content that will be parsed correctly within the Any syntax. For example JSON file can always be included as Any syntax is a superset of JSON. Alternatively a CSV file can be included to populate an array as demonstrated below:

```
"myArray" = (#include("array_contents.csv"))        // Include CSV file as the contents of an array
```

### Integration with C++
In addition to the comments and `#include` statements described above, the `.Any` file specification also supports direct inlining of certain C++ style "instructions" into many of its substructures. As an example refer to the [preprocess field](https://casual-effects.com/g3d/G3D10/build/manual/class_g3_d_1_1_articulated_model_1_1_specification.html#details) in the `ArticulatedModel::Specification` which makes use of `ArticulatedModel::Specification::Instruction`s to allow C++-esque transforms of a model file.

## Supported Types
In order to effectively use  understanding the possible types that can be serialized in/out is a huge benefit.

### Primitive Types
The `.Any` format supports a variety of common types (usually through the built-in G3D type). Some supported types and examples of value assignment are provided in the table below:

| type   | example           |
|--------|-------------------|
|bool    |`value = true`     |
|int     |`value = 6`        |
|float   |`value = 6.0`      |
|String  |`value = "test"`   |

### G3D Specific Types
Many 2D/3D G3D primitives support direct serialization from `.Any` to allow additional configuration. Some of these supported types are outlined in the table below.

| type   | example                        |
|--------|--------------------------------|
|Point2  |`test = Point2(1.0,1.0)`        |
|Point3  |`test = Point3(1.0,1.0,1.0)`    |
|Vector2 |`test = Vector2(1.0,1.0)`       |
|Vector3 |`test = Vector3(1.0,1.0,1.0)`   |
|Color3  |`test = Color3(1.0,1.0,1.0)`    |
|Color4  |`test = Color4(1.0,1.0,1.0,1.0)`|
|AABox   |`test = AABox{Point3(...), Point3(...)`|

### Higher Order Data Structures
In addition to the primitive types described above, `.Any` also supports organizatin of these types into higher-order data structures like arrays, and tables.

Some examples of these types are provided below:

| type   | example                              |
|--------|--------------------------------------|
|Array   |`value = [1,2,3]`                     |
|Table   |`value = {v1 = 1.0, v2 = 0.0}`        |
|Object  |`value = {v=1.0, name="test"}`        |

In the `Object` example above different data types are used to indicate that an underlying serializable C++ object has been created that expects a float (`v`) and a string (`name`). For more infomration on writing C++ for input serialization refer to the [C++ for Any Parsing](###-C++-for-Any-Parsing) section below. 

## Uses
### Any File Input
One of the most common uses of `.Any` files is in setting up scene/object properties at runtime (similar to the app.config in C#). Arbitrary objects can be constructed from `.Any` by implementing a constructor that uses an Any reader to parse objects from a file.

Since `.Any` files are stored as text and can be edited in amost any text editor, using a `.Any` file as input provides an option for runtime control without a need to compile/rebuild code or use a source editor.

### C++ for Any Parsing
Parsing information into custom objects from `.Any` files in C++ is relatively easy as a result of the built-in functions provided in the default G3D includes. An example (intended for a header file) is provided below:

```
#include <G3D/G3D.h>

class myClass {
public:
    int ver = 0;            // Versions are a recommended parsing trick
    float v = 1.0f;         // Values can have defaults specified here
    String str;             // This is a G3D::String
    String fname = "";      // This is a filename as G3D::String
    bool flag;          

    // Default constructor
    myClass(version=0, flag=true) {ver=version, flag=flag};

    // Any constructor
    myClass(const Any& any){
        AnyTableReader reader(any);
        reader.getIfPresent("version", ver);        // Get the version

        if(ver == 0){
            reader.getIfPresent("value", v);        // Optional values
            reader.get("name", str);                // Require this string
            reader.get("flag", flag);               // Require a flag
            reader.getFilenameIfPresent("filename", fname); 
        }
        // Could define other versions here...

        if(filename == ""){
            filename = default;
        }
    }
}
```

Once the code above has been implemented the structure can be loaded from a `.Any` file using:

```
myClass c = Any::fromFile(filename);
```
Where `filename` is a string containing the name of the `.Any` file to load (relative to the runtime directory). If you want to search the full G3D path for a file you can use `System::findDataFile()` to get this path from a file's name as shown below.

```
myClass c = Any::fromFile(System::findDataFile(filename));
```

### Examples
This section provides some examples of using `.Any` files as input in the G3D Innovation Engine.

#### Scene File Specification
By default, G3D scenes (and their affiliated entities such as models and cameras) are specified using the `.Any` format, and as such these files are a great place to look for uses of the format.

Scene `.Any` files typically begin with a `description` field (optional for use) followed by an `entities` object that can specify:

* Scene `name`
* `building` or the "scene geometry" model as a `VisibleEntity`
* `player` as a `PlayerEntity`
* `camera` as a `Camera` complete w/ position and render settings
* `skybox` as a `SkyBox` (including a `Texture::Specification`)
* `sun` as a `Light`
* `lightingEnvironment` as a `LightingEnvironment`
* `environmentMap` as a `Texture::Specification`
* `models` as a `Table` of `modelName, Specification` pairs

Many of the objects above are themselves complicated `.Any` structures. Some of these are outlined in the [Object Creation and Manipulation](##-Object-Creation-and-Maipulation-(G3D-Specific)) section below.

### Any File Output
In addition to its uses in input/control the `.Any` specification can also be easily used for output of structured data (as JSON and XML are often used for structured output/logging).

An example of Any file output for the previously provided [C++ example](###-C++-for-Any-Parsing) above.

```
    Any toAny(const bool forceAll=true) const{
        // Create and assign values
        Any a(Any::TABLE);
        a["version"] = ver;
        a["value"] = v;
        a["name"] = str;
        a["flag"] = flag;
        a["filename"] = fname;
        return a;
    }
```

## Object Creation and Maipulation (G3D Specific)
Many G3D types include `.Any` serializable `Specification`s which can be used to create their parent objects within G3D. These `Specification`s are useful since they allow us to manipulate complex objects (like 3D models) outside of the compiled source at runtime. G3D [scene files](####-Scene-File-Specification) are a good example of the use of these specifications, but some examples will also be provided in this section.

#### ArticulatedModel::Specification Preprocessing
The [`ArticulatedModel::Specification`](https://casual-effects.com/g3d/G3D10/build/manual/class_g3_d_1_1_articulated_model_1_1_specification.html#details) is useful as it allows us to load/manipulate a 3D model (from a variety of supported files) within the `.Any` file. 

An example is provided below:

```
"modelSpec": ArticulatedModel::Specification{
    filename = "[modelname].obj",
  	preprocess = {
		  transformGeometry(all(), Matrix4::yawDegrees(90)),
		  transformGeometry(all(), Matrix4::scale(1.2,1,0.4)),
		  transformGeometry(all(), Matrix4::translation(0.5, 0, 0))
	  },
	  scale = 0.25
}
```
In the `ArticulatedModel::Specification` above the author loads `[modelname].obj` and `preprocesses` the geometry to rotate it 90° in yaw, scale it (asymmetrically) in 3 axes, then translate it to the correct position within its frame. In addition to these `preprocess` steps a uniform scale factor (applied to all axes) is specified.


