#include "TargetExperiment.h"
#include "App.h"

namespace AbstractFPS
{
	void TargetExperiment::onInit() {
		// Add conditions.

		// load the experiment background scene.
		m_app->loadScene(m_app->m_experimentConfig.sceneName);
	}

	void TargetExperiment::onGraphics3D(RenderDevice* rd, Array<shared_ptr<Surface> >& surface)
	{

	}

	void TargetExperiment::onSimulation(RealTime rdt, SimTime sdt, SimTime idt)
	{

	}

	void TargetExperiment::onUserInput(UserInput* ui)
	{

	}

	void TargetExperiment::onGraphics2D(RenderDevice* rd, Array<shared_ptr<Surface2D>>& posed2D)
	{

	}

	void TargetExperiment::createResultFile()
	{
		// create a unique file name
		std::string timeStr = genUniqueTimestamp();
		mResultFileName = m_app->m_experimentConfig.taskType + "/" + m_app->m_user.subjectID + "_" + timeStr + ".db"; // we may include subject name here.
		// create the file

		// write column names

		// close the file
	}

	void TargetExperiment::recordTrialResponse()
	{

	}

	void TargetExperiment::closeResultFile()
	{

	}
}
