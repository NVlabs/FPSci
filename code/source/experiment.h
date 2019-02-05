#pragma once
#include <G3D/G3D.h>
#include "psychophysics.h"

namespace Psychophysics
{
	class EccentricityExperiment : public Experiment
	{
		/*
		This is an example experiment
		Timed TwoInterval experiment with 3 interleaved MCS experiments

		The update function progresses the experiment
		---> input arg is keypress string from G3D (can be NONE)
		*/
	protected:
		virtual void updateHelper(const std::string& keyInput, const FSM::State& pastState) override;

		// experimental setting
		class ConditionParams
		{
		public:
			// Initial displacement in visual field.
			// This is expressed in a 2D polar coordinate system as the following.
			// element0: Angle of direction in degrees, 0 being to the right and increasing in the counter-clock direction.
			// element1: Deviation from the front-shooting line, expressed in degrees.
			std::vector<G3D::Vector2> initialDisplacements;
			std::vector<float> visualSizes; // size of the target expressed as visual angle in degrees.
			
			// TODO: The following should ideally also be vectors. But that requires combo generation to be automated for arbitrary dimensions.
			// For the initial version I am going with one value per each parameter.
			//std::vector<float> speeds; // speed of motion expressed as visual angle (in degrees) per sec.
			//std::vector<float> motionChangeChances; // probability of motion direction change expressed as probability per sec.
			//std::vector<int> trialCounts; // how many trials per condition?
			//std::vector<double> timeLimitLevels; // time limit in a trial

			float speed; // speed of motion expressed as visual angle (in degrees) per sec.
			float motionChangeChance; // probability of motion direction change expressed as probability per sec.
			double readyDuration; // 'be ready' duration in sec.
			double feedbackDuration; // feedback duration in sec.
			std::vector<double> taskDurationLevels; // time limit in performing the task
			int trialCount; // how many trials per condition?
			std::string weaponType; // hitscan, tracking, projectile
			float weaponStrength; // dmg per shots (hitscan or projectile) or dmg per sec (tracking)
			float frameRate;
			int numFrameDelay;

			ConditionParams()
			{
				// TODO: Could we do the following in-place for the member variables of m_conditionParams?
				std::vector<G3D::Vector2> temp_initialDisplacements{ // in visual angles
					//G3D::Vector2(0.0f, 0.0f),
					G3D::Vector2(-45.0f, 15.0f),
					G3D::Vector2(0.0f, 20.0f),
					G3D::Vector2(45.0f, 15.0f),
					G3D::Vector2(135.0f, 15.0f),
					G3D::Vector2(180.0f, 20.0f),
					G3D::Vector2(225.0f, 15.0f),
				};
				initialDisplacements = temp_initialDisplacements;

				std::vector<float> temp_visualSizes = { // in visual angles
					1.f,
				};
				visualSizes = temp_visualSizes;

				//std::vector<double> temp_taskDurationLevels = { 0.5, 0.7, 1.0, 1.4, 2.0 };
				std::vector<double> temp_taskDurationLevels = { 100 }; // basically, infinite amount of time.
				taskDurationLevels = temp_taskDurationLevels;

				speed = 15.0f;
				motionChangeChance = 1.5f;
				trialCount = 5;
				readyDuration = 0.5;
				feedbackDuration = 0.3;

				//weaponType = "tracking"; // set by the app.
				weaponStrength = 1.f;
			}
		} m_conditionParams;

	public:
		/////// RENDER-RELEVANT PARAMS ///////
		struct RRP
		{
			// these will be updated per trial and sent to the shader
			G3D::Vector2 initialDisplacement; // initial displacement expressed in polar form (unit is deg in visual angle)
			float visualSize; // 3D position of the target
			double taskDuration;
			double readyDuration;
			double feedbackDuration;
			float speed;
			float motionChangeChance;
			std::string weaponType;
			float weaponStrength;
			std::string sceneType;
		};
		RRP renderParams;

		/////// REQUIRED FUNCTIONS ///////
		void setFrameRate(float fr);
		void setWeaponType(std::string wt);
		void setNumFrameDelay(int fd);
		void init(std::string subjectID, std::string expVersion, int sessionNum, std::string savePath, bool trainingMode) override;

		FSM::State getFSMState() override { return fsm->currState; };
		bool isExperimentDone();

		void startTimer() override { fsm->startTimer(); };
		float getTime() { int t = fsm->elapsedTimeMS(); return ((float)t) / 1000.0f; };

		void printDebugInfo();

		std::string getDebugStr();

		void updateRenderParamsForCurrentTrial();
	};
}