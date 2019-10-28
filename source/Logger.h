#pragma once
#include <G3D/G3D.h>
#include <ctime>
#include "ParameterTable.h"
#include "ExperimentConfig.h"
#include "sqlHelpers.h"

/** Simple class to log data from trials */
class Logger : public ReferenceCountedObject {
protected:
	sqlite3* m_db = nullptr;						///< The db used for logging

public:
	
	Logger() : m_db(nullptr) {}
	
	static shared_ptr<Logger> create() {
		return createShared<Logger>();
	}

	/** Record a response for a trial */
	void recordTrialResponse(Array<String> values);

	/** Close the results file */
	void closeResultsFile(void);
	
	/** Generate a timestamp for logging */
	static String genUniqueTimestamp();
	
	/** Genearte a timestamp for filenames */
	static String genFileTimestamp();

	/** Add conditions to an experiment */
	void addTargets(Array<Array<ParameterTable>> targetParams);

	/** Create a results file */
	void createResultsFile(String filename, String subjectID, String description="None");

	/** Record an array of target locations */
	void recordTargetTrajectory(Array<Array<String>> trajectory);

	/** Record an array of player actions */
	void recordPlayerActions(Array<Array<String>> actions);

	/** Record an array of frame timing info */
	void recordFrameInfo(Array<Array<String>> info);
		
	/** Record a question and its response */
	void addQuestion(Question question, String session);
};