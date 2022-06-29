#include "ExperimentConfig.h"

ExperimentConfig::ExperimentConfig(const Any& any) : FpsConfig(any) {
	FPSciAnyTableReader reader(any);
	switch (settingsVersion) {
	case 1:
		// Setup the default FPS config based on this
		SessionConfig::defaultConfig() = (FpsConfig)(*this);	// Setup the default configuration here
		// Experiment-specific info
		reader.getIfPresent("description", description);
		reader.getIfPresent("closeOnComplete", closeOnComplete);
		reader.get("targets", targets, "Issue in the (required) \"targets\" array for the experiment!");	// Targets must be specified for the experiment
		reader.get("sessions", sessions, "Issue in the (required) \"sessions\" array for the experiment config!");
		reader.getIfPresent("serverAddress", serverAddress);
		reader.getIfPresent("serverPort", serverPort);
		logPrintf("serverAddress is : %s:%d\n", serverAddress.c_str(), serverPort);
		break;
	default:
		debugPrintf("Settings version '%d' not recognized in ExperimentConfig.\n", settingsVersion);
		break;
	}

	init();
}

void ExperimentConfig::init() {
	// This method handles setting up default targets and sessions when none are provided
	bool addedTargets = false;
	if (targets.size() == 0) {
		addedTargets = true;
		TargetConfig tStatic;
		tStatic.id = "static";
		tStatic.destSpace = "player";
		tStatic.speed = Array<float>({ 0.f, 0.f });
		tStatic.size = Array<float>({ 0.05f, 0.05f });

		targets.append(tStatic);

		TargetConfig tMove;
		tMove.id = "moving";
		tMove.destSpace = "player";
		tMove.size = Array<float>({ 0.05f, 0.05f });
		tMove.speed = Array<float>({ 7.f, 10.f });
		tMove.motionChangePeriod = Array<float>({ 0.8f, 1.5f });
		tMove.axisLock = Array<bool>({ false, false, true });

		targets.append(tMove);

		TargetConfig tJump;
		tJump.id = "jumping";
		tJump.destSpace = "player";
		tJump.size = Array<float>({ 0.05f, 0.05f });
		tJump.speed = Array<float>({ 10.f, 10.f });
		tJump.motionChangePeriod = Array<float>({ 0.8f, 1.5f });
		tJump.jumpEnabled = true;
		tJump.jumpSpeed = Array<float>({ 10.f, 10.f });
		tJump.jumpPeriod = Array<float>({ 0.5f, 1.0f });
		tMove.axisLock = Array<bool>({ false, false, true });

		targets.append(tJump);
	}
	else {
		// Targets are present (make sure no 2 have the same ID)
		Array<String> ids;
		for (TargetConfig target : targets) {
			if (!ids.contains(target.id)) { ids.append(target.id); }
			else {
				// This is a repeat entry, throw an exception
				throw format("Found duplicate target configuration for target: \"%s\"", target.id);
			}
		}

	}

	if (sessions.size() == 0 && addedTargets) {
		SessionConfig sess60;
		sess60.id = "60Hz";
		sess60.description = "60Hz trials";
		sess60.render.frameRate = 60.0f;
		sess60.trials = Array<TrialCount>({ TrialCount(Array<String>({ "static", "moving", "jumping" }), 2) });

		sessions.append(sess60);

		SessionConfig sess30;
		sess30.id = "30Hz";
		sess30.description = "30Hz trials";
		sess30.render.frameRate = 30.0f;
		sess30.trials = Array<TrialCount>({ TrialCount(Array<String>({ "static", "moving", "jumping" }), 2) });

		sessions.append(sess30);
	}
}

ExperimentConfig ExperimentConfig::load(const String& filename, bool saveJSON) {
	ExperimentConfig ex;
	if (!FileSystem::exists(System::findDataFile(filename, false))) {
		// if file not found, save the default
		SessionConfig::defaultConfig() = FpsConfig();		// Need to reinitialize default experiment-level config here (if not will copy from any previously loaded valid experiment)
		ex = ExperimentConfig();							// Recreate this config (w/ reset default config)
		ex.toAny().save(filename, saveJSON);				// Save the defaults
	}
	else {
		ex = Any::fromFile(System::findDataFile(filename));	// Load from existing Any file
	}
	return ex;
}

void ExperimentConfig::getSessionIds(Array<String>& ids) const {
	ids.fastClear();
	for (const SessionConfig& session : sessions) { ids.append(session.id); }
}

shared_ptr<SessionConfig> ExperimentConfig::getSessionConfigById(const String& id) const {
	for (const SessionConfig& session : sessions) {
		if (!session.id.compare(id)) {
			return SessionConfig::createShared<SessionConfig>(session);
		}
	}
	return nullptr;
}

int ExperimentConfig::getSessionIndex(const String& id) const {
	debugAssert(sessions.size() >= 0 && sessions.size() < 100000);
	for (int i = 0; i < sessions.size(); ++i) {
		if (!sessions[i].id.compare(id)) {
			return i;
		}
	}
	throw format("Could not find session:\"%s\"", id);
}

shared_ptr<TargetConfig> ExperimentConfig::getTargetConfigById(const String& id) const {
	for (const TargetConfig& target : targets) {
		if (!target.id.compare(id)) {
			return TargetConfig::createShared<TargetConfig>(target);
		}
	}
	return nullptr;
}

Array<Array<shared_ptr<TargetConfig>>> ExperimentConfig::getTargetsByTrial(const String& id) const {
	return getTargetsByTrial(getSessionIndex(id));
}

Array<Array<shared_ptr<TargetConfig>>> ExperimentConfig::getTargetsByTrial(int sessionIndex) const {
	Array<Array<shared_ptr<TargetConfig>>> trials;
	// Iterate through the trials
	for (int i = 0; i < sessions[sessionIndex].trials.size(); i++) {
		Array<shared_ptr<TargetConfig>> targets;
		for (String id : sessions[sessionIndex].trials[i].ids) {
			const shared_ptr<TargetConfig> t = getTargetConfigById(id);
			targets.append(t);
		}
		trials.append(targets);
	}
	return trials;
}

Array<shared_ptr<TargetConfig>> ExperimentConfig::getSessionTargets(const String& id) const {
	const int idx = getSessionIndex(id);		// Get session index
	Array<shared_ptr<TargetConfig>> targets;
	Array<String> loggedIds;
	for (auto trial : sessions[idx].trials) {
		for (String& id : trial.ids) {
			if (!loggedIds.contains(id)) {
				loggedIds.append(id);
				targets.append(getTargetConfigById(id));
			}
		}
	}
	return targets;
}

bool ExperimentConfig::validate(bool throwException) const {
	bool valid = true;
	// Build list of valid target ids
	Array<String> validTargetIds;
	for (TargetConfig target : targets) { validTargetIds.append(target.id); }

	// Validate session targets against provided experiment target list
	for (SessionConfig session : sessions) {
		Array<String> sessionTargetIds;
		// Build a list of target ids used in this session
		for (TrialCount trial : session.trials) {
			for (String id : trial.ids) { if (!sessionTargetIds.contains(id)) sessionTargetIds.append(id); }
		}
		// Check each ID against the experiment targets array
		for (String targetId : sessionTargetIds) {
			if (!validTargetIds.contains(targetId)) {
				if (throwException) {
					throw format("Could not find target ID \"%s\" used in session \"%s\"!", targetId, session.id);
				}
				else {
					logPrintf("  Could not find target ID \"%s\" used in session \"%s\"!\n", targetId, session.id);
				}
				valid = false;
			}
		}
	}

	// Check that the serverAddress is valid
	
	return valid;
}

Any ExperimentConfig::toAny(const bool forceAll) const {
	// Get the base any config
	Any a = FpsConfig::toAny(forceAll);
	SessionConfig def;
	// Write the experiment configuration-specific 
	if (forceAll || def.description != description)			a["description"] = description;
	if (forceAll || def.closeOnComplete != closeOnComplete) a["closeOnComplete"] = closeOnComplete;
	a["targets"] = targets;
	a["sessions"] = sessions;
	return a;
}

void ExperimentConfig::printToLog() const{
	logPrintf("\n-------------------\nExperiment Config\n-------------------\nappendingDescription = %s\nscene name = %s\nTrial Feedback Duration = %f\nPretrial Duration = %f\nMax Trial Task Duration = %f\nMax Clicks = %d\nServer Address = %s\nServer Port = %d\n",
		description.c_str(), scene.name.c_str(), timing.trialFeedbackDuration, timing.pretrialDuration, timing.maxTrialDuration, weapon.maxAmmo, serverAddress.c_str(), serverPort);
	// Iterate through sessions and print them
	for (int i = 0; i < sessions.size(); i++) {
		SessionConfig sess = sessions[i];
		logPrintf("\t-------------------\n\tSession Config\n\t-------------------\n\tID = %s\n\tFrame Rate = %f\n\tFrame Delay = %d\n",
			sess.id.c_str(), sess.render.frameRate, sess.render.frameDelay);
		// Now iterate through each run
		for (int j = 0; j < sess.trials.size(); j++) {
			String ids;
			for (String id : sess.trials[j].ids) { ids += format("%s, ", id.c_str()); }
			if (ids.length() > 2) ids = ids.substr(0, ids.length() - 2);
			logPrintf("\t\tTrial Run Config: IDs = [%s], Count = %d\n",
				ids.c_str(), sess.trials[j].count);
		}
	}
	// Iterate through trials and print them
	for (int i = 0; i < targets.size(); i++) {
		TargetConfig target = targets[i];
		logPrintf("\t-------------------\n\tTarget Config\n\t-------------------\n\tID = %s\n\tMotion Change Period = [%f-%f]\n\tMin Speed = %f\n\tMax Speed = %f\n\tVisual Size = [%f-%f]\n\tUpper Hemisphere Only = %s\n\tJump Enabled = %s\n\tJump Period = [%f-%f]\n\tjumpSpeed = [%f-%f]\n\tAccel Gravity = [%f-%f]\n\tAxis Lock = [%s, %s, %s]\n",
			target.id.c_str(), target.motionChangePeriod[0], target.motionChangePeriod[1], target.speed[0], target.speed[1], target.size[0], target.size[1], target.upperHemisphereOnly ? "True" : "False", target.jumpEnabled ? "True" : "False", target.jumpPeriod[0], target.jumpPeriod[1], target.jumpSpeed[0], target.jumpSpeed[1], target.accelGravity[0], target.accelGravity[1],
			target.axisLock[0] ? "true" : "false", target.axisLock[1] ? "true" : "false", target.axisLock[2] ? "true" : "false");
	}
}