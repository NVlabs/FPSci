# Introduction
The user status file tracks all user infomration specific to a given experiment (unlike the [userconfig.Any](./userConfigReadme.md) file which tracks experiment-indepdent user settings). 

The user status is the primary mechanism by which sessions are ordered and progress is tracked in `FirstPersonScience`. Similarly to the [`userconfig.Any`](./userConfigReadme.md) this file controls per-user actions using a user table and also records completed sessions so that the application can track user progress over multiple runtimes.

## File Location
The [`userstatus.Any` file](../data-files/userconfig.Any) is located in the [`data-files` directory](../data-files) at the root of the project. If no `userstatus.Any` file is present at startup the application copies [`SAMPLEuserstatus.Any`](../data-files/SAMPLEuserstatus.Any) to `userstatus.Any`.

# User Table
The top-level user status table contains the following fields:

* `sequence` determines whether or not to (strictly) sequence the trials (this allows repeated trials)
* `sessions` is the default sessions list for any user in the file that does not have a sessions list specified

Each entry in the user table contains the following fields:

* `id` a quick ID used to identify the user
* `sessions` a list of sessions to be completed, in order
* `completedSessions` a list of sessions completed by any given user

Refer to the [SAMPLEuserstatus.Any file](../data-files/SAMPLEuserstatus.Any) for an example of these settings.

The `sessions` list above can be used to control session ordering (this is an ordered list). If random ordering is desired a quick script can be written to read all users from the `userconfig.Any` file and write a new sequence of sessions for each user present. Alternatively the top-level `sessions` list can be used to specify a single ordering for all participants:

```
sessions = ("s1", "s2", "s3"),
users = (
    {id = "User1";}
    {id = "User2";}
    {id = "User3";}
);
```

Once all the items from the `sessions` list are present in the `completedSessions` list the experiment is considered "done" for this user. At this point, if a user wants to re-run the experiment they need to open the `userstatus.Any` file and delete all items from their `completedSessions` list. Alternatively if new trials will be run for all users a new copy of the file (w/ empty `completedSessions` lists for all users) can be copy-pasted over the full one.

If the `sequence` flag is set to `true` then repeated sessions may appear in the `sessions` array as these will be strictly ordered. In addition when `sequence = true` the `completedSession` array is expected to match the `sessions` array item-for-item (as ordered). If this is not the case the application may not behave as intended. For this reason it is a good idea to make sure the `completeSessions` arrays are emptied following any change to the `sessions` arrays within `userstatus.Any`.