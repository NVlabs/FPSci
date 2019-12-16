#include <G3D/G3D.h>

class KeyMapping {
public:
	// Default key mapping
	Array<GKey> dropWaypoint			= {(GKey)'q'};
	Array<GKey> toggleRecording			= {(GKey)'r'};
	Array<GKey> toggleRenderWindow		= {(GKey)'1'};
	Array<GKey> togglePlayerWindow		= {(GKey)'2'};
	Array<GKey> toggleWeaponWindow		= {(GKey)'3'};
	Array<GKey> toggleWaypointWindow	= {(GKey)'4'};
	Array<GKey> moveWaypointUp			= {GKey::PAGEUP};
	Array<GKey> moveWaypointDown		= {GKey::PAGEDOWN};
	Array<GKey> moveWaypointIn			= {GKey::HOME};
	Array<GKey> moveWaypointOut			= {GKey::END};
	Array<GKey> moveWaypointRight		= {GKey::INSERT};
	Array<GKey> moveWaypointLeft		= {GKey::DELETE};
	Array<GKey> openMenu				= {GKey::ESCAPE, GKey::TAB};
	Array<GKey> quit					= {GKey::MINUS};
	Array<GKey> crouch					= {GKey::LCTRL};
	Array<GKey> jump					= {GKey::SPACE};
	Array<GKey> shoot					= {GKey::LEFT_MOUSE};
	Array<GKey> dummyShoot				= {GKey::RSHIFT};

	KeyMapping() {};

	KeyMapping(const Any& any) {
		AnyTableReader reader = AnyTableReader(any);
		reader.getIfPresent("dropWaypoint", dropWaypoint);
		reader.getIfPresent("toggleRecording", toggleRecording);
		reader.getIfPresent("toggleRenderWindow", toggleRenderWindow);
		reader.getIfPresent("togglePlayerWindow", togglePlayerWindow);
		reader.getIfPresent("toggleWeaponWindow", toggleWeaponWindow);
		reader.getIfPresent("toggleWaypintWindow", toggleWaypointWindow);
		reader.getIfPresent("moveWaypointUp", moveWaypointUp);
		reader.getIfPresent("moveWaypointDown", moveWaypointDown);
		reader.getIfPresent("moveWaypointIn", moveWaypointIn);
		reader.getIfPresent("moveWaypointOut", moveWaypointOut);
		reader.getIfPresent("moveWaypointLeft", moveWaypointLeft);
		reader.getIfPresent("moveWaypointRight", moveWaypointRight);
		reader.getIfPresent("openMenu", openMenu);
		reader.getIfPresent("quit", quit);
		reader.getIfPresent("crouch", crouch);
		reader.getIfPresent("jump", jump);
		reader.getIfPresent("shoot", shoot);
		reader.getIfPresent("dummyShoot", dummyShoot);
	}

	Any toAny(const bool forceAll=true) const{
		Any a(Any::TABLE);
		a["dropWaypoint"] = dropWaypoint;
		a["toggleRecording"] = toggleRecording;
		a["toggleRenderWindow"] = toggleRenderWindow;
		a["togglePlayerWindow"] = togglePlayerWindow;
		a["toggleWeaponWindow"] = toggleWeaponWindow;
		a["toggleWaypintWindow"] = toggleWaypointWindow;
		a["moveWaypointUp"] = moveWaypointUp;
		a["moveWaypointDown"] = moveWaypointDown;
		a["moveWaypointIn"] = moveWaypointIn;
		a["moveWaypointOut"] = moveWaypointOut;
		a["moveWaypointLeft"] = moveWaypointLeft;
		a["moveWaypointRight"] = moveWaypointRight;
		a["openMenu"] = openMenu;
		a["quit"] = quit;
		a["crouch"] = crouch;
		a["jump"] = jump;
		a["shoot"] = shoot;
		a["dummyShoot"] = dummyShoot;
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

	// Option for friendly string-based conversion of GKey (will stick w/ default serializer for now...)
	//GKey stringToKey(String key) {
	//	key = toLower(key);
	//	if (key == "pageup") return GKey::PAGEUP;
	//	else if (key == "pagedown") return GKey::PAGEDOWN;
	//	else if (key == "home") return GKey::HOME;
	//	else if (key == "end") return GKey::END;
	//	else if (key == "insert") return GKey::INSERT;
	//	else if (key == "delete") return GKey::DELETE;
	//	else if (key == "esc") return GKey::ESCAPE;
	//	else if (key == "tab") return GKey::TAB;
	//	else if (key == "lctrl"||key == "leftctrl"||key == "lcontrol"||key == "leftcontrol") return GKey::LCTRL;
	//	else if (key == "rctrl"||key == "rightctrl"||key=="rcontrol"||key == "rightcontrol") return GKey::RCTRL;
	//	else if (key == "lshift"||key == "leftshift") return GKey::LSHIFT;
	//	else if (key == "rshift"||key == "rightshift") return GKey::RSHIFT;
	//	else return GKey::fromString(key);
	//}

};