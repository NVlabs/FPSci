#pragma once
#include <G3D/G3D.h>


class TargetConfig;

//#define DRAW_BOUNDING_SPHERES	1		// Uncomment this to draw bounding spheres (useful for target sizing)
#define BOUNDING_SPHERE_RADIUS	0.5		///< Use a 0.5m radius for sizing here

// Scale and offset for target
const float TARGET_MODEL_ARRAY_SCALING = 0.2f;
const float TARGET_MODEL_ARRAY_OFFSET = 20;

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
	String	m_id;									///< Target ID
	float	m_health			= 1.0f;				///< Target health
	Color3	m_color				= Color3::red();	///< Default color
	int		destinationIdx		= 0;				///< Current index into the destination array
	SimTime m_spawnTime			= 0;				///< Time initiatlly spawned
	int		m_respawnCount		= 0;				///< Number of times to respawn
	int		m_paramIdx			= -1;				///< Parameter index of this item
	bool	m_worldSpace		= false;			///< World space coordiantes?
	int		m_scaleIdx			= 0;				///< Index for scaled model
	bool	m_isLogged			= true;				///< Control flag for logging
	Point3	m_offset;								///< Offset for initial spawn
	bool	m_canHit			= true;				///< Can this target be hit?	
	Array<Destination> m_destinations;				///< Array of destinations to visit
	shared_ptr<Sound> m_hitSound;					///< Sound to play when hit
	float m_hitSoundVol;							///< Volume to play hit sound at
	shared_ptr<Sound> m_destroyedSound;				///< Sound to play when destroyed
	float m_destroyedSoundVol;						///< Volume to play destroyed sound at

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
		int								scaleIdx,
		int								paramIdx,
		bool							isLogged=false
	);

	static shared_ptr<TargetEntity> create(
		shared_ptr<TargetConfig>		config,
		const String&					name,
		Scene*							scene,
		const shared_ptr<Model>&		model,
		const Point3&					offset,
		int								scaleIdx,
		int								paramIdx
	);

	void init(Array<Destination> dests, int paramIdx, Point3 staticOffset = Point3(0.0, 0.0, 0.0), int respawnCount = 0, int scaleIdx = 0, bool isLogged = true) {
		m_offset = staticOffset;
		m_respawnCount = respawnCount;
		m_paramIdx = paramIdx;
		m_scaleIdx = scaleIdx;
		m_isLogged = isLogged;
		destinationIdx = 0;
	}

	void setColor(const Color3& color) {
		UniversalMaterial::Specification materialSpecification;
		materialSpecification.setLambertian(Texture::Specification(color));
		materialSpecification.setEmissive(Texture::Specification(color * 0.7f));
		materialSpecification.setGlossy(Texture::Specification(Color4(0.4f, 0.2f, 0.1f, 0.8f)));

		const shared_ptr<ArticulatedModel::Pose>& amPose = ArticulatedModel::Pose::create();
		amPose->materialTable.set("core/icosahedron_default", UniversalMaterial::create(materialSpecification));
		setPose(amPose);
	}

	void setWorldSpace(bool worldSpace) { m_worldSpace = worldSpace; }
	void setCanHit(bool active) { m_canHit = active; }

	void setHitSound(const String& hitSoundFilename, float hitSoundVol = 1.0f) {
		if (hitSoundFilename == "") { m_hitSound = nullptr; }
		else { 
			m_hitSound = Sound::create(System::findDataFile(hitSoundFilename)); 
			m_hitSoundVol = hitSoundVol;
		}
	}

	void setDestoyedSound(const String& destroyedSoundFilename, float destroyedSoundVol = 1.0f){
		if (destroyedSoundFilename == "") { m_destroyedSound = nullptr;  }
		else {
			m_destroyedSound = Sound::create(System::findDataFile(destroyedSoundFilename));
			m_destroyedSoundVol = destroyedSoundVol;
		}
	}

	void playHitSound(float volume=0.0f) {
		if (notNull(m_hitSound)) {
			if (volume > 0.0f) { m_hitSound->play(volume); }
			else { m_hitSound->play(m_hitSoundVol); }
		}
	}

	void playDestroySound(float volume = 0.0f) {
		if (notNull(m_destroyedSound)) {
			if (volume > 0.0f) { m_destroyedSound->play(volume);  }
			else { m_destroyedSound->play(m_destroyedSoundVol); }
		}
	}

	/**Apply damage to health and return true if health <= 0 */
	bool doDamage(float damage) {
		m_health -= damage;
		return m_health <= 0;
	}
	
	bool tryRespawn() {
		if (m_respawnCount == 0) {		// Target does not respawn
			return false;
		} else if(m_respawnCount > 0){	// Target respawns 
			m_respawnCount -= 1;
		}
		respawn();
		return true;					// Also returns true for any target w/ negative m_respawnCount
	}

	void respawn() {
		// Reset target parameters
		m_spawnTime = 0;
		m_health = 1.0f;
	}

	void resetMotionParams() {
		m_nextChangeTime = 0;
	}
	
	/** Get the target ID */
	const String& id() const { return m_id; }
	/** Getter for scale index */
	int scaleIndex() const { return m_scaleIdx; }
	/** Getter for logging */
	bool isLogged() const { return m_isLogged; }
	/** Getter for health */
	float health() const { return m_health; }
	/** Getter for the total time for a path*/
	float getPathTime() const { return m_destinations.last().time; }
	/** Get the target size */
	float size() const { return pow(1.0f + TARGET_MODEL_ARRAY_SCALING, m_scaleIdx - TARGET_MODEL_ARRAY_OFFSET); }
	/** Getter for the target destinations */
	Array<Destination> destinations() const { return m_destinations; }
	/** Getter for remaining respawn count */
	int respawnsRemaining() const { return m_respawnCount; }
	/** Getter for parameter index */
	int paramIdx() const { return m_paramIdx; }
	/** Getter for active/can hit */
	bool canHit() const { return m_canHit; }

	void drawHealthBar(RenderDevice* rd, const Camera& camera, const Framebuffer& framebuffer, Point2 size, Point3 offset, Point2 border, Array<Color4> colors, Color4 borderColor) const;
	virtual void onSimulation(SimTime absoluteTime, SimTime deltaTime) override;
	void setDestinations(const Array<Destination> destinationArray);

};

class FlyingEntity : public TargetEntity {
protected:
	float			m_speed = 0.0f;									///< Speed of the target (deg/s or m/s depending on space)
	Point3			m_orbitCenter;									///< World space point at center of orbit
	Vector2			m_angularSpeedRange = Vector2{ 0.0f, 4.0f };	///< Angular Speed Range(deg / s) x = min y = max
    Vector2			m_motionChangePeriodRange = Vector2{ 10000.0f, 10000.0f };	  ///< Motion Change period in seconds (x=min y=max)

    /** The target will move through these points along arcs around
        m_orbitCenter at m_speed. As each point is hit, it is
        removed from the queue.*/
    Queue<Point3>	m_destinationPoints;

	/** Limits flying target entity motion for the upper hemisphere only.
		OnSimulation will y-invert target position & destination points
		whenever the target enters into the lower hemisphere.
		*/
	bool			m_upperHemisphereOnly;
	AABox			m_bounds = AABox();							///< Bounds (for world space motion)
	bool			m_axisLocks[3] = { false };					///< Axis locks (for world space motion)

	FlyingEntity() {}
    void init(AnyTableReader& propertyTable);

	void init();

	void init(Vector2 angularSpeedRange, Vector2 motionChangePeriodRange, bool upperHemisphereOnly, Point3 orbitCenter, int paramIdx, Array<bool> axisLock, int respawns = 0, int scaleIdx=0, bool isLogged=true);

public:
	bool tryRespawn() {
		TargetEntity::tryRespawn();
		// clear all destination points
		m_destinationPoints.fastClear();
	}

    /** Destinations must be no more than 170 degrees apart to avoid ambiguity in movement direction */
    void setDestinations(const Array<Point3>& destinationArray, const Point3 orbitCenter);

	void setBounds(AABox bounds) { m_bounds = bounds; }
	AABox bounds() { return m_bounds; }

	void setSpeed(float speed) {
		m_speed = speed;
	}

	// TODO: After other implementations are complete.
    /** For deserialization from Any / loading from file */
    static shared_ptr<Entity> create (
	const String&					name,
     Scene*                         scene,
     AnyTableReader&                propertyTable,
     const ModelTable&				modelTable,
     const Scene::LoadOptions&		loadOptions
	);

	/** For programmatic construction at runtime */
	static shared_ptr<FlyingEntity> create(
		const String&					name,
		Scene*							scene,
		const shared_ptr<Model>&		model,
		const CFrame&					position
	);

	static shared_ptr<FlyingEntity> create(
		shared_ptr<TargetConfig>		config,
		const String&					name,
		Scene*							scene,
		const shared_ptr<Model>&		model,
		const Point3&					orbitCenter,
		int								scaleIdx,
		int								paramIdx
	);

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
	Point2			m_speed;

	/** Position is calculated as sphieral motion + jump.
	Animation is done by projecting it toward the spherical surface. */
	Point3          m_simulatedPos;

	/** Parameters reset every time motion change or jump happens */
	float           m_planarSpeedGoal;			///< the speed value m_speed tries to approach.
	Point2          m_acc;						///< Acceleration storage
	float			m_jumpSpeed;				///< Jump speed storage

	Point3          m_orbitCenter;				///< World-space center of orbit (player space)
	float           m_orbitRadius;				///< Radius of orbit path (player space)
	
	float           m_motionChangeTimer;		///< Time remaining to motion change (in seconds)
	float			m_jumpTimer;				///< Time remaining in jump (in seconds)
	
	bool			m_inJump;					///< In a jump currently?
	SimTime			m_jumpTime;					///< Time at which a jump started
	float           m_standingHeight;			///< Default or "pre-jump" height
	Vector2			m_angularSpeedRange;		///< Angular Speed Range in deg/s (x=min y=max)
	Vector2         m_motionChangePeriodRange;	///< Motion Change period in seconds (x=min y=max)
    Vector2			m_jumpPeriodRange;			///< Jump period in seconds (x=min y=max)
    Vector2         m_jumpSpeedRange;			///< Jump initial speed in m/s (x=min y=max)
    Vector2			m_gravityRange;				///< Gravitational Acceleration in m/s^2 (x=min y=max)

	Vector2         m_distanceRange;
	float           m_planarAcc = 0.3f;
	bool            m_isFirstFrame = true;		///< Initializer flag
	SimTime			m_nextJumpTime = 0;			///< Next time at which to jump
	
	AABox			m_moveBounds = AABox();
	bool			m_axisLocks[3] = { false };	///< Axis locks (for world space motion)


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
		Array<bool> axisLock,
		int respawns = 0,
		int scaleIdx=0,
		bool isLogged=true
	);

public:
	bool tryRespawn() {
		TargetEntity::tryRespawn();
		m_isFirstFrame = true;
	}

	void setMoveBounds(AABox bounds) { m_moveBounds = bounds; }

	/** For deserialization from Any / loading from file */
	static shared_ptr<Entity> create (
		const String&					name,
		Scene*							scene,
		AnyTableReader&					propertyTable,
		const ModelTable&				modelTable,
		const Scene::LoadOptions&		loadOptions
	);

	/** For programmatic construction at runtime */
	static shared_ptr<JumpingEntity> create(
		shared_ptr<TargetConfig>		config,
		const String&					name,
		Scene*							scene,
		const shared_ptr<Model>&		model,
		int								scaleIdx,
		const Point3&					orbitCenter,
		float							targetDistance,
		int								paramIdx
	);

	/** Converts the current VisibleEntity to an Any.  Subclasses should
		modify at least the name of the Table returned by the base class, which will be "Entity"
		if not changed. */
	virtual Any toAny(const bool forceAll = false) const override;

	virtual void onSimulation(SimTime absoluteTime, SimTime deltaTime) override;

};
