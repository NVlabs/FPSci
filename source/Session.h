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
#include "ParameterTable.h"
#include "ExperimentConfig.h"
#include "sqlHelpers.h"
#include "Logger.h"
#include "Dialogs.h"
#include <ctime>

class App;

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

class Session : public ReferenceCountedObject {
protected:
	App* m_app = nullptr;								///< Pointer to the app
	shared_ptr<SessionConfig> m_config;					///< The session this experiment will run
	shared_ptr<Logger> m_logger;						///< Output results logger

	// Experiment management					
	int m_response;										///< 0 indicates failure (didn't hit the target), 1 indicates sucess (hit the target)
	int m_destroyedTargets = 0;							///< Number of destroyed target
	int m_clickCount = 0;								///< Count of total clicks in this trial
	bool m_hasSession;									///< Flag indicating whether psych helper has loaded a valid session
	String m_feedbackMessage;							///< Message to show when trial complete

	int m_currTrialIdx;									///< Current trial
	int m_currQuestionIdx = -1;							///< Current question index
	Array<int> m_remaining;								///< Completed flags
	Array<Array<ParameterTable>> m_trialParams;			///< Trial (target) parameters

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
	
	// Reported data storage
	Array<Array<String>> m_playerActions;				///< Storage for player action (hit, miss, aim)
	Array<Array<String>> m_targetTrajectory;			///< Storage for target trajectory (vector3 cartesian)
	Array<Array<String>> m_frameInfo;					///< Storage for frame info (sdt, idt, rdt)

	Session(App* app, shared_ptr<SessionConfig> config) : m_app(app) {
		m_config = config;
		m_hasSession = notNull(m_config);
		// secure vector capacity large enough so as to avoid memory allocation time.
		m_playerActions.reserve(5000);
		m_targetTrajectory.reserve(5000);
	};

	Session(App* app) : m_app(app){
		// secure vector capacity large enough so as to avoid memory allocation time.
		m_playerActions.reserve(5000);
		m_targetTrajectory.reserve(5000);
		m_hasSession = false;
	}

public:
	float initialHeadingRadians = 0.0f;

	static shared_ptr<Session> create(App* app) {
		return createShared<Session>(app);
	}
	static shared_ptr<Session> create(App* app, shared_ptr<SessionConfig> config) {
		return createShared<Session>(app, config);
	}

	void randomizePosition(const shared_ptr<TargetEntity>& target) const;
	void initTargetAnimation();
	double weaponCooldownPercent() const;
	int remainingAmmo() const;

	void addTrial(Array<ParameterTable> params);
	bool isComplete() const;
	void nextCondition();

	/** randomly returns either +1 or -1 **/	
	static float randSign() {
		if (Random::common().uniform() > 0.5) {
			return 1;
		} else {
			return -1;
		}
	}
	
	void updatePresentationState();
	void onInit(String filename, String userName, String description);
	void onSimulation(RealTime rdt, SimTime sdt, SimTime idt);
	void processResponse();
	void recordTrialResponse();
	void accumulateTrajectories();
	void accumulateFrameInfo(RealTime rdt, float sdt, float idt);

	void countDestroy() {
		m_destroyedTargets += 1;
	}

	float getRemainingTrialTime();
	float getProgress();
	int getScore();
	String getFeedbackMessage();

	/** queues action with given name to insert into database when trial completes
	@param action - one of "aim" "hit" "miss" or "invalid (shots limited by fire rate)" */
	void accumulatePlayerAction(String action, String target="");
	bool canFire();

	using TargetParameters = Array<ParameterTable>;
	using SessionParameters = Array<TargetParameters>;
	bool setupTrialParams(SessionParameters params);
	
	bool moveOn = false;								///< Flag indicating session is complete
	enum PresentationState presentationState;			///< Current presentation state

	/** result recording */
	void countClick() { m_clickCount++; }
};
