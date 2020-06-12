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
#include "ConfigFiles.h"
#include <ctime>

class FPSciApp;
class PlayerEntity;
class TargetEntity;
class FPSciLogger;

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
	Point3 position = Point3::zero();

	TargetLocation() {};

	TargetLocation(FILETIME t, String targetName, Point3 targetPosition) {
		time = t;
		name = targetName;
		position = targetPosition;
	}
};

enum PlayerActionType{
	None,
	Aim,
	Invalid,
	Nontask,
	Miss,
	Hit,
	Destroy
};

struct PlayerAction {
	FILETIME			time;
	Point2				viewDirection = Point2::zero();
	Point3				position = Point3::zero();
	PlayerActionType	action = PlayerActionType::None;
	String				targetName = "";

	PlayerAction() {};

	PlayerAction(FILETIME t, Point2 playerViewDirection, Point3 playerPosition, PlayerActionType playerAction, String name) {
		time = t;
		viewDirection = playerViewDirection;
		position = playerPosition;
		action = playerAction;
		targetName = name;
	}
};

class Session : public ReferenceCountedObject {
protected:
	FPSciApp* m_app = nullptr;								///< Pointer to the app
	Scene* m_scene = nullptr;							///< Pointer to the scene
	
	shared_ptr<SessionConfig> m_config;					///< The session this experiment will run
	shared_ptr<FPSciLogger> m_logger;						///< Output results logger
	shared_ptr<PlayerEntity> m_player;					///< Player entity
	shared_ptr<Camera> m_camera;						///< Camera entity

	// Experiment management					
	int m_remainingTargets;								///< Number of remaining targets (calculated at the end of the session)
	int m_destroyedTargets = 0;							///< Number of destroyed target
	int m_clickCount = 0;								///< Count of total clicks in this trial
	bool m_hasSession;									///< Flag indicating whether psych helper has loaded a valid session
	int	m_currBlock = 1;								///< Index to the current block of trials
	Array<Array<shared_ptr<TargetConfig>>> m_trials;	///< Storage for trials (to repeat over blocks)
	String m_feedbackMessage;							///< Message to show when trial complete

	// Target management
	Table<String, Array<shared_ptr<ArticulatedModel>>>* m_targetModels;
	int m_modelScaleCount;
	int m_lastUniqueID = 0;								///< Counter for creating unique names for various entities
	Array<shared_ptr<TargetEntity>> m_targetArray;		///< Array of drawn targets

	int m_currTrialIdx;									///< Current trial
	int m_currQuestionIdx = -1;							///< Current question index
	Array<int> m_remainingTrials;								///< Completed flags
	Array<Array<shared_ptr<TargetConfig>>> m_targetConfigs;		///< Target configurations by trial

	// Time-based parameters
	RealTime m_taskExecutionTime;						///< Task completion time for the most recent trial
	String m_taskStartTime;								///< Recorded task start timestamp							
	String m_taskEndTime;								///< Recorded task end timestamp
	RealTime m_totalRemainingTime = 0;					///< Time remaining in the trial
	RealTime m_scoreboardDuration = 10.0;				///< Show the score for at least this amount of seconds.
	RealTime m_lastFireAt = 0.f;						///< Time of the last shot
	Timer m_timer;										///< Timer used for timing tasks	
	// Could move timer above to stopwatch in future
	//Stopwatch stopwatch;			

	// Target parameters
	const float m_targetDistance = 1.0f;				///< Actual distance to target
	
	Session(FPSciApp* app, shared_ptr<SessionConfig> config) : m_app(app), m_config(config)
	{
		m_hasSession = notNull(m_config);
	}

	Session(FPSciApp* app) : m_app(app)
	{
		m_hasSession = false;
	}

	~Session(){
		clearTargets();		// Clear the targets when the session is done
	}

	/** Creates a random target with motion based on parameters
	@param motionDuration time in seconds to produce a motion path for
	@param motionDecisionPeriod time in seconds when new motion direction is chosen
	@param speed world-space velocity (m/s) of target
	@param radius world-space distance to target
	@param scale size of target TODO: is this radius or diameter in meters?*/
	//void spawnParameterizedRandomTarget(float motionDuration, float motionDecisionPeriod, float speed, float radius, float scale);
	/** Creates a random target in front of the player */
	//void spawnRandomTarget();
	/** Creates a spinning target */

	//shared_ptr<FlyingEntity> spawnTarget(const Point3& position, float scale, bool spinLeft = true, const Color3& color = Color3::red(), String modelName = "model/target/target.obj");

		/** Insert a target into the target array/scene */
	inline void insertTarget(shared_ptr<TargetEntity> target);

	shared_ptr<TargetEntity> spawnDestTarget(
		shared_ptr<TargetConfig> config,
		const Point3& position,
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

	inline Point2 getViewDirection()
	{   // returns (azimuth, elevation), where azimuth is 0 deg when straightahead and + for right, - for left.
		Point3 view_cartesian = m_camera->frame().lookVector();
		float az = atan2(-view_cartesian.z, -view_cartesian.x) * 180 / pif();
		float el = atan2(view_cartesian.y, sqrtf(view_cartesian.x * view_cartesian.x + view_cartesian.z * view_cartesian.z)) * 180 / pif();
		return Point2(az, el);
	}

	inline Point3 getPlayerLocation()
	{
		return m_camera->frame().translation;
	}


public:
	float initialHeadingRadians = 0.0f;

	static shared_ptr<Session> create(FPSciApp* app) {
		return createShared<Session>(app);
	}
	static shared_ptr<Session> create(FPSciApp* app, shared_ptr<SessionConfig> config) {
		return createShared<Session>(app, config);
	}

	void randomizePosition(const shared_ptr<TargetEntity>& target) const;
	void initTargetAnimation();
	float weaponCooldownPercent() const;
	RealTime lastFireTime() const {
		return m_lastFireAt;
	}
	int remainingAmmo() const;

	bool blockComplete() const;
	bool complete() const;
	void nextCondition();

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
		m_destroyedTargets += 1;
	}

	/** Destroy a target from the targets array */
	void destroyTarget(int index);
	void destroyTarget(shared_ptr<TargetEntity> target);

	/** clear all targets (used when clearing remaining targets at the end of a trial) */
	void clearTargets();

	float getRemainingTrialTime();
	float getProgress();
	int getScore();
	String getFeedbackMessage();

	/** queues action with given name to insert into database when trial completes
	@param action - one of "aim" "hit" "miss" or "invalid (shots limited by fire rate)" */
	void accumulatePlayerAction(PlayerActionType action, String target="");
	bool canFire();

	bool updateBlock(bool updateTargets = false);

	bool moveOn = false;								///< Flag indicating session is complete
	enum PresentationState presentationState;			///< Current presentation state

	/** result recording */
	void countClick() { m_clickCount++; }

	const Array<shared_ptr<TargetEntity>>& targetArray() const {
		return m_targetArray;
	}
};
