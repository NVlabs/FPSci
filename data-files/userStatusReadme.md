# Introduction
The user status file tracks all user infomration specific to a given experiment (unlike the [userconfig.Any](./userConfigReadme.md) file which tracks experiment-indepdent user settings). 

The user status is the primary mechanism by which sessions are ordered and progress is tracked in the `abstract-fps` application. Similarly to the [`userconfig.Any`](./userConfigReadme.md) this file controls per-user actions using a user table and also records completed sessions so that the application can track user progress over multiple runtimes.

# User Table
Each entry in the user table contains the following fields:

* `id` a quick ID used to identify the user
* `sessions` a list of sessions to be completed, in order
* `completedSessions` a list of sessions completed by any given user

Refer to the [SAMPLEuserstatus.Any file](SAMPLEuserstatus.Any) for an example of these settings.

The `sessions` list above can be used to control session ordering (this is an ordered list). If random ordering is desired a quick script can be written to read all users from the `userconfig.Any` file and write a new sequence of sessions for each user present.

Once all the items from the `sessions` list are present in the `completedSessions` list the experiment is considered "done" for this user. At this point, if a user wants to re-run the experiment they need to open the `userstatus.Any` file and delete all items from their `completedSessions` list. Alternatively if new trials will be run for all users a new copy of the file (w/ empty `completedSessions` lists for all users) can be copy-pasted over the full one.