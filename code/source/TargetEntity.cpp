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
        for (const Point3& P : destinationArray) {
            m_destinationPoints.pushBack((P - orbitCenter).direction() * distance + orbitCenter);
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

    if (deltaTime > 0) {
        // TODO: Morgan
    }
}
