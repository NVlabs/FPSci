#pragma once
#include <G3D/G3D.h>


class FlyingEntity : public VisibleEntity {
protected:

	/** Spherical component characteristics */
	float                           m_speed = 0.0f; // deg / sec
	Point3                          m_orbitCenter;

	/** Properties defining target behavior */
	Array<float>                    m_speedRange = Array<float>{ 0.0f, 4.0f };
	Array<float>                    m_motionChangePeriodRange = Array<float>{ 10000.0f, 10000.0f };

    /** The target will move through these points along arcs around
        m_orbitCenter at m_speed. As each point is hit, it is
        removed from the queue.*/
    Queue<Point3>                   m_destinationPoints;

	FlyingEntity() {}
    void init(AnyTableReader& propertyTable);

	void init();

	void init(Array<float> speedRange, Array<float> motionChangePeriodRange);

public:

    /** Destinations must be no more than 170 degrees apart to avoid ambiguity in movement direction */
    void setDestinations(const Array<Point3>& destinationArray, const Point3 orbitCenter);

	void setSpeed(float speed) {
		m_speed = speed;
	}

	// TODO: After other implementations are complete.
    /** For deserialization from Any / loading from file */
    static shared_ptr<Entity> create 
    (const String&                  name,
     Scene*                         scene,
     AnyTableReader&                propertyTable,
     const ModelTable&              modelTable,
     const Scene::LoadOptions&      loadOptions);

	/** For programmatic construction at runtime */
	static shared_ptr<FlyingEntity> create
	(const String&                  name,
		Scene*                         scene,
		const shared_ptr<Model>&       model,
		const CFrame&                  position);

	/** For programmatic construction at runtime */
	static shared_ptr<FlyingEntity> create
	(const String&                  name,
		Scene*                         scene,
		const shared_ptr<Model>&       model,
		const CFrame&                  position,
		Array<float>                   m_speedRange,
		Array<float>                   m_motionChangePeriodRange);

	/** Converts the current VisibleEntity to an Any.  Subclasses should
        modify at least the name of the Table returned by the base class, which will be "Entity"
        if not changed. */
    virtual Any toAny(const bool forceAll = false) const override;
    
    virtual void onSimulation(SimTime absoluteTime, SimTime deltaTime) override;
};
//
//class JumpingEntity : public VisibleEntity {
//protected:
//
//	// Motion is calculated in three steps.
//	// 1. Spherical component that is horizontal motion only and precisely staying on the spherical surface.
//	// 2. Jump component that target moves vertically following constant acceleration downward (gravity)
//	// 3. If deviated from spherical surface (can be true in jumping cases), project toward orbit center to snap on the spherical surface.
//
//	/** Motion kinematics, x is spherical (horizontal) component and y is vertical (jump) component. */
//	Point2                          m_speed;
//
//	/** Position is calculated as sphieral motion + jump. Animated position is after snapping to the spherical surface. */
//	Point3                          m_animatedPosition;
//
//	/** Parameters reset every time motion change or jump happens */
//	Point2                          m_targetSpeed;
//	Point2                          m_acc;
//
//	/** Parameters constant during a trial */
//	Point3                          m_orbitCenter;
//
//	/** variables for motion changes */
//	float                           m_nextChangeIn;
//
//	/** Properties defining target behavior */
//	Array<float>                    m_speedRange;
//	Array<float>                    m_motionChangePeriodRange;
//	Array<float>                    m_jumpPeriodRange;
//	Array<float>                    m_jumpSpeedRange;
//	Array<float>                    m_gravityRange;
//
//	JumpingEntity() {}
//
//	void init(AnyTableReader& propertyTable);
//
//	void init();
//
//public:
//
//	// TODO: After other implementations are complete.
//	///** For deserialization from Any / loading from file */
//	//static shared_ptr<Entity> create 
//	//(const String&                  name,
//	// Scene*                         scene,
//	// AnyTableReader&                propertyTable,
//	// const ModelTable&              modelTable,
//	// const Scene::LoadOptions&      loadOptions);
//
//	/** For programmatic construction at runtime */
//	static shared_ptr<JumpingEntity> create
//	(const String&                  name,
//		Scene*                         scene,
//		const shared_ptr<Model>&       model,
//		const CFrame&                  position);
//
//	/** Converts the current VisibleEntity to an Any.  Subclasses should
//		modify at least the name of the Table returned by the base class, which will be "Entity"
//		if not changed. */
//	virtual Any toAny(const bool forceAll = false) const override;
//
//	virtual void onSimulation(SimTime absoluteTime, SimTime deltaTime) override;
//
//	void onSimulationPlannedSphereMotion(SimTime absoluteTime, SimTime deltaTime);
//
//	void onSimulationSphereMotion(SimTime absoluteTime, SimTime deltaTime);
//
//	void onSimulationJumpingMotion(SimTime absoluteTime, SimTime deltaTime);
//
//	void snapToSphere();
//
//	void changeDirection(float rollAngle);
//};
