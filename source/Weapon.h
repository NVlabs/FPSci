#pragma once
#include <G3D/G3D.h>
#include "TargetEntity.h"

class Projectile : public VisibleEntity {
protected:
	// Timed mode
	double							m_totalTime = 5.0;
	// Propagation mode
	bool							m_collision = false;
	float							m_velocity = 0.0f;
	float							m_gravity = 10.0f;
	float							m_gravVel = 0.0f;
	float							m_maxVel = 100.0f;
	Point3							m_lastPos = Point3::zero();

	Projectile() : m_totalTime(0) {}
	Projectile(const shared_ptr<VisibleEntity>& e, float velocity, bool collision = true, float gravity = 0.0f, SimTime t = 5.0) :
		VisibleEntity(*e),
		m_totalTime(t),
		m_collision(collision),
		m_velocity(velocity),
		m_gravity(gravity) {}

public:
	static shared_ptr<Projectile> create() { return createShared<Projectile>(); }
	static shared_ptr<Projectile> create(const shared_ptr<VisibleEntity>& e, SimTime t = 0) {
		return createShared<Projectile>(e, t);
	}
	static shared_ptr<Projectile> create(const shared_ptr<VisibleEntity>& e, float velocity, bool collision = true, float gravity = 0.0f, SimTime t = 5.0) {
		return createShared<Projectile>(e, velocity, collision, gravity, t);
	}
	
	void onSimulation(SimTime dt) {
		// Manage time to display
		m_totalTime -= (float)dt;
		// Update gravitational velocity component
		m_gravVel += m_gravity * (float)dt;
		m_gravVel = fmin(m_gravVel, m_maxVel);
		// Save the last position and update
		m_lastPos = frame().translation;
		setFrame(frame() + frame().lookVector()*m_velocity*(float)dt - Vector3(0,m_gravVel,0)*(float)dt);
	}

	LineSegment getCollisionSegment() {
		return LineSegment::fromTwoPoints(m_lastPos, frame().translation);
	}

	Ray getCollisionRay() {
		const Point3 currPos = frame().translation;
		return Ray::fromOriginAndDirection(currPos, (m_lastPos-currPos).unit());
	}

	Ray getDecalRay() {
		const Point3 currPos = frame().translation;
		return Ray::fromOriginAndDirection(currPos, (currPos - m_lastPos).unit());
	}

	void getLastTwoPoints(Point3& p_old, Point3& p_new) {
		p_old = m_lastPos;
		p_new = frame().translation;
	}

	double remainingTime() { return m_totalTime; }
	void clearRemainingTime() { m_totalTime = 0.0f; }

};

/** Weapon configuration class */
class WeaponConfig {
public:
	String	id = "default";												///< Id by which to refer to this weapon
	int		maxAmmo = 10000;											///< Max ammo (clicks) allowed per trial (set large for laser mode)
	float	firePeriod = 0.5;											///< Minimum fire period (set to 0 for laser mode)
	bool	autoFire = false;											///< Fire repeatedly when mouse is held? (set true for laser mode)
	float	damagePerSecond = 2.0f;										///< Damage per second delivered (compute shot damage as damagePerSecond/firePeriod)
	String	fireSound = "sound/fpsci_fire_100ms.wav"; 					///< Sound to play on fire
	float	fireSoundVol = 1.0f;										///< Volume for fire sound
	bool	loopFireSound = false;											///< Loop weapon audio (override for auto fire weapons)
	bool	renderModel = false;										///< Render a model for the weapon?
	bool	hitScan = true;												///< Is the weapon a projectile or hitscan

	//Vector3	muzzleOffset = Vector3(0, 0, 0);							///< Offset to the muzzle of the weapon model
	//bool	renderMuzzleFlash = false;									///< Render a muzzle flash when the weapon fires?

	bool	renderBullets = false;										///< Render bullets leaving the weapon
	float	bulletSpeed = 100.0f;										///< Speed to draw at for rendered rounds (in m/s)
	float	bulletGravity = 0.0f;										///< Gravity to use for bullets (default is no droop)
	Vector3 bulletScale = Vector3(0.05f, 0.05f, 2.f);					///< Scale to use on bullet object
	Color3  bulletColor = Color3(5, 4, 0);								///< Color/power for bullet emissive texture
	Vector3 bulletOffset = Vector3(0, 0, 0);								///< Offset to start the bullet from (along the look direction)

	bool	renderDecals = true;										///< Render decals when the shots miss?
	String	missDecal = "bullet-decal-256x256.png";						///< The decal to place where the shot misses
	String	hitDecal = "";												///< The decal to place where the shot hits
	int		missDecalCount = 2;											///< Number of miss decals to draw
	float	missDecalScale = 1.0f;										///< Scale to apply to the miss decal
	float	hitDecalScale = 1.0f;										///< Scale to apply to the hit decal
	float	hitDecalDurationS = 0.1f;									///< Duration to show the hit decal for (in seconds)
	float	hitDecalColorMult = 2.0f;									///< "Encoding" field (aka color multiplier) for hit decal

	float	fireSpreadDegrees = 0;										///< The spread of the fire
	String  fireSpreadShape = "uniform";								///< The shape of the fire spread distribution
	float	damageRollOffAim = 0;										///< Damage roll off w/ aim
	float	damageRollOffDistance = 0;									///< Damage roll of w/ distance

	float	scopeFoV = 0.0f;											///< Field of view when scoped
	bool	scopeToggle = false;										///< Scope toggle behavior
	//String reticleImage;												///< Reticle image to show for this weapon

	float	kickAngleDegrees = 0.0f;									///< Angle for the weapon to kick when fired
	float	kickDuration = 0.0f;										///< Kick duration

	ArticulatedModel::Specification modelSpec;							///< Model to use for the weapon (must be specified when renderModel=true)

	/** Returns true if firePeriod == 0 and autoFire == true */
	inline bool isContinuous() const { return firePeriod == 0 && autoFire; }
	inline bool loopAudio() const { return isContinuous() || (loopFireSound && autoFire); }

	WeaponConfig() {}
	WeaponConfig(const Any& any);

	Any toAny(const bool forceAll = false) const;
};

class Weapon : Entity {
protected:
	Weapon(WeaponConfig* config, shared_ptr<Scene>& scene, shared_ptr<Camera>& cam) :
		m_config(config), m_scene(scene), m_camera(cam), m_ammo(config->maxAmmo) {};

	shared_ptr<ArticulatedModel>    m_viewModel;						///< Model for the weapon
	shared_ptr<ArticulatedModel>    m_bulletModel;						///< Model for the "bullet"
	shared_ptr<Sound>               m_fireSound;						///< Sound for weapon firing
	shared_ptr<AudioChannel>		m_fireAudio;						///< Audio channel for fire sound
	WeaponConfig*					m_config;							///< Weapon configuration

	Array<shared_ptr<Projectile>>	m_projectiles;						///< Arrray of drawn projectiles

	int								m_lastBulletId = 0;					///< Bullet ID (auto incremented)
	int								m_ammo;								///< Remaining ammo

	RealTime						m_lastFireTime = 0;					///< The time of the last fire event up to which time damage has been applied

	bool							m_scoped = false;					///< Flag used for scope management

	shared_ptr<Scene>				m_scene;							///< Scene for weapon
	shared_ptr<Camera>				m_camera;							///< Camera for weapon

	std::function<void(shared_ptr<TargetEntity>)> m_hitCallback;		///< This is set to FPSciApp::hitTarget
	std::function<void(void)> m_missCallback;							///< This is set to FPSciApp::missEvent

	int										m_lastDecalID = 0;
	shared_ptr<ArticulatedModel>			m_missDecalModel;					///< Model for the miss decal
	shared_ptr<ArticulatedModel>			m_hitDecalModel;					///< Model for the hit decal
	shared_ptr<VisibleEntity>				m_hitDecal;							///< Pointer to hit decal
	RealTime								m_hitDecalTimeRemainingS = 0.f;		///< Remaining duration to show the decal for
	Array<shared_ptr<VisibleEntity>>		m_currentMissDecals;				///< Pointers to miss decals

	Random									m_rand;

public:
	static shared_ptr<Weapon> create(WeaponConfig* config, shared_ptr<Scene> scene, shared_ptr<Camera> cam) {
		return createShared<Weapon>(config, scene, cam);
	};

	//virtual void onSimulation(SimTime absoluteTime, SimTime deltaTime) override;

	void resetCooldown() { m_lastFireTime = 0; }
	bool canFire(RealTime now) const;
	RealTime lastFireTime() { return m_lastFireTime; }
	/** 
		Returns a value between 0 and 1 that is the difference between 
		now and the last fire time over the weapon cooldown period
	*/
	float cooldownRatio(RealTime now) const;
	/** 
		Returns a value between 0 and 1 that is the difference between 
		now and the last fire time over the duration
	*/
	float cooldownRatio(RealTime now, float duration) const;
	/**
		Returns the damage per shot 
	*/
	float damagePerShot() const;
	const WeaponConfig* config() const { return m_config; }

	int remainingAmmo() const { 
		if (isNull(m_config)) return 100;
		return m_ammo; 
	}
	int shotsTaken() const { return m_config->maxAmmo - m_ammo; }
	void reload() { m_ammo = m_config->maxAmmo; }

	/**
		targets is the list of targets to try to hit
		Ignore anything in the dontHit list
		dummyShot controls whether it's a shot at the test target (is this true?)
		targetIdx, hitDist and hitInfo are all returned along with the targetEntity that was hit
	*/
	shared_ptr<TargetEntity> fire(const Array<shared_ptr<TargetEntity>>& targets,
		int& targetIdx,
		float& hitDist, 
		Model::HitInfo& hitInfo, 
		Array<shared_ptr<Entity>>& dontHit,
		bool dummyShot);

	// Records provided lastFireTime 
	void setLastFireTime(RealTime lastFireTime);
	// Computes duration from last fire time until given currentTime
	RealTime fireDurationUntil(RealTime currentTime);
	// Computes an integer number of shots that can happen until given currentTime
	int numShotsUntil(RealTime currentTime);

	void onPose(Array<shared_ptr<Surface> >& surface);

	void loadSounds() {
		// Check for play mode specific parameters
		if (notNull(m_fireAudio)) { m_fireAudio->stop(); }
		if(!m_config->fireSound.empty()) m_fireSound = Sound::create(System::findDataFile(m_config->fireSound), m_config->loopAudio());
		else { m_fireSound = nullptr; }
	}
	// Plays the sound based on the weapon fire mode
	void playSound(bool shotFired, bool shootButtonUp);
	
	void setHitCallback(std::function<void(shared_ptr<TargetEntity>)> callback) { m_hitCallback = callback; }
	void setMissCallback(std::function<void(void)> callback) { m_missCallback = callback; }
	
	void setConfig(WeaponConfig* config) { m_config = config; }
	void setCamera(const shared_ptr<Camera>& cam) { m_camera = cam; }
	void setScene(const shared_ptr<Scene>& scene) { m_scene = scene; }
	void setScoped(bool state = true) { m_scoped = state; }

	void simulateProjectiles(SimTime sdt, const Array<shared_ptr<TargetEntity>>& targets, const Array<shared_ptr<Entity>>& dontHit = {});
	void drawDecal(const Point3& point, const Vector3& normal, bool hit = false);
	void clearDecals();
	void loadDecals();
	void loadModels();

	bool scoped() { return m_scoped;  }
};