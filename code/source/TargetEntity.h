#pragma once
#include <G3D/G3D.h>

class TargetEntity : public VisibleEntity {
protected:

    TargetEntity() {}

    void init(AnyTableReader& propertyTable);

    void init();

public:

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
