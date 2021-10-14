#include "PhysicsScene.h"
#include "PlayerEntity.h"
#include "FPSciAnyTableReader.h"

shared_ptr<PhysicsScene> PhysicsScene::create(const shared_ptr<AmbientOcclusion>& ao) {
    return createShared<PhysicsScene>(ao);
}

void PhysicsScene::poseExceptExcluded(Array<shared_ptr<Surface> >& surfaceArray, const String& excludedEntity) {
    for (int e = 0; e < m_entityArray.size(); ++e) {
        if (m_entityArray[e]->name() != excludedEntity) {
            m_entityArray[e]->onPose(surfaceArray);
        }
    }
}
 
Any PhysicsScene::toAny() const {
    Any a = Scene::toAny();
    Any physicsTable(Any::TABLE, "Physics");
    physicsTable.set("gravity", m_gravity);
    a["Physics"] = physicsTable;

    return a;
}

Any PhysicsScene::load(const String& sceneName, const LoadOptions& loadOptions) {
    Any resultAny = Scene::load(sceneName, loadOptions);
	Vector3 m_gravity(0, -5 * units::meters() / square(units::seconds()), 0);

    if ( resultAny.containsKey("Physics") ) {
        const Any& physics = resultAny["Physics"];
        FPSciAnyTableReader physicsTable(physics);
        physicsTable.getIfPresent("gravity", m_gravity);
		physicsTable.getIfPresent("minHeight", m_resetHeight);
    }
    
    // Set the initial positions
	float minHeight = 1e6;
    Array<shared_ptr<Surface>> collisionSurfaces;
    for (int e = 0; e < m_entityArray.size(); ++e) {
        shared_ptr<VisibleEntity> entity = dynamic_pointer_cast<VisibleEntity>(m_entityArray[e]);
		if (notNull(entity)) {
			if (!entity->canChange()) {
				entity->onSimulation(0, 0);
				entity->onPose(collisionSurfaces);
			}
		}
    }
    m_collisionTree->setContents(collisionSurfaces, IMAGE_STORAGE_CURRENT);
    return resultAny;
}

void PhysicsScene::staticIntersectSphere(const Sphere& sphere, Array<Tri>& triArray) const {
    if (m_collisionTree) {
        m_collisionTree->intersectSphere(sphere, triArray);
    }
}

void PhysicsScene::staticIntersectBox(const AABox& box, Array<Tri>& triArray) const {
    if (m_collisionTree) {
        m_collisionTree->intersectBox(box, triArray);
    }
}

