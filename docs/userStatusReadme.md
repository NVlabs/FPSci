# Introduction
The user status file tracks all user infomration specific to a given experiment (unlike the [userconfig.Any](./userConfigReadme.md) file which tracks experiment-indepdent user settings). 

The user status is the primary mechanism by which sessions are ordered and progress is tracked in `FirstPersonScience`. Similarly to the [`userconfig.Any`](./userConfigReadme.md) this file controls per-user actions using a user table and also records completed sessions so that the application can track user progress over multiple runtimes.

## File Location
The `userstatus.Any` file is located in the [`data-files` directory](../data-files) at the root of the project. If no `userstatus.Any` file is present at startup the application writes a set of default values to `userstatus.Any`. The default user name in this file is `anon` and the sessions assigned to this user refer to the default values for `experimentconfig.Any` to make the solution work as-is without any config files present.

# User Table
The top-level user status table contains the following fields:

* `allowRepeat` determines whether or not to (strictly) sequence the trials, allowing for repeats
* `randomizeSessionOrder` determines whether or not individual users are assigned randomized orderings from the (default) `sessions` array (defined below)
* `sessions` is the default sessions list for any user in the file that does not have a sessions list specified
* `completedLogFilename` is the (optional) filename to use to store the completed session log (formatted as a CSV with `user id, session id` format). If unspecified the completed log filename will be `[user status filename].sessions.csv`.

Each entry in the user table contains the following fields:

* `id` a quick ID used to identify the user
* `sessions` a list of sessions to be completed, in order

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