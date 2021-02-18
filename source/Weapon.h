#pragma once
#include <G3D/G3D.h>
#include "ConfigFiles.h"

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

class Weapon : Entity {
protected:
	Weapon(WeaponConfig* config, shared_ptr<Scene>& scene, shared_ptr<Camera>& cam) :
		m_config(config), m_scene(scene), m_camera(cam), m_ammo(config->maxAmmo) {
		m_lastFireTime = System::time();
	};

	shared_ptr<ArticulatedModel>    m_viewModel;						///< Model for the weapon
	shared_ptr<ArticulatedModel>    m_bulletModel;						///< Model for the "bullet"
	shared_ptr<Sound>               m_fireSound;						///< Sound for weapon firing
	shared_ptr<AudioChannel>		m_fireAudio;						///< Audio channel for fire sound
	WeaponConfig*					m_config;							///< Weapon configuration

	Array<shared_ptr<Projectile>>	m_projectiles;						///< Arrray of drawn projectiles

	int								m_lastBulletId = 0;					///< Bullet ID (auto incremented)
	int								m_ammo;								///< Remaining ammo

	RealTime						m_lastFireTime;						///< The time of the last fire event up to which time damage has been applied

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
		m_fireSound = Sound::create(System::findDataFile(m_config->fireSound), m_config->isContinuous());
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