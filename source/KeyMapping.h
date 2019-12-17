#include <G3D/G3D.h>

class KeyMapping {
public:
	Table<String, Array<GKey>> map;

	// Default key mapping
	KeyMapping() {
		map.set("openMenu",				Array<GKey>{ GKey::ESCAPE, GKey::TAB });
		map.set("quit",					Array<GKey>{ GKey::MINUS });
		map.set("crouch",				Array<GKey>{ GKey::LCTRL });
		map.set("jump",					Array<GKey>{ GKey::SPACE });
		map.set("shoot",				Array<GKey>{ GKey::LEFT_MOUSE });
		map.set("dummyShoot",			Array<GKey>{ GKey::RSHIFT });
		map.set("dropWaypoint",			Array<GKey>{ (GKey)'q' });
		map.set("toggleRecording",		Array<GKey>{ (GKey)'r' });
		map.set("toggleRenderWindow",	Array<GKey>{ (GKey)'1' });
		map.set("togglePlayerWindow",	Array<GKey>{ (GKey)'2' });
		map.set("toggleWeaponWindow",	Array<GKey>{ (GKey)'3' });
		map.set("toggleWaypointWindow",	Array<GKey>{ (GKey)'4' });
		map.set("moveWaypointUp",		Array<GKey>{ GKey::PAGEUP });
		map.set("moveWaypointDown",		Array<GKey>{ GKey::PAGEDOWN });
		map.set("moveWaypointIn",		Array<GKey>{ GKey::HOME });
		map.set("moveWaypointOut",		Array<GKey>{ GKey::END });
		map.set("moveWaypointRight",	Array<GKey>{ GKey::INSERT });
		map.set("moveWaypointLeft",		Array<GKey>{ GKey::DELETE });
	};

	KeyMapping(const Any& any) : KeyMapping() {
		AnyTableReader reader = AnyTableReader(any);
		for (String actionName : map.getKeys()) {
			reader.getIfPresent(actionName, map[actionName]);
		}
	}

	Any toAny(const bool forceAll=true) const{
		Any a(Any::TABLE);
		for (String actionName : map.getKeys()) {
			a[actionName] = map[actionName];
		}
		return a;
	}

	static KeyMapping load(String filename="keymap.Any") {
		if (!FileSystem::exists(System::findDataFile(filename, false))) {
			KeyMapping mapping = KeyMapping();
			mapping.toAny().save("keymap.Any");
			return mapping;
		}
		return Any::fromFile(System::findDataFile(filename));
	}
};