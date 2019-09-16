# Logger Scripts
These are the minimal set of logging scripts required for use with the application when measuring realtime click-to-photon in app. For those not interested in per click click-to-photon timing and/or without access to the hardware event monitor tool developed for this measurement this directory can be ignored (set `HasLogger = false` in [`systemconfig.Any`](../../../data-files/systemConfigReadme.md)).

For those interested in using the hardware logger review the documentation for [`systemconfig.Any`](../../../data-files/systemConfigReadme.md) and the hardware system before using the tool.

## Plotter
The `event_plotter.py` file included in this directory is not used by the abstract-fps tool, but is a useful way to interactive debug odd behavior involving the logger.