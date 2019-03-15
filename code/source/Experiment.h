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
#include <ctime>

class App;

// TODO: Replace with the G3D timestamp uses.
// utility function for generating a unique timestamp.
std::string genUniqueTimestamp();

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

	/** Pick next condition
	*/
	void chooseNextCondition();

	/** Get current condtion parameter
	*/
	Param getParam();

	/** Get stimulus level for current trial
	*/
	float getStimLevel();

	/** Process user response and record it in the result file.
		\param[in] response Integer indicating user response
	*/
	void processResponse(int32_t response);

	/** Check whether experiment is complete
	*/
	bool isComplete();

	/** Reset the experiment state
	*/
	void clear();

	std::vector<SingleThresholdMeasurement> mMeasurements;
	int32_t mCurrentConditionIndex;
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
	double m_taskStartTime;
	double m_taskEndTime;
	int m_response;

	String m_feedbackMessage;
	Stopwatch stopwatch;

	Experiment(App* app) :m_app(app) {};

public:

	PsychHelper m_psych;

	//Experiment() : ReferenceCountedObject() {
	//}

	virtual void onInit() = 0;
	virtual void onGraphics3D(RenderDevice * rd, Array<shared_ptr<Surface>>& surface) = 0;
	virtual void onSimulation(RealTime rdt, SimTime sdt, SimTime idt) = 0;
	virtual void onUserInput(UserInput * ui) = 0;
	//virtual void onGraphics2D(RenderDevice * rd, Array<shared_ptr<Surface2D>>& posed2D) = 0;
	virtual void onGraphics2D(RenderDevice * rd) = 0;

	/** result recording */
	virtual void createResultFile() = 0;
	virtual void recordTrialResponse() = 0;
	virtual void closeResultFile() = 0;

	/** PsychHelper-related */
	virtual void initPsychHelper() = 0;

	std::string mResultFileName;
	App* m_app;
};
