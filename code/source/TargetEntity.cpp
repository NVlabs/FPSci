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
	Array<float>                            speedRange,
	Array<float>                            motionChangePeriodRange,
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


void FlyingEntity::init(Array<float> angularSpeedRange, Array<float> motionChangePeriodRange, Point3 orbitCenter) {
	m_angularSpeedRange = angularSpeedRange;
	m_motionChangePeriodRange = motionChangePeriodRange;
	m_orbitCenter = orbitCenter;

	const float radius = (m_frame.translation - m_orbitCenter).length();
	float angularSpeed = G3D::Random().common().uniform(m_angularSpeedRange[0], m_angularSpeedRange[1]);
	// [m/s] = [m/radians] * [radians/s]
	m_speed = radius * (angularSpeed * pif() / 180.0f);
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

    if (! (isNaN(deltaTime) || (deltaTime == 0))) {
        m_previousFrame = m_frame;
    }

    simulatePose(absoluteTime, deltaTime);

    while ((deltaTime > 0.000001f) && m_speed > 0.0f) {
		if (m_destinationPoints.empty()) {
			float motionChangePeriod = Random::common().uniform(m_motionChangePeriodRange[0], m_motionChangePeriodRange[1]);
			float angularSpeed = G3D::Random().common().uniform(m_angularSpeedRange[0], m_angularSpeedRange[1]);
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
        } else {
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
            } else {
                // Consumed the entire time step
                deltaTime = 0;
            }

            // Transform to spherical coordinates in the plane of the arc
            const Vector3& U = currentVector;
            const Vector3& V = (destinationVector - currentVector * projection).direction();
            
            m_frame.translation = m_orbitCenter + (cos(angleChange) * U + sin(angleChange) * V) * radius;
        }
    }
}
