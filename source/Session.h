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
#pragma once

#include <G3D/G3D.h>
#include "FpsConfig.h"
#include <ctime>

class FPSciApp;
class PlayerEntity;
class TargetEntity;
class FPSciLogger;
class Weapon;
enum PresentationState;

// Simple timer for measuring time offsets
class Timer
{
public:
	std::chrono::steady_clock::time_point startTime;
	void startTimer() { startTime = std::chrono::steady_clock::now(); };
	float getTime()
	{
		auto now = std::chrono::steady_clock::now();
		int t = std::chrono::duration_cast<std::chrono::duration<int, std::milli>>(now - startTime).count();
		return ((float)t) / 1000.0f;
	};
};

 struct FrameInfo {
	FILETIME time;
	//float idt = 0.0f;
	float sdt = 0.0f;

	FrameInfo() {};

	FrameInfo(FILETIME t, float simDeltaTime) {
		time = t;
		sdt = simDeltaTime;
	}
};

struct TargetLocation {
	FILETIME time;
	String name = "";
	PresentationState state;
	Point3 position = Point3::zero();

	TargetLocation() {};

	TargetLocation(FILETIME t, String targetName, PresentationState trialState, Point3 targetPosition) {
		time = t;
		name = targetName;
		state = trialState;
		position = targetPosition;
	}

	inline bool noChangeFrom(const TargetLocation& other) const {
		return state == other.state && position == other.position;
	}
};

enum PlayerActionType{
	None,
	Aim,
	FireCooldown,
	Miss,
	Hit,
	Destroy
};

struct PlayerAction {
	FILETIME			time;
	Point2				viewDirection = Point2::zero();
	Point3				position = Point3::zero();
	PresentationState	state;
	PlayerActionType	action = PlayerActionType::None;
	String				targetName = "";

	PlayerAction() {};

	PlayerAction(FILETIME t, Point2 playerViewDirection, Point3 playerPosition, PresentationState trialState, PlayerActionType playerAction, String name) {
		time = t;
		viewDirection = playerViewDirection;
		position = playerPosition;
		action = playerAction;
		state = trialState;
		targetName = name;
	}

	inline bool noChangeFrom(const PlayerAction& other) const {
		return viewDirection == other.viewDirection && position == other.position && action == other.action && state == other.state && targetName == other.targetName;
	}
};

/** Trial count class (optional for alternate TargetConfig/count table lookup) */
class TrialConfig : public FpsConfig {
public:
	String			id;				///< Trial ID (used for logging and required for task affiliation)
	Array<String>	targetIds;		///< Trial ID list
	int				count = 1;		///< Count of trials to be performed
	static int		defaultCount;	///< Default count to use

	TrialConfig() : FpsConfig(defaultConfig()) {};
	TrialConfig(const Array<String>& targetIds, int trialCount) : targetIds(targetIds), count(trialCount) {};
	TrialConfig(const Any& any);

	static shared_ptr<TrialConfig> create() { return createShared<TrialConfig>(); }

	// Use a static method to bypass order of declaration for static members (specific to Sampler s_freeList in GLSamplerObect)
	// Trick from: https://www.cs.technion.ac.il/users/yechiel/c++-faq/static-init-order-on-first-use.html	
	static FpsConfig& defaultConfig() {
		static FpsConfig def;
		def.questionArray = Array<Question>();		// Clear the questions array (don't inherit)
		return def;
	}

	Any toAny(const bool forceAll = false) const;
};

class TrialOrder {
public:
	Array<String>	order;			// The order of trials (by id)
	//int				count = 1;		// The count of this order
	String			correctAnswer = ""; // The correct answer for the task question in this order

	TrialOrder() {};
	TrialOrder(const Any& any) {
		FPSciAnyTableReader reader(any);
		reader.get("order", order);
		reader.getIfPresent("correctAnswer", correctAnswer);
		//reader.getIfPresent("count", count);
		
	}
	Any toAny(const bool forceAll = false) const {
		TrialOrder def;
		Any a(Any::TABLE);
		a["order"] = order;
		if (forceAll || def.correctAnswer != correctAnswer) a["correctAnswer"] = correctAnswer;
		//if (forceAll || count != def.count) a["count"] = count;
		return a;
	}
};

class TaskConfig {
public:
	String		id;						///< Task ID (used for logging)
	Array<TrialOrder> trialOrders;		///< Valid trial orders for this task
	Array<Question>	questions;			///< Task-level questions
	int				questionIdx = -1;	// The index of the (task-level) question corresponding to the correct answer above (populated automatically)
	int count = 1;						///< Count of times to present this task (in each of the trialOrders, currently unused)

	TaskConfig() {};
	TaskConfig(const Any& any);
	Any toAny(const bool forceAll = false) const;
};

/** Configuration for a session worth of trials */
class SessionConfig : public FpsConfig {
public:
	String				id;								///< Session ID
	String				description = "Session";		///< String indicating whether session is training or real
	int					blockCount = 1;					///< Default to just 1 block per session
	Array<TaskConfig>   tasks;
	Array<TrialConfig>	trials;							///< Array of trials (and their counts) to be performed
	bool				closeOnComplete = false;		///< Close application on session completed?
	bool				randomizeTaskOrder = true;		///< Randomize order of tasks presented within the session?
	bool				weightByCount = true;			///< Ranomized based on count of task remaining

	SessionConfig() : FpsConfig(defaultConfig()) {}
	SessionConfig(const Any& any);

	// Use a static method to bypass order of declaration for static members (specific to Sampler s_freeList in GLSamplerObect)
	// Trick from: https://www.cs.technion.ac.il/users/yechiel/c++-faq/static-init-order-on-first-use.html
	static FpsConfig& defaultConfig() {
		static FpsConfig def;				// This is NOT freed ever (in our code)
		return def;
	}

	static shared_ptr<SessionConfig> create() { return createShared<SessionConfig>(); }
	Any toAny(const bool forceAll = false) const;
	float getTrialOrdersPerBlock(void) const;			// Get the total number of trials in this session
	int getTrialIndex(const String& id) const;
	Array<String> getUniqueTargetIds() const;
};

class Session : public ReferenceCountedObject {
protected:
	FPSciApp* m_app = nullptr;							///< Pointer to the app
	Scene* m_scene = nullptr;							///< Pointer to the scene
	
	shared_ptr<SessionConfig> m_sessConfig;				///< Configuration for this session
	shared_ptr<TrialConfig> m_trialConfig;				///< Configuration for the trial we are in 
	
	String m_dbFilename;								///< Filename for output logging (less the .db extension)

	shared_ptr<PlayerEntity> m_player;					///< Player entity
	shared_ptr<Weapon> m_weapon;						///< Weapon
	shared_ptr<Camera> m_camera;						///< Camera entity

	// Experiment management					
	int m_destroyedTargets = 0;							///< Number of destroyed target
	int m_trialShotsHit = 0;							///< Count of total hits in this trial
	bool m_firstTrial = true;							///< Is this the first trial in this session?
	bool m_hasSession;									///< Flag indicating whether psych helper has loaded a valid session
	int	m_currBlock = 1;								///< Index to the current block of trials
	String m_feedbackMessage;							///< Message to show when trial complete

	// Target management
	Table<String, Array<shared_ptr<ArticulatedModel>>>* m_targetModels;
	int m_lastUniqueID = 0;									///< Counter for creating unique names for various entities
	
	Array<shared_ptr<TargetEntity>> m_targetArray;			///< Array of drawn targets
	Array<shared_ptr<TargetEntity>> m_hittableTargets;		///< Array of targets that can be hit
	Array<shared_ptr<TargetEntity>> m_unhittableTargets;	///< Array of targets that can't be hit

	Table<String, TargetLocation> m_lastLogTargetLoc;		///< Last logged target location (used for logOnChange)
	Point3 m_lastRefTargetPos;								///< Last reference target location (used for aim invalidation)

	int m_frameTimeIdx = 0;									///< Frame time index
	int m_currTaskIdx;										///< Current task index (from tasks array)
	int m_currOrderIdx;										///< Current trial order index
	int m_currTrialIdx;										///< Current trial index (from the trials array)
	int m_currQuestionIdx = -1;								///< Current question index
	Array<int> m_taskTrials;								///< Indexes of trials (from trials array) for this task
	Array<Array<int>> m_remainingTasks;						///< Count of remaining trials of each order
	Array<Array<int>> m_completedTasks;						///< Count of completions of each trial order
	int m_lastTaskIndex = 0;								///< Used for providing task index on questions
	Table<String, int> m_completedTaskTrials;				///< Count of completed trial types in this task
	Array<Array<shared_ptr<TargetConfig>>> m_targetConfigs;	///< Target configurations by trial

	// Time-based parameters
	float m_pretrialDuration;							///< (Possibly) randomized pretrial duration
	RealTime m_taskExecutionTime;						///< Task completion time for the most recent trial
	String m_taskStartTime;								///< Recorded task start timestamp							
	String m_taskEndTime;								///< Recorded task end timestamp
	Timer m_timer;										///< Timer used for timing tasks	
	// Could move timer above to stopwatch in future
	//Stopwatch stopwatch;			

	// Scoring mechanisms
	RealTime m_totalRemainingTime = 0;					///< Time remaining in the trial
	int m_totalTargetsDestroyed = 0;					///< Total number of targets destroyed
	int m_totalShots = 0;								///< Total number of shots taken
	int m_totalShotsHit = 0;							///< Total number of shots hit
	float m_accuracy = 0;								///< Overall accuracy
	int m_totalTrialSuccesses = 0;						///< Total number of successful trials

	Array<HANDLE> m_sessProcesses;						///< Handles for session-level processes
	Array<HANDLE> m_trialProcesses;						///< Handles for trial-level processes

	// Target parameters
	const float m_targetDistance = 1.0f;				///< Actual distance to target
	
	Session(FPSciApp* app, shared_ptr<SessionConfig> config);
	Session(FPSciApp* app);

	~Session(){
		clearTargets();					// Clear the targets when the session is done
		// For now leave "orphaned" processes to allow (session) end commands to run until completion
		//closeTrialProcesses();		// Close any trial processes affiliated with this session
		//closeSessionProcesses();		// Close any processes affiliated with this session
	}

	inline void runTrialCommands(String evt) {
		evt = toLower(evt);
		auto cmds = (evt == "start") ? m_sessConfig->commands.trialStartCmds : m_sessConfig->commands.trialEndCmds;
		for (auto cmd : cmds) { 
			m_trialProcesses.append(runCommand(cmd, evt + " of trial")); 
		}
	}

	inline void closeTrialProcesses() {
		for (auto handle : m_trialProcesses) { 
			TerminateProcess(handle, 0); 
		}
		m_trialProcesses.clear();
	}

	inline void runSessionCommands(String evt) {
		evt = toLower(evt);
		auto cmds = (evt == "start") ? m_sessConfig->commands.sessionStartCmds : m_sessConfig->commands.sessionEndCmds;
		for (auto cmd : cmds) { 
			m_sessProcesses.append(runCommand(cmd, evt + " of session")); 
		}
	}

	inline void closeSessionProcesses() {
		for (auto handle : m_sessProcesses) { 
			TerminateProcess(handle, 0); 
		}
		m_sessProcesses.clear();
	}

	String formatFeedback(const String& input);
	String formatCommand(const String& input);
	bool presentQuestions(Array<Question>& questions);

	/** Insert a target into the target array/scene */
	inline void insertTarget(shared_ptr<TargetEntity> target);

	/** Get the total target count for the current trial */
	int totalTrialTargets() const {
		int totalTargets = 0;
		for (shared_ptr<TargetConfig> target : m_targetConfigs[m_currTrialIdx]) {
			if (target->respawnCount == -1) {
				totalTargets = -1;		// Ininite spawn case
				break;
			}
			else {
				totalTargets += (target->respawnCount + 1);
			}
		}
		return totalTargets;
	}

	shared_ptr<TargetEntity> spawnDestTarget(
		shared_ptr<TargetConfig> config,
		const Point3& offset,
		const Color3& color,
		const int paramIdx,
		const String& name = "");

	shared_ptr<FlyingEntity> spawnReferenceTarget(
		const Point3& position,
		const Point3& orbitCenter,
		const float size,
		const Color3& color
	);

	shared_ptr<FlyingEntity> spawnFlyingTarget(
		shared_ptr<TargetConfig> config,
		const Point3& position,
		const Point3& orbitCenter,
		const Color3& color,
		const int paramIdx,
		const String& name = ""
	);

	shared_ptr<JumpingEntity> spawnJumpingTarget(
		shared_ptr<TargetConfig> config,
		const Point3& position,
		const Point3& orbitCenter,
		const Color3& color,
		const float targetDistance,
		const int paramIdx,
		const String& name = ""
	);

	inline float drawTruncatedExp(float lambda, float min, float max) {
		const float p = Random::common().uniform();
		const float R = max - min;
		if (lambda == 0.f) return min + p * R;
		if (lambda < -88.f) return max;				// This prevents against numerical errors in the expression below
		return -log(1 - p * (1 - exp(-lambda * R))) / lambda + min;
	}

	inline Point2 getViewDirection()
	{   // returns (azimuth, elevation), where azimuth is 0 deg when straightahead and + for right, - for left.
		Point3 view_cartesian = m_camera->frame().lookVector();
		float az = atan2(view_cartesian.x, -view_cartesian.z) * 180 / pif();
		float el = atan2(view_cartesian.y, sqrtf(view_cartesian.x * view_cartesian.x + view_cartesian.z * view_cartesian.z)) * 180 / pif();
		return Point2(az, el);
	}

	inline Point3 getPlayerLocation()
	{
		return m_camera->frame().translation;
	}

	HANDLE runCommand(CommandSpec cmd, String evt) {
		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));

		// Run the (formatted command)
		LPSTR command = LPSTR(formatCommand(cmd.cmdStr).c_str());
		bool success;
		if (cmd.foreground) {	// Run process in the foreground
			success = CreateProcess(NULL, command, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);
		}
		else {				// Run process silently in the background
			success = CreateProcess(NULL, command, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
		}

		if (!success) {
			logPrintf("Failed to run %s command: \"%s\". %s\n", evt, cmd, GetLastErrorString());
		}

		if (cmd.blocking) {	// Optional blocking behavior
			WaitForSingleObject(pi.hProcess, INFINITE);
		}

		return pi.hProcess;
	}

	String GetLastErrorString() {
		DWORD error = GetLastError();
		if (error) {
			LPVOID lpMsgBuf;
			DWORD bufLen = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
			if (bufLen) {
				LPCSTR lpMsgStr = (LPCSTR)lpMsgBuf;
				std::string result(lpMsgStr, lpMsgStr + bufLen);
				LocalFree(lpMsgBuf);
				return String(result);
			}
		}
		return String();
	}

public:
	float initialHeadingRadians = 0.0f;
	shared_ptr<FPSciLogger> logger;					///< Output results logger

	static shared_ptr<Session> create(FPSciApp* app) {
		return createShared<Session>(app);
	}
	static shared_ptr<Session> create(FPSciApp* app, shared_ptr<SessionConfig> config) {
		return createShared<Session>(app, config);
	}

	void randomizePosition(const shared_ptr<TargetEntity>& target) const;
	void initTargetAnimation(const bool task);
	void spawnTrialTargets(Point3 initialSpawnPos, bool previewMode = false);

	bool blockComplete() const;
	bool nextTrial();

	const RealTime targetFrameTime();

	void endLogging();

	/** randomly returns either +1 or -1 **/	
	static float randSign() {
		if (Random::common().uniform() > 0.5) {
			return 1;
		} else {
			return -1;
		}
	}
	
	void updatePresentationState();
	void onInit(String filename, String description);
	void onSimulation(RealTime rdt, SimTime sdt, SimTime idt);
	void processResponse();
	void recordTrialResponse(int destroyedTargets, int totalTargets);
	void accumulateTrajectories();
	void accumulateFrameInfo(RealTime rdt, float sdt, float idt);

	void countDestroy() {
		m_destroyedTargets++;
	}

	/** Destroy a target from the targets array */
	void destroyTarget(shared_ptr<TargetEntity> target);

	/** clear all targets (used when clearing remaining targets at the end of a trial) */
	void clearTargets();

	bool inTask();
	float getElapsedTrialTime();
	float getRemainingTrialTime();
	float getProgress();
	double getScore();
	String getFeedbackMessage();
	int getTaskCount(const int currTaskIdx) const;

	/** queues action with given name to insert into database when trial completes
	@param action - one of "aim" "hit" "miss" or "invalid (shots limited by fire rate)" */
	void accumulatePlayerAction(PlayerActionType action, String target="");
	
	bool nextBlock(bool init = false);

	bool moveOn = false;								///< Flag indicating session is complete
	PresentationState currentState;						///< Current presentation state

	const Array<shared_ptr<TargetEntity>>& targetArray() const {
		return m_targetArray;
	}

	/** dynamically allocates a new array of pointers to the hittable targets in the session */
	const Array<shared_ptr<TargetEntity>>& hittableTargets() const {
		return m_hittableTargets;
	}

	/** dynamically allocates a new array of pointers to the unhittable (visible but inactive) targets in the session */
	const Array<shared_ptr<TargetEntity>>& unhittableTargets() const {
		return m_unhittableTargets;
	}
};
