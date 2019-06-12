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
#include "SingleThresholdMeasurement.h"
#include "ExperimentConfig.h"
#include "sqlHelpers.h"
#include <ctime>

class App;

/** A class representing a psychophysical experiment
*/
class PsychHelper
{
public:
	PsychHelper() {
		srand((unsigned int)time(NULL));
	}

	/////////////////////////// initialization ///////////////////////////
	// Add all the conditions you want.
	//////////////////////////////////////////////////////////////////////

	/** Add condition
		\param[in] newConditionParam New condition
		\param[in] newExpParam New experiment design parameter
	*/
	void addCondition(Param newConditionParam, PsychophysicsDesignParameter newExpParam);

	/** Pick next condition*/
	void chooseNextCondition();

	/** Get current condtion parameter*/
	Param getParam();

	/** Get stimulus level for current trial*/
	float getStimLevel();

	/** Process user response and record it in the result file.
		\param[in] response Integer indicating user response
	*/
	void processResponse(int32_t response);

	/** Check whether experiment is complete*/
	bool isComplete();

	/** Reset the experiment state*/
	//void clear();

	Array<SingleThresholdMeasurement> mMeasurements;
	int32_t mCurrentConditionIndex = 0;
	int32_t mTrialCount = 0;

	/** Description of an experiment: Any information that could be useful
		in future, in case an experiment grows while piloting or when
		between-experiment analysis becomes necessary later.
	*/
	Param mExpDesc;

private:
};

class Experiment : public ReferenceCountedObject {
//class Experiment {
protected:
	double m_taskExecutionTime;
	String m_taskStartTime;
	String m_taskEndTime;
	int m_response;
	int m_clickCount = 0;
	bool m_hasSession;

	double m_totalRemainingTime = 0;
	double m_lastMotionChangeAt;
	double m_scoreboardDuration = 10.0; // show the score for at least this amount of seconds.
	double m_lastFireAt = 0.f;

	Array<Array<String>> m_playerActions;
	Array<Array<String>> m_targetTrajectory;
	Array<Array<String>> m_frameInfo;

	String m_feedbackMessage;
	Stopwatch stopwatch;

	Experiment(App* app) : m_app(app) {
		// secure vector capacity large enough so as to avoid memory allocation time.
		m_playerActions.reserve(5000);
		m_targetTrajectory.reserve(5000);
	};

public:
	String mResultFileName;
	App* m_app;
	static shared_ptr<Experiment> create(App* app) {
		return createShared<Experiment>(app);
	}
	static shared_ptr<Experiment> create_empty(App* app) {
		shared_ptr<Experiment> texp = create(app);
		texp->m_psych.mMeasurements = Array<SingleThresholdMeasurement>();
	}

	/**creates a new target with randomized motion path and gives it to the app */
	void initTargetAnimation();
	double weaponCooldownPercent();
	float randSign();
	void updatePresentationState();
	void onInit();
	void onGraphics3D(RenderDevice * rd, Array<shared_ptr<Surface>>& surface);
	void onSimulation(RealTime rdt, SimTime sdt, SimTime idt);
	void onUserInput(UserInput * ui);
	void onGraphics2D(RenderDevice * rd);
	void processResponse();
	void recordTrialResponse();
	void accumulateTrajectories();
	void accumulateFrameInfo(RealTime rdt, float sdt, float idt);

	/** queues action with given name to insert into database when trial completes
	@param action - one of "aim" "hit" "miss" or "invalid (shots limited by fire rate)" */
	void accumulatePlayerAction(String action);
	bool responseReady();
	bool initPsychHelper();
	bool moveOn = false;
	PsychHelper m_psych;
	ExperimentConfig m_config;

	/** result recording */
	void countClick() { m_clickCount++; }
};
