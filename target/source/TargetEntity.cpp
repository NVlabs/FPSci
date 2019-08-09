#include "TargetEntity.h"

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
	const CFrame&					position,
	Point3							offset)
{
	const shared_ptr<TargetEntity>& target = createShared<TargetEntity>();
	target->Entity::init(name, scene, CFrame(dests[0].position), shared_ptr<Entity::Track>(), true, true);
	target->VisibleEntity::init(model, true, Surface::ExpressiveLightScatteringProperties(), ArticulatedModel::PoseSpline());
	target->TargetEntity::init(dests, offset);
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
	destinations = destinationArray;
}

void TargetEntity::onSimulation(SimTime absoluteTime, SimTime deltaTime) {
	// Check whether we have any destinations yet...
	if (destinations.size() < 2)
		return;

	SimTime time = fmod(absoluteTime, getPathTime());			// Compute a local time (modulus the path time)
	
	// Check if its time to move to the next segment
	while(time < destinations[destinationIdx].time || time >= destinations[destinationIdx+1].time) {
		destinationIdx++;										// Increment the destination index
		destinationIdx %= destinations.size();					// Wrap if time goes over (works well for looped paths)
	}
	
	// Get the current and next destination index
	Destination currDest = destinations[destinationIdx];
	Destination nextDest = destinations[(destinationIdx + 1) % destinations.size()];

	// Compute the position by interpolating
	float duration = nextDest.time - currDest.time;			// Get the total time for this "step
	duration = max(duration, 0.0f);							// In "wrap" case immediately teleport back to start (0 duration step)
	
	float prog = 1.0f;										// By default make the "full step"
	if (duration > 0.0f) {
		// Handle time "wrap" case here
		if (nextDest.time < time) {
			time -= getPathTime();							// Fix the "wrap" math for prog below
		}
		prog = (nextDest.time - time) / duration;			// Get the ratio of time in this step completed
	}
	
	Point3 delta = currDest.position - nextDest.position; 	// Get the delta vector to move along
	setFrame((prog*delta) + currDest.position + offset);	// Set the new positions

#ifdef DRAW_BOUNDING_SPHERES
	// Draw a 1m sphere at this position
	debugDraw(Sphere(m_frame, BOUNDING_SPHERE_RADIUS), 0.0f, Color4::clear(), Color3::black());
#endif
}

shared_ptr<Entity> FlyingEntity::create
(const String&                  name,
	Scene*                         scene,
	AnyTableReader&                propertyTable,
	const ModelTable&              modelTable,
	const Scene::LoadOptions&      loadOptions) {

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


shared_ptr<FlyingEntity> FlyingEntity::create
(const String&                           name,
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


shared_ptr<FlyingEntity> FlyingEntity::create
(const String&                           name,
	Scene*                                  scene,
	const shared_ptr<Model>&                model,
	const CFrame&                           position,
	const Vector2&                          speedRange,
	const Vector2&                          motionChangePeriodRange,
	Point3                                  orbitCenter) {

	// Don't initialize in the constructor, where it is unsafe to throw Any parse exceptions
	const shared_ptr<FlyingEntity>& flyingEntity = createShared<FlyingEntity>();

	// Initialize each base class, which parses its own fields
	flyingEntity->Entity::init(name, scene, position, shared_ptr<Entity::Track>(), true, true);
	flyingEntity->VisibleEntity::init(model, true, Surface::ExpressiveLightScatteringProperties(), ArticulatedModel::PoseSpline());
	flyingEntity->FlyingEntity::init(speedRange, motionChangePeriodRange, orbitCenter);

	return flyingEntity;
}


void FlyingEntity::init(AnyTableReader& propertyTable) {
	//TODO: implement load from any file here...
	init();
}


void FlyingEntity::init() {
}


void FlyingEntity::init(Vector2 angularSpeedRange, Vector2 motionChangePeriodRange, Point3 orbitCenter) {
	m_angularSpeedRange = angularSpeedRange;
	m_motionChangePeriodRange = motionChangePeriodRange;
	m_orbitCenter = orbitCenter;
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

	while ((deltaTime > 0.000001f) && m_angularSpeedRange[0] > 0.0f) {
		if (m_destinationPoints.empty()) {
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

			m_frame.translation = m_orbitCenter + (cos(angleChange) * U + sin(angleChange) * V) * radius;
		}
	}
#ifdef DRAW_BOUNDING_SPHERES
	// Draw a 1m sphere at this position
	debugDraw(Sphere(m_frame.translation, BOUNDING_SPHERE_RADIUS), 0.0f, Color4::clear(), Color3::black());
#endif
}


shared_ptr<Entity> JumpingEntity::create
(const String&                  name,
	Scene*                         scene,
	AnyTableReader&                propertyTable,
	const ModelTable&              modelTable,
	const Scene::LoadOptions&      loadOptions) {

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


shared_ptr<JumpingEntity> JumpingEntity::create
(const String&                           name,
	Scene*                                  scene,
	const shared_ptr<Model>&                model,
	const CFrame&                           position,
    const Vector2&                          angularSpeedRange,
    const Vector2&                          motionChangePeriodRange,
	const Vector2&                          jumpPeriodRange,
	const Vector2&                          distanceRange,
	const Vector2&                          jumpSpeedRange,
	const Vector2&                          gravityRange,
	Point3                                  orbitCenter,
	float                                   orbitRadius) {

	// Don't initialize in the constructor, where it is unsafe to throw Any parse exceptions
	const shared_ptr<JumpingEntity>& jumpingEntity = createShared<JumpingEntity>();

	// Initialize each base class, which parses its own fields
	jumpingEntity->Entity::init(name, scene, position, shared_ptr<Entity::Track>(), true, true);
	jumpingEntity->VisibleEntity::init(model, true, Surface::ExpressiveLightScatteringProperties(), ArticulatedModel::PoseSpline());
	jumpingEntity->JumpingEntity::init(
		angularSpeedRange,
		motionChangePeriodRange,
		jumpPeriodRange,
		distanceRange,
		jumpSpeedRange,
		gravityRange,
		orbitCenter,
		orbitRadius);

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
	float orbitRadius
){
	m_angularSpeedRange = angularSpeedRange;
	m_motionChangePeriodRange = motionChangePeriodRange;
	m_jumpPeriodRange = jumpPeriodRange;
	m_distanceRange = distanceRange;
	m_jumpSpeedRange = jumpSpeedRange;
	m_gravityRange = gravityRange;
	m_orbitCenter = orbitCenter;

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

	// a["velocity"] = m_velocity;

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
	}

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
				- m_speed.y / m_acc.y
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
				float gravity = - Random::common().uniform(m_gravityRange[0], m_gravityRange[1]);
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
#ifdef DRAW_BOUNDING_SPHERES
	// Draw a 1m sphere at this position
	debugDraw(Sphere(m_frame.translation, BOUNDING_SPHERE_RADIUS), 0.0f, Color4::clear(), Color3::black());
#endif
}

