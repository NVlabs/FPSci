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
#include "SingleThresholdMeasurement.h"

namespace AbstractFPS
{
	void SingleThresholdMeasurement::initMeasurement(Param initConditionParam, PsychophysicsDesignParameter initExpParam) // return true if successfully initialized, false if not
	{
		if (mIsInitialized)
		{
			// already initialized: print out an error message
		}
		else
		{
			mPsyParam = initExpParam;
			mParam = initConditionParam;
			if (mPsyParam.mMeasuringMethod == DiscreteStaircase)
			{
				if (mPsyParam.mIsDefault) // only mMinLevel, mMaxLevel, mMinLevelStepSize were defined
				{
					mPsyParam.mInitLevelRandomRange = 0; // no random change
					mPsyParam.mInitLevel = mPsyParam.mMaxLevel; // start with the easiest
					mPsyParam.mInitLevelStepSize = 4 * mPsyParam.mMinLevelStepSize;
					mPsyParam.mNumUp = 1; // assuming 2AFC design, 1 up 2 down is standard
					mPsyParam.mNumDown = 2;
					mPsyParam.mMaxReversals = 50;
					mPsyParam.mMaxTotalTrialCount = 150;
					mPsyParam.mMaxLimitHitCount = 2;
				}
				// First, set the stimulus level perturbed within the initial random range
				float perturbation;
				if (mPsyParam.mMinLevelStepSize == 0)
				{
					perturbation = 0;
				}
				else
				{
					int32_t numMinSteps = (int32_t)(mPsyParam.mInitLevelRandomRange / mPsyParam.mMinLevelStepSize);
					int32_t randomSign = 2 * (rand() % 2) - 1;
					int32_t stepsForPerturbation = randomSign * (rand() % numMinSteps);
					perturbation = stepsForPerturbation * mPsyParam.mMinLevelStepSize;
				}
				// set initial stim level
				mCurrentLevel = mPsyParam.mInitLevel + perturbation;
				if (mCurrentLevel < mPsyParam.mMinLevel)
				{
					mCurrentLevel = mPsyParam.mMinLevel;
				}
				else if (mCurrentLevel > mPsyParam.mMaxLevel)
				{
					mCurrentLevel = mPsyParam.mMaxLevel;
				}
				// Initialize all other necessary values
				mLevelStepSize = mPsyParam.mInitLevelStepSize;
				mUpCount = 0;
				mDownCount = 0;
				mCurrentDirection = 0;
				mReversalCount = 0;
				mLimitHitCount = 0;
			}
			else if (mPsyParam.mMeasuringMethod == BucketStaircase) // SC with pre-determined stimLevels
			{
				if (mPsyParam.mIsDefault) // only stimLevels were defined
				{
					mPsyParam.mInitIndexRandomRange = 0; // no random change
					mPsyParam.mInitIndex = (int32_t)mPsyParam.mStimLevels.size() - 1; // start with the easiest
					mPsyParam.mInitIndexStepSize = 4;
					mPsyParam.mNumUp = 1;
					mPsyParam.mNumDown = 2;
					mPsyParam.mMaxReversals = 15;
					mPsyParam.mMaxTotalTrialCount = 50;
					mPsyParam.mMaxLimitHitCount = 2;
				}
				// First, set the stimulus level perturbed within the initial random range
				int32_t randomSign = 2 * (rand() % 2) - 1;
				int32_t perturbation;
				if (mPsyParam.mInitIndexRandomRange == 0)
				{
					perturbation = 0;
				}
				else
				{
					perturbation = randomSign * (rand() % mPsyParam.mInitIndexRandomRange);
				}
				// set initial stim level
				mCurrentIndex = mPsyParam.mInitIndex + perturbation;
				if (mCurrentIndex < 0)
				{
					mCurrentIndex = 0;
				}
				else if (mCurrentIndex >= (int32_t)mPsyParam.mStimLevels.size())
				{
					mCurrentIndex = (int32_t)mPsyParam.mStimLevels.size() - 1;
				}
				mCurrentLevel = mPsyParam.mStimLevels[mCurrentIndex];
				// Initialize all other necessary values
				mIndexStepSize = mPsyParam.mInitIndexStepSize;
				mUpCount = 0;
				mDownCount = 0;
				mCurrentDirection = 0;
				mReversalCount = 0;
				mLimitHitCount = 0;
			}
			else if (mPsyParam.mMeasuringMethod == MethodOfConstantStimuli) // MCS
			{
				if (mPsyParam.mIsDefault) // only stimLevels were defined
				{
					int32_t trialCount = (int32_t)(200 / (int32_t)mPsyParam.mStimLevels.size()); // let's do ~200 trials per each condition
					for (int32_t i = 0; i < (int32_t)mPsyParam.mStimLevels.size(); i++)
					{
						mPsyParam.mMaxTrialCounts.push_back(trialCount);
					}
				}
				// Set initial stimulus level
				mCurrentLevel = mPsyParam.mStimLevels[rand() % (int32_t)mPsyParam.mStimLevels.size()];
				// Initialize all other necessary values
				for (int32_t i = 0; i < (int32_t)mPsyParam.mStimLevels.size(); i++)
				{
					mTrialCounts.push_back(0);
				}
			}
			mIsInitialized = true;
		}
	}

	float SingleThresholdMeasurement::getCurrentLevel()
	{
		return mCurrentLevel;
	}

	Param SingleThresholdMeasurement::getParam()
	{
		return mParam;
	}

	void SingleThresholdMeasurement::processResponse(int32_t response)
	{
		// record current response
		Response res;
		res.mStimLevel = mCurrentLevel;
		res.mResponse = response;
		if ((mPsyParam.mMeasuringMethod == DiscreteStaircase) || (mPsyParam.mMeasuringMethod == BucketStaircase)) // SC. This is for debugging.
		{
			res.mReversalCount = mReversalCount;
		}
		mResponses.push_back(res);

		// select next stim level based on measuring strategy (SC or MCS)
		if (mPsyParam.mMeasuringMethod == DiscreteStaircase) // SC. Count reversals and select next stim level.
		{
			if (mResponses.back().mResponse == 0) // telling 'signal level too low!', attempting to go up.
			{
				mUpCount++; // increment up count by one
				mDownCount = 0; // reset down count
				if (mUpCount == mPsyParam.mNumUp) // time to move up.
				{
					if (mCurrentDirection == -1) // Direction reversal. Increment reversal count. halve the step size.
					{
						mReversalCount++;
						mLevelStepSize = mLevelStepSize / 2;
						if (mLevelStepSize < mPsyParam.mMinLevelStepSize) // step size too small
						{
							mLevelStepSize = mPsyParam.mMinLevelStepSize;
						}
					}
					mCurrentDirection = 1;
					mCurrentLevel = mCurrentLevel + mCurrentDirection * mLevelStepSize; // move one step up.
					if (mCurrentLevel > mPsyParam.mMaxLevel)
					{
						mCurrentLevel = mPsyParam.mMaxLevel;
						mLimitHitCount++;
						if (mLimitHitCount >= mPsyParam.mMaxLimitHitCount)
						{
							mReversalCount++;
							mLimitHitCount = 0;
						}
					}
					else
					{
						mLimitHitCount = 0;
					}
					mUpCount = 0; // reset up count
				}
				std::cout << "Processed a response that was incorrect. Reversal count is: " << mReversalCount << "\n";
			}
			else // telling 'signal level too high!', attempting to go down.
			{
				mDownCount++; // increment down count by one
				mUpCount = 0; // reset up count
				if (mDownCount == mPsyParam.mNumDown) // time to move down.
				{
					if (mCurrentDirection == 1) // Direction reversal. Increment reversal count. halve the step size.
					{
						mReversalCount++;
						mLevelStepSize = mLevelStepSize / 2;
						if (mLevelStepSize < mPsyParam.mMinLevelStepSize) // step size too small
						{
							mLevelStepSize = mPsyParam.mMinLevelStepSize;
						}
					}
					mCurrentDirection = -1;
					mCurrentLevel = mCurrentLevel + mCurrentDirection * mLevelStepSize; // move one step down.
					if (mCurrentLevel < mPsyParam.mMinLevel)
					{
						mCurrentLevel = mPsyParam.mMinLevel;
						mLimitHitCount++;
						if (mLimitHitCount >= mPsyParam.mMaxLimitHitCount)
						{
							mReversalCount++;
							mLimitHitCount = 0;
						}
					}
					else
					{
						mLimitHitCount = 0;
					}
					mDownCount = 0; // reset down count
				}
				std::cout << "Processed a response that was correct. Reversal count is: " << mReversalCount << "\n";

			}
		}
		else if (mPsyParam.mMeasuringMethod == BucketStaircase) // SC with pre-determined stimLevels
		{
			if (mResponses.back().mResponse == 0) // telling 'signal level too low!', attempting to go up.
			{
				mUpCount++; // increment up count by one
				mDownCount = 0; // reset down count
				if (mUpCount == mPsyParam.mNumUp) // time to move up.
				{
					if (mCurrentDirection == -1) // Direction reversal. Increment reversal count. halve the step size.
					{
						mReversalCount++;
						mIndexStepSize = mIndexStepSize / 2;
						if (mIndexStepSize < 1) // step size too small
						{
							mIndexStepSize = 1;
						}
					}
					mCurrentDirection = 1;
					mCurrentIndex = mCurrentIndex + mCurrentDirection * mIndexStepSize; // move one step up.
					if (mCurrentIndex >= (int32_t)mPsyParam.mStimLevels.size())
					{
						mCurrentIndex = (int32_t)mPsyParam.mStimLevels.size() - 1;
						mLimitHitCount++;
						if (mLimitHitCount >= mPsyParam.mMaxLimitHitCount)
						{
							mReversalCount++;
							mLimitHitCount = 0;
						}
					}
					else
					{
						mLimitHitCount = 0;
					}
					mUpCount = 0; // reset up count
				}
				mCurrentLevel = mPsyParam.mStimLevels[mCurrentIndex];
				std::cout << "Processed a response that was incorrect. Reversal count is: " << mReversalCount << "\n";
			}
			else // telling 'signal level too high!', attempting to go down.
			{
				mDownCount++; // increment down count by one
				mUpCount = 0; // reset up count
				if (mDownCount == mPsyParam.mNumDown) // time to move down.
				{
					if (mCurrentDirection == 1) // Direction reversal. Increment reversal count. halve the step size.
					{
						mReversalCount++;
						mIndexStepSize = mIndexStepSize / 2;
						if (mIndexStepSize < 1) // step size too small
						{
							mIndexStepSize = 1;
						}
					}
					mCurrentDirection = -1;
					mCurrentIndex = mCurrentIndex + mCurrentDirection * mIndexStepSize; // move one step down.
					if (mCurrentIndex < 0)
					{
						mCurrentIndex = 0;
						mLimitHitCount++;
						if (mLimitHitCount >= mPsyParam.mMaxLimitHitCount)
						{
							mReversalCount++;
							mLimitHitCount = 0;
						}
					}
					else
					{
						mLimitHitCount = 0;
					}
					mDownCount = 0; // reset down count
				}
				mCurrentLevel = mPsyParam.mStimLevels[mCurrentIndex];
				std::cout << "Processed a response that was correct. Reversal count is: " << mReversalCount << "\n";
			}
		}
		else if (mPsyParam.mMeasuringMethod == MethodOfConstantStimuli) // MCS. Count numTrials and select next stim level.
		{
			// count number of trials & calculate progress ratio per each stimuli level
			std::vector<float> progressRatio;
			float minimumProgressRatio = 1;
			for (int32_t i = 0; i < (int32_t)mTrialCounts.size(); i++)
			{
				if (mPsyParam.mStimLevels[i] == mCurrentLevel)
				{
					mTrialCounts[i]++;
					break;
				}
				float pr = (float)mTrialCounts[i] / (float)mPsyParam.mMaxTrialCounts[i];
				if (pr < minimumProgressRatio)
				{
					minimumProgressRatio = pr;
				}
				progressRatio.push_back(pr);
			}
			// select one stimuli level from those with minimum progress ratio
			std::vector<int32_t> validIndex;
			for (int32_t i = 0; i < (int32_t)progressRatio.size(); i++)
			{
				if (minimumProgressRatio == progressRatio[i])
				{
					validIndex.push_back(i);
				}
			}
			// Now choose any one from validIndex
			int32_t chosenIndex = validIndex[rand() % (int32_t)validIndex.size()];
			mCurrentLevel = mPsyParam.mStimLevels[chosenIndex];
			std::cout << "Next chosen MCS stim level is: " << mCurrentLevel << '\n';
		}
	}

	float SingleThresholdMeasurement::getProgressRatio()
	{
		if ((mPsyParam.mMeasuringMethod == DiscreteStaircase) || (mPsyParam.mMeasuringMethod == BucketStaircase)) // SC
		{
			// if maximum trial count reached, report 1
			if ((int)mResponses.size() >= mPsyParam.mMaxTotalTrialCount)
			{
				return 1.0f;
			}
			else
			{
				return (float)mReversalCount / (float)mPsyParam.mMaxReversals;
			}
		}
		else if (mPsyParam.mMeasuringMethod == MethodOfConstantStimuli) // MCS
		{
			int32_t totalCount = 0;
			int32_t totalMax = 0;
			for (int32_t i = 0; i < (int32_t)mPsyParam.mStimLevels.size(); i++)
			{
				totalCount = totalCount + mTrialCounts[i];
				totalMax = totalMax + mPsyParam.mMaxTrialCounts[i];
			}
			return (float)totalCount / (float)totalMax;
		}
		return 0; // shouldn't reach here...
	}

	bool SingleThresholdMeasurement::isComplete()
	{
		if ((mPsyParam.mMeasuringMethod == DiscreteStaircase) || (mPsyParam.mMeasuringMethod == BucketStaircase)) // SC
		{
			if ((mReversalCount >= mPsyParam.mMaxReversals) || ((int32_t)mResponses.size() >= mPsyParam.mMaxTotalTrialCount))
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		else if (mPsyParam.mMeasuringMethod == MethodOfConstantStimuli) // MCS
		{
			int32_t totalCount = 0;
			int32_t totalMax = 0;
			for (int32_t i = 0; i < (int32_t)mPsyParam.mStimLevels.size(); i++)
			{
				totalCount = totalCount + mTrialCounts[i];
				totalMax = totalMax + mPsyParam.mMaxTrialCounts[i];
			}
			if (((float)totalCount / (float)totalMax) == 1)
			{
				return true;
			}
			else
			{
				return false;
			}
		}
		return false; // shouldn't reach here
	}

}
