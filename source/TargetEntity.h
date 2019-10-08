#pragma once
#include <G3D/G3D.h>

//#define DRAW_BOUNDING_SPHERES	1		// Uncomment this to draw bounding spheres (useful for target sizing)
#define BOUNDING_SPHERE_RADIUS	0.5		///< Use a 0.5m radius for sizing here

struct Destination{
public:
	Point3 position = Point3(0,0,0);
	SimTime time = 0.0;

	Destination() {
		position = Point3(0, 0, 0);
		time = 0.0;
	}
	
	Destination(Point3 pos, SimTime t) {
		position = pos;
		time = t;
	}

	Destination(const Any& any) {
		int settingsVersion = 1;
		AnyTableReader reader(any);
		reader.getIfPresent("settingsVersion", settingsVersion);

		switch (settingsVersion) {
		case 1:
			reader.get("t", time);
			reader.get("xyz", position);
			break;
		default:
			debugPrintf("Settings version '%d' not recognized in Destination configuration");
			break;
		}
	}

	Any toAny(const bool forceAll = true) const {
		Any a(Any::TABLE);
		a["t"] = time;
		a["xyz"] = position;
		return a;
	}

	size_t hash(void) {
		return HashTrait<Point3>::hashCode(position) ^ (int)time;
	}
};

class TargetEntity : public VisibleEntity {
protected:
	float m_health			= 1.0f;				///< Target health
	Color3 m_color			= Color3::red();	///< Default color
	Array<Destination> m_destinations;			///< Array of destinations to visit
	int destinationIdx		= 0;				///< Current index into the destination array
	Point3 m_offset;							///< Offset for initial spawn
	SimTime m_spawnTime		= 0;				///< Time initiatlly spawned
	int m_respawnCount		= 0;				///< Number of times to respawn
	int m_paramIdx			= -1;				///< Parameter index of this item
	bool m_worldSpace		= false;			///< World space coordiantes?
	int m_scaleIdx = 0;

	// Only used for flying/jumping entities
	SimTime m_nextChangeTime = 0;
	Vector3 m_velocity = Vector3::zero();

public:
	TargetEntity() {}

	static shared_ptr<TargetEntity> create(
		Array<Destination>				dests,
		const String&					name,
		Scene*							scene,
		const shared_ptr<Model>&		model,
		float							scaleIdx,
		const CFrame&					position,
		int								paramIdx,
		Point3							offset=Point3::zero(),
		int								respawns=0);

	void init(Array<Destination> dests, int paramIdx, Point3 staticOffset = Point3(0.0, 0.0, 0.0), int respawnCount=0, int scaleIdx=0) {
		setDestinations(dests);
		m_offset = staticOffset;
		m_respawnCount = respawnCount;
		m_paramIdx = paramIdx;
		m_scaleIdx = scaleIdx;
		destinationIdx = 0;
	}

	void setWorldSpace(bool worldSpace) { m_worldSpace = worldSpace; }

	/**Simple routine to do damage */
	bool doDamage(float damage) {
		m_health -= damage;
		return m_health <= 0;
	}
	
	bool respawn() {
		if (m_respawnCount == -1) {
			m_spawnTime = 0;
			m_health = 1.0f;
			return true;		// Infinite respawn target
		}
		else if (m_respawnCount > 0) {
			m_respawnCount -= 1;
			m_spawnTime = 0;		// Reset the path (only works for destination target)
			m_health = 1.0f;		// Reset the health
		}
		return (m_respawnCount>0);
	}

	void resetMotionParams() {
		m_nextChangeTime = 0;
	}

	int scaleIndex() {
		return m_scaleIdx;
	}

	/** Getter for health */
	float health() { return m_health; }
	/**Get the total time for a path*/
	float getPathTime() { return m_destinations.last().time; }
	Array<Destination> destinations() { return m_destinations; }
	int respawnsRemaining() { return m_respawnCount; }
	int paramIdx() { return m_paramIdx; }

	void drawHealthBar(RenderDevice* rd, const Camera& camera, const Framebuffer& framebuffer, Point2 size, Point3 offset, Point2 border, Array<Color4> colors, Color4 borderColor) const;
	virtual void onSimulation(SimTime absoluteTime, SimTime deltaTime) override;
	void setDestinations(const Array<Destination> destinationArray);

};

class FlyingEntity : public TargetEntity {
protected:
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

	/** Limits flying target entity motion for the upper hemisphere only.
		OnSimulation will y-invert target position & destination points
		whenever the target enters into the lower hemisphere.
		*/
	bool							m_upperHemisphereOnly;

	AABox m_bounds = AABox();

	FlyingEntity() {}
    void init(AnyTableReader& propertyTable);

	void init();

	void init(Vector2 angularSpeedRange, Vector2 motionChangePeriodRange, bool upperHemisphereOnly, Point3 orbitCenter, int paramIdx, int respawns = 0, int scaleIdx=0);

public:

    /** Destinations must be no more than 170 degrees apart to avoid ambiguity in movement direction */
    void setDestinations(const Array<Point3>& destinationArray, const Point3 orbitCenter);

	void setBounds(AABox bounds) { m_bounds = bounds; }
	AABox bounds() { return m_bounds; }

	void setSpeed(float speed) {
		m_speed = speed;
	}

	// TODO: After other implementations are complete.
    /** For deserialization from Any / loading from file */
    static shared_ptr<Entity> create 
    (const String&                  name,
     Scene*                         scene,
     AnyTableReader&                propertyTable,
     const ModelTable&				modelTable,
     const Scene::LoadOptions&		loadOptions);

	/** For programmatic construction at runtime */
	static shared_ptr<FlyingEntity> create
	(const String&						name,
		Scene*							scene,
		const shared_ptr<Model>&		model,
		const CFrame&					position);

	/** For programmatic construction at runtime */
	static shared_ptr<FlyingEntity> create
	(const String&						name,
		Scene*							scene,
		const shared_ptr<Model>&		model,
		int								scaleIdx,
		const CFrame&					position,
		const Vector2&				    speedRange,
		const Vector2&					motionChangePeriodRange,
		bool							upperHemisphereOnly,
		Point3							orbitCenter,
		int								paramIdx,
		int								respawns=0);

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
	float							m_jumpSpeed;

	/** World space point at center of orbit */
	Point3                          m_orbitCenter;
	float                           m_orbitRadius;

	/** variables for motion changes */
	float                           m_motionChangeTimer;
	float                           m_jumpTimer;

	/** jump state */
	bool                            m_inJump;
	SimTime							m_jumpTime;
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
	SimTime							m_nextJumpTime = 0;
	AABox m_bounds = AABox();

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
		float orbitRadius,
		int paramIdx,
		int respawns = 0,
		int scaleIdx=0
	);

public:
	bool respawn() {
		TargetEntity::respawn();
		m_isFirstFrame = true;
	}

	void setBounds(AABox bounds) { m_bounds = bounds; }
	AABox bounds() { return m_bounds; }

	/** For deserialization from Any / loading from file */
	static shared_ptr<Entity> create 
	(const String&                  name,
	 Scene*                         scene,
	 AnyTableReader&                propertyTable,
	 const ModelTable&              modelTable,
	 const Scene::LoadOptions&      loadOptions);

	/** For programmatic construction at runtime */
	static shared_ptr<JumpingEntity> create
	(const String&						name,
		Scene*							scene,
		const shared_ptr<Model>&		model,
		int								scaleIdx,
		const CFrame&					position,
        const Vector2&					speedRange,
        const Vector2&					motionChangePeriodRange,
        const Vector2&					jumpPeriodRange,
		const Vector2&					distanceRange,
		const Vector2&					jumpSpeedRange,
        const Vector2&					gravityRange,
		Point3							orbitCenter,
		float							orbitRadius,
		int								paramIdx,
		int								respawns=0);

	/** Converts the current VisibleEntity to an Any.  Subclasses should
		modify at least the name of the Table returned by the base class, which will be "Entity"
		if not changed. */
	virtual Any toAny(const bool forceAll = false) const override;

	virtual void onSimulation(SimTime absoluteTime, SimTime deltaTime) override;

};
