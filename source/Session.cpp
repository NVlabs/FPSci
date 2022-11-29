/***************************************************************************
# Copyright (c) 2015, NVIDIA CORPORATION. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of NVIDIA CORPORATION nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************/
#include "Session.h"
#include "FPSciApp.h"
#include "Logger.h"
#include "TargetEntity.h"
#include "PlayerEntity.h"
#include "Dialogs.h"
#include "Weapon.h"
#include "FPSciAnyTableReader.h"

TrialConfig::TrialConfig(const Any& any) : FpsConfig(any, defaultConfig()) {
	int settingsVersion = 1;
	FPSciAnyTableReader reader(any);
	reader.getIfPresent("settingsVersion", settingsVersion);

	switch (settingsVersion) {
	case 1:
		reader.getIfPresent("id", id);
		reader.get("ids", ids, "An \"ids\" field must be provided for each set of trials!");
		if (!reader.getIfPresent("count", count)) {
			count = defaultCount;
		}
		if (count < 1) {
			throw format("Trial count < 1 not allowed! (%d count for trial with targets: %s)", count, Any(ids).unparse());
		}
		break;
	default:
		debugPrintf("Settings version '%d' not recognized in SessionConfig.\n", settingsVersion);
		break;
	}
}

Any TrialConfig::toAny(const bool forceAll) const {
	Any a = FpsConfig::toAny(forceAll);
	a["id"] = id;
	a["ids"] = ids;
	a["count"] = count;
	return a;
}

TaskConfig::TaskConfig(const Any& any) {
	FPSciAnyTableReader reader(any);
	reader.get("id", id, "Tasks must be provided w/ an \"id\" field!");
	reader.get("trialOrders", trialOrders, "Tasks must be provided w/ trial orders!");
	reader.getIfPresent("count", count);
	reader.getIfPresent("questions", questions);
}

Any TaskConfig::toAny(const bool forceAll) const {
	TaskConfig def;
	Any a(Any::TABLE);
	a["id"] = id;
	a["trialOrders"] = trialOrders;
	if (forceAll || def.count != count) a["count"] = count;
	if (forceAll || questions.length() > 0) a["questions"] = questions;
	return a;
}

SessionConfig::SessionConfig(const Any& any) : FpsConfig(any, defaultConfig()) {
	TrialConfig::defaultCount = timing.defaultTrialCount;
	FPSciAnyTableReader reader(any);
	Set<String> uniqueIds;

	switch (settingsVersion) {
	case 1:
		TrialConfig::defaultConfig() = (FpsConfig)(*this);		// Setup the default configuration for trials here
		// Unique session info
		reader.get("id", id, "An \"id\" field must be provided for each session!");
		reader.getIfPresent("description", description);
		reader.getIfPresent("closeOnComplete", closeOnComplete);
		reader.getIfPresent("randomizeTrialOrder", randomizeTrialOrder);
		reader.getIfPresent("randomizeTaskOrder", randomizeTaskOrder);
		reader.getIfPresent("blockCount", blockCount);
		reader.get("trials", trials, format("Issues in the (required) \"trials\" array for session: \"%s\"", id));
		for (int i = 0; i < trials.length(); i++) {
			if (trials[i].id.empty()) {						// Look for trials without an id
				trials[i].id = String(std::to_string(i));	// Autoname w/ index
			}
			uniqueIds.insert(trials[i].id);
			if (uniqueIds.size() != i + 1) {
				logPrintf("ERROR: Duplicate trial ID \"%s\" found (trials without IDs are assigned an ID equal to their index in the trials array)!\n", trials[i].id);
			}
		}
		if (uniqueIds.size() != trials.size()) {
			throw "Duplicate trial IDs found in experiment config. Check log.txt for details!";
		}
		reader.getIfPresent("tasks", tasks);
		break;
	default:
		debugPrintf("Settings version '%d' not recognized in SessionConfig.\n", settingsVersion);
		break;
	}
}

Any SessionConfig::toAny(const bool forceAll) const {
	// Get the base any config
	Any a = FpsConfig::toAny(forceAll);
	SessionConfig def;

	// Update w/ the session-specific fields
	a["id"] = id;
	a["description"] = description;
	if (forceAll || def.closeOnComplete != closeOnComplete)	a["closeOnComplete"] = closeOnComplete;
	if (forceAll || def.randomizeTrialOrder != randomizeTrialOrder) a["randomizeTrialOrder"] = randomizeTrialOrder;
	if (forceAll || def.randomizeTaskOrder != randomizeTaskOrder) a["randomizeTaskOrder"] = randomizeTaskOrder;
	if (forceAll || def.blockCount != blockCount)				a["blockCount"] = blockCount;
	a["trials"] = trials;
	if (forceAll || tasks.length() > 0) a["tasks"] = tasks;
	return a;
}

float SessionConfig::getTrialOrdersPerBlock(void) const {
	float count = 0.f;
	if (tasks.length() == 0) {
		for (const TrialConfig& tc : trials) {
			if (tc.count < 0) { return finf(); }
			else { count += tc.count; }
		}
	}
	else {
		for (const TaskConfig& tc : tasks) {
			if (tc.count < 0) { return finf(); }
			else {
				// Total trials are count * length of trial orders
				count += tc.count * tc.trialOrders.length();
			}
		}
	}
	return count;
}

int SessionConfig::getTrialIndex(const String& id) const {
	for (int i = 0; i < trials.length(); i++) {
		if (trials[i].id == id) return i;			// Return the index of the trial
	}
	return -1;		// Return invalid index if not found
}

Array<String> SessionConfig::getUniqueTargetIds() const {
	Array<String> ids;
	for (TrialConfig trial : trials) {
		for (String id : trial.ids) {
			if (!ids.contains(id)) { ids.append(id); }
		}
	}
	return ids;
}

Session::Session(FPSciApp* app, shared_ptr<SessionConfig> config) : m_app(app), m_sessConfig(config), m_weapon(app->weapon) {
	m_hasSession = notNull(m_sessConfig);
}

Session::Session(FPSciApp* app) : m_app(app), m_weapon(app->weapon) {
	m_hasSession = false;
}

const RealTime Session::targetFrameTime()
{
	const RealTime defaultFrameTime = 1.0 / m_app->window()->settings().refreshRate;
	if (!m_hasSession) return defaultFrameTime;

	uint arraySize = m_trialConfig->render.frameTimeArray.size();
	if (arraySize > 0) {
		if ((m_trialConfig->render.frameTimeMode == "taskonly" || m_trialConfig->render.frameTimeMode == "restartwithtask") && currentState != PresentationState::trialTask) {
			// We are in a frame time mode which specifies only to change frame time during the task
			return 1.0f / m_trialConfig->render.frameRate;
		}

		if (m_trialConfig->render.frameTimeRandomize) {
			return m_trialConfig->render.frameTimeArray.randomElement();
		}
		else {
			RealTime targetTime = m_trialConfig->render.frameTimeArray[m_frameTimeIdx % arraySize];
			m_frameTimeIdx += 1;
			m_frameTimeIdx %= arraySize;
			return targetTime;
		}
	}

	// The below matches the functionality in FPSciApp::updateParameters()
	if (m_trialConfig->render.frameRate > 0) {
		return 1.0f / m_trialConfig->render.frameRate;
	}
	return defaultFrameTime;
}

bool Session::nextTrial() {
	// Do we need to create a new task?
	if(m_taskTrials.length() == 0) {
		// Build an array of unrun tasks in this block
		Array<Array<int>> unrunTaskIdxs;
		for (int i = 0; i < m_remainingTasks.size(); i++) {
			for (int j = 0; j < m_remainingTasks[i].size(); j++) {
				if (m_remainingTasks[i][j] > 0 || m_remainingTasks[i][j] == -1) {
					unrunTaskIdxs.append(Array<int>(i,j));
				}
			}
		}
		if (unrunTaskIdxs.size() == 0) return false;		// If there are no remaining tasks return

		// Pick the new task and trial order index
		int idx = 0;
		// Are we randomizing task order (or randomizing trial order when trials are treated as tasks)?
		if (m_sessConfig->randomizeTaskOrder || (m_sessConfig->tasks.size() == 0 && m_sessConfig->randomizeTrialOrder)) {				
			idx = Random::common().integer(0, unrunTaskIdxs.size() - 1);		// Pick a random trial from within the array
		}
		m_currTaskIdx = unrunTaskIdxs[idx][0];
		m_currOrderIdx = unrunTaskIdxs[idx][1];
		
		m_completedTaskTrials.clear();
		// Populate the task trial and completed task trials array
		Array<String> trialIds;
		String taskId;
		if (m_sessConfig->tasks.size() == 0) {
			// There are no tasks in this session, we need to create this task based on a single trial
			trialIds = Array<String>(m_sessConfig->trials[m_currTaskIdx].id);
			taskId = m_sessConfig->trials[m_currTaskIdx].id;
		}
		else {
			trialIds = m_sessConfig->tasks[m_currTaskIdx].trialOrders[m_currOrderIdx].order;
			taskId = m_sessConfig->tasks[m_currTaskIdx].id;
		}
		for (const String& trialId : trialIds) {
			m_taskTrials.insert(0, m_sessConfig->getTrialIndex(trialId));	// Insert trial at the front of the task trials
			m_completedTaskTrials.set(trialId, 0);
		}

		// If we are using tasks and we are randomizing trial order, shuffle the trials array
		if(m_sessConfig->tasks.length() > 0 && m_sessConfig->randomizeTrialOrder){
			m_taskTrials.randomize();
		}

		// Add task to tasks table in database
		logger->addTask(m_sessConfig->id, m_currBlock-1, taskId, getTaskCount(m_currTaskIdx), trialIds);
	}

	// Select the next trial from this task (pop trial from back of task trails)
	m_currTrialIdx = m_taskTrials.pop();

	// Get and update the trial configuration
	m_trialConfig = TrialConfig::createShared<TrialConfig>(m_sessConfig->trials[m_currTrialIdx]);
	m_app->updateTrial(m_trialConfig);

	// Produce (potentially random in range) pretrial duration
	if (isNaN(m_trialConfig->timing.pretrialDurationLambda)) m_pretrialDuration = m_trialConfig->timing.pretrialDuration;
	else m_pretrialDuration = drawTruncatedExp(m_trialConfig->timing.pretrialDurationLambda, m_trialConfig->timing.pretrialDurationRange[0], m_trialConfig->timing.pretrialDurationRange[1]);
	
	return true;	// Successfully loaded the new trial
}

int Session::getTaskCount(const int currTaskIdx) const {
	int idx = 0;
	for (int trialOrderCount : m_completedTasks[currTaskIdx]) {
		idx += trialOrderCount;
	}
	return idx;
}

bool Session::blockComplete() const {
	for (Array<int> trialOrders : m_remainingTasks) {
		for (int remaining : trialOrders) {
			if (remaining != 0) return false;		// If any trials still need to be run block isn't complete
		}
	}
	return true;		// Block is complete
}

bool Session::nextBlock(bool init) {
	if (m_sessConfig->tasks.size() == 0) {									// If there are no tasks make each trial its own task
		for (int i = 0; i < m_targetConfigs.size(); i++) {					// There is 1 trial per task, and 1 set of targets specified per trial in the target configs array
			if (init) { // If this is the first block in the session
				m_completedTasks.append(Array<int>(0));
				m_remainingTasks.append(Array<int>(m_sessConfig->trials[i].count));
			}
			else { // Update for a new block in the session
				m_completedTasks[i][0] = 0;
				m_remainingTasks[i][0] += m_trialConfig->count;		// Add another set of trials of this type
			}
		}
	}
	else {
		for (int i = 0; i < m_sessConfig->tasks.size(); i++) {
			if (init) {
				m_completedTasks.append(Array<int>());		// Initialize task-level completed trial orders array
				m_remainingTasks.append(Array<int>());		// Initialize task-level remaining trial orders array
			}
			for (int j = 0; j < m_sessConfig->tasks[i].trialOrders.size(); j++) {
				if (init) {
					m_completedTasks[i].append(0);	// Zero completed count
					m_remainingTasks[i].append(m_sessConfig->tasks[i].count); // Append to complete count
				}
				else {
					m_completedTasks[i][j] = 0;
					m_remainingTasks[i][j] += m_sessConfig->tasks[i].count;
				}
			}
		}
	}
	return nextTrial();
}

void Session::onInit(String filename, String description) {
	// Initialize presentation states
	currentState = PresentationState::initial;
	if (m_sessConfig) {
		m_feedbackMessage = formatFeedback(m_sessConfig->targetView.showRefTarget ? m_sessConfig->feedback.initialWithRef: m_sessConfig->feedback.initialNoRef);
	}

	// Get the player from the app
	m_player = m_app->scene()->typedEntity<PlayerEntity>("player");
	m_scene = m_app->scene().get();
	m_camera = m_app->activeCamera();

	m_targetModels = &(m_app->targetModels);

	// Check for valid session
	if (m_hasSession) {
		if (m_sessConfig->logger.enable) {
			UserConfig user = *m_app->currentUser();
			// Setup the logger and create results file
			logger = FPSciLogger::create(filename + ".db", user.id, 
				m_app->startupConfig.experimentList[m_app->experimentIdx].experimentConfigFilename, 
				m_sessConfig, description);
			logger->logTargetTypes(m_app->experimentConfig.getSessionTargets(m_sessConfig->id));			// Log target info at start of session
			logger->logUserConfig(user, m_sessConfig->id, m_sessConfig->player.turnScale);					// Log user info at start of session
			m_dbFilename = filename;
		}

		runSessionCommands("start");				// Run start of session commands

		// Iterate over the sessions here and add a config for each
		m_targetConfigs = m_app->experimentConfig.getTargetsByTrial(m_sessConfig->id);
		nextBlock(true);
	}
	else {	// Invalid session, move to displaying message
		currentState = PresentationState::sessionFeedback;
	}
}

void Session::randomizePosition(const shared_ptr<TargetEntity>& target) const {
	static const Point3 initialSpawnPos = m_camera->frame().translation;
	const int trialIdx = m_sessConfig->getTrialIndex(m_trialConfig->id);
	shared_ptr<TargetConfig> config = m_targetConfigs[trialIdx][target->paramIdx()];
	const bool isWorldSpace = config->destSpace == "world";
	Point3 loc;

	if (isWorldSpace) {
		loc = config->spawnBounds.randomInteriorPoint();		// Set a random position in the bounds
		target->resetMotionParams();							// Reset the target motion behavior
	}
	else {
		const float rot_pitch = (config->symmetricEccV ? randSign() : 1) * Random::common().uniform(config->eccV[0], config->eccV[1]);
		const float rot_yaw = (config->symmetricEccH ? randSign() : 1) * Random::common().uniform(config->eccH[0], config->eccH[1]);
		const CFrame f = CFrame::fromXYZYPRDegrees(initialSpawnPos.x, initialSpawnPos.y, initialSpawnPos.z, - 180.0f/(float)pi()*initialHeadingRadians - rot_yaw, rot_pitch, 0.0f);
		loc = f.pointToWorldSpace(Point3(0, 0, -m_targetDistance));
	}
	target->setFrame(loc);
}

void Session::initTargetAnimation(const bool task) {
	// initialize target location based on the initial displacement values
	// Not reference: we don't want it to change after the first call.
	const Point3 initialSpawnPos = m_player->getCameraFrame().translation;

	// In task state, spawn a test target. Otherwise spawn a target at straight ahead.
	if (task) {
		if (m_trialConfig->targetView.previewWithRef && m_trialConfig->targetView.showRefTarget) {
			// Activate the preview targets
			for (shared_ptr<TargetEntity> target : m_targetArray) {
				target->setCanHit(true);
				m_app->updateTargetColor(target);
				m_hittableTargets.append(target);
			}
		}
		else {
			spawnTrialTargets(initialSpawnPos);			// Spawn all the targets normally
			m_weapon->drawsDecals = true;				// Enable drawing decals
		}
	}
	else { // State is feedback and we are spawning a reference target
		CFrame f = CFrame::fromXYZYPRRadians(initialSpawnPos.x, initialSpawnPos.y, initialSpawnPos.z, -initialHeadingRadians, 0.0f, 0.0f);
		// Spawn the reference target
		auto t = spawnReferenceTarget(
			f.pointToWorldSpace(Point3(0, 0, -m_targetDistance)),
			initialSpawnPos,
			m_trialConfig->targetView.refTargetSize,
			m_trialConfig->targetView.refTargetColor
		);
		m_hittableTargets.append(t);
		m_lastRefTargetPos = t->frame().translation;		// Save last spawned reference target position

		if (m_trialConfig->targetView.previewWithRef) {
			spawnTrialTargets(initialSpawnPos, true);		// Spawn all the targets in preview mode
		}

		// Set weapon decal state to match configuration for reference targets
		m_weapon->drawsDecals = m_trialConfig->targetView.showRefDecals;
	
		// Clear target logOnChange management
		m_lastLogTargetLoc.clear();
	}

	// Reset number of destroyed targets (in the trial)
	m_destroyedTargets = 0;
	// Reset shot and hit counters (in the trial)
	m_weapon->reload();
	m_trialShotsHit = 0;
}

void Session::spawnTrialTargets(Point3 initialSpawnPos, bool previewMode) {
	// Iterate through the targets
	for (int i = 0; i < m_targetConfigs[m_currTrialIdx].size(); i++) {
		const Color3 previewColor = m_trialConfig->targetView.previewColor;
		shared_ptr<TargetConfig> target = m_targetConfigs[m_currTrialIdx][i];
		const String name = format("%s_%d_%d_%d_%s_%d", m_sessConfig->id, m_currTaskIdx, m_currOrderIdx, m_completedTasks[m_currTaskIdx][m_currOrderIdx], target->id, i);

		const float spawn_eccV = (target->symmetricEccV ? randSign() : 1) * Random::common().uniform(target->eccV[0], target->eccV[1]);
		const float spawn_eccH = (target->symmetricEccH ? randSign() : 1) * Random::common().uniform(target->eccH[0], target->eccH[1]);
		const float targetSize = G3D::Random().common().uniform(target->size[0], target->size[1]);
		bool isWorldSpace = target->destSpace == "world";

		// Log the target if desired
		if (m_sessConfig->logger.enable) {
			const String spawnTime = FPSciLogger::genUniqueTimestamp();
			logger->addTarget(name, target, spawnTime, targetSize, Point2(spawn_eccH, spawn_eccV));
		}

		CFrame f = CFrame::fromXYZYPRDegrees(initialSpawnPos.x, initialSpawnPos.y, initialSpawnPos.z, -initialHeadingRadians * 180.0f / pif() - spawn_eccH, spawn_eccV, 0.0f);

		// Check for case w/ destination array
		shared_ptr<TargetEntity> t;
		if (target->destinations.size() > 0) {
			Point3 offset = isWorldSpace ? Point3(0.f, 0.f, 0.f) : f.pointToWorldSpace(Point3(0, 0, -m_targetDistance));
			t = spawnDestTarget(target, offset, previewColor, i, name);
		}
		// Otherwise check if this is a jumping target
		else if (target->jumpEnabled) {
			Point3 offset = isWorldSpace ? target->spawnBounds.randomInteriorPoint() : f.pointToWorldSpace(Point3(0, 0, -m_targetDistance));
			t = spawnJumpingTarget(target, offset, initialSpawnPos, previewColor, m_targetDistance, i, name);
		}
		else {
			Point3 offset = isWorldSpace ? target->spawnBounds.randomInteriorPoint() : f.pointToWorldSpace(Point3(0, 0, -m_targetDistance));
			t = spawnFlyingTarget(target, offset, initialSpawnPos, previewColor, i, name);
		}

		if (!previewMode) m_app->updateTargetColor(t);		// If this isn't a preview target update its color now

		// Set whether the target can be hit based on whether we are in preview mode
		t->setCanHit(!previewMode);
		previewMode ? m_unhittableTargets.append(t) : m_hittableTargets.append(t);
	}
}

void Session::processResponse() {
	m_taskExecutionTime = m_timer.getTime();							// Get time to copmplete the task

	const int totalTargets = totalTrialTargets();
	recordTrialResponse(m_destroyedTargets, totalTargets);				// Record the trial response into the database

	// Update completed/remaining task state
	if (m_taskTrials.size() == 0) {										// Task is complete update tracking
		m_completedTasks[m_currTaskIdx][m_currOrderIdx] += 1;			// Mark task trial order as completed
		if (m_remainingTasks[m_currTaskIdx][m_currOrderIdx] > 0) {		// Remove task trial order from remaining
			m_remainingTasks[m_currTaskIdx][m_currOrderIdx] -= 1;
		}
	}
	m_completedTaskTrials[m_trialConfig->id] += 1;						// Incrememnt count of this trial type in task

	// This update is only used for completed trials
	if (notNull(logger)) {
		// Get completed task and trial count
		int completeTrials = 0;
		int completeTasks = 0;
		for (int i = 0; i < m_completedTasks.length(); i++) {
			for (int count : m_completedTasks[i]) { 
				completeTrials += count;	
			}
			bool taskComplete = true;
			for (int remaining : m_remainingTasks[i]) {
				if (remaining > 0) {
					taskComplete = false;
					break;
				}
			}
			if (taskComplete) { completeTasks += 1; }
		}

		// Update session entry in database
		logger->updateSessionEntry(m_currBlock > m_sessConfig->blockCount, completeTasks, completeTrials);			
	}

	// Check for whether all targets have been destroyed
	if (m_destroyedTargets == totalTargets) {
		m_totalRemainingTime += (double(m_trialConfig->timing.maxTrialDuration) - m_taskExecutionTime);
		m_feedbackMessage = formatFeedback(m_trialConfig->feedback.trialSuccess);
		m_totalTrialSuccesses += 1;
	}
	else {
		m_feedbackMessage = formatFeedback(m_trialConfig->feedback.trialFailure);
	}
}

void Session::updatePresentationState() {
	// This updates presentation state and also deals with data collection when each trial ends.
	PresentationState newState;
	int remainingTargets = m_hittableTargets.size();
	float stateElapsedTime = m_timer.getTime();
	newState = currentState;

	if (currentState == PresentationState::initial)
	{
		if (m_trialConfig->player.stillBetweenTrials) {
			m_player->setMoveEnable(false);
		}
		if (!(m_app->shootButtonUp && m_trialConfig->timing.clickToStart)) {
			newState = PresentationState::referenceTarget;
		}
	}
	else if (currentState == PresentationState::referenceTarget) {
		// State for showing the trial reference target
		if (remainingTargets == 0) {
			newState = PresentationState::pretrial;
		}

	}
	else if (currentState == PresentationState::pretrial)
	{
		if (stateElapsedTime > m_pretrialDuration)
		{
			newState = PresentationState::trialTask;
			if (m_trialConfig->player.stillBetweenTrials) {
				m_player->setMoveEnable(true);
			}

			closeTrialProcesses();						// End previous process (if running)
			runTrialCommands("start");					// Run start of trial commands

		}
	}
	else if (currentState == PresentationState::trialTask)
	{
		if ((stateElapsedTime > m_trialConfig->timing.maxTrialDuration) || (remainingTargets <= 0) || (m_weapon->remainingAmmo() == 0))
		{
			m_taskEndTime = FPSciLogger::genUniqueTimestamp();
			processResponse();
			clearTargets(); // clear all remaining targets
			newState = PresentationState::trialFeedback;
			if (m_trialConfig->player.stillBetweenTrials) {
				m_player->setMoveEnable(false);
			}
			if (m_trialConfig->player.resetPositionPerTrial) {
				m_player->respawn();
			}

			closeTrialProcesses();				// Stop start of trial processes
			runTrialCommands("end");			// Run the end of trial processes

			// Reset weapon cooldown
			m_weapon->resetCooldown();
			if (m_weapon->config()->clearTrialMissDecals) {		// Clear weapon's decals if specified
				m_weapon->clearDecals(false);
			}
		}
	}
	else if (currentState == PresentationState::trialFeedback)
	{
		if (stateElapsedTime > m_trialConfig->timing.trialFeedbackDuration)
		{
			bool allAnswered = presentQuestions(m_trialConfig->questionArray);	// Present any trial-level questions
			if (allAnswered) { 
				m_currQuestionIdx = -1;		// Reset the question index
				m_feedbackMessage = "";		// Clear the feedback message
				if (m_taskTrials.length() == 0) newState = PresentationState::taskFeedback;	// Move forward to providing task-level feedback
				else {		// Individual trial complete, go back to reference target
					logger->updateTaskEntry(m_sessConfig->tasks[m_currTaskIdx].trialOrders[m_currOrderIdx].order.size() - m_taskTrials.size(), false);
					nextTrial();
					newState = PresentationState::referenceTarget;
				}
			}
		}
	}
	else if (currentState == PresentationState::taskFeedback) {
		bool allAnswered = true;
		if (m_sessConfig->tasks.size() > 0) {
			// Only ask questions if a task is specified (otherwise trial-level questions have already been presented)
			allAnswered = presentQuestions(m_sessConfig->tasks[m_currTaskIdx].questions);
		}
		if (allAnswered) {
			m_currQuestionIdx = -1;		// Reset the question index
			int completeTrials = 1;		// Assume 1 completed trial (if we don't have specified tasks)
			if (m_sessConfig->tasks.size() > 0) completeTrials = m_sessConfig->tasks[m_currTaskIdx].trialOrders[m_currOrderIdx].order.size() - m_taskTrials.size();
			logger->updateTaskEntry(completeTrials, true);
			if (blockComplete()) {
				m_currBlock++;
				if (m_currBlock > m_sessConfig->blockCount) {	// End of session (all blocks complete)
					newState = PresentationState::sessionFeedback;
				}
				else {	// Block is complete but session isn't
					m_feedbackMessage = formatFeedback(m_sessConfig->feedback.blockComplete);
					nextBlock();
					newState = PresentationState::initial;
				}
			}
			else {	// Individual trial complete, go back to reference target
				m_feedbackMessage = "";	// Clear the feedback message
				nextTrial();
				newState = PresentationState::referenceTarget;
			}
		}
	}
	else if (currentState == PresentationState::sessionFeedback) {
		if (m_hasSession) {
			if (stateElapsedTime > m_sessConfig->timing.sessionFeedbackDuration && (!m_sessConfig->timing.sessionFeedbackRequireClick || !m_app->shootButtonUp)) {
				bool allAnswered = presentQuestions(m_sessConfig->questionArray);	// Ask session-level questions
				if (allAnswered) {			// Present questions until done here
					// Write final session timestamp to log
					if (notNull(logger) && m_sessConfig->logger.enable) {
						int completeTrials = 0;
						int completeTasks = 0;
						for (int i = 0; i < m_completedTasks.length(); i++) {
							for (int count : m_completedTasks[i]) {
								completeTrials += count;
							}
							bool taskComplete = true;
							for (int remaining : m_remainingTasks[i]) {
								if (remaining > 0) {
									taskComplete = false;
									break;
								}
							}
							if (taskComplete) { completeTasks += 1; }
						}
						logger->updateSessionEntry(m_currBlock > m_sessConfig->blockCount, completeTasks, completeTrials);			// Update session entry in database
					}
					if (m_sessConfig->logger.enable) {
						endLogging();
					}
					m_app->markSessComplete(m_sessConfig->id);														// Add this session to user's completed sessions

					m_feedbackMessage = formatFeedback(m_sessConfig->feedback.sessComplete);						// Update the feedback message
					m_currQuestionIdx = -1;

					newState = PresentationState::complete;

					// Save current user config and status
					m_app->saveUserConfig(true);

					closeSessionProcesses();					// Close the process we started at session start (if there is one)
					runSessionCommands("end");					// Launch processes for the end of the session

					Array<String> remaining = m_app->updateSessionDropDown();
					if (remaining.size() == 0) {
						m_feedbackMessage = formatFeedback(m_sessConfig->feedback.allSessComplete); // Update the feedback message
						moveOn = false;
						if (m_app->experimentConfig.closeOnComplete || m_sessConfig->closeOnComplete) {
							m_app->quitRequest();
						}
					}
					else {
						m_feedbackMessage = formatFeedback(m_sessConfig->feedback.sessComplete);	// Update the feedback message
						if (m_sessConfig->closeOnComplete) {
							m_app->quitRequest();
						}
						moveOn = true;							// Check for session complete (signal start of next session)
					}

					if (m_app->experimentConfig.closeOnComplete) {
						m_app->quitRequest();
					}
				}
				moveOn = true;									// Check for session complete (signal start of next session)
			}
		}
		else {
			// Go ahead and move to the complete state since there aren't any valid sessions
			newState = PresentationState::complete;
			m_feedbackMessage = formatFeedback(m_app->experimentConfig.feedback.allSessComplete);
			moveOn = false;
			if (m_app->experimentConfig.closeOnComplete) {		// This is the case that is used for experiment config closeOnComplete!
				m_app->quitRequest();
			}
		}
	}
	else {
		newState = currentState;
	}

	if (currentState != newState)
	{ // handle state transition.
		m_timer.startTimer();
		if (newState == PresentationState::referenceTarget) {
			initTargetAnimation(false);	// Spawn the reference (and also preview if requested) target(s)
		}
		else if (newState == PresentationState::pretrial) {
			// Clear weapon miss decals (if requested)
			if (m_trialConfig->targetView.clearDecalsWithRef) {
				m_weapon->clearDecals();
			}
		}
		else if (newState == PresentationState::trialTask) {
			if (m_sessConfig->render.frameTimeMode == "restartwithtask") {
				m_frameTimeIdx = 0;		// Reset the frame time index with the task if requested
			}
			// Test for aiming in valid region before spawning task targets				
			if (m_trialConfig->timing.maxPretrialAimDisplacement >= 0) {
				Vector3 aim = m_camera->frame().lookVector().unit();
				Vector3 ref = (m_lastRefTargetPos - m_camera->frame().translation).unit();
				// Get the view displacement as the arccos of view/reference direction dot product (should never exceed 180 deg)
				float viewDisplacement = 180 / pif() * acosf(aim.dot(ref));
				if (viewDisplacement > m_trialConfig->timing.maxPretrialAimDisplacement) {
					clearTargets();		// Clear targets (in case preview targets are being shown)
					m_feedbackMessage = formatFeedback(m_trialConfig->feedback.aimInvalid);
					newState = PresentationState::trialFeedback;		// Jump to feedback state w/ error message
				}
			}
			m_taskStartTime = FPSciLogger::genUniqueTimestamp();
			initTargetAnimation(true);		// Spawn task targets (or convert from previews)
		}
		currentState = newState;
	}
}

void Session::onSimulation(RealTime rdt, SimTime sdt, SimTime idt) {
	// 1. Update presentation state and send task performance to psychophysics library.
	updatePresentationState();

	// 2. Record target trajectories, view direction trajectories, and mouse motion.
	accumulateTrajectories();
	accumulateFrameInfo(rdt, sdt, idt);
}

bool Session::presentQuestions(Array<Question>& questions) {
	if (questions.size() > 0 && m_currQuestionIdx < questions.size()) {
		// Initialize if needed
		if (m_currQuestionIdx == -1) {
			m_currQuestionIdx = 0;
			m_app->presentQuestion(questions[m_currQuestionIdx]);
		}
		// Manage answered quesions
		else if (!m_app->dialog->visible()) {											// Check for whether dialog is closed (otherwise we are waiting for input)
			if (m_app->dialog->complete) {												// Has this dialog box been completed? (or was it closed without an answer?)
				questions[m_currQuestionIdx].result = m_app->dialog->result;			// Store response w/ question
				if (m_sessConfig->logger.enable) {										// Log the question and its answer
					if (currentState == PresentationState::trialFeedback) {
						// End of trial question, log trial id and index
						logger->addQuestion(questions[m_currQuestionIdx], m_sessConfig->id, m_app->dialog, m_sessConfig->tasks[m_currTaskIdx].id, m_lastTaskIndex, m_trialConfig->id, m_completedTaskTrials[m_trialConfig->id]-1);
					}
					else if (currentState == PresentationState::taskFeedback) {
						// End of task question, log task id (no trial info)
						logger->addQuestion(questions[m_currQuestionIdx], m_sessConfig->id, m_app->dialog, m_sessConfig->tasks[m_currTaskIdx].id, m_lastTaskIndex);
					}
					else {	// End of session question, don't need to log a task/trial id/index
						logger->addQuestion(questions[m_currQuestionIdx], m_sessConfig->id, m_app->dialog);
					}
				}
				m_currQuestionIdx++;													// Move to the next question
				if (m_currQuestionIdx < questions.size()) {								// Double check we have a next question before launching the next question
					m_app->presentQuestion(questions[m_currQuestionIdx]);				// Present the next question (if there is one)
				}
				else {	// All questions complete
					m_app->dialog.reset();												// Null the dialog pointer when all questions complete
					m_app->setMouseInputMode(FPSciApp::MouseInputMode::MOUSE_FPM);		// Go back to first-person mouse
					return true;
				}
			}
			else {	// Dialog closed w/o a response (re-present the question)
				m_app->presentQuestion(questions[m_currQuestionIdx]);					// Relaunch the same dialog (this wasn't completed)
			}
		}
		return false;
	}
	else return true;
}

void Session::recordTrialResponse(int destroyedTargets, int totalTargets) {
	if (!m_sessConfig->logger.enable) return;		// Skip this if the logger is disabled
	if (m_trialConfig->logger.logTrialResponse) {
		String taskId;
		if (m_sessConfig->tasks.size() == 0) taskId = m_trialConfig->id;
		else taskId = m_sessConfig->tasks[m_currTaskIdx].id;
		// Get the (unique) index for this run of the task
		m_lastTaskIndex = getTaskCount(m_currTaskIdx);
		// Trials table. Record trial start time, end time, and task completion time.
		FPSciLogger::TrialValues trialValues = {
			"'" + m_sessConfig->id + "'",
			format("'Block %d'", m_currBlock),
			"'" + taskId + "'",
			String(std::to_string(m_lastTaskIndex)),
			"'" + m_trialConfig->id + "'",
			String(std::to_string(m_completedTasks[m_currTaskIdx][m_currOrderIdx])),
			"'" + m_taskStartTime + "'",
			"'" + m_taskEndTime + "'",
			String(std::to_string(m_pretrialDuration)),
			String(std::to_string(m_taskExecutionTime)),
			String(std::to_string(destroyedTargets)),
			String(std::to_string(totalTargets))
		};
		logger->addTrialParamValues(trialValues, m_trialConfig);
		logger->logTrial(trialValues);
	}
}

void Session::accumulateTrajectories() {
	if (notNull(logger) && m_trialConfig->logger.logTargetTrajectories) {
		for (shared_ptr<TargetEntity> target : m_targetArray) {
			if (!target->isLogged()) continue;
			String name = target->name();
			Point3 pos = target->frame().translation;
			TargetLocation location = TargetLocation(FPSciLogger::getFileTime(), name, currentState, pos);
			if (m_trialConfig->logger.logOnChange) {
				// Check for target in logged position table
				if (m_lastLogTargetLoc.containsKey(name)  && location.noChangeFrom(m_lastLogTargetLoc[name])) {	
					continue; // Duplicates last logged position/state (don't log)
				}
			}
			//// below for 2D direction calculation (azimuth and elevation)
			//Point3 t = targetPosition.direction();
			//float az = atan2(-t.z, -t.x) * 180 / pif();
			//float el = atan2(t.y, sqrtf(t.x * t.x + t.z * t.z)) * 180 / pif();
			logger->logTargetLocation(location);
			m_lastLogTargetLoc.set(name, location);					// Update the last logged location
		}
	}
	// recording view direction trajectories
	accumulatePlayerAction(PlayerActionType::Aim);
}

void Session::accumulatePlayerAction(PlayerActionType action, String targetName) {
	// Count hits (in task state) here
	if (currentState == PresentationState::trialTask) {
		if (action == PlayerActionType::Miss) {
			m_totalShots += 1;
			m_accuracy = (float) m_totalShotsHit / (float) m_totalShots * 100.f;
		}
		if ((action == PlayerActionType::Hit || action == PlayerActionType::Destroy)) {
			m_trialShotsHit++;
			// Update scoring parameters
			m_totalShotsHit++;
			m_totalShots += 1;
			m_accuracy = (float) m_totalShotsHit / (float) m_totalShots * 100.f;
			if (action == PlayerActionType::Destroy) m_totalTargetsDestroyed += 1;
		}
	}

	static PlayerAction lastPA;

	if (notNull(logger) && m_trialConfig->logger.logPlayerActions) {
		BEGIN_PROFILER_EVENT("accumulatePlayerAction");
		// recording target trajectories
		Point2 dir = getViewDirection();
		Point3 loc = getPlayerLocation();
		PlayerAction pa = PlayerAction(FPSciLogger::getFileTime(), dir, loc, currentState, action, targetName);
		// Check for log only on change condition
		if (m_trialConfig->logger.logOnChange && pa.noChangeFrom(lastPA)) {
			return;		// Early exit for (would be) duplicate log entry
		}
		logger->logPlayerAction(pa);
		lastPA = pa;				// Update last logged values
		END_PROFILER_EVENT();
	}
}

void Session::accumulateFrameInfo(RealTime t, float sdt, float idt) {
	if (notNull(logger) && m_trialConfig->logger.logFrameInfo) {
		logger->logFrameInfo(FrameInfo(FPSciLogger::getFileTime(), sdt));
	}
}

bool Session::inTask() {
	return currentState == PresentationState::trialTask;
}

float Session::getElapsedTrialTime() {
	return m_timer.getTime();
}

float Session::getRemainingTrialTime() {
	if (isNull(m_trialConfig)) return 10.0;
	return m_trialConfig->timing.maxTrialDuration - m_timer.getTime();
}

float Session::getProgress() {
	if (notNull(m_sessConfig)) {
		// Get progress across tasks
		float remainingTrialOrders = 0.f;
		for (Array<int> trialOrderCounts : m_remainingTasks) {
			for (int orderCount : trialOrderCounts) {
				if (orderCount < 0) return 0.f;				// Infinite trials, never make any progress
				remainingTrialOrders += (float)orderCount;
			}
		}

		// Get progress in current task
		int completedTrialsInOrder = 0;
		int totalTrialsInOrder = 1; // If there aren't tasks specified there is always 1 trial in this order (single order/trial task)
		if(m_sessConfig->tasks.size() > 0) totalTrialsInOrder = m_sessConfig->tasks[m_currTaskIdx].trialOrders[m_currOrderIdx].order.length();
		for (const String& trialId : m_completedTaskTrials.getKeys()) {
			completedTrialsInOrder += m_completedTaskTrials[trialId];
		}
		float currTaskProgress = (float) completedTrialsInOrder / (float) totalTrialsInOrder;
		
		// Start by getting task-level progress (based on m_remainingTrialOrders)
		float overallProgress = 1.f - (remainingTrialOrders / m_sessConfig->getTrialOrdersPerBlock());
		// Special case to avoid "double counting" completed tasks (if incomplete add progress in the current task, if complete it has been counted)
		if (currTaskProgress < 1) overallProgress += currTaskProgress / m_sessConfig->getTrialOrdersPerBlock();
		return overallProgress;
	}
	return fnan();
}

double Session::getScore() {
	if (isNull(m_trialConfig)) return 0;

	double score = 0;
	switch (m_trialConfig->feedback.scoreModel) {
	case FeedbackConfig::ScoreType::TimeRemaining:
		score = m_totalRemainingTime;
		break;
	case FeedbackConfig::ScoreType::TargetsDestroyed:
		score = m_totalTargetsDestroyed;
		break;
	case FeedbackConfig::ScoreType::ShotsHit:
		score = m_totalShotsHit;
		break;
	case FeedbackConfig::ScoreType::Accuracy:
		score = m_accuracy;
		break;
	case FeedbackConfig::ScoreType::TrialSuccesses:
		score = m_totalTrialSuccesses;
		break;
	default:
		break;
	}

	return m_trialConfig->feedback.scoreMultiplier * score;
}

String Session::formatCommand(const String& input) {
	const char delimiter = '%';			///< Start of all valid substrings
	String formatted = input;			///< Output string
	int foundIdx = 0;					///< Index for searching for substrings

	const String loggerComPort = "%loggerComPort";
	const String syncComPort = "%loggerSyncComPort";
	const String dbFilename = "%dbFilename";

	while ((foundIdx = (int)formatted.find(delimiter, (size_t)foundIdx)) > -1) {
		if (!formatted.compare(foundIdx, loggerComPort.length(), loggerComPort)) {
			if (m_app->systemConfig.loggerComPort.empty()) {
				throw "Found \"%loggerComPort\" substring in a command, but no \"loggerComPort\" is provided in the config!";
			}
			formatted = formatted.substr(0, foundIdx) + m_app->systemConfig.loggerComPort + formatted.substr(foundIdx + loggerComPort.length());
		}
		else if (!formatted.compare(foundIdx, syncComPort.length(), syncComPort)) {
			if (m_app->systemConfig.syncComPort.empty()) {
				throw "Found \"%loggerSyncComPort\" substring in a command, but no \"loggerSyncComPort\" is provided in the config!";
			}
			formatted = formatted.substr(0, foundIdx) + m_app->systemConfig.syncComPort + formatted.substr(foundIdx + syncComPort.length());
		}
		else if (!formatted.compare(foundIdx, dbFilename.length(), dbFilename)) {
			if (m_dbFilename.empty()) {
				throw "No database filename found to support the %dbFilename substring!";
			}
			formatted = formatted.substr(0, foundIdx) + m_dbFilename + formatted.substr(foundIdx + dbFilename.length());
		}
		else {
			foundIdx++;
		}
	}
	return formatted;
}

String Session::formatFeedback(const String& input) {
	const char delimiter = '%';			///< Start of all valid substrings
	String formatted = input;			///< Output string
	int foundIdx = 0;					///< Index for searching for substrings

	// Substrings for replacement
	const String totalTimeLeftS			= "%totalTimeLeftS";				///< Sum of time remaining over all completed trials
	const String lastBlock				= "%lastBlock";						///< The last (completed) block in this session
	const String currBlock				= "%currBlock";						///< The current block of the session (next block at end of session)
	const String totalBlocks			= "%totalBlocks";					///< The total blocks specified in the session
	const String trialTaskTimeMs		= "%trialTaskTimeMs";				///< The time spent in the task state of this trial (in ms)
	const String trialTargetsDestroyed	= "%trialTargetsDestroyed";			///< The number of targets destroyed in this trial
	const String trialTotalTargets		= "%trialTotalTargets";				///< The number of targets in this trial ("infinite" if any target respawns infinitely)
	const String trialShotsHit			= "%trialShotsHit";					///< The number of shots hit in this trial
	const String trialTotalShots		= "%trialTotalShots";				///< The number of shots taken in this trial
	const String sessionScore			= "%sessionScore";					///< The score for this session

	// Walk through the string looking for instances of the delimiter
	while ((foundIdx = (int)formatted.find(delimiter, (size_t)foundIdx)) > -1) {
		if(!formatted.compare(foundIdx, totalTimeLeftS.length(), totalTimeLeftS)){
			formatted = formatted.substr(0, foundIdx) + format("%.2f", m_totalRemainingTime) + formatted.substr(foundIdx + totalTimeLeftS.length());
		}
		else if (!formatted.compare(foundIdx, lastBlock.length(), lastBlock)) {
			formatted = formatted.substr(0, foundIdx) + format("%d", m_currBlock - 1) + formatted.substr(foundIdx + lastBlock.length());
		}
		else if (!formatted.compare(foundIdx, currBlock.length(), currBlock)) {
			formatted = formatted.substr(0, foundIdx) + format("%d", m_currBlock) + formatted.substr(foundIdx + currBlock.length());
		}
		else if (!formatted.compare(foundIdx, totalBlocks.length(), totalBlocks)) {
			formatted = formatted.substr(0, foundIdx) + format("%d", m_sessConfig->blockCount) + formatted.substr(foundIdx + totalBlocks.length());
		}
		else if (!formatted.compare(foundIdx, trialTaskTimeMs.length(), trialTaskTimeMs)) {
			formatted = formatted.substr(0, foundIdx) + format("%d", (int)(m_taskExecutionTime * 1000)) + formatted.substr(foundIdx + trialTaskTimeMs.length());
		}
		else if (!formatted.compare(foundIdx, trialTargetsDestroyed.length(), trialTargetsDestroyed)) {
			formatted = formatted.substr(0, foundIdx) + format("%d", m_destroyedTargets) + formatted.substr(foundIdx + trialTargetsDestroyed.length());
		}
		else if (!formatted.compare(foundIdx, trialTotalTargets.length(), trialTotalTargets)) {
			String totalTargetsString;
			int totalTargets = totalTrialTargets();
			if (totalTargets > 0) {
				// Finite target count
				totalTargetsString += format("%d", totalTargets);
			}
			else { 
				// Inifinite target count case
				totalTargetsString += "infinite";
			}
			formatted = formatted.substr(0, foundIdx) + totalTargetsString + formatted.substr(foundIdx + trialTotalTargets.length());
		}
		else if (!formatted.compare(foundIdx, trialShotsHit.length(), trialShotsHit)) {
			formatted = formatted.substr(0, foundIdx) + format("%d", m_trialShotsHit) + formatted.substr(foundIdx + trialShotsHit.length());
		}
		else if (!formatted.compare(foundIdx, trialTotalShots.length(), trialTotalShots)) {
			formatted = formatted.substr(0, foundIdx) + format("%d", m_weapon->shotsTaken()) + formatted.substr(foundIdx + trialTotalShots.length());
		}
		else if (!formatted.compare(foundIdx, sessionScore.length(), sessionScore)) {
			formatted = formatted.substr(0, foundIdx) + format("%d", (int)G3D::round(getScore())) + formatted.substr(foundIdx + sessionScore.length());
		}
		else {
			// Bump the found index past this character (not a valid substring)
			foundIdx++;
		}
	}
	return formatted;
}

String Session::getFeedbackMessage() {
	return m_feedbackMessage;
}

void Session::endLogging() {
	if (notNull(logger)) {

		//m_logger->logUserConfig(*m_app->currentUser(), m_config->id, m_config->player.turnScale);
		logger->flush(false);
		logger.reset();
	}
}

shared_ptr<TargetEntity> Session::spawnDestTarget(
	shared_ptr<TargetConfig> config,
	const Point3& offset,
	const Color3& color,
	const int paramIdx,
	const String& name)
{
	// Create the target
	const float targetSize = G3D::Random().common().uniform(config->size[0], config->size[1]);
	const String nameStr = name.empty() ? format("target%03d", ++m_lastUniqueID) : name;
	const int scaleIndex = clamp(iRound(log(targetSize) / log(1.0f + TARGET_MODEL_ARRAY_SCALING) + TARGET_MODEL_ARRAY_OFFSET), 0, TARGET_MODEL_SCALE_COUNT - 1);

	const shared_ptr<TargetEntity>& target = TargetEntity::create(config, nameStr, m_scene, (*m_targetModels)[config->id][scaleIndex], offset, scaleIndex, paramIdx);

	// Update parameters for the target
	target->setHitSound(config->hitSound, m_app->soundTable, config->hitSoundVol);
	target->setDestoyedSound(config->destroyedSound, m_app->soundTable, config->destroyedSoundVol);

	Color4 gloss = config->hasGloss ? config->gloss : m_app->experimentConfig.targetView.gloss;
	target->setColor(color, gloss);

	// Add target to array and scene
	insertTarget(target);
	return target;
}

shared_ptr<FlyingEntity> Session::spawnReferenceTarget(
	const Point3& position,
	const Point3& orbitCenter,
	const float size,
	const Color3& color)
{
	const int scaleIndex = clamp(iRound(log(size) / log(1.0f + TARGET_MODEL_ARRAY_SCALING) + TARGET_MODEL_ARRAY_OFFSET), 0, TARGET_MODEL_SCALE_COUNT - 1);

	String refId = m_sessConfig->id + "_reference";
	if (isNull(m_targetModels->getPointer(refId))) {
		// This session doesn't have a custom reference target
		refId = "reference";
	}

	const shared_ptr<FlyingEntity>& target = FlyingEntity::create("reference", m_scene, (*m_targetModels)[refId][scaleIndex], CFrame());

	// Setup additional target parameters
	target->setFrame(position);
	target->setColor(color, m_app->experimentConfig.targetView.gloss);

	// Add target to array and scene
	insertTarget(target);
	return target;
}

shared_ptr<FlyingEntity> Session::spawnFlyingTarget(
	shared_ptr<TargetConfig> config,
	const Point3& position,
	const Point3& orbitCenter,
	const Color3& color,
	const int paramIdx,
	const String& name)
{
	const float targetSize = G3D::Random().common().uniform(config->size[0], config->size[1]);
	const int scaleIndex = clamp(iRound(log(targetSize) / log(1.0f + TARGET_MODEL_ARRAY_SCALING) + TARGET_MODEL_ARRAY_OFFSET), 0, TARGET_MODEL_SCALE_COUNT - 1);
	const String nameStr = name.empty() ? format("target%03d", ++m_lastUniqueID) : name;
	const bool isWorldSpace = config->destSpace == "world";

	// Setup the target
	const shared_ptr<FlyingEntity>& target = FlyingEntity::create(config, nameStr, m_scene, (*m_targetModels)[config->id][scaleIndex], orbitCenter, scaleIndex, paramIdx);
	target->setFrame(position);
	target->setWorldSpace(isWorldSpace);
	if (isWorldSpace) {
		target->setBounds(config->moveBounds);
	}
	target->setHitSound(config->hitSound, m_app->soundTable,  config->hitSoundVol);
	target->setDestoyedSound(config->destroyedSound, m_app->soundTable, config->destroyedSoundVol);

	Color4 gloss = config->hasGloss ? config->gloss : m_app->experimentConfig.targetView.gloss;
	target->setColor(color, gloss);

	// Add the target to the scene/target array
	insertTarget(target);
	return target;
}

shared_ptr<JumpingEntity> Session::spawnJumpingTarget(
	shared_ptr<TargetConfig> config,
	const Point3& position,
	const Point3& orbitCenter,
	const Color3& color,
	const float targetDistance,
	const int paramIdx,
	const String& name)
{
	const float targetSize = G3D::Random().common().uniform(config->size[0], config->size[1]);
	const int scaleIndex = clamp(iRound(log(targetSize) / log(1.0f + TARGET_MODEL_ARRAY_SCALING) + TARGET_MODEL_ARRAY_OFFSET), 0, TARGET_MODEL_SCALE_COUNT - 1);
	const String nameStr = name.empty() ? format("target%03d", ++m_lastUniqueID) : name;
	const bool isWorldSpace = config->destSpace == "world";

	// Setup the target
	const shared_ptr<JumpingEntity>& target = JumpingEntity::create(config, nameStr, m_scene, (*m_targetModels)[config->id][scaleIndex], scaleIndex, orbitCenter, targetDistance, paramIdx);
	target->setFrame(position);
	target->setWorldSpace(isWorldSpace);
	if (isWorldSpace) {
		target->setMoveBounds(config->moveBounds);
	}
	target->setHitSound(config->hitSound, m_app->soundTable, config->hitSoundVol);
	target->setDestoyedSound(config->destroyedSound, m_app->soundTable, config->destroyedSoundVol);

	Color4 gloss = config->hasGloss ? config->gloss : m_app->experimentConfig.targetView.gloss;
	target->setColor(color, gloss);

	// Add the target to the scene/target array
	insertTarget(target);
	return target;
}

void Session::insertTarget(shared_ptr<TargetEntity> target) {
	target->setShouldBeSaved(false);
	m_targetArray.append(target);
	m_scene->insert(target);
}

void Session::destroyTarget(shared_ptr<TargetEntity> target) {
	// Remove target from the scene
	m_scene->removeEntity(target->name());
	// Remove target from master list
	for (int i = 0; i < m_targetArray.size(); i++) {
		if (m_targetArray[i]->name() == target->name()) { m_targetArray.fastRemove(i); }
	}
	// Remove target from (un)hittable array
	for (int i = 0; i < m_hittableTargets.size(); i++) {
		if (m_hittableTargets[i]->name() == target->name()) { 
			m_hittableTargets.fastRemove(i); 
			return;	// Target can't be both hittable and unhittable
		}
	}
	for (int i = 0; i < m_unhittableTargets.size(); i++) {
		if (m_unhittableTargets[i]->name() == target->name()) { m_unhittableTargets.fastRemove(i); }
	}

}

/** Clear all targets one by one */
void Session::clearTargets() {
	for(auto target : m_targetArray) {
		destroyTarget(target);
	}
}
