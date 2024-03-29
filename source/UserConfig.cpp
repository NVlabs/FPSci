#include "UserConfig.h"
#include "FPSciAnyTableReader.h"

template <class T>
static bool operator!=(Array<T> a1, Array<T> a2) {
	for (int i = 0; i < a1.size(); i++) {
		if (a1[i] != a2[i]) return true;
	}
	return false;
}
template <class T>
static bool operator==(Array<T> a1, Array<T> a2) {
	return !(a1 != a2);
}

void ReticleConfig::load(FPSciAnyTableReader reader, int settingsVersion) {
	switch (settingsVersion) {
	case 1:
		if (reader.getIfPresent("reticleIndex", index)) indexSpecified = true;
		if (reader.getIfPresent("reticleScale", scale)) scaleSpecified = true;
		if (reader.getIfPresent("reticleColor", color)) colorSpecified = true;
		if (reader.getIfPresent("reticleChangeTime", changeTimeS)) changeTimeSpecified = true;
		break;
	default:
		throw format("Did not recognize settings version: %d", settingsVersion);
	}
}

Any ReticleConfig::addToAny(Any a, bool forceAll) const {
	ReticleConfig def;
	if (forceAll || def.index != index)				a["reticleIndex"] = index;
	if (forceAll || def.scale != scale)				a["reticleScale"] = scale;
	if (forceAll || def.color != color)				a["reticleColor"] = color;
	if (forceAll || def.changeTimeS != changeTimeS)	a["reticleChangeTime"] = changeTimeS;
	return a;
}

UserConfig::UserConfig(const Any& any) {
	// for loading old user configs
	double cmp360 = 12.75;
	bool foundMouseDegPerMm = false;

	int settingsVersion = 1; // used to allow different version numbers to be loaded differently
	FPSciAnyTableReader reader(any);
	reader.getIfPresent("settingsVersion", settingsVersion);
	switch (settingsVersion) {
	case 1:
		reader.getIfPresent("id", id);
		reader.getIfPresent("mouseDPI", mouseDPI);
		foundMouseDegPerMm = reader.getIfPresent("mouseDegPerMillimeter", mouseDegPerMm);
		reader.getIfPresent("cmp360", cmp360);
		reader.getIfPresent("turnScale", turnScale);
		reader.getIfPresent("invertY", invertY);
		reader.getIfPresent("scopeTurnScale", scopeTurnScale);
		reticle.load(reader, settingsVersion);
		break;
	default:
		debugPrintf("Settings version '%d' not recognized in UserConfig.\n", settingsVersion);
		break;
	}

	// Set mouseDPmm if not found
	if (!foundMouseDegPerMm) {
		mouseDegPerMm = 36.0 / cmp360;
	}
}

Any UserConfig::toAny(const bool forceAll) const {
	UserConfig def;
	Any a(Any::TABLE);
	a["id"] = id;										// Include subject ID
	a["mouseDPI"] = mouseDPI;							// Include mouse DPI
	a["mouseDegPerMillimeter"] = mouseDegPerMm;		    // Include sensitivity
	if (forceAll || def.turnScale != turnScale)						a["turnScale"] = turnScale;
	if (forceAll || def.invertY != invertY)							a["invertY"] = invertY;
	if (forceAll || def.scopeTurnScale != scopeTurnScale)			a["scopeTurnScale"] = scopeTurnScale;
	a = reticle.addToAny(a);
	return a;
}

static bool operator==(ReticleConfig r1, ReticleConfig r2) {
	return r1.index == r2.index && r1.scale == r2.scale && r1.color == r2.color && r1.changeTimeS == r2.changeTimeS;
}
bool UserConfig::operator==(const UserConfig& other) const {
	bool eq = id == other.id && mouseDegPerMm == other.mouseDegPerMm && reticle == other.reticle &&
		turnScale == other.turnScale && invertY == other.invertY && scopeTurnScale == other.scopeTurnScale;
	return eq;
}

UserTable:: UserTable(const Any& any) {
	int settingsVersion = 1;
	FPSciAnyTableReader reader(any);
	reader.getIfPresent("settingsVersion", settingsVersion);

	switch (settingsVersion) {
	case 1:
		reader.getIfPresent("requireUnique", requireUnique);
		reader.getIfPresent("defaultUser", defaultUser);
		reader.get("users", users, "Issue in the (required) \"users\" array in the user config file!");
		// Unique user check (if required)
		if (requireUnique) {
			const Array<String> userIds = getIds();
			for (String id : userIds) {
				if (userIds.findIndex(id) != userIds.rfindIndex(id)) {
					throw "Multiple users with the same ID (\"" + id + "\") specified in the user config file!";
				}
			}
		}
		if (users.size() == 0) {
			throw "At least 1 user must be specified in the \"users\" array within the user configuration file!";
		}
		break;
	default:
		debugPrintf("Settings version '%d' not recognized in UserTable.\n", settingsVersion);
		break;
	}
}

UserTable UserTable::load(const String& filename, bool saveJSON) {
	// Create default UserConfig file
	if (!FileSystem::exists(System::findDataFile(filename, false))) { // if file not found, generate a default user config table
		UserTable defTable = UserTable();
		defTable.users.append(UserConfig());			// Append one default user
		defTable.save(filename, saveJSON);				// Save the .any file
		return defTable;
	}
	return Any::fromFile(System::findDataFile(filename));
}

Any UserTable::toAny(const bool forceAll) const {
	Any a(Any::TABLE);
	a["settingsVersion"] = 1;						// Create a version 1 file
	if (forceAll || !(defaultUser == UserConfig())) a["defaultUser"] = defaultUser;					// Add default user
	a["users"] = users;								// Include updated subject table
	return a;
}

Array<String> UserTable::getIds() const {
	Array<String> ids;
	for (UserConfig user : users) ids.append(user.id);
	return ids;
}

shared_ptr<UserConfig> UserTable::getUserById(const String& id) const {
	for (UserConfig user : users) {
		if (!user.id.compare(id)) return std::make_shared<UserConfig>(user);
	}
	return nullptr;
}

int UserTable::getUserIndex(String userId) const {
	for (int i = 0; i < users.length(); ++i) {
		if (!users[i].id.compare(userId)) return i;
	}
	// return the first user by default
	return 0;
}

void UserTable::printToLog() const {
	for (UserConfig user : users) {
		logPrintf("\tUser ID: %s, sensitivity = %f deg/mm, mouseDPI = %d\n", user.id.c_str(), user.mouseDegPerMm, user.mouseDPI);
	}
}