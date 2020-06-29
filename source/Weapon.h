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
	Weapon(shared_ptr<WeaponConfig> config, shared_ptr<Scene>& scene, shared_ptr<Camera>& cam) :
		m_config(config), m_scene(scene), m_camera(cam) {};

	shared_ptr<ArticulatedModel>    m_viewModel;						///< Model for the weapon
	shared_ptr<ArticulatedModel>    m_bulletModel;						///< Model for the "bullet"
	shared_ptr<Sound>               m_fireSound;						///< Sound for weapon firing
	shared_ptr<AudioChannel>		m_fireAudio;						///< Audio channel for fire sound

	Array<shared_ptr<Projectile>>	m_projectiles;						///< Arrray of drawn projectiles

	shared_ptr<WeaponConfig>		m_config;							///< Weapon configuration
	int								m_lastBulletId = 0;					///< Bullet ID (auto incremented)

	bool							m_scoped = false;					///< Flag used for scope management
	bool							m_firing = false;					///< Flag used for auto fire management

	shared_ptr<Scene>				m_scene;							///< Scene for weapon
	shared_ptr<Camera>				m_camera;							///< Camera for weapon

	std::function<void(shared_ptr<TargetEntity>)> m_hitCallback;
	std::function<void(void)> m_missCallback;

	int										m_lastDecalID = 0;
	shared_ptr<ArticulatedModel>			m_missDecalModel;					///< Model for the miss decal
	shared_ptr<ArticulatedModel>			m_hitDecalModel;					///< Model for the hit decal
	shared_ptr<VisibleEntity>				m_hitDecal;							///< Pointer to hit decal
	RealTime								m_hitDecalTimeRemainingS = 0.f;		///< Remaining duration to show the decal for
	Array<shared_ptr<VisibleEntity>>		m_currentMissDecals;				///< Pointers to miss decals

public:
	static shared_ptr<Weapon> create(shared_ptr<WeaponConfig> config, shared_ptr<Scene> scene, shared_ptr<Camera> cam) {
		return createShared<Weapon>(config, scene, cam);
	};

	shared_ptr<TargetEntity> fire(const Array<shared_ptr<TargetEntity>>& targets,
		int& targetIdx,
		float& hitDist, 
		Model::HitInfo& hitInfo, 
		Array<shared_ptr<Entity>>& dontHit);

	void onPose(Array<shared_ptr<Surface> >& surface);

	void loadSounds() {
		// Check for play mode specific parameters
		if (notNull(m_fireAudio)) { m_fireAudio->stop(); }
		m_fireSound = Sound::create(System::findDataFile(m_config->fireSound), m_config->isLaser());
	}
	
	void setHitCallback(std::function<void(shared_ptr<TargetEntity>)> callback) { m_hitCallback = callback; }
	void setMissCallback(std::function<void(void)> callback) { m_missCallback = callback; }
	
	void setConfig(const WeaponConfig& config) { m_config = std::make_shared<WeaponConfig>(config); }
	void setCamera(const shared_ptr<Camera>& cam) { m_camera = cam; }
	void setScene(const shared_ptr<Scene>& scene) { m_scene = scene; }
	void setScoped(bool state = true) { m_scoped = state; }

	void setFiring(bool firing = true) {
		if (firing && !m_firing) {
			m_fireAudio = m_fireSound->play();
		}
		else if (m_firing && !firing && notNull(m_fireAudio)) {
			m_fireAudio->stop();
		}
		m_firing = firing;
	}

	void simulateProjectiles(SimTime sdt, const Array<shared_ptr<TargetEntity>>& targets, const Array<shared_ptr<Entity>>& dontHit = {});
	void drawDecal(const Point3& point, const Vector3& normal, bool hit = false);
	void loadDecals();
	void loadModels();

	bool scoped() { return m_scoped;  }
	bool firing() { return m_firing; }
};