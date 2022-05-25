# Using samples to design your own experiment

You can try these samples by setting `developerMode` to `True` in the `startupconfig.Any`. Once you find one you like, and want to base a new experiment on, we highly recommend copying the sample into either the main config files, or into a different config from your startup config list. For example, if you wanted to use the weapons experiment as a starting point, you'd copy the 3 following files as follows:

* `samples/weapons.Experiment.Any` copied to `experimentconfig.Any`
* `samples/weapons.Status.Any` copied to `userstatus.Any`
* `samples/sample.Users.Any` copied to `userconfig.Any`