#include "WaypointManager.h"
#include "FPSciApp.h"

void WaypointManager::dropWaypoint(Destination dest, Point3 offset) {
	// Apply the offset
	dest.position += offset;

	// If this isn't the first point, connect it to the last one with a line
	if (m_waypoints.size() > 0) {
		shared_ptr<CylinderShape> shape = std::make_shared<CylinderShape>(CylinderShape(Cylinder(
			m_waypoints.last().position,
			dest.position,
			m_waypointConnectRad)));
		DebugID arrowID = debugDraw(shape, finf(), m_waypointColor, Color4::clear());
		m_arrowIDs.append(arrowID);
	}

	// Draw the waypoint (as a sphere)
	DebugID pointID = debugDraw(Sphere(dest.position, m_waypointRad), finf(), m_waypointColor, Color4::clear());

	// Update the arrays and time tracking
	m_waypoints.append(dest);
	m_waypointIDs.append(pointID);

	// Print to the log
	logPrintf("Dropped waypoint... Time: %f, XYZ:[%f,%f,%f]\n", dest.time, dest.position[0], dest.position[1], dest.position[2]);
}

void WaypointManager::dropWaypoint(Point3 pos) {
	SimTime t = m_waypoints.size() == 0 ? 0.0f : m_waypoints.last().time + waypointDelay;
	dropWaypoint(Destination(pos, t));
}

void WaypointManager::dropWaypoint() {
	Point3 pos = m_app->activeCamera()->frame().translation;
	dropWaypoint(pos);
}

bool WaypointManager::updateWaypoint(Destination dest, int idx) {
	// Check for valid indexing
	if (idx < 0) {
		idx = m_waypointControls->getSelected();	// Use the selected item if no index is passed in
	}
	else if (idx > m_waypoints.lastIndex())
		return false;							// If the index is too high don't update

	// Handle drawing highlight
	if (m_highlighted != m_waypointIDs[idx]) {
		m_app->removeDebugShape(m_highlighted);
		m_highlighted = debugDraw(Sphere(dest.position, m_waypointRad*1.1f), finf(), Color4::clear(), m_highlightColor);
	}

	// Skip points w/ no change
	if (dest.position == m_waypoints[idx].position) {
		return true;
	}

	// Array management
	m_waypoints[idx] = dest;

	// Remove the waypoint debugDraw, then replace it w/ a new one
	DebugID pointID = m_waypointIDs[idx];
	m_app->removeDebugShape(pointID);
	pointID = debugDraw(Sphere(dest.position, m_waypointRad), finf(), m_waypointColor, Color4::clear());
	m_waypointIDs[idx] = pointID;

	// Update the arrows around this point
	if (idx > 0 && idx < m_waypoints.lastIndex()) {
		shared_ptr<CylinderShape> toShape = std::make_shared<CylinderShape>(CylinderShape(Cylinder(
			m_waypoints[idx - 1].position,
			dest.position,
			m_waypointConnectRad)));
		shared_ptr<CylinderShape> fromShape = std::make_shared<CylinderShape>(CylinderShape(Cylinder(
			dest.position,
			m_waypoints[idx + 1].position,
			m_waypointConnectRad)));
		// Internal point (2 arrows to update)
		m_app->removeDebugShape(m_arrowIDs[idx - 1]);
		m_app->removeDebugShape(m_arrowIDs[idx]);
		m_arrowIDs[idx - 1] = debugDraw(toShape, finf(), m_waypointColor, Color4::clear());
		m_arrowIDs[idx] = debugDraw(fromShape, finf(), m_waypointColor, Color4::clear());
	}
	else if (idx == m_waypoints.lastIndex()) {
		shared_ptr<CylinderShape> toShape = std::make_shared<CylinderShape>(CylinderShape(Cylinder(
			m_waypoints[idx - 1].position,
			dest.position,
			m_waypointConnectRad)));
		// Last point (just remove "to" arrow)
		m_app->removeDebugShape(m_arrowIDs[idx - 1]);
		m_arrowIDs[idx - 1] = debugDraw(toShape, finf(), m_waypointColor, Color4::clear());
	}
	else if (idx == 0) {
		shared_ptr<CylinderShape> fromShape = std::make_shared<CylinderShape>(CylinderShape(Cylinder(
			dest.position,
			m_waypoints[idx + 1].position,
			m_waypointConnectRad)));
		// First point (just remove the "from" arrow)
		m_app->removeDebugShape(m_arrowIDs[idx]);
		m_arrowIDs[idx] = debugDraw(fromShape, finf(), m_waypointColor, Color4::clear());
	}
	return true;
}

bool WaypointManager::removeWaypoint(int idx) {
	// Check for valid idx
	if (idx >= 0 && idx < m_waypoints.size()) {
		// Check if we are not at the first or last point
		if (idx > 0 && idx < m_waypoints.lastIndex()) {
			// Remove the arrows to and from this point
			m_app->removeDebugShape(m_arrowIDs[idx]);
			m_app->removeDebugShape(m_arrowIDs[idx - 1]);
			m_arrowIDs.remove(idx - 1, 2);
			// Draw a new arrow "around" the point
			shared_ptr<CylinderShape> shape = std::make_shared<CylinderShape>(CylinderShape(Cylinder(
				m_waypoints[idx - 1].position,
				m_waypoints[idx + 1].position,
				m_waypointConnectRad)));
			DebugID arrowID = debugDraw(shape, finf(), m_waypointColor, Color4::clear());
			m_arrowIDs.insert(idx - 1, arrowID);
		}
		// Otherwise check if we are at the last index (just remove the arrow to this point)
		else if (idx == m_waypoints.lastIndex() && idx > 0) {
			// Remove the arrow from this point
			m_app->removeDebugShape(m_arrowIDs.last());
			m_arrowIDs.remove(m_arrowIDs.lastIndex());
		}

		// Remove the waypoint and its debug shape
		m_app->removeDebugShape(m_waypointIDs[idx]);
		m_waypointIDs.fastRemove(idx);
		m_waypoints.fastRemove(idx);

		// Get rid of the debug highlight and clear selection
		m_waypointControls->setSelected(-1);
		m_app->removeDebugShape(m_highlighted);
		return true;
	}
	else
		return false;
}

void WaypointManager::removeLastWaypoint(void) {
	if (m_waypoints.size() > 0) {
		removeWaypoint(m_waypoints.lastIndex());
	}
}

void WaypointManager::removeHighlighted(void) {
	// Remove the selected waypoint
	removeWaypoint(m_waypointControls->getSelected());
}

void WaypointManager::clearWaypoints(void) {
	m_waypoints.clear();
	for (DebugID id : m_waypointIDs) {
		m_app->removeDebugShape(id);
	}
	m_waypointIDs.clear();
	for (DebugID id : m_arrowIDs) {
		m_app->removeDebugShape(id);
	}
	m_arrowIDs.clear();
	// Clear the highlighted shape
	m_app->removeDebugShape(m_highlighted);
	m_waypointControls->setSelected(-1);
}

void WaypointManager::exportWaypoints(String filename, bool saveJSON) {
	TargetConfig t = TargetConfig();
	t.id = "test";
	t.destSpace = "world";
	t.destinations = m_waypoints;
	t.toAny().save(filename, saveJSON);		// Save the file
}

void WaypointManager::exportWaypoints(void) {
	exportWaypoints(exportFilename, m_app->startupConfig.jsonAnyOutput);
}

void WaypointManager::setWaypoints(Array<Destination> waypoints) {
	waypoints = waypoints;
	for (Destination d : waypoints) {
		dropWaypoint(d);
	}
}

shared_ptr<TargetEntity> WaypointManager::spawnDestTargetPreview(
	const Array<Destination>& dests,
	const float size,
	const Color3& color,
	const String& id,
	const String& name)
{
	// Create the target
	const String nameStr = name.empty() ? format("destPreview") : name;
	const int scaleIndex = clamp(iRound(log(size) / log(1.0f + TARGET_MODEL_ARRAY_SCALING) + TARGET_MODEL_ARRAY_OFFSET), 0, TARGET_MODEL_SCALE_COUNT - 1);
	const shared_ptr<TargetEntity>& target = TargetEntity::create(dests, nameStr, m_app->scene().get(), m_app->targetModels[id][scaleIndex], scaleIndex, 0);

	// Setup (additional) target parameters
	target->setFrame(dests[0].position);
	target->setColor(color);

	// Add target to array and scene
	m_app->scene()->insert(target);
	return target;
}

void WaypointManager::destroyPreviewTarget() {
	if (isNull(m_previewTarget)) return;
	m_app->scene()->removeEntity(m_previewTarget->name());
	m_previewTarget.reset();
}

void WaypointManager::previewWaypoints(void) {
	// Check if a preview target exists, if so remove it
	destroyPreviewTarget();
	if (m_waypoints.size() > 1) {
		// Create a new target and set its index
		m_previewTarget = spawnDestTargetPreview(m_waypoints, 1.0, Color3::white(), "reference", "preview");
	}
}

void WaypointManager::stopPreview(void) {
	destroyPreviewTarget();
}

bool WaypointManager::loadWaypoints(String filename) {
	recordMotion = false;		// Stop recording (if doing so)
	clearWaypoints();			// Clear the current waypoints

	TargetConfig t = TargetConfig::load(filename);	// Load the target config
	if (t.destinations.size() > 0) {
		setWaypoints(t.destinations);
	}

	if (t.destSpace == "player") {
		CFrame f = m_app->playerCamera->frame();
		Point3 offset = f.pointToWorldSpace(Point3(0, 0, -1.0f));
		// Adjust player space target destinations for preview
		for (Destination& d : m_waypoints) {
			d.position += offset;	// Add current camera frame as offset
		}
	}
	return true;
}

void WaypointManager::loadWaypoints(void) {
	String fname;
	bool gotName = FileDialog::getFilename(fname, "Any", false);
	if (!gotName) return;
	loadWaypoints(fname);
}

bool WaypointManager::aimSelectWaypoint(shared_ptr<Camera> cam) {
	float closest = 1e6;
	int closestIdx = -1;
	// Crude hit-scane here w/ spheres
	for (int i = 0; i < m_waypoints.size(); i++) {
		Sphere pt = Sphere(m_waypoints[i].position, m_waypointRad);
		float distance = (m_waypoints[i].position - cam->frame().translation).magnitude();	// Get distance to the target
		const Point3 center = cam->frame().translation + cam->frame().lookRay().direction()*distance;
		Sphere probe = Sphere(center, m_waypointRad / 4);
		// Get closest intersection
		if (pt.intersects(probe) && distance < closest) {
			closestIdx = i;
			closest = distance;
		}
	}
	if (closestIdx != -1) {							// We are "hitting" this item
		m_waypointControls->setSelected(closestIdx);
		return true;
	}
	return false;
}

void WaypointManager::updateSelected() {
	int idx = m_waypointControls->getSelected();
	if (idx >= 0 && idx < m_waypoints.length()) {
		Destination dest = m_waypoints[idx];
		dest.position += (moveRate * moveMask);
		updateWaypoint(dest);
	}
}

void WaypointManager::updatePlayerPosition(Point3 pos) {
	if (recordMotion) {
		RealTime now = System::time();
		if (isnan(recordStart)) {
			recordStart = now;
			clearWaypoints();
			dropWaypoint(Destination(pos, 0.0f));
		}
		else {
			SimTime t = static_cast<SimTime>(now - recordStart);
			float distance = (m_waypoints.last().position - pos).magnitude();
			switch (recordMode) {
			case 0: // Fixed distance
				if (distance > recordInterval) {
					t = m_waypoints.size() * waypointDelay;
					dropWaypoint(Destination(pos, t));
				}
				break;
			case 1: // Fixed time
				if ((t - lastRecordTime) > recordInterval) {
					lastRecordTime = t;
					dropWaypoint(Destination(pos, t / recordTimeScaling));
				}
				break;
			default:
				throw format("Did not recognize waypoint record mode: %d", recordMode);
				break;
			}
		}
	}
	else {
		recordStart = nan();
		lastRecordTime = 0.0f;
	}
}

void WaypointManager::updateControls(){
	// Setup the waypoint config/display
	WaypointDisplayConfig config = WaypointDisplayConfig();
	if (notNull(m_waypointControls)) m_app->removeWidget(m_waypointControls);
	m_waypointControls = WaypointDisplay::create(m_app, m_app->theme, config, (shared_ptr<Array<Destination>>)&m_waypoints);
	m_waypointControls->setVisible(false);
	m_app->addWidget(m_waypointControls);
}
