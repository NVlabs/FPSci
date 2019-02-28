#pragma once
#include <G3D/G3D.h>


class TargetEntity : public VisibleEntity {
protected:

    /** World-space speed in meters/second */
    float                           m_speed = 10.0f;
    Point3                          m_orbitCenter;

    /** The target will move through these points along arcs around
        m_orbitCenter at m_speed. As each point is hit, it is
        removed from the queue.*/
    Queue<Point3>                   m_destinationPoints;

    TargetEntity() {}

    void init(AnyTableReader& propertyTable);

    void init();

public:

    void setSpeed(float s) {
        m_speed = s;    
    }

    /** Destinations must be no more than 170 degrees apart to avoid ambiguity in movement direction */
    void setDestinations(const Array<Point3>& destinationArray, const Point3 orbitCenter);

    /** For deserialization from Any / loading from file */
    static shared_ptr<Entity> create 
    (const String&                  name,
     Scene*                         scene,
     AnyTableReader&                propertyTable,
     const ModelTable&              modelTable,
     const Scene::LoadOptions&      loadOptions);

    /** For programmatic construction at runtime */
    static shared_ptr<TargetEntity> create 
    (const String&                  name,
     Scene*                         scene,
     const shared_ptr<Model>&       model,
     const CFrame&                  position);

    /** Converts the current VisibleEntity to an Any.  Subclasses should
        modify at least the name of the Table returned by the base class, which will be "Entity"
        if not changed. */
    virtual Any toAny(const bool forceAll = false) const override;
    
    virtual void onSimulation(SimTime absoluteTime, SimTime deltaTime) override;

};
