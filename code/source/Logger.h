#pragma once
#include <G3D/G3D.h>
#include <ctime>
#include "SingleThresholdMeasurement.h"
#include "sqlHelpers.h"

class Logger : public ReferenceCountedObject {
protected:
	sqlite3* m_db = nullptr;
public:
	Logger() : m_db(nullptr) {}
	void recordTrialResponse(Array<String> values);
	void closeResultsFile(void);
	static String genUniqueTimestamp();
	static String genFileTimestamp();
	virtual void addConditions(Array<SingleThresholdMeasurement> measurements) {};
	virtual void createResultsFile(String filename, String subjectID) {};
	virtual	void recordTargetTrajectory(Array<Array<String>> trajectory) {};
	virtual void recordPlayerActions(Array<Array<String>> actions) {};
	virtual void recordFrameInfo(Array<Array<String>> info) {};
};

class TargetLogger : public Logger
{
protected:
	TargetLogger() : Logger() {}
public:
	static shared_ptr<TargetLogger> create() {
		return createShared<TargetLogger>();
	}
	void addConditions(Array<SingleThresholdMeasurement> measurements);
	void createResultsFile(String filename, String subjectID);
	void recordTargetTrajectory(Array<Array<String>> trajectory);
	void recordPlayerActions(Array<Array<String>> actions);
	void recordFrameInfo(Array<Array<String>> info);
};

class ReactionLogger : public Logger
{
protected:
	ReactionLogger() : Logger() {}
public:
	static shared_ptr<ReactionLogger> create() {
		return createShared<ReactionLogger>();
	}
	void addConditions(Array<SingleThresholdMeasurement> measurements);
	void createResultsFile(String filename, String subjectID);
	void recordTargetTrajectory(Array<Array<String>> trajectory) {};
	void recordPlayerActions(Array<Array<String>> actions) {};
	void recordFrameInfo(Array<Array<String>> info) {};
};