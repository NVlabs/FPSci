#include "KeyMapping.h"

// Default key mapping
KeyMapping::KeyMapping() {
	map.set("moveForward", Array<GKey>{ (GKey)'w', GKey::UP });
	map.set("strafeLeft", Array<GKey>{ (GKey)'a', GKey::LEFT });
	map.set("moveBackward", Array<GKey>{ (GKey)'s', GKey::DOWN });
	map.set("strafeRight", Array<GKey>{ (GKey)'d', GKey::RIGHT });
	map.set("openMenu", Array<GKey>{ GKey::ESCAPE });
	map.set("quit", Array<GKey>{ GKey::KP_MINUS, GKey::PAUSE });
	map.set("crouch", Array<GKey>{ GKey::LCTRL });
	map.set("jump", Array<GKey>{ GKey::SPACE });
	map.set("shoot", Array<GKey>{ GKey::LEFT_MOUSE });
	map.set("scope", Array<GKey>{ GKey::RIGHT_MOUSE});
	map.set("dummyShoot", Array<GKey>{ GKey::LSHIFT });
	map.set("dropWaypoint", Array<GKey>{ (GKey)'q' });
	map.set("toggleRecording", Array<GKey>{ (GKey)'r' });
	map.set("toggleRenderWindow", Array<GKey>{ (GKey)'1' });
	map.set("togglePlayerWindow", Array<GKey>{ (GKey)'2' });
	map.set("toggleWeaponWindow", Array<GKey>{ (GKey)'3' });
	map.set("toggleWaypointWindow", Array<GKey>{ (GKey)'4' });
	map.set("selectWaypoint", Array<GKey>{ GKey::LEFT_MOUSE });
	map.set("moveWaypointUp", Array<GKey>{ GKey::PAGEUP });
	map.set("moveWaypointDown", Array<GKey>{ GKey::PAGEDOWN });
	map.set("moveWaypointIn", Array<GKey>{ GKey::HOME });
	map.set("moveWaypointOut", Array<GKey>{ GKey::END });
	map.set("moveWaypointRight", Array<GKey>{ GKey::INSERT });
	map.set("moveWaypointLeft", Array<GKey>{ GKey::DELETE });
	map.set("reloadConfigs", Array<GKey>{GKey::F5});
	getUiKeyMapping();
};

KeyMapping::KeyMapping(const Any& any) : KeyMapping() {
	AnyTableReader reader = AnyTableReader(any);
	for (String actionName : map.getKeys()) {
		reader.getIfPresent(actionName, map[actionName]);
	}
	getUiKeyMapping();
}

KeyMapping KeyMapping::load(const String& filename) {
	if (!FileSystem::exists(System::findDataFile(filename, false))) {
		KeyMapping mapping = KeyMapping();
		mapping.toAny().save(filename, true);
		return mapping;
	}
	return Any::fromFile(System::findDataFile(filename));
}

Any KeyMapping::toAny(const bool forceAll) const {
	Any a(Any::TABLE);
	for (String actionName : map.getKeys()) {
		a[actionName] = map[actionName];
	}
	return a;
}

void KeyMapping::getUiKeyMapping() {
	// Bind keys to UIFunctions here
	uiMap.clear();
	for (GKey key : map["moveForward"])	uiMap.set(key, UserInput::UIFunction::UP);
	for (GKey key : map["strafeLeft"])	uiMap.set(key, UserInput::UIFunction::LEFT);
	for (GKey key : map["strafeRight"])	uiMap.set(key, UserInput::UIFunction::RIGHT);
	for (GKey key : map["moveBackward"])	uiMap.set(key, UserInput::UIFunction::DOWN);
}
