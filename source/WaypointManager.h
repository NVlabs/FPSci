#pragma once
#include <G3D/G3D.h>
#include "TargetEntity.h"
#include "GuiElements.h"

class FPSciApp;
class WaypointDisplay;

class WaypointManager : ReferenceCountedObject {
private:
	FPSciApp* m_app = nullptr;
	Scene* m_scene = nullptr;

	shared_ptr<WaypointDisplay> m_waypointControls;

	Array<Destination> m_waypoints;			///< Array of waypoints
	Array<DebugID> m_waypointIDs;
	Array<DebugID> m_arrowIDs;
	DebugID m_highlighted;
	int m_previewIdx = -1;

	shared_ptr<TargetEntity> m_previewTarget;

	const float m_waypointRad = 0.1f;
	const float m_waypointConnectRad = 0.02f;
	const Color4 m_waypointColor = Color4(0.0f, 1.0f, 0.0f, 1.0f);
	const Color4 m_highlightColor = Color4(1.0f, 1.0f, 0.0f, 1.0f);

	shared_ptr<TargetEntity> spawnDestTargetPreview(
		const Array<Destination>& dests,
		const float size,
		const Color3& color,
		const String& id,
		const String& name = ""
	);

	void destroyPreviewTarget();

public:
	WaypointManager() {}

	WaypointManager(FPSciApp* app) {
		m_app = app;
	}

	static shared_ptr<WaypointManager> create(FPSciApp* app) {
		return createShared<WaypointManager>(app);
	}

	float waypointDelay		 = 0.5f;		///< Delay between waypoints
	float waypointVertOffset = 0.2f;		///< Vertical offset from waypoint to camera position
	String exportFilename	= "target.Any";	///< Filename to export waypoints to
	
	bool recordMotion		= false;		///< Player motion recording
	int recordMode			= 0;			///< Recording mode
	float recordInterval	= 0.1f;			///< Recording interval (either time or distance)
	float recordTimeScaling = 1.0;			///< Time scaling for time-based recording
	RealTime recordStart	= nan();		///< Start time for recording
	float lastRecordTime	= 0.0;			///< Time storage for recording

	Vector3	 moveMask = Vector3::zero();					///< Mask for moving waypoints
	Vector3  moveRate = Vector3(0.01f, 0.01f, 0.01f);		///< Movement rate (m/s) for targets

	void dropWaypoint(Destination dest, Point3 offset=Point3::zero());
	void dropWaypoint(Point3 pos);
	void dropWaypoint();
	bool updateWaypoint(Destination dest, int idx=-1);
	bool removeWaypoint(int idx);
	void removeHighlighted();
	void removeLastWaypoint();
	void clearWaypoints();
	void exportWaypoints();
	void exportWaypoints(String filename);
	void setWaypoints(Array<Destination> waypoints);
	void previewWaypoints();
	void stopPreview();
	bool loadWaypoints(String filename);
	void loadWaypoints();
	bool aimSelectWaypoint(shared_ptr<Camera> cam);
	void updatePlayerPosition(Point3 pos);
	void updateSelected();
	void updateControls();


	void showWaypointWindow() {
		m_waypointControls->setVisible(true);
	}
	void toggleWaypointWindow() {
		m_waypointControls->setVisible(!m_waypointControls->visible());
	}
};