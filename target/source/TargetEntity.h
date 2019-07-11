#pragma once
#include <G3D/G3D.h>

class TargetEntity : public VisibleEntity {
protected:
	float m_health = 1.0f;			///< Target health
	Color3 m_color = Color3::red();

public:

	/** Getter for health */
	float health() { 
		return m_health; 
	}

	/**Simple routine to do damage */
	bool doDamage(float damage) {
		m_health -= damage;
		return m_health <= 0;
	}

};

class FlyingEntity : public TargetEntity {
protected:

	float							m_health = 1.0f;
	/** Angular speed in degress/sec */
	float                           m_speed = 0.0f;
    /** World space point at center of orbit */
	Point3                          m_orbitCenter;

	/** Angular Speed Range (deg/s) x=min y=max */
	Vector2                         m_angularSpeedRange = Vector2{ 0.0f, 4.0f };
    /** Motion Change period x=min y=max */
    Vector2                         m_motionChangePeriodRange = Vector2{ 10000.0f, 10000.0f };

    /** The target will move through these points along arcs around
        m_orbitCenter at m_speed. As each point is hit, it is
        removed from the queue.*/
    Queue<Point3>                   m_destinationPoints;

	FlyingEntity() {}
    void init(AnyTableReader& propertyTable);

	void init();

	void init(Vector2 angularSpeedRange, Vector2 motionChangePeriodRange, Point3 orbitCenter);

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
		const Vector2&                 speedRange,
		const Vector2&                 motionChangePeriodRange,
		Point3                         orbitCenter);

	/** Converts the current VisibleEntity to an Any.  Subclasses should
        modify at least the name of the Table returned by the base class, which will be "Entity"
        if not changed. */
    virtual Any toAny(const bool forceAll = false) const override;
    
    virtual void onSimulation(SimTime absoluteTime, SimTime deltaTime) override;
};


class JumpingEntity : public TargetEntity {
protected:

	// Motion is calculated in three steps.
	// 1. Spherical component that is horizontal motion only and precisely staying on the spherical surface.
	// 2. Jump component that target moves vertically following constant acceleration downward (gravity)
	// 3. If deviated from spherical surface (can be true in jumping cases), project toward orbit center to snap on the spherical surface.

	/** Motion kinematic parameters, x is spherical (horizontal) component and y is vertical (jump) component. */
	Point2                          m_speed;

	/** Position is calculated as sphieral motion + jump.
	Animation is done by projecting it toward the spherical surface. */
	Point3                          m_simulatedPos;

	/** Parameters reset every time motion change or jump happens */
	float                           m_planarSpeedGoal; // the speed value m_speed tries to approach.
	Point2                          m_acc;

	/** World space point at center of orbit */
	Point3                          m_orbitCenter;
	float                           m_orbitRadius;

	/** variables for motion changes */
	float                           m_motionChangeTimer;
	float                           m_jumpTimer;

	/** jump state */
	bool                            m_inJump;
	float                           m_standingHeight;

	/** Angular Speed Range (deg/s) x=min y=max */
	Vector2                         m_angularSpeedRange;
	/** Motion Change period x=min y=max */
	Vector2                         m_motionChangePeriodRange;
	/** Jump period x=min y=max */
    Vector2                         m_jumpPeriodRange;
	/** Jump initial speed (m/s) x=min y=max */
    Vector2                         m_jumpSpeedRange;
	/** Gravitational Acceleration (m/s^2) x=min y=max */
    Vector2                         m_gravityRange;
	/** Distance range */
	Vector2                         m_distanceRange;
	float                           m_planarAcc = 0.3f;

	/** check first frame */
	bool                            m_isFirstFrame = true;

	JumpingEntity() {}

	void init(AnyTableReader& propertyTable);

	void init();

	void init(
		const Vector2& angularSpeedRange,
        const Vector2& motionChangePeriodRange,
        const Vector2& jumpPeriodRange,
		const Vector2& distanceRange,
        const Vector2& jumpSpeedRange,
        const Vector2& gravityRange,
		Point3 orbitCenter,
		float orbitRadius
	);

public:

	/** For deserialization from Any / loading from file */
	static shared_ptr<Entity> create 
	(const String&                  name,
	 Scene*                         scene,
	 AnyTableReader&                propertyTable,
	 const ModelTable&              modelTable,
	 const Scene::LoadOptions&      loadOptions);

	/** For programmatic construction at runtime */
	static shared_ptr<JumpingEntity> create
	(const String&                  name,
		Scene*                         scene,
		const shared_ptr<Model>&       model,
		const CFrame&                  position,
        const Vector2&                 speedRange,
        const Vector2&                 motionChangePeriodRange,
        const Vector2&                 jumpPeriodRange,
		const Vector2&                 distanceRange,
		const Vector2&                 jumpSpeedRange,
        const Vector2&                 gravityRange,
		Point3                         orbitCenter,
		float                          orbitRadius);

	/** Converts the current VisibleEntity to an Any.  Subclasses should
		modify at least the name of the Table returned by the base class, which will be "Entity"
		if not changed. */
	virtual Any toAny(const bool forceAll = false) const override;

	virtual void onSimulation(SimTime absoluteTime, SimTime deltaTime) override;

};
