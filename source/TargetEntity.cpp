#include "TargetEntity.h"

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

TargetConfig::TargetConfig(const Any& any) {
	int settingsVersion = 1;
	FPSciAnyTableReader reader(any);
	reader.getIfPresent("settingsVersion", settingsVersion);

	switch (settingsVersion) {
	case 1:
		reader.get("id", id, "An \"id\" field must be provided for every target config!");
		//reader.getIfPresent("elevationLocked", elevLocked);
		reader.getIfPresent("upperHemisphereOnly", upperHemisphereOnly);
		reader.getIfPresent("logTargetTrajectory", logTargetTrajectory);
		reader.getIfPresent("distance", distance);
		reader.getIfPresent("motionChangePeriod", motionChangePeriod);
		reader.getIfPresent("speed", speed);
		reader.getIfPresent("visualSize", size);
		reader.getIfPresent("symmetricEccH", symmetricEccH);
		reader.getIfPresent("symmetricEccV", symmetricEccV);
		reader.getIfPresent("eccH", eccH);
		reader.getIfPresent("eccV", eccV);
		reader.getIfPresent("jumpEnabled", jumpEnabled);
		reader.getIfPresent("jumpSpeed", jumpSpeed);
		reader.getIfPresent("jumpPeriod", jumpPeriod);
		reader.getIfPresent("accelGravity", accelGravity);
		reader.getIfPresent("modelSpec", modelSpec);

		reader.getIfPresent("destroyDecal", destroyDecal);
		reader.getIfPresent("destroyDecalScale", destroyDecalScale);
		reader.getIfPresent("destroyDecalDuration", destroyDecalDuration);

		reader.getIfPresent("destSpace", destSpace);
		reader.getIfPresent("destinations", destinations);
		reader.getIfPresent("respawnCount", respawnCount);
		if (destSpace == "world" && destinations.size() == 0) {
			reader.get("moveBounds", moveBounds, format("A world-space target must either specify destinations or a movement bounding box. See target: \"%s\"", id));
			spawnBounds = moveBounds;
		}
		else {
			if (reader.getIfPresent("moveBounds", moveBounds)) {
				spawnBounds = moveBounds;
			}
		}
		reader.getIfPresent("spawnBounds", spawnBounds);
		if (destSpace == "world" && destinations.size() == 0 && !moveBounds.contains(spawnBounds)) {
			String moveBoundStr = format("AABox{%s, %s}", moveBounds.high().toString(), moveBounds.low().toString());
			String spawnBoundStr = format("AABox{%s, %s}", spawnBounds.high().toString(), spawnBounds.low().toString());
			throw format("The \"moveBounds\" AABox (=%s) must contain the \"spawnBounds\" AABox (=%s)!", moveBoundStr, spawnBoundStr);
		}
		if (reader.getIfPresent("axisLocked", axisLock)) {
			if (axisLock.size() < 3) {
				throw format("Must provide 3 fields (X,Y,Z) for axis lock! Only %d provided! See target: \"%s\"", axisLock.size(), id);
			}
			else if (axisLock.size() > 3) {
				logPrintf("Provided axis lock for target \"%s\" has >3 fields, using the first 3...", id);
			}
			if (axisLock[0] && axisLock[1] && axisLock[2] && speed[0] != 0.0f && speed[1] != 0.0f) {
				throw format("Target \"%s\" locks all axes but has non-zero speed!", id);
			}
		}
		reader.getIfPresent("hitSound", hitSound);
		reader.getIfPresent("hitSoundVol", hitSoundVol);
		reader.getIfPresent("destroyedSound", destroyedSound);
		reader.getIfPresent("destroyedSoundVol", destroyedSoundVol);
		reader.getIfPresent("colors", colors);
		reader.getIfPresent("emissive", emissive);
		hasGloss = reader.getIfPresent("gloss", gloss);

		break;
	default:
		debugPrintf("Settings version '%d' not recognized in TargetConfig.\n", settingsVersion);
		break;
	}
}

TargetConfig TargetConfig::load(const String& filename) {
	return TargetConfig(Any::fromFile(System::findDataFile(filename)));
}

Any TargetConfig::toAny(const bool forceAll) const {
	Any a(Any::TABLE);
	TargetConfig def;
	a["id"] = id;
	if (forceAll || !(def.modelSpec == modelSpec))							a["modelSpec"] = modelSpec;
	if (forceAll || def.destSpace != destSpace)								a["destSpace"] = destSpace;
	if (forceAll || def.respawnCount != respawnCount)						a["respawnCount"] = respawnCount;
	if (forceAll || def.size != size)										a["visualSize"] = size;
	if (forceAll || def.logTargetTrajectory != logTargetTrajectory)			a["logTargetTrajectory"] = logTargetTrajectory;
	// Destination-based target
	if (destinations.size() > 0) 											a["destinations"] = destinations;
	// Parametric target
	else {
		if (forceAll || def.upperHemisphereOnly != upperHemisphereOnly)		a["upperHemisphereOnly"] = upperHemisphereOnly;
		if (forceAll || def.distance != distance)							a["distance"] = distance;
		if (forceAll || def.motionChangePeriod != motionChangePeriod)		a["motionChangePeriod"] = motionChangePeriod;
		if (forceAll || def.speed != speed)									a["speed"] = speed;
		if (forceAll || def.symmetricEccH != symmetricEccH)					a["symmetricEccH"] = symmetricEccH;
		if (forceAll || def.symmetricEccV != symmetricEccV)					a["symmetricEccV"] = symmetricEccV;
		if (forceAll || def.eccH != eccH)									a["eccH"] = eccH;
		if (forceAll || def.eccV != eccV)									a["eccV"] = eccV;
		if (forceAll || def.jumpEnabled != jumpEnabled)						a["jumpEnabled"] = jumpEnabled;
		if (forceAll || def.jumpPeriod != jumpPeriod)						a["jumpPeriod"] = jumpPeriod;
		if (forceAll || def.accelGravity != accelGravity)					a["accelGravity"] = accelGravity;
		if (forceAll || def.axisLock != axisLock)							a["axisLocked"] = axisLock;
	}

	if (forceAll || def.destroyDecal != destroyDecal)						a["destroyDecal"] = destroyDecal;
	if (forceAll || def.destroyDecalScale != destroyDecalScale)				a["destroyDecalScale"] = destroyDecalScale;
	if (forceAll || def.destroyDecalDuration != destroyDecalDuration)		a["destroyDecalDuration"] = destroyDecalDuration;

	if (forceAll || def.hitSound != hitSound)								a["hitSound"] = hitSound;
	if (forceAll || def.hitSoundVol != hitSoundVol)							a["hitSoundVol"] = hitSoundVol;
	if (forceAll || def.destroyedSound != destroyedSound)					a["destroyedSound"] = destroyedSound;
	if (forceAll || def.destroyedSoundVol != destroyedSoundVol)				a["destroyedSoundVol"] = destroyedSoundVol;
	return a;
};

// Find an arbitrary vector perpendicular to and in equal length as inputV.
// The sampling distribution is uniform along the circular line, the set of possible candidates of a perpendicular vector.,
Point3 findPerpendicularVector(Point3 inputV) { // Note that the output vector has equal length as the input vector.
	Point3 perpen;
	while (true) {
		Point3 r = Point3::random();
		if (r.dot(inputV) > 0.1) { // avoid r being sharply aligned with the position vector
			// calculate a perpendicular vector
			perpen = r.cross(inputV.direction()) * inputV.length();
			break;
		}
	}
	return perpen;
}

// rotate the inputV toward destinationV by angle ang_deg.
Point3 rotateToward(Point3 inputV, Point3 destinationV, float ang_deg) {
	const float projection = inputV.direction().dot(destinationV.direction());
	Point3 U = inputV.direction();
	Point3 V = (destinationV.direction() - inputV * projection).direction();

	return (cos(ang_deg * pif() / 180.0f) * U + sin(ang_deg * pif() / 180.0f) * V) * inputV.length();
}

shared_ptr<TargetEntity> TargetEntity::create(	
	Array<Destination>				dests, 										
	const String&					name,
	Scene*							scene,
	const shared_ptr<Model>&		model,
	int								scaleIdx,
	int								paramIdx,
	bool							isLogged)
{
	const shared_ptr<TargetEntity>& target = createShared<TargetEntity>();
	target->Entity::init(name, scene, CFrame(dests[0].position), shared_ptr<Entity::Track>(), true, true);
	target->VisibleEntity::init(model, true, Surface::ExpressiveLightScatteringProperties(), ArticulatedModel::PoseSpline());
	target->TargetEntity::init(dests, paramIdx, Point3::zero(), 0, scaleIdx, isLogged);
	return target;
}

shared_ptr<TargetEntity> TargetEntity::create(
	shared_ptr<TargetConfig>		config,
	const String&					name,
	Scene*							scene,
	const shared_ptr<Model>&		model,
	const Point3&					offset,
	int								scaleIdx,
	int								paramIdx) 
{
	const shared_ptr<TargetEntity>& target = createShared<TargetEntity>();
	target->Entity::init(name, scene, CFrame(config->destinations[0].position), shared_ptr<Entity::Track>(), true, true);
	target->VisibleEntity::init(model, true, Surface::ExpressiveLightScatteringProperties(), ArticulatedModel::PoseSpline());
	target->TargetEntity::init(config->destinations, paramIdx, offset, config->respawnCount, scaleIdx, config->logTargetTrajectory);
	target->m_id = config->id;
	return target;
}

void TargetEntity::drawHealthBar(RenderDevice* rd, const Camera& camera, const Framebuffer& framebuffer, Point2 size, Point3 offset, Point2 border, Array<Color4> colors, Color4 borderColor) const
{
	// Abort if the target is not in front of the camera 
	Vector3 diffVector = frame().translation - camera.frame().translation;
	if (camera.frame().lookRay().direction().dot(diffVector) < 0.0f) {
		return;
	}

	// Project entity position into image space
	Rect2D viewport = Rect2D(framebuffer.vector2Bounds());
	Point3 hudPoint = camera.project(frame().translation, viewport);

	// Abort if the target is not in the view frustum
	if (hudPoint == Point3::inf()) {
		return;
	}
	hudPoint += offset;		// Apply offset in pixels

	// Draws a bar
	const Color4 color = colors[1]*(1.0f-m_health) + colors[0]*m_health;

	Draw::rect2D(
		Rect2D::xywh(hudPoint.xy() - size * 0.5f - border, size + border + border), rd, borderColor
	);
	Draw::rect2D(
		Rect2D::xywh(hudPoint.xy() - size * 0.5f, size*Point2(m_health, 1.0f)), rd, color
	);

}

void TargetEntity::setDestinations(const Array<Destination> destinationArray) {
	m_destinations = destinationArray;
}

void TargetEntity::onSimulation(SimTime absoluteTime, SimTime deltaTime) {
	// Check whether we have any destinations yet...
	if (m_destinations.size() < 2) {
		setFrame(m_destinations[0].position + m_offset);
		return;
	}

	if (m_spawnTime == 0) m_spawnTime = absoluteTime;						// Get a spawn time (if we don't have one already)
	SimTime time = fmod(absoluteTime-m_spawnTime, getPathTime());			// Compute a local time (modulus the path time)
	
	int nextDestIdx = (destinationIdx + 1) % m_destinations.size();			// Get the next destination's index

	// Check if its time to move to the next segment
	while(time < m_destinations[destinationIdx].time || time >= m_destinations[nextDestIdx].time) {
		destinationIdx = nextDestIdx;										// Increment the desintation index
		nextDestIdx = (destinationIdx + 1) % m_destinations.size();			// Update next destination index
	}
	
	// Get the current and next destination index
	Destination currDest = m_destinations[destinationIdx];
	Destination nextDest = m_destinations[nextDestIdx];

	// Compute the position by interpolating
	float duration = nextDest.time - currDest.time;			// Get the total time for this "step"
	float prog = 0.0f;										// Use no progress for the "wrap" case (duration < 0 above)
	if (duration > 0.0f) {
		prog = 1 - ((nextDest.time - time) / duration);		// Get the ratio of time in this step completed
	}
	
	Point3 delta = nextDest.position - currDest.position; 	// Get the delta vector to move along
	setFrame((prog*delta) + currDest.position + m_offset);	// Set the new positions

	// Set changed time if target moved
	if (delta != Point3(0.f, 0.f, 0.f)) {
		m_lastChangeTime = System::time();
	}

#ifdef DRAW_BOUNDING_SPHERES
	// Draw a 1m sphere at this position
	debugDraw(Sphere(m_frame, BOUNDING_SPHERE_RADIUS), 0.0f, Color4::clear(), Color3::black());
#endif
}

shared_ptr<Entity> FlyingEntity::create(
	const String&                  name,
	Scene*                         scene,
	AnyTableReader&                propertyTable,
	const ModelTable&              modelTable,
	const Scene::LoadOptions&      loadOptions) 
{
	// Don't initialize in the constructor, where it is unsafe to throw Any parse exceptions
	const shared_ptr<FlyingEntity>& flyingEntity = createShared<FlyingEntity>();

	// Initialize each base class, which parses its own fields
	flyingEntity->Entity::init(name, scene, propertyTable);
	flyingEntity->VisibleEntity::init(propertyTable, modelTable);
	flyingEntity->FlyingEntity::init(propertyTable);

	// Verify that all fields were read by the base classes
	propertyTable.verifyDone();

	return flyingEntity;
}


shared_ptr<FlyingEntity> FlyingEntity::create(
	const String&                           name,
	Scene*                                  scene,
	const shared_ptr<Model>&                model,
	const CFrame&                           position) {

	// Don't initialize in the constructor, where it is unsafe to throw Any parse exceptions
	const shared_ptr<FlyingEntity>& flyingEntity = createShared<FlyingEntity>();

	// Initialize each base class, which parses its own fields
	flyingEntity->Entity::init(name, scene, position, shared_ptr<Entity::Track>(), true, true);
	flyingEntity->VisibleEntity::init(model, true, Surface::ExpressiveLightScatteringProperties(), ArticulatedModel::PoseSpline());
	flyingEntity->FlyingEntity::init();

	return flyingEntity;
}

shared_ptr<FlyingEntity> FlyingEntity::create(
	shared_ptr<TargetConfig>		config,
	const String&					name,
	Scene*							scene,
	const shared_ptr<Model>&		model,
	const Point3&					orbitCenter,
	int								scaleIdx,
	int								paramIdx)
{
	// Don't initialize in the constructor, where it is unsafe to throw Any parse exceptions
	const shared_ptr<FlyingEntity>& flyingEntity = createShared<FlyingEntity>();

	// Initialize each base class, which parses its own fields
	flyingEntity->Entity::init(name, scene, CFrame(), shared_ptr<Entity::Track>(), true, true);
	flyingEntity->VisibleEntity::init(model, true, Surface::ExpressiveLightScatteringProperties(), ArticulatedModel::PoseSpline());
	flyingEntity->FlyingEntity::init(
		{ config->speed[0], config->speed[1] }, 
		{ config->motionChangePeriod[0], config->motionChangePeriod[1] },
		config->upperHemisphereOnly, 
		orbitCenter, 
		paramIdx, 
		config->axisLock, 
		config->respawnCount, 
		scaleIdx, 
		config->logTargetTrajectory);
	flyingEntity->m_id = config->id;
	return flyingEntity;
}



void FlyingEntity::init(AnyTableReader& propertyTable) {
	//TODO: implement load from any file here...
	init();
}


void FlyingEntity::init() {
}


void FlyingEntity::init(Vector2 angularSpeedRange, Vector2 motionChangePeriodRange, bool upperHemisphereOnly, Point3 orbitCenter, int paramIdx, Array<bool> axisLock, int respawns, int scaleIdx, bool isLogged) {
	m_angularSpeedRange = angularSpeedRange;
	m_motionChangePeriodRange = motionChangePeriodRange;
	m_upperHemisphereOnly = upperHemisphereOnly;
	m_orbitCenter = orbitCenter;
	m_paramIdx = paramIdx;
	m_respawnCount = respawns;
	m_scaleIdx = scaleIdx;
	m_isLogged = isLogged;
	alwaysAssertM(axisLock.size() == 3, "Axis lock must have size 3!");
	for (int i = 0; i < 3; i++) {
		m_axisLocks[i] = axisLock[i];
	}
}

void FlyingEntity::setDestinations(const Array<Point3>& destinationArray, const Point3 orbitCenter) {
	m_destinationPoints.fastClear();
	if (destinationArray.size() > 0) {
		const float distance = (destinationArray[0] - orbitCenter).length();

		// Insert all points, ensuring that they maintain a constant radius about the
		Vector3 previousDirection = (destinationArray[0] - orbitCenter).direction();
		for (const Point3& P : destinationArray) {
			const Vector3& direction = (P - orbitCenter).direction();
			m_destinationPoints.pushBack(direction * distance + orbitCenter);

			alwaysAssertM(direction.dot(previousDirection) > cos(170 * units::degrees()),
				"Destinations must be separated by no more than 170 degrees");
			previousDirection = direction;
		}

	}

	m_orbitCenter = orbitCenter;
}


Any FlyingEntity::toAny(const bool forceAll) const {
	Any a = VisibleEntity::toAny(forceAll);
	a.setName("FlyingEntity");

	// a["velocity"] = m_velocity;

	return a;
}

void FlyingEntity::onSimulation(SimTime absoluteTime, SimTime deltaTime) {
	// Do not call Entity::onSimulation; that will override with spline animation

	if (!(isNaN(deltaTime) || (deltaTime == 0))) { // first frame?
		m_previousFrame = m_frame;
	}

	simulatePose(absoluteTime, deltaTime);

	if (m_worldSpace) {
		Point3 pos = m_frame.translation;
		// Handle world-space target here
		// Check for change in direction
		if (absoluteTime > m_nextChangeTime) {
			// Update the next change time
			float motionChangeTime = Random::common().uniform(m_motionChangePeriodRange[0], m_motionChangePeriodRange[1]);
			m_nextChangeTime = absoluteTime + motionChangeTime;
			// Velocity to use for this next interval
			float vel = Random::common().uniform(m_angularSpeedRange[0], m_angularSpeedRange[1]);
			Point3 destination = m_bounds.randomInteriorPoint();
			if (m_axisLocks[0]) {
				destination.x = pos.x;
			}
			if (m_axisLocks[1]) {
				destination.y = pos.y;
			}
			if (m_axisLocks[2]) {
				destination.z = pos.z;
			}
			if (m_axisLocks[0] && m_axisLocks[1] && m_axisLocks[2] && vel > 0) {
				throw "Cannot lock all axes for non-static target!";
			}
			m_velocity = vel * (destination - m_frame.translation).direction();
		}
		// Check for whether the target has "left" the bounds, if so "reflect" it about the wall
		else if (!m_bounds.contains(pos)) {
			if (pos.x >= m_bounds.high().x) {
				m_velocity.x = -abs(m_velocity.x);
			}
			else if (pos.x <= m_bounds.low().x) {
				m_velocity.x = abs(m_velocity.x);
			}
			if (pos.y >= m_bounds.high().y) {
				m_velocity.y = -abs(m_velocity.y);
			}
			else if (pos.y <= m_bounds.low().y) {
				m_velocity.y = abs(m_velocity.y);
			}
			if (pos.z >= m_bounds.high().z) {
				m_velocity.z = -abs(m_velocity.z);
			}
			else if (pos.z <= m_bounds.low().z) {
				m_velocity.z = abs(m_velocity.z);
			}
		}

		// Update the position and set the frame
		pos += m_velocity*deltaTime;		
		setFrame(pos);

		// Set changed time if it moved
		if (m_velocity != Vector3(0.f, 0.f, 0.f)) {
			m_lastChangeTime = System::time();
		}
	}
	else {
		// Handle non-world space (player projection here)
		while ((deltaTime > 0.000001f) && m_angularSpeedRange[0] > 0.0f) {
			if (m_destinationPoints.empty()) {
				// Add destimation points if no destination points.
				float motionChangePeriod = Random::common().uniform(m_motionChangePeriodRange[0], m_motionChangePeriodRange[1]);
				float angularSpeed = Random::common().uniform(m_angularSpeedRange[0], m_angularSpeedRange[1]);
				float angularDistance = motionChangePeriod * angularSpeed;
				angularDistance = angularDistance > 170.f ? 170.0f : angularDistance; // replace with 170 deg if larger than 170.

				// [m/s] = [m/radians] * [radians/s]
				const float radius = (m_frame.translation - m_orbitCenter).length();
				m_speed = radius * (angularSpeed * pif() / 180.0f);

				// relative position to orbit center
				Point3 relPos = m_frame.translation - m_orbitCenter;
				// find a vector perpendicular to the current position
				Point3 perpen = findPerpendicularVector(relPos);
				// calculate destination point
				Point3 dest = m_orbitCenter + rotateToward(relPos, perpen, angularDistance);
				// add destination point.
				m_destinationPoints.pushBack(dest);
			}

			if ((m_frame.translation - m_destinationPoints[0]).length() < 0.001f) {
				// Retire this destination. We are almost at the destination (linear and geodesic distances 
				// are the same when small), and the following math will be numerically imprecise if we
				// use such a close destination.
				m_destinationPoints.popFront();
			}
			else {
				const Point3 destinationPoint = m_destinationPoints[0];
				const Point3 currentPoint = m_frame.translation;

				// Transform to directions
				const float radius = (destinationPoint - m_orbitCenter).length();
				const Vector3& destinationVector = (destinationPoint - m_orbitCenter).direction();
				const Vector3& currentVector = (currentPoint - m_orbitCenter).direction();

				// The direction is always "from current to destination", so we can use acos here
				// and not worry about it being unsigned.
				const float projection = currentVector.dot(destinationVector);
				const float destinationAngle = G3D::acos(projection);

				// [radians/s] = [m/s] / [m/radians]
				const float angularSpeed = m_speed / radius;

				// [rad] = [rad/s] * [s] 
				float angleChange = angularSpeed * deltaTime;

				if (angleChange > destinationAngle) {
					// We'll reach the destination before the time step ends.
					// Record how much time was consumed by this step.
					deltaTime -= destinationAngle / angularSpeed;
					angleChange = destinationAngle;
					m_destinationPoints.popFront();
				}
				else {
					// Consumed the entire time step
					deltaTime = 0;
				}

				// Transform to spherical coordinates in the plane of the arc
				const Vector3& U = currentVector;
				const Vector3& V = (destinationVector - currentVector * projection).direction();

				setFrame(m_orbitCenter + (cos(angleChange) * U + sin(angleChange) * V) * radius);

				// Set changed time if it moved
				if (angleChange != 0.f) {
					m_lastChangeTime = System::time();
				}
			}

			if (m_upperHemisphereOnly) {
				// Target position must be always above the orbit horizon (plane defined by "y = m_orbitCenter.y")
				// If target is below the orbit horizon, y-invert position & destination points w.r.t. the orbit horizon.
				if (m_frame.translation.y < m_orbitCenter.y) {
					m_frame.translation.y = m_orbitCenter.y + (m_orbitCenter.y - m_frame.translation.y);
					for (int i = 0; i < m_destinationPoints.length(); ++i) { // iterate by the number of elements in m_destinationPoints.
						Point3 t_dp = m_destinationPoints.popFront(); // pop first element.
						t_dp.y = m_orbitCenter.y + (m_orbitCenter.y - t_dp.y);
						m_destinationPoints.pushBack(t_dp); // push the newly processed destination points at the back.
					}
				}
			}
		}
	}
#ifdef DRAW_BOUNDING_SPHERES
	// Draw a 1m sphere at this position
	debugDraw(Sphere(m_frame.translation, BOUNDING_SPHERE_RADIUS), 0.0f, Color4::clear(), Color3::black());
#endif
}


shared_ptr<Entity> JumpingEntity::create(
	const String&                  name,
	Scene*                         scene,
	AnyTableReader&				   propertyTable,
	const ModelTable&              modelTable,
	const Scene::LoadOptions&      loadOptions)
{
	// Don't initialize in the constructor, where it is unsafe to throw Any parse exceptions
	const shared_ptr<JumpingEntity>& jumpingEntity = createShared<JumpingEntity>();

	// Initialize each base class, which parses its own fields
	jumpingEntity->Entity::init(name, scene, propertyTable);
	jumpingEntity->VisibleEntity::init(propertyTable, modelTable);
	jumpingEntity->JumpingEntity::init(propertyTable);

	// Verify that all fields were read by the base classes
	propertyTable.verifyDone();

	return jumpingEntity;
}

shared_ptr<JumpingEntity> JumpingEntity::create(
	shared_ptr<TargetConfig>		config,
	const String&					name,
	Scene*							scene,
	const shared_ptr<Model>&		model,
	int								scaleIdx,
	const Point3&					orbitCenter,
	float							targetDistance,
	int								paramIdx)
{
	// Don't initialize in the constructor, where it is unsafe to throw Any parse exceptions
	const shared_ptr<JumpingEntity>& jumpingEntity = createShared<JumpingEntity>();

	// Initialize each base class, which parses its own fields
	jumpingEntity->Entity::init(name, scene, CFrame(), shared_ptr<Entity::Track>(), true, true);
	jumpingEntity->VisibleEntity::init(model, true, Surface::ExpressiveLightScatteringProperties(), ArticulatedModel::PoseSpline());
	jumpingEntity->JumpingEntity::init(
		{ config->speed[0], config->speed[1] },
		{ config->motionChangePeriod[0], config->motionChangePeriod[1] },
		{ config->jumpPeriod[0], config->jumpPeriod[1] },
		{ config->distance[0], config->distance[1] },
		{ config->jumpSpeed[0], config->jumpSpeed[1] },
		{ config->accelGravity[0], config->accelGravity[1] },
		orbitCenter,
		targetDistance,
		paramIdx,
		config->axisLock,
		config->respawnCount,
		scaleIdx,
		config->logTargetTrajectory);
	jumpingEntity->m_id = config->id;

	return jumpingEntity;
}


void JumpingEntity::init(AnyTableReader& propertyTable) {
	//TODO: implement load from any file here...
	init();
}


void JumpingEntity::init() {
}

void JumpingEntity::init(
	const Vector2& angularSpeedRange,
	const Vector2& motionChangePeriodRange,
	const Vector2& jumpPeriodRange,
	const Vector2& distanceRange,
	const Vector2& jumpSpeedRange,
	const Vector2& gravityRange,
	Point3 orbitCenter,
	float orbitRadius,
	int paramIdx,
	Array<bool> axisLock,
	int respawns,
	int scaleIdx,
	bool isLogged)
{
	m_angularSpeedRange = angularSpeedRange;
	m_motionChangePeriodRange = motionChangePeriodRange;
	m_jumpPeriodRange = jumpPeriodRange;
	m_distanceRange = distanceRange;
	m_jumpSpeedRange = jumpSpeedRange;
	m_gravityRange = gravityRange;
	m_orbitCenter = orbitCenter;
	m_respawnCount = respawns;
	m_paramIdx = paramIdx;
	m_scaleIdx = scaleIdx;
	m_isLogged = isLogged;
	alwaysAssertM(axisLock.size() == 3, "Axis lock must have size 3!");
	for (int i = 0; i < 3; i++) {
		m_axisLocks[i] = axisLock[i];
	}
	m_orbitRadius = orbitRadius;
	float angularSpeed = Random::common().uniform(m_angularSpeedRange[0], m_angularSpeedRange[1]);
	m_planarSpeedGoal = m_orbitRadius * (angularSpeed * pif() / 180.0f);
	if (Random::common().uniform() > 0.5f) {
		m_planarSpeedGoal = -m_planarSpeedGoal;
	}
	// [m/s] = [m/radians] * [radians/s]
	m_speed.x = m_planarSpeedGoal;
	m_speed.y = 0.0f;

	m_inJump = false;
	m_motionChangeTimer = Random::common().uniform(m_motionChangePeriodRange[0], m_motionChangePeriodRange[1]);
	m_jumpTimer = Random::common().uniform(m_jumpPeriodRange[0], m_jumpPeriodRange[1]);
}

Any JumpingEntity::toAny(const bool forceAll) const {
	Any a = VisibleEntity::toAny(forceAll);
	a.setName("JumpingEntity");
	return a;
}

void JumpingEntity::onSimulation(SimTime absoluteTime, SimTime deltaTime) {
	// Do not call Entity::onSimulation; that will override with spline animation

	if (!(isNaN(deltaTime) || (deltaTime == 0))) {
		m_previousFrame = m_frame;
	}

	simulatePose(absoluteTime, deltaTime);

	if (m_isFirstFrame) {
		m_simulatedPos = m_frame.translation;
		m_standingHeight = m_frame.translation.y;
		m_isFirstFrame = false;
		m_acc.y = -Random::common().uniform(m_gravityRange[0], m_gravityRange[1]);
		m_jumpSpeed = Random::common().uniform(m_jumpSpeedRange[0], m_jumpSpeedRange[1]);
	}

	if (m_worldSpace) {
		// Implement world-space target logic here
		Point3 pos = m_frame.translation;			// Get the starting position
		// Check for time for motion (direction) change
		if (absoluteTime > m_nextChangeTime) {
			// Update the next change time
			float motionChangeTime = Random::common().uniform(m_motionChangePeriodRange[0], m_motionChangePeriodRange[1]);
			m_nextChangeTime = absoluteTime + motionChangeTime;
			// Velocity to use for this next interval
			float vel = Random::common().uniform(m_angularSpeedRange[0], m_angularSpeedRange[1]);
			Point3 destination = m_moveBounds.randomInteriorPoint();
			if (m_axisLocks[0]) {
				destination.x = frame().translation.x;
			}
			if (m_axisLocks[1]) {
				destination.y = frame().translation.y;
			}
			if (m_axisLocks[2]) {
				destination.z = frame().translation.z;
			}
			if (m_axisLocks[0] && m_axisLocks[1] && m_axisLocks[2] && vel > 0) {
				throw "Cannot lock all axes for non-static target!";
			}
			m_velocity = vel * (destination - m_frame.translation).direction();
		}
		// Check for time for jump
		if (absoluteTime > m_nextJumpTime && !m_inJump) {
			m_standingHeight = pos.y;
			m_inJump = true;				// Note we are in the jump
			m_jumpTime = absoluteTime;
		}
		
		// Add the velocity
		pos += m_velocity * deltaTime;

		// Check for whether the target has "left" the bounds
		float height = pos.y;
		if (m_inJump) {
			pos.y = m_standingHeight;
		}
		if (!m_moveBounds.contains(pos)) {
			m_velocity = Vector3(-m_velocity.x, -m_velocity.y, -m_velocity.z);
		}
		if (m_inJump) {
			pos.y = height;
		}

		// Check for jump condition
		if (m_inJump) {
			SimTime dt = absoluteTime - m_jumpTime;
			float jumpTime = -m_jumpSpeed / m_acc.y
				- sqrtf(m_jumpSpeed*m_jumpSpeed - m_acc.y * pos.y + m_acc.y * m_standingHeight) / m_acc.y;
			// Check if jump is over (time-based)
			if (dt > jumpTime) {
				m_inJump = false;
				m_jumpTime = 0;
				pos.y = m_standingHeight;		// Reset to the original height
				// Schedule the next jump here
				float nextJump = Random::common().uniform(m_jumpPeriodRange[0], m_jumpPeriodRange[1]);
				m_nextJumpTime = absoluteTime + nextJump;
			}
			else {
				// Attempt jump simulation here
				pos.y += 0.5f * m_acc.y * dt * dt + m_jumpSpeed * dt;
			}
		}

		// Update the position
		setFrame(pos);

		// Set changed time if it moved
		if (m_velocity != Vector3(0.f, 0.f, 0.f) || m_inJump) {
			m_lastChangeTime = System::time();
		}
	}
	else {
		while (deltaTime > 0.000001f) {
			/// Decide time step size for motion simulation.
			// Calculate remaining time until next state change for motion and jump
			float nextJumpStateChange;
			if (m_inJump) {
				// Find when the current jump ends from now. We need to solve a kinematic equation here.
				// 0 = a * t ^ 2 + 2 * v * t + (y - y0)
				// a: acceleration, t: time, v: velocity, y: height, y0: standing height (before jump started)
				// The equation comes with one positive and one negative solution. Ignore negative (it's "when did the jump start?")
				// The following is the positive solution. Note that a is negative (- g).
				// t = - v / a - sqrt(v ^ 2 - ah + ah0) / a
				nextJumpStateChange =
					-m_speed.y / m_acc.y
					- sqrtf(m_speed.y * m_speed.y - m_acc.y * m_simulatedPos.y + m_acc.y * m_standingHeight) / m_acc.y;
				int aa = 1;
			}
			else {
				nextJumpStateChange = m_jumpTimer;
			}
			// Decide time step size t where motion state is consistent.
			float t = G3D::min(deltaTime, m_motionChangeTimer, nextJumpStateChange);

			/// Update position, planar component
			Point3 planar_center = Point3(m_orbitCenter.x, m_standingHeight, m_orbitCenter.z);
			Point3 planar_pos = Point3(m_simulatedPos.x, m_standingHeight, m_simulatedPos.z); // purely rotation component of the position vector
			float radius = (planar_pos - planar_center).length();
			float d;
			if (m_inJump) {
				d = m_speed.x * t + m_acc.x * t * t / 2; // metric distance to travel.
			}
			else {
				d = m_speed.x * t;
			}
			float angularDistance = d / radius; // unit is radian
			// Calculate the position.
			Point3 U = (planar_pos - planar_center).direction();
			// Find a perpendicular vector toward the direction of rotation.
			Point3 V;
			V = U.cross(Point3(0.f, 1.f, 0.f));
			Point3 o = m_orbitCenter + (cos(angularDistance) * U + sin(angularDistance) * V) * radius;
			m_simulatedPos.x = o.x;
			m_simulatedPos.z = o.z;

			/// Update position, jump component
			if (m_inJump) {
				m_simulatedPos.y = 0.5f * m_acc.y * t * t + m_speed.y * t + m_simulatedPos.y;
			}

			/// Update animated position.
			// Project to the spherical surface, and update the frame translation vector.
			Point3 relativePos = m_simulatedPos - m_orbitCenter;
			m_frame.translation = relativePos.direction() * m_orbitRadius + m_orbitCenter;

			// Set changed time since we don't track whether we moved
			m_lastChangeTime = System::time();

			/// Update velocity
			if (m_inJump) {
				m_speed = m_speed + m_acc * t;
				if ((m_planarSpeedGoal > 0 && m_speed.x > m_planarSpeedGoal) || (m_planarSpeedGoal < 0 && m_speed.x < m_planarSpeedGoal)) {
					m_speed.x = m_planarSpeedGoal;
				}
			}

			/// Update motion state (includes updating acceleration)
			if (t == m_motionChangeTimer) { // changing motion direction
				float new_AngularSpeedGoal = Random::common().uniform(m_angularSpeedRange[0], m_angularSpeedRange[1]);
				float new_planarSpeedGoal = m_orbitRadius * (new_AngularSpeedGoal * pif() / 180.0f);
				// change direction
				if (m_planarSpeedGoal > 0) {
					new_planarSpeedGoal = -new_planarSpeedGoal;
				}
				// assign as the new speed goal
				m_planarSpeedGoal = new_planarSpeedGoal;
				if (m_inJump) { // if in jump, flip planar acceleration direction
					m_acc.x = sign(m_planarSpeedGoal) * m_planarAcc;
				}
				else { // if not in jump, immediately apply direction change
					m_speed.x = m_planarSpeedGoal;
				}
				m_motionChangeTimer = Random::common().uniform(m_motionChangePeriodRange[0], m_motionChangePeriodRange[1]);
			}
			if (t == nextJumpStateChange) { // either starting or finishing jump
				if (m_inJump) { // finishing jump
					m_simulatedPos.y = m_standingHeight; // hard-set to non-jumping height.
					m_acc.y = 0; // remove gravity effect
					m_speed.x = m_planarSpeedGoal; // instantly gain the running speed. (general behavior in games)
					m_inJump = false;
					m_jumpTimer = Random::common().uniform(m_jumpPeriodRange[0], m_jumpPeriodRange[1]);
				}
				else { // starting jump
					m_acc.x = sign(m_planarSpeedGoal) * m_planarAcc;
					float gravity = -Random::common().uniform(m_gravityRange[0], m_gravityRange[1]);
					float jumpSpeed = Random::common().uniform(m_jumpSpeedRange[0], m_jumpSpeedRange[1]);
					float distance = Random::common().uniform(m_distanceRange[0], m_distanceRange[1]);
					m_acc.y = gravity * m_orbitRadius / distance;
					m_speed.y = jumpSpeed * m_orbitRadius / distance;
					m_planarAcc = m_acc.y / 3.f;
					m_inJump = true;
				}
			}

			/// decrement deltaTime and timers by t
			deltaTime -= t;
			m_jumpTimer -= t;
			m_motionChangeTimer -= t;
		}
	}
#ifdef DRAW_BOUNDING_SPHERES
	// Draw a 1m sphere at this position
	debugDraw(Sphere(m_frame.translation, BOUNDING_SPHERE_RADIUS), 0.0f, Color4::clear(), Color3::black());
#endif
}

/*NETWORKED STUFF HGOES HERE*/

shared_ptr<Entity> NetworkedEntity::create(
	const String& name,
	Scene* scene,
	AnyTableReader& propertyTable,
	const ModelTable& modelTable,
	const Scene::LoadOptions& loadOptions)
{
	// Don't initialize in the constructor, where it is unsafe to throw Any parse exceptions
	const shared_ptr<NetworkedEntity>& betworkedEntity = createShared<NetworkedEntity>();

	// Initialize each base class, which parses its own fields
	networkedEntity->Entity::init(name, scene, propertyTable);
	networkedEntity->VisibleEntity::init(propertyTable, modelTable);
	networkedEntity->NetworkedEntity::init(propertyTable);

	// Verify that all fields were read by the base classes
	propertyTable.verifyDone();

	return networkedEntity;
}


shared_ptr<NetworkedEntity> NetworkedEntity::create(
	const String& name,
	Scene* scene,
	const shared_ptr<Model>& model,
	const CFrame& position) {

	// Don't initialize in the constructor, where it is unsafe to throw Any parse exceptions
	const shared_ptr<NetworkedEntity>& networkedEntity = createShared<NetworkedEntity>();

	// Initialize each base class, which parses its own fields
	networkedEntity->Entity::init(name, scene, position, shared_ptr<Entity::Track>(), true, true);
	networkedEntity->VisibleEntity::init(model, true, Surface::ExpressiveLightScatteringProperties(), ArticulatedModel::PoseSpline());
	networkedEntity->NetworkedEntity::init();

	return networkedEntity;
}

shared_ptr<NetworkedEntity> NetworkedEntity::create(
	shared_ptr<TargetConfig>		config,
	const String& name,
	Scene* scene,
	const shared_ptr<Model>& model,
	const Point3& orbitCenter,
	int								scaleIdx,
	int								paramIdx)
{
	// Don't initialize in the constructor, where it is unsafe to throw Any parse exceptions
	const shared_ptr<NetworkedEntity>& networkedEntity = createShared<NetworkedEntity>();

	// Initialize each base class, which parses its own fields
	networkedEntity->Entity::init(name, scene, CFrame(), shared_ptr<Entity::Track>(), true, true);
	networkedEntity->VisibleEntity::init(model, true, Surface::ExpressiveLightScatteringProperties(), ArticulatedModel::PoseSpline());
	networkedEntity->NetworkedEntity::init(
		{ config->speed[0], config->speed[1] },
		{ config->motionChangePeriod[0], config->motionChangePeriod[1] },
		config->upperHemisphereOnly,
		orbitCenter,
		paramIdx,
		config->axisLock,
		config->respawnCount,
		scaleIdx,
		config->logTargetTrajectory);
	networkedEntity->m_id = config->id;
	return networkedEntity;
}



void NetworkedEntity::init(AnyTableReader& propertyTable) {
	//TODO: implement load from any file here...
	init();
}

//init from network TODO

void NetworkedEntity::init() {
}


void NetworkedEntity::init(Vector2 angularSpeedRange, Vector2 motionChangePeriodRange, bool upperHemisphereOnly, Point3 orbitCenter, int paramIdx, Array<bool> axisLock, int respawns, int scaleIdx, bool isLogged) {
	m_angularSpeedRange = angularSpeedRange;
	m_motionChangePeriodRange = motionChangePeriodRange;
	m_upperHemisphereOnly = upperHemisphereOnly;
	m_orbitCenter = orbitCenter;
	m_paramIdx = paramIdx;
	m_respawnCount = respawns;
	m_scaleIdx = scaleIdx;
	m_isLogged = isLogged;
	alwaysAssertM(axisLock.size() == 3, "Axis lock must have size 3!");
	for (int i = 0; i < 3; i++) {
		m_axisLocks[i] = axisLock[i];
	}
}

void NetworkedEntity::fromNetwork(void* buff) {
	//set frame
}

void* NetworkedEntity::toNetwork() {
	//serialize frame
}

Any NetworkedEntity::toAny(const bool forceAll) const {
	Any a = VisibleEntity::toAny(forceAll);
	a.setName("NetworkedEntity");

	// a["velocity"] = m_velocity;

	return a;
}

void NetworkedEntity::onSimulation(SimTime absoluteTime, SimTime deltaTime) {
	// Do not call Entity::onSimulation; that will override with spline animation

	if (!(isNaN(deltaTime) || (deltaTime == 0))) { // first frame?
		m_previousFrame = m_frame;
	}

	simulatePose(absoluteTime, deltaTime);

	if (m_worldSpace) {
		Point3 pos = m_frame.translation;
		// Handle world-space target here
		// Check for change in direction
		if (absoluteTime > m_nextChangeTime) {
			// Update the next change time
			float motionChangeTime = Random::common().uniform(m_motionChangePeriodRange[0], m_motionChangePeriodRange[1]);
			m_nextChangeTime = absoluteTime + motionChangeTime;
			// Velocity to use for this next interval
			float vel = Random::common().uniform(m_angularSpeedRange[0], m_angularSpeedRange[1]);
			Point3 destination = m_bounds.randomInteriorPoint();
			if (m_axisLocks[0]) {
				destination.x = pos.x;
			}
			if (m_axisLocks[1]) {
				destination.y = pos.y;
			}
			if (m_axisLocks[2]) {
				destination.z = pos.z;
			}
			if (m_axisLocks[0] && m_axisLocks[1] && m_axisLocks[2] && vel > 0) {
				throw "Cannot lock all axes for non-static target!";
			}
			m_velocity = vel * (destination - m_frame.translation).direction();
		}
		// Check for whether the target has "left" the bounds, if so "reflect" it about the wall
		else if (!m_bounds.contains(pos)) {
			if (pos.x >= m_bounds.high().x) {
				m_velocity.x = -abs(m_velocity.x);
			}
			else if (pos.x <= m_bounds.low().x) {
				m_velocity.x = abs(m_velocity.x);
			}
			if (pos.y >= m_bounds.high().y) {
				m_velocity.y = -abs(m_velocity.y);
			}
			else if (pos.y <= m_bounds.low().y) {
				m_velocity.y = abs(m_velocity.y);
			}
			if (pos.z >= m_bounds.high().z) {
				m_velocity.z = -abs(m_velocity.z);
			}
			else if (pos.z <= m_bounds.low().z) {
				m_velocity.z = abs(m_velocity.z);
			}
		}

		// Update the position and set the frame
		pos += m_velocity * deltaTime;
		setFrame(pos);

		// Set changed time if it moved
		if (m_velocity != Vector3(0.f, 0.f, 0.f)) {
			m_lastChangeTime = System::time();
		}
	}
	else {
		// Handle non-world space (player projection here)
		while ((deltaTime > 0.000001f) && m_angularSpeedRange[0] > 0.0f) {
			if (m_destinationPoints.empty()) {
				// Add destimation points if no destination points.
				float motionChangePeriod = Random::common().uniform(m_motionChangePeriodRange[0], m_motionChangePeriodRange[1]);
				float angularSpeed = Random::common().uniform(m_angularSpeedRange[0], m_angularSpeedRange[1]);
				float angularDistance = motionChangePeriod * angularSpeed;
				angularDistance = angularDistance > 170.f ? 170.0f : angularDistance; // replace with 170 deg if larger than 170.

				// [m/s] = [m/radians] * [radians/s]
				const float radius = (m_frame.translation - m_orbitCenter).length();
				m_speed = radius * (angularSpeed * pif() / 180.0f);

				// relative position to orbit center
				Point3 relPos = m_frame.translation - m_orbitCenter;
				// find a vector perpendicular to the current position
				Point3 perpen = findPerpendicularVector(relPos);
				// calculate destination point
				Point3 dest = m_orbitCenter + rotateToward(relPos, perpen, angularDistance);
				// add destination point.
				m_destinationPoints.pushBack(dest);
			}

			if ((m_frame.translation - m_destinationPoints[0]).length() < 0.001f) {
				// Retire this destination. We are almost at the destination (linear and geodesic distances 
				// are the same when small), and the following math will be numerically imprecise if we
				// use such a close destination.
				m_destinationPoints.popFront();
			}
			else {
				const Point3 destinationPoint = m_destinationPoints[0];
				const Point3 currentPoint = m_frame.translation;

				// Transform to directions
				const float radius = (destinationPoint - m_orbitCenter).length();
				const Vector3& destinationVector = (destinationPoint - m_orbitCenter).direction();
				const Vector3& currentVector = (currentPoint - m_orbitCenter).direction();

				// The direction is always "from current to destination", so we can use acos here
				// and not worry about it being unsigned.
				const float projection = currentVector.dot(destinationVector);
				const float destinationAngle = G3D::acos(projection);

				// [radians/s] = [m/s] / [m/radians]
				const float angularSpeed = m_speed / radius;

				// [rad] = [rad/s] * [s] 
				float angleChange = angularSpeed * deltaTime;

				if (angleChange > destinationAngle) {
					// We'll reach the destination before the time step ends.
					// Record how much time was consumed by this step.
					deltaTime -= destinationAngle / angularSpeed;
					angleChange = destinationAngle;
					m_destinationPoints.popFront();
				}
				else {
					// Consumed the entire time step
					deltaTime = 0;
				}

				// Transform to spherical coordinates in the plane of the arc
				const Vector3& U = currentVector;
				const Vector3& V = (destinationVector - currentVector * projection).direction();

				setFrame(m_orbitCenter + (cos(angleChange) * U + sin(angleChange) * V) * radius);

				// Set changed time if it moved
				if (angleChange != 0.f) {
					m_lastChangeTime = System::time();
				}
			}

			if (m_upperHemisphereOnly) {
				// Target position must be always above the orbit horizon (plane defined by "y = m_orbitCenter.y")
				// If target is below the orbit horizon, y-invert position & destination points w.r.t. the orbit horizon.
				if (m_frame.translation.y < m_orbitCenter.y) {
					m_frame.translation.y = m_orbitCenter.y + (m_orbitCenter.y - m_frame.translation.y);
					for (int i = 0; i < m_destinationPoints.length(); ++i) { // iterate by the number of elements in m_destinationPoints.
						Point3 t_dp = m_destinationPoints.popFront(); // pop first element.
						t_dp.y = m_orbitCenter.y + (m_orbitCenter.y - t_dp.y);
						m_destinationPoints.pushBack(t_dp); // push the newly processed destination points at the back.
					}
				}
			}
		}
	}
#ifdef DRAW_BOUNDING_SPHERES
	// Draw a 1m sphere at this position
	debugDraw(Sphere(m_frame.translation, BOUNDING_SPHERE_RADIUS), 0.0f, Color4::clear(), Color3::black());
#endif
}