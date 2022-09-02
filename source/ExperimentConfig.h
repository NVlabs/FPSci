#pragma once
#include <G3D/G3D.h>
#include "FpsConfig.h"
#include "TargetEntity.h"
#include "Session.h"

/** Experiment configuration */
class ExperimentConfig : public FpsConfig {
public:
	
	String description = "Experiment";					///< Experiment description
	Array<SessionConfig> sessions;						///< Array of sessions
	Array<TargetConfig> targets;						///< Array of trial configs   
	bool closeOnComplete = false;						///< Close application on all sessions complete
	String serverAddress = "";							///< Address for server
	int serverPort = 12345;								///< Port for server to listen to
	int clientPort = 12350;								///< Port for the client to listen to
	bool isNetworked;									///< Checks if the experiment is networked or not
	
	ExperimentConfig() { init(); }
	ExperimentConfig(const Any& any);

	void init();
	static ExperimentConfig load(const String& filename, bool saveJSON = false); // Get the experiment config from file
	Any toAny(const bool forceAll = false) const;

	void getSessionIds(Array<String>& ids) const;								// Get an array of session IDs
	shared_ptr<SessionConfig> getSessionConfigById(const String& id) const;		// Get a session config based on its ID
	int getSessionIndex(const String& id) const;								// Get the index of a session in the session array (by ID)
	shared_ptr<TargetConfig> getTargetConfigById(const String& id) const;		// Get a pointer to a target config by ID

	Array<Array<shared_ptr<TargetConfig>>> getTargetsByTrial(const String& id) const;			// Get target configs by trial (not recommended for repeated calls)
	Array<Array<shared_ptr<TargetConfig>>> getTargetsByTrial(int sessionIndex) const;			// Get target configs by trial (not recommended for repeated calls)
	Array<shared_ptr<TargetConfig>> getSessionTargets(const String& id) const;					// Get all targets affiliated with a session (not recommended for repeated calls)

	bool validate(bool throwException) const;									// Validate the session/target configuration

	void printToLog() const;													// Print the config to the log
};
