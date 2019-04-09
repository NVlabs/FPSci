#pragma once
#include <G3D/G3D.h>
#include "sqlHelpers.h"

class Logger : public ReferenceCountedObject {
protected:
	sqlite3* m_db = nullptr;
public:
	Logger() : m_db(nullptr) {}
	void recordTrialResponse(std::vector<std::string> values);
	void closeResultsFile(void);
	static String genUniqueTimestamp();
	virtual void addConditions(std::vector<SingleThresholdMeasurement> measurements);
	virtual void createResultsFile(String filename, String subjectID);
	virtual	void recordTargetTrajectory(std::vector<std::vector<std::string>> trajectory);
	virtual void recordPlayerActions(std::vector<std::vector<std::string>> actions);
};

class TargetLogger : public Logger
{
protected:
	TargetLogger() : Logger() {}
public:
	static shared_ptr<TargetLogger> create() {
		return createShared<TargetLogger>();
	}
	void addConditions(std::vector<SingleThresholdMeasurement> measurements);
	void createResultsFile(String filename, String subjectID);
	void recordTargetTrajectory(std::vector<std::vector<std::string>> trajectory);
	void recordPlayerActions(std::vector<std::vector<std::string>> actions);
};

class ReactionLogger : public Logger
{
protected:
	ReactionLogger() : Logger() {}
public:
	static shared_ptr<ReactionLogger> create() {
		return createShared<ReactionLogger>();
	}
	void addConditions(std::vector<SingleThresholdMeasurement> measurements);
	void createResultsFile(String filename, String subjectID);

};