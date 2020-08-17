# Documentation
Welcome to the `FirstPersonScience` Documentation! The experiment design is primarily controlled through a series of text files using the `.Any` extension and file format. As an experiment designer, you'll want to familiarize yourself with the format somewhat, then take a look at the configuration file descriptions from the list below to understand how to configure the various components of the experiment. The bulk of the experiment design is controlled through the `experimentconfig.Any` file.

Configuration
---
* [`.Any` file introduction](./AnyFile.md)
* Configuration file descriptions:
    * [General parameters](general_config.md)
    * [`experimentconfig.Any`](experimentConfigReadme.md)
    * [`startupconfig.Any`](startupConfigReadme.md)
    * [`systemconfig.Any`](systemConfigReadme.md)
    * [`userconfig.Any`](userConfigReadme.md)
    * [`userstatus.Any`](userStatusReadme.md)
    * [`.weapon.Any` files](weaponConfigReadme.md)
    * [`keymap.Any`](keymap.md)

In addition to configuration, [the results file format is documented here](resultsFiles.md).

# Development
FPSci gives you full source level control over modifying its functionality however you'd like. The "parent" readme below will help you get started with building FPSci from source. The Developer Mode and Path Editor are tools that can be helpful for experiment designers or software developers.

* [Parent readme.md](../readme.md) has many useful links
* [Developer Mode](./developermode.md)
* [Path Editor](./patheditor.md)
