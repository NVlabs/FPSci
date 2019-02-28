#include "TargetEntity.h"

shared_ptr<Entity> TargetEntity::create 
    (const String&                  name,
     Scene*                         scene,
     AnyTableReader&                propertyTable,
     const ModelTable&              modelTable,
     const Scene::LoadOptions&      loadOptions) {

    // Don't initialize in the constructor, where it is unsafe to throw Any parse exceptions
    const shared_ptr<TargetEntity>& targetEntity = createShared<TargetEntity>();

    // Initialize each base class, which parses its own fields
    targetEntity->Entity::init(name, scene, propertyTable);
    targetEntity->VisibleEntity::init(propertyTable, modelTable);
    targetEntity->TargetEntity::init(propertyTable);

    // Verify that all fields were read by the base classes
    propertyTable.verifyDone();

    return targetEntity;
}


shared_ptr<TargetEntity> TargetEntity::create 
(const String&                           name,
 Scene*                                  scene,
 const shared_ptr<Model>&                model,
 const CFrame&                           position) {

    // Don't initialize in the constructor, where it is unsafe to throw Any parse exceptions
    const shared_ptr<TargetEntity>& targetEntity = createShared<TargetEntity>();

    // Initialize each base class, which parses its own fields
    targetEntity->Entity::init(name, scene, position, shared_ptr<Entity::Track>(), true, true);
    targetEntity->VisibleEntity::init(model, true, Surface::ExpressiveLightScatteringProperties(), ArticulatedModel::PoseSpline());
    targetEntity->TargetEntity::init();
 
    return targetEntity;
}


void TargetEntity::init(AnyTableReader& propertyTable) {
    init();
}


void TargetEntity::init() {
}


void TargetEntity::setDestinations(const Array<Point3>& destinationArray, const Point3 orbitCenter) {
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


Any TargetEntity::toAny(const bool forceAll) const {
    Any a = VisibleEntity::toAny(forceAll);
    a.setName("TargetEntity");

    // a["velocity"] = m_velocity;

    return a;
}
    
 
void TargetEntity::onSimulation(SimTime absoluteTime, SimTime deltaTime) {
    // Do not call Entity::onSimulation; that will override with spline animation

    if (! (isNaN(deltaTime) || (deltaTime == 0))) {
        m_previousFrame = m_frame;
    }

    simulatePose(absoluteTime, deltaTime);

    if ((deltaTime > 0) && ! m_destinationPoints.empty()) {
        if ((m_frame.translation - m_destinationPoints[0]).length() < 0.01f) {
            // Retire this destination. Our steps are so small and we're about to
            // change direction, so don't bother using the rest of the time interval on
            // the next point.
        } else {
            const Point3& destinationPoint = m_destinationPoints[0];
            const Point3& currentPoint = m_frame.translation;

            // Transform to directions
            const float radius = (destinationPoint - m_orbitCenter).length();
            const Vector3& destinationVector = (destinationPoint - m_orbitCenter).direction();
            const Vector3& currentVector = (currentPoint - m_orbitCenter).direction();

            // The direction is always "from current to destination", so we can use acos here
            // and not worry about it being unsigned.
            const float projection = currentVector.dot(destinationVector);
            const float destinationAngle = G3D::acos(projection);

            const float angularSpeed = m_speed / (2.0f * pif() * radius);
            const float angleChange = min(destinationAngle, angularSpeed * deltaTime);

            // Transform to spherical coordinates in the plane of the arc
            const Vector3& U = currentVector;
            const Vector3& V = (destinationVector - currentVector * projection).direction();

            const float newAngle = destinationAngle + angleChange;

            m_frame.translation = m_orbitCenter + cos(newAngle) * U + sin(newAngle) * V;            
        }
    }
}
