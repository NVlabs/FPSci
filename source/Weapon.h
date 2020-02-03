#pragma once
#include <G3D/G3D.h>
#include "ConfigFiles.h"

class Projectile {
public:
	shared_ptr<VisibleEntity>       entity;
	/** When in hitscan mode */
	RealTime                        endTime;
	Projectile() : endTime(0) {}
	Projectile(const shared_ptr<VisibleEntity>& e, RealTime t = 0) : entity(e), endTime(t) {}
};


class Weapon : Entity {
public:
	static shared_ptr<Weapon> create(shared_ptr<WeaponConfig> config) {
		return createShared<Weapon>(config);
	};

	shared_ptr<TargetEntity> fire(const Array<shared_ptr<TargetEntity>>& targets, 
		int& targetIdx,
		float& hitDist, 
		Model::HitInfo &hitInfo, 
		Array<shared_ptr<Entity>> dontHit = Array<shared_ptr<Entity>>());

	void onSimulation(RealTime rdt, const shared_ptr<Scene>& scene);
	void onPose(Array<shared_ptr<Surface> >& surface, const shared_ptr<Camera>& camera);

	void loadModels() {
		m_viewModel = ArticulatedModel::create(m_config->modelSpec, "viewModel");
		const static Any bulletSpec = PARSE_ANY(ArticulatedModel::Specification{
			filename = "ifs/d10.ifs";
			preprocess = {
				transformGeometry(all(), Matrix4::pitchDegrees(90));
				transformGeometry(all(), Matrix4::scale(0.05,0.05,2));
				setMaterial(all(), UniversalMaterial::Specification {
					lambertian = Color3(0);
					emissive = Power3(5,4,0);
				});
			}; 
		});
		m_bulletModel = ArticulatedModel::create(bulletSpec, "bulletModel");
	}

	void loadSounds() {
		// Check for play mode specific parameters
		m_fireSound = Sound::create(System::findDataFile(m_config->fireSound));

	}

	void setConfig(const WeaponConfig& config) {
		m_config = std::make_shared<WeaponConfig>(config);
	}
	void setCamera(const shared_ptr<Camera>& cam) {
		m_camera = cam;
	}

	void setScene(const shared_ptr<Scene>& scene) {
		m_scene = scene;
	}

protected:
	Weapon(shared_ptr<WeaponConfig> config) : m_config(config) {};
	
	shared_ptr<ArticulatedModel>    m_viewModel;						///< Model for the weapon
	shared_ptr<ArticulatedModel>    m_bulletModel;						///< Model for the "bullet"
	shared_ptr<Sound>               m_fireSound;						///< Sound for weapon firing

	shared_ptr<WeaponConfig>		m_config;
	int								m_lastBulletId = 0;

	shared_ptr<Scene>				m_scene;
	shared_ptr<Camera>				m_camera;

	Array<Projectile>               m_projectileArray;					///< Arrray of drawn projectiles
};