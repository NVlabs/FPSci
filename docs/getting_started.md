# Getting Started with First Person Science
Once you have downloaded/installed G3D and successfully built FPSci for the first time it makes sense to design a small experiment to help familiarize yourself with the FPSci experiment design experience.

## First Run
The first run of the FPSci application results in generation of "default" config files which do not contain interesting sessions that can be used to test out the application. However, for this reason it is a good idea to use the first run of FPSci to ensure that the tool is built correctly and separate configuration file issues from build/environment issues.

If you are ever running into persistent issues with FPSci throwing exceptions/failing to launch, you can always try deleting/reverting the config files to the default to see if the issue is in source/the G3D environment versus an issue of configuration. We've tried to make configuration-based exception messages as descriptive as possible, but this may not always be the case.

## Setting up the Config Files
To start out with we don't have any of the required configuration files we will need for our experiment. This results in the app loading with the "All Sessions Complete!" message displayed on the screen. In addition the only user present is `anon`, a default user that we will want to change to be able to track individual performance.

### Experiment Configuraion
Generally speaking, when starting a new experiment design, it makes sense to start from the [`experimentconfig.Any` file](experimentConfigReadme.md). This file contains both the experiment-wide application parameters and the configurations for sessions and targets/trials within these sessions.

There are 2 ways to get started on an `experimentconfig.Any` file. You can either start writing a file from scrath, or run the application the first time to produce the deafult `experimentconfig.Any` and edit from here.

#### General Configuration
To begin with we need to specify the `settingsVersion` (used for parsing) as `1` (currently the only supported settings verison). We should also provide a `description` for the experiment.

```
{
    settingsVersion: 1,
    description: "My first experiment",
```

When authoring an initial experiment config we can rely on many of the deafult values for the experiment configuration (particularly in the [general parameters](general_config.md)). If a value isn't specified in the config file, it is assumed to have its default value. We can modify any (global) configuration parameter by simply specifying it at the top-level of the experiment config.

```
    moveRate : 5.0,     // Default here is 0 m/s
```

#### Setting Up a Target
There are a few fields we will need to populate manually to provide an interesting experiment. One of the first non-generic things to specify in an experiment config is the `targets` array. This array provides (named) targets that can be used to setup trials within sessions later in the configuration.

To start with we will setup a simple target that uses the apps default target model and avoids world-space coordinates since these would be specific to a particular scene. For more information on setting up targets refer to [the `experimenyconfig.Any` readme](experimentConfigReadme.md#target-configuration).

```
    targets = [
        {
            id : "first_target",            // This is the name we refer to throughout the config
            destSpace: "player",            // This avoids world space coordinates (too specific)
            speed: [5, 10],                 // This sets a range of speeds for the target
            visualSize: [0.1, 0.1],         // This sets the size of the target to a constant
            eccH: [5, 10],                  // Spawn the target between 5-10° eccentricity (horizontal)
            eccV: [0, 5],                   // Spawn the target between 0-5° eccentricity (vertical)
            motionChangePeriod: [1, 2],     // Have the target change direction every 1-2s
            jumpEnabled: false              // Don't let this target "jump"
        }
    ]
```

#### Adding Sessions
Let's add a session to the experiment. We do this by creating a `sessions` array within the top-level experiment config. Within this `sessions` array we can add an insteance of a session. Within this session we can (again) modify any of the [general parameters](general_config.md).

The only _required_ field within any given session configuration is the `trials` array. Which specifies a series of targets (using their `id` field) and counts for each of these targets.

```
    sessions = [
        {
            id : "test_sess",
            description: "my first session",
            frameRate : 120,
            trials: [
                ids: ["first_target", "first_target"],              // Spawn two "first_target"s at a time
                count: 5                                            // Perform 5 of this trial
            ]
        }
    ]
```

### Setting Up User Parameters
User configuration is managed via 2 files ([`userconfig.Any`](userConfigReadme.md) and [`userstatus.Any`](userStatusReadme.md)). The [`userconfig.Any` file](userConfigReadme.md) specifies per-user settings (DPI and mouse sensitivity in cm/360°) as well as the `currentUser` (user to set as active when loading the app). All of these values are editable from within the application runtime.

The [`userstatus.Any` file](userStatusReadme.md) tracks the `sessions` to be completed as well as the `completedSessions` for any given user. These values cannot be edited from within the application.


#### User Configuration
Let's create a single user for our new experiment. To do this we will start in the `userconfig.Any` file, where we can modify its content as follows:

```
{
    settingsVersion: 1,
    currentUser: "user",
    users = [
        {
            id: "user",
            mouseDPI: 800,
            cmp360: 30
        }
    ]
}
```

In the file above we create a new user (user name is "user") and set them as the `currentUser` for the application. Note that the user id set as the `currentUser` should always appear in the `users` array within the `userconfig.Any` file.

#### User Status
Now that we have a configuration for our new user, we can update the [`userstatus.Any` file](userStatusReadme.md) to link our new session (created [above](#adding-sessions)) to this user. This is done as shown below:

```
{
    settingsVersion: 1,
    users = [
        {
            id: "user",
            sessions: ["test_sess"],
            completedSessions = []
        }
    ]
}
```

In the `userstatus.Any` file above, we include our new session from above (`"test_sess"`) in the `sessions` list for the user, and then empty the `completedSessions` list for the user to make sure that this sessions is not marked as completed. Once the user finishes running this trial, the application will automatically update `userstatus.Any` to add `"test_sess"` to  the `completedSessions` array.
