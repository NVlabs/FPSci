# Developer Mode
During development of `FirstPersonScience` we created a number of utilities for testing target motion and game play outside of the formal experiment definition. The `developerMode` flag in [`startupconfig.Any`](../data-files/startupConfigReadme.md) controls whether the application is run in experiment "play" mode (`developerMode=False`) or developer mode `(developerMode=True`).

## Developer Mode Features
When running in developer mode a number of the otherwise static features of the tool are available for dynamic toggle via an expanded Heads Up Display menu accessible during the normally available pause (Tab) menu.

Features include:
* Dynamic control of bullet rendering
* Dynamic control of HUD rendering
* Dynamic control of on-screen FPS monitoring
* Optional turbo mode (decreases visual quality)
* On-demand target spawn
* Dynamic frame rate control
* Dynamic output latency control
* Dynamic reticle selection
* Dynamic brightness (exposure) control
* Waypoint-based [target path creation](./patheditor.md)
* Initialization of player position and view direction
* Dynamic config reload in app via the `reloadConfigs` mapped key

## Enabling Developer Mode
As mentioned above, all that needs to be done to enabled developer mode is modifying the `developerMode` field in [`startupconfig.Any`](../data-files/startupconfig.Any) to `True`, then running the application to enter developer mode.

## Player Position Initialization
Once in developer mode the initial player position can be set by pressing the menu key (see `openMenu` [keymap](keymap.md)) to enter the pause menu (pointer mode) then clicking the `Set Start Pos` button while positioned/aiming in your desired initial location/direction.