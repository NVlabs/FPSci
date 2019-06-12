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
    static shared_ptr<Logger> create() {
        return createShared<Logger>();
    }
	void recordTrialResponse(Array<String> values);
	void closeResultsFile(void);
	static String genUniqueTimestamp();
	static String genFileTimestamp();
	void addConditions(Array<SingleThresholdMeasurement> measurements);
	void createResultsFile(String filename, String subjectID);
};