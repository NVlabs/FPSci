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

#include <time.h>
#include <vector>
#include <map>
#include <stdint.h>

namespace AbstractFPS
{
	/** Psychophysics method types
	*/
	enum PsychophysicsMethod { DiscreteStaircase, BucketStaircase, MethodOfConstantStimuli };

	/** Struct for experimental design parameters: Contains all parameters for both
		Staircase and Method of Constant Stimuli.
	*/
	struct PsychophysicsDesignParameter
	{
		PsychophysicsMethod mMeasuringMethod; // 0 = General staircase, 1 = Staircase with pre-determined stimLevels, 2 = Method of Constant Stimuli
		bool mIsDefault;
		float mInitLevel, mInitLevelRandomRange, mMinLevel, mMaxLevel, mInitLevelStepSize, mMinLevelStepSize; // for SC
		int32_t mNumUp, mNumDown, mMaxReversals, mMaxTotalTrialCount, mMaxLimitHitCount; // for SC
		int32_t mInitIndex, mInitIndexRandomRange, mInitIndexStepSize; // SC with pre-determined stimLevels. Some values are self-obvious (minIndexLevel = 0, maxIndexLevel = stimLevels.size(), minIndexStepSize = 1)
		std::vector<float> mStimLevels; // for SC with pre-determined stimLevels or Method of Constant Stimuli
		std::vector<int32_t> mMaxTrialCounts; // for Method of Constant Stimuli
	};

	/** Struct for condition parameter or experiment description
	*/
	struct Param
	{
		std::map<std::string, float> val;
		std::map<std::string, std::string> str;
		// TODO: check the following.
		Param() { val = std::map<std::string, float>(); str = std::map<std::string, std::string>(); }
		void add(std::string s, float f) {
			val.insert(std::pair<std::string, float>(s, f));
		}
		void add(std::string str_key, std::string str_val) {
			str.insert(std::pair<std::string, std::string>(str_key, str_val));
		}
	};

	/** Response struct
	*/
	struct Response
	{
		float mStimLevel;
		int32_t mResponse; // 1 = signal level too high, 0 = signal level too low
		int32_t mReversalCount; // for debugging
	};

	/** Class to abstract single threshold measurement
	*/
	class SingleThresholdMeasurement
	{
	public:

		/** Initialize measurement
		*/
		void initMeasurement(Param initConditionParam, PsychophysicsDesignParameter initExpParam); // return true if successfully initialized, false if not

		/** Get current level
		*/
		float getCurrentLevel();

		/** Get current condition parameter
		*/
		Param getParam();

		/** Process response
			\param[in] response response to process
		*/
		void processResponse(int32_t response);

		/** Get progress ratio
		*/
		float getProgressRatio();

		/** Checke whether measurement is complete
		*/
		bool isComplete();

		PsychophysicsDesignParameter mPsyParam;
		std::vector<Response> mResponses;
		float mCurrentLevel; // universal for both general SC and MCS
		float mLevelStepSize; // for general SC only
		int32_t mCurrentStimIndex, mIndexStepSize; // for SC with pre-determined stimulus levels
		int32_t mUpCount, mDownCount, mCurrentDirection, mReversalCount, mLimitHitCount; // for SC only
		std::vector<int32_t> mTrialCounts; // for MCS only
											// description of condition for the current measurement
		Param mParam;

		bool mIsInitialized = false;
	};


}
