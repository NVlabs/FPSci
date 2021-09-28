#pragma once
#include <G3D/G3D.h>

/** Key mapping */
class KeyMapping {
public:
	Table<String, Array<GKey>> map;
	Table<GKey, UserInput::UIFunction> uiMap;

	KeyMapping();								// Default key mapping
	KeyMapping(const Any& any);					// From any

	static KeyMapping load(const String& filename, bool saveJSON = true);			// Load key map from file (or create default if not present)
	Any toAny(const bool forceAll = true) const;
	void getUiKeyMapping();									// Update the uiMap
};