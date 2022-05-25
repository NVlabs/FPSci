#pragma once
#include <G3D/G3D.h>

class PlayerEntity : public VisibleEntity {
protected:
    Vector3         m_desiredOSVelocity;
    /** In object-space */
    Sphere          m_collisionProxySphere;

    // Radians per frame
    float           m_desiredYawVelocity;
    float           m_desiredPitchVelocity;

    // Radians
	float           m_spawnHeadingRadians = 0.0f;
	float           m_headingRadians = 0.0f;
    /** Unused for rendering, for use by a fps cam. */
    float           m_headTilt;

	float			m_respawnHeight = fnan();
	Point3			m_respawnPosition;

	RealTime		m_lastJumpTime;

	bool			m_crouched = false;					///< Is the player crouched?
	bool			m_inAir = true;						///< Is the player in the air (i.e. not in collision w/ a ground plane)?
	float			m_lastJumpVelocity;
	float			m_health = 1.0f;					///< Player health storage

	bool			m_inContact = false;				///< Is the player in contact w/ anything?
	bool			m_motionEnable = true;				///< Flag to disable player motion
	bool			m_jumpPressed = false;				///< Indicates whether jump buton was pressed

    PlayerEntity() {}

#ifdef G3D_OSX
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Woverloaded-virtual"
#endif
    void init(AnyTableReader& propertyTable);
    void init(const Sphere& collisionSphere);
#ifdef G3D_OSX
    #pragma clang diagnostic pop
#endif

public:
	float			m_cameraRadiansPerMouseDot;		///< Player mouse sensitivity
	Vector2			turnScale;				///< Player asymmetric mouse scaler - typically near 1:1

	float*			moveRate = nullptr;	        ///< Player movement rate (m/s)
	Vector2*		moveScale = nullptr;	    ///< Player X/Y movement scale vector (interpreted as unit vector)
	Array<bool>*	axisLock = nullptr;		    ///< World-space axis lock
	
	float*			jumpVelocity = nullptr;		///< Player vertical (+Y) jump velocity
	float*			jumpInterval = nullptr;		///< Player minimum jump interval limit
	bool*			jumpTouch = nullptr;	    ///< Require contact for jump?

	float*			height = nullptr;			///< Player height when standing
	float*			crouchHeight = nullptr;		///< Player height when crouched

    /** \brief Computes all triangles that could be hit during a
        slideMove with the current \a velocity, allowing that the
        velocity may be decreased along some axes during movement.

        Called from slideMove(). */
    void getConservativeCollisionTris(Array<Tri>& triArray, const Vector3& velocity, float deltaTime) const;
    
    /** Finds the first collision between m_collisionProxySphere
        travelling with \a velocity and the triArray.  Travels for at
        most \a stepTime, and updates \a stepTime with the
        collision time if there is one.  Returns true if there is a
        collision before the end of the original \a stepTime.

        \param collisionNormal Inward-pointing normal to the sphere at
        the collision time (separating axis).
    */
    bool findFirstCollision
    (const Array<Tri>&      triArray, 
     const Vector3&         velocity, 
     float&                 stepTime, 
     Vector3&               collisionNormal,
     Point3&                collisionPoint) const;

    /** Moves linearly for deltaTime using the current
     m_desiredLinearVelocity, decreasing velocity as needed to avoid
     collisions.  Called from onSimulation(). */
    bool slideMove(SimTime deltaTime);

	float heightOffset(float height) const;
    float respawnPosHeight()  { return m_respawnPosition.y; }
    bool doDamage(float damage);

    /** In world space */
    Sphere collisionProxy() const {
        return Sphere(m_frame.pointToWorldSpace(m_collisionProxySphere.center), m_collisionProxySphere.radius);
    }

	const CFrame getCameraFrame() const {
		CFrame f = frame();
        if (notNull(height)) {
            f.translation += Point3(0.0f, heightOffset(m_crouched ? *crouchHeight : *height), 0.0f);
        }
		return f;
	}

	void setCrouched(bool crouched) { m_crouched = crouched; };
	void setJumpPressed(bool pressed=true) { m_jumpPressed = pressed; }
	void setMoveEnable(bool enabled) { m_motionEnable = enabled; }

	void setRespawnPosition(Point3 pos) { m_respawnPosition = pos; }
    void setRespawnHeadingDegrees(float headingDeg) { m_spawnHeadingRadians = pif() / 180.f * headingDeg; }
	void setRespawnHeight(float height) { m_respawnHeight = height; }

	void respawn() {
		m_frame.translation = m_respawnPosition;
		m_headingRadians = m_spawnHeadingRadians;
		m_headTilt = 0.0f;                              // Reset heading tilt
        m_inAir = true;                                 // Set in air to let player "fall" if needed
		setDesiredOSVelocity(Vector3::zero());
		setDesiredAngularVelocity(0.0f, 0.0f);
	}

	float health(void) { return m_health; }
    /** In radians... not used for rendering, use for first-person cameras */
    float headTilt() const { return m_headTilt; }
    float heading() const { return m_headingRadians;  }
    float respawnHeadingDeg() const { return m_spawnHeadingRadians * 180 / pif(); }

    /** For deserialization from Any / loading from file */
    static shared_ptr<Entity> create 
    (const String&                           name,
     Scene*                                  scene,
     AnyTableReader&                         propertyTable,
     const ModelTable&                       modelTable,
     const Scene::LoadOptions&               loadOptions);

    /** For programmatic construction at runtime */
    static shared_ptr<Entity> create 
    (const String&                           name,
     Scene*                                  scene,
     const CFrame&                           position,
     const shared_ptr<Model>&                model);

    void setDesiredOSVelocity(const Vector3& objectSpaceVelocity) {  m_desiredOSVelocity = objectSpaceVelocity; }
    const Vector3& desiredOSVelocity() { return m_desiredOSVelocity; }

    void setDesiredAngularVelocity(const float y, const float p) {
        m_desiredYawVelocity    = y;
        m_desiredPitchVelocity  = p;
    }

    /** Converts the current VisibleEntity to an Any.  Subclasses should
        modify at least the name of the Table returned by the base class, which will be "PlayerEntity"
        if not changed. */
    virtual Any toAny(const bool forceAll = false) const override;
    
    virtual void onPose(Array<shared_ptr<Surface> >& surfaceArray) override;
	virtual void onSimulation(SimTime absoluteTime, SimTime deltaTime) override;
	void updateFromInput(UserInput* ui);

};
