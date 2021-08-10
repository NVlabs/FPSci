#pragma once
#include <G3D/G3D.h>

/** Class for handling user status */
class UserSessionStatus {
public:
	String					id = "anon";					///< User ID
	Array<String>			sessionOrder;					///< Array containing session ordering
	Array<String>			completedSessions;				///< Array containing all completed session ids for this user
	static Array<String>	defaultSessionOrder;			///< Default session order
	static bool				randomizeDefaults;				///< Randomize default session order when applying to individual?

	UserSessionStatus() {}
	UserSessionStatus(const Any& any);

	Any toAny(const bool forceAll = true) const;
};

/** Class for representing user status tables */
class UserStatusTable {
public:
	bool allowRepeat = false;							///< Flag for whether to (strictly) sequence these experiments (allow duplicates)
	bool randomizeDefaults = false;						///< Randomize from default session order when applying to user
	String currentUser;									///< Currently selected user
	Array<String> defaultSessionOrder = {};				///< Default session ordering (for all unspecified users)
	Array<UserSessionStatus> userInfo = {};				///< Array of user status

	UserStatusTable() {}
	UserStatusTable(const Any& any);

	static UserStatusTable load(const String& filename);
	Any toAny(const bool forceAll = false) const;

	inline void save(const String& filename) { toAny().save(filename, true); }

	shared_ptr<UserSessionStatus> getUserStatus(const String& id);			// Get a given user's status from the table by ID
	String getNextSession(String userId = "");								// Get the next session ID for a given user (by ID)
	void addCompletedSession(const String& userId, const String& sessId);	// Add a completed session to a given user's completedSessions array
	void validate(const Array<String>& sessions, const Array<String>& users);
	void printToLog() const;
};