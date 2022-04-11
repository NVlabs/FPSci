#pragma once
#include <G3D/G3D.h>
#include "FPSciAnyTableReader.h"

class ReticleConfig {
public:
	int				index = 39;								///< Index of reticle to show in array
	Array<float>	scale = { 1.0f, 1.0f };					///< Scale for the reticle (min size, max size)
	Array<Color4>	color = { Color4(1.0, 0.0, 0.0, 1.0),	///< Color for the reticle (min size color, max size color)
							  Color4(1.0, 0.0, 0.0, 1.0) };
	float			changeTimeS = 0.3f;						///< Time for reticle to contract after expand on shot (in seconds)

	// Flags to store whether fields above were specified
	bool			indexSpecified = false;
	bool			scaleSpecified = false;
	bool			colorSpecified = false;
	bool			changeTimeSpecified = false;

	void load(FPSciAnyTableReader reader, int settingsVersion = 1);
	Any addToAny(Any a, bool forceAll = false) const;
};

/**Class for managing user configuration*/
class UserConfig {
public:
	String			id = "anon";								///< Subject ID (as recorded in output DB)
	double			mouseDPI = 800.0;							///< Mouse DPI setting
	double			mouseDegPerMm = 2.824;						///< Mouse sensitivity, reported as degree per mm
	Vector2			turnScale = Vector2(1.0f, 1.0f);			///< Turn scale for player, can be used to invert controls in either direction
	bool			invertY = false;							///< Extra flag for Y-invert (duplicates turn scale, but very common)
	Vector2			scopeTurnScale = Vector2(0.0f, 0.0f);		///< Scoped turn scale (0's imply default scaling)

	ReticleConfig   reticle;									///< Reticle configuration

	UserConfig() {};
	UserConfig(const Any& any);

	Any toAny(const bool forceAll = false) const;
	bool operator==(const UserConfig& other) const;
};

/** Class for loading a user table and getting user info */
class UserTable {
public:
	bool					requireUnique = true;			///< Require users to be unique by ID
	UserConfig				defaultUser;					///< Default user settings to use for new user
	Array<UserConfig>		users = {};						///< A list of valid users

	UserTable() {};
	UserTable(const Any& any);

	Any toAny(const bool forceAll = false) const;

	shared_ptr<UserConfig> getUserById(const String& id) const;				// Get a user config based on a user ID
	static UserTable load(const String& filename, bool saveJSON);			// Get the user config from file (or create it if it doesn't exist)
	
	inline void save(const String& filename, bool json) { toAny().save(filename, json); }	// Save to Any file

	Array<String> getIds() const;						// Get an array of user IDs
	int getUserIndex(String userId) const;				// Get the index of the current user from the user table
	void printToLog() const;							// Print the user table to the log

};