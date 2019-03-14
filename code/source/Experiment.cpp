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
#include "Experiment.h"

// TODO: Replace with the G3D timestamp uses.
// utility function for generating a unique timestamp.
std::string genUniqueTimestamp() {
	time_t t = std::time(nullptr);
	std::tm tmbuf;
	localtime_s(&tmbuf, &t);
	char tmCharArray[17];
	std::strftime(tmCharArray, sizeof(tmCharArray), "%Y%m%d_%H%M%S", &tmbuf);
	std::string timeStr(tmCharArray);
	return timeStr;
}

void PsychHelper::addCondition(Param newConditionParam, PsychophysicsDesignParameter newPsychParam)
{
	SingleThresholdMeasurement m(newConditionParam, newPsychParam);
	mMeasurements.push_back(m);
}

void PsychHelper::chooseNextCondition()
{
	// Choose any staircase whose progress ratio is minimum
	float minimumProgressRatio = 1;
	// Find minimum progress ratio
	for (int32_t i = 0; i < (int32_t)mMeasurements.size(); i++)
	{
		if (mMeasurements[i].getProgressRatio() < minimumProgressRatio)
		{
			minimumProgressRatio = mMeasurements[i].getProgressRatio();
		}
	}
	// Make a vector with all the measurement cells with minimum progress ratio
	std::vector<int32_t> validIndex;
	for (int32_t i = 0; i < (int32_t)mMeasurements.size(); i++)
	{
		if (mMeasurements[i].getProgressRatio() == minimumProgressRatio)
		{
			validIndex.push_back(i);
		}
	}
	// Now choose any one from validIndex
	mCurrentConditionIndex = validIndex[rand() % (int32_t)validIndex.size()];
	std::cout << "Next chosen staircase is: " << mCurrentConditionIndex << '\n';
}

Param PsychHelper::getParam()
{
	return mMeasurements[mCurrentConditionIndex].getParam();
}

float PsychHelper::getStimLevel()
{
	return mMeasurements[mCurrentConditionIndex].getCurrentLevel();
}

void PsychHelper::processResponse(int32_t response)
{
	// Process the response.
	mMeasurements[mCurrentConditionIndex].processResponse(response);
	mTrialCount++;
}

bool PsychHelper::isComplete() // did the experiment end?
{
	bool allMeasurementComplete = true;
	for (int32_t i = 0; i < (int32_t)mMeasurements.size(); i++)
	{
		if (!mMeasurements[i].isComplete()) // if any one staircase is incomplete, set allSCCompleted to false and break.
		{
			allMeasurementComplete = false;
			break;
		}
	}
	return allMeasurementComplete;
}

void PsychHelper::clear()
{ // probably not necessary?
	mMeasurements.clear();
	mCurrentConditionIndex = 0;
	mTrialCount = 0;
}

