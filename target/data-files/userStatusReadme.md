# Introduction
The user status is the primary mechanism by which sessions are ordered and progress is tracked in the `abstract-fps` application. Similarly to the [`userconfig.Any`](./userConfigReadme.md) this file controls per-user actions using a user table.

## User Table
Each entry in the user table contains the following fields:

* `id` a quick ID used to identify the user
* `sessions` a list of sessions to be completed, in order (this may move to `userconfig.Any` shortly)
* `completedSessions` a list of sessions completed by any given user