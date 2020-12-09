#include "Weapon.h"

void Weapon::loadDecals() {
	if (m_config->missDecal.empty()) {
		m_missDecalModel.reset();
	}
	else {
		String missDecalSpec = format("\
			ArticulatedMode::Specification {\
				filename = \"ifs/square.ifs\";\
				preprocess = {\
					transformGeometry(all(), Matrix4::scale(0.1, 0.1, 0.1));\
					setMaterial(all(), UniversalMaterial::Specification{\
						lambertian = Texture::Specification {\
							filename = \"%s\";\
							encoding = Color3(1, 1, 1);\
						};\
					});\
				};\
				scale = %f;\
			};", m_config->missDecal.c_str(), m_config->missDecalScale);
		m_missDecalModel = ArticulatedModel::create(Any::parse(missDecalSpec), "missDecalModel");
	}

	if (m_config->hitDecal.empty()) {
		m_hitDecalModel.reset();
	}
	else {
		const float cmul = m_config->hitDecalColorMult;
		String hitDecalSpec = format("\
			ArticulatedMode::Specification {\
				filename = \"ifs/square.ifs\";\
				preprocess = {\
					transformGeometry(all(), Matrix4::scale(0.1, 0.1, 0.1));\
					setMaterial(all(), UniversalMaterial::Specification{\
						lambertian = Texture::Specification {\
							filename = \"%s\";\
							encoding = Color3(%f, %f, %f);\
						};\
					});\
				};\
				scale = %f;\
			};", m_config->hitDecal.c_str(), cmul, cmul, cmul, m_config->hitDecalScale);
		m_hitDecalModel = ArticulatedModel::create(Any::parse(hitDecalSpec), "hitDecalModel");
	}
}

void Weapon::loadModels() {
	// Load decals
	loadDecals();

	// Create the view model
	if (m_config->modelSpec.filename != "") {
		m_viewModel = ArticulatedModel::create(m_config->modelSpec, "viewModel");
	}
	else {
		const static Any modelSpec = PARSE_ANY(ArticulatedModel::Specification{
			filename = "model/sniper/sniper.obj";
			preprocess = {
				transformGeometry(all(), Matrix4::yawDegrees(90));
				transformGeometry(all(), Matrix4::scale(1.2,1,0.4));
			};
			scale = 0.25;
			});
		m_viewModel = ArticulatedModel::create(modelSpec, "viewModel");
	}

	// Create the bullet model
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

void Weapon::onPose(Array<shared_ptr<Surface> >& surface) {
	if (m_camera && (m_config->renderModel || m_config->renderBullets)) { // || m_config->renderMuzzleFlash) {
		// Update the weapon frame for all of these cases
		const float yScale = -0.12f;
		const float zScale = -yScale * 0.5f;
		float kick = 0.f;
		const RealTime timeSinceFire = timeSinceLastFire();
		if (timeSinceFire < m_config->kickDuration) {
			kick = m_config->kickAngleDegrees * sinf((float)timeSinceFire / m_config->kickDuration * pif());
		}
		const float lookY = m_camera->frame().lookVector().y - 6.f * sin(2 * pif() / 360.0f * kick);
		m_frame = m_camera->frame() * CFrame::fromXYZYPRDegrees(0.3f, -0.4f + lookY * yScale, -1.1f + lookY * zScale, 10, 5+kick);
		// Pose the view model (weapon) for render here
		if (m_config->renderModel) {
			const float prevLookY = m_camera->previousFrame().lookVector().y;
			const CFrame prevWeaponPos = CFrame::fromXYZYPRDegrees(0.3f, -0.4f + prevLookY * yScale, -1.1f + prevLookY * zScale, 10, 5);
			m_viewModel->pose(surface, m_frame, m_camera->previousFrame() * prevWeaponPos, nullptr, nullptr, nullptr, Surface::ExpressiveLightScatteringProperties());
		}
	}
}

void Weapon::simulateProjectiles(SimTime sdt, const Array<shared_ptr<TargetEntity>>& targets, const Array<shared_ptr<Entity>>& dontHit) {
	// Iterate through projectiles for hit/miss detection here
	for (int p = 0; p < m_projectiles.size(); p++) {
		shared_ptr<Projectile> projectile = m_projectiles[p];
		projectile->onSimulation(sdt);
		// Remove the projectile for timeout
		if (projectile->remainingTime() <= 0) {
			// Expire
			m_scene->removeEntity(projectile->name());
			m_projectiles.remove(p);
			--p;
		}
		else if (!m_config->hitScan) {
			// Distance at which to delcare a hit
			const float hitThreshold = m_config->bulletSpeed * 2.0f * (float)sdt;
			// Look for collision with the targets
			const Ray ray = projectile->getCollisionRay();
			float closest = finf();
			Model::HitInfo info;
			shared_ptr<TargetEntity> closestTarget;
			for (shared_ptr<TargetEntity> t : targets) {
				if (t->intersect(ray, closest, info)) {
					closestTarget = t;
				}
			}
			// Check for target hit
			if (closest < hitThreshold) {
				m_hitCallback(closestTarget);
				// Offset position slightly along normal to avoid Z-fighting the target
				drawDecal(info.point + 0.01 * info.normal, m_camera->frame().lookVector(), true);
				projectile->clearRemainingTime();
			}
			// Handle (miss) decals here
			else {
				// Build a list of entities not to hit in the scene
				Array<shared_ptr<Entity>> dontHitItems = dontHit;
				dontHitItems.append(m_currentMissDecals);
				dontHitItems.append(targets);					// This is a miss, don't plan to hit targets here
				dontHitItems.append(m_projectiles);
				// Check for closest hit (in scene, otherwise this ray hits the skybox)
				//closest = finf();
				const Ray ray = projectile->getDecalRay();
				m_scene->intersect(ray, closest, false, dontHitItems, info);

				// If we are within 2 simulation cycles of a wall, create the decal
				if (closest < hitThreshold) {
					// Offset position slightly along normal to avoid Z-fighting the wall
					drawDecal(info.point + 0.01 * info.normal, info.normal);
					projectile->clearRemainingTime();							// Stop the projectile here
					m_missCallback();
				}
			}
		}
	}

	// Handle hit "animation" (i.e. remove when done)
	if (notNull(m_hitDecal) && m_hitDecalTimeRemainingS <= 0) {
		m_scene->remove(m_hitDecal);
		m_hitDecal.reset();
	}
	else {
		m_hitDecalTimeRemainingS -= sdt;
	}
}

void Weapon::drawDecal(const Point3& point, const Vector3& normal, bool hit) {
	// End here if we're not drawing decals
	if (!m_config->renderDecals) return;
	// Don't draw decals for these cases
	if (!hit && (m_config->missDecalCount == 0 || m_missDecalModel == nullptr)) return;
	else if (hit && m_hitDecalModel == nullptr) return;

	// Set the decal rotation to match the normal here
	CFrame decalFrame = CFrame(point);
	decalFrame.lookAt(decalFrame.translation - normal);

	// If we have the maximum amount of decals remove the oldest one
	if (!hit) {
		while (m_currentMissDecals.size() >= m_config->missDecalCount) {
			m_scene->remove(m_currentMissDecals.pop());
		}
	}
	// Handle hit decal here (only show 1 at a time)
	else if (hit && notNull(m_hitDecal)) {
		m_scene->remove(m_hitDecal);
	}

	// Add the new decal to the scene
	shared_ptr<ArticulatedModel> decalModel = hit ? m_hitDecalModel : m_missDecalModel;
	const shared_ptr<VisibleEntity>& newDecal = VisibleEntity::create(format("decal%03d", ++m_lastDecalID), &(*m_scene), decalModel, decalFrame);
	newDecal->setCastsShadows(false);
	m_scene->insert(newDecal);
	if (!hit) m_currentMissDecals.insert(0, newDecal);	// Add the new decal to the front of the Array (if a miss)
	else {
		m_hitDecal = newDecal;
		m_hitDecalTimeRemainingS = m_config->hitDecalDurationS;
	}
}

void Weapon::clearDecals() {
	while (m_currentMissDecals.size() > 0) { m_scene->remove(m_currentMissDecals.pop()); }
	if (notNull(m_hitDecal)) { m_scene->remove(m_hitDecal); }
}

shared_ptr<TargetEntity> Weapon::fire(
	const Array<shared_ptr<TargetEntity>>& targets,
	int& targetIdx, 
	float& hitDist, 
	Model::HitInfo& hitInfo, 
	Array<shared_ptr<Entity>>& dontHit,
	bool dummyShot)
{
	static RealTime lastTime;
	m_lastFireAt = System::time();						// Capture the time

	Ray ray = m_camera->frame().lookRay();		// Use the camera lookray for hit detection
	float spread = m_config->fireSpreadDegrees * 2.f * pif() / 360.f;

	// ignore bullet spread on dummy targets
	if (dummyShot) { spread = 0.f; }
	else { m_ammo -= 1; }

	// Apply random rotation (for fire spread)
	Matrix3 rotMat = Matrix3::fromEulerAnglesXYZ(0.f,0.f,0.f);
	if (m_config->fireSpreadShape == "uniform") {
		rotMat = Matrix3::fromEulerAnglesXYZ(m_rand.uniform(-spread / 2, spread / 2), m_rand.uniform(-spread / 2, spread / 2), 0);
	}
	else if (m_config->fireSpreadShape == "gaussian") {
		rotMat = Matrix3::fromEulerAnglesXYZ(m_rand.gaussian(0, spread / 3), m_rand.gaussian(0, spread / 3), 0);
	}
	Vector3 dir = Vector3(0.f, 0.f, -1.f) * rotMat;
	ray.set(ray.origin(), m_camera->frame().rotation * dir);

	// Check for closest hit (in scene, otherwise this ray hits the skybox)
	float closest = finf();
	Array<shared_ptr<Entity>> dontHitItems = dontHit;
	dontHitItems.append(m_currentMissDecals);
	dontHitItems.append(targets);
	dontHitItems.append(m_projectiles);
	m_scene->intersect(ray, closest, false, dontHitItems, hitInfo);
	if (closest < finf()) { hitDist = closest; }

	// Create the bullet (if we need to draw it or are using non-hitscan behavior)
	if (m_config->renderBullets || !m_config->hitScan) {
		// Create the bullet start frame from the weapon frame plus muzzle offset
		CFrame bulletStartFrame = m_camera->frame();
		
		// Apply bullet offset w/ camera rotation here
		bulletStartFrame.translation += ray.direction() * m_config->bulletOffset;

		// Angle the bullet start frame towards the aim point
		Point3 aimPoint = m_camera->frame().translation + ray.direction() * 1000.0f;
		// If we hit the scene w/ this ray, angle it towards that collision point
		if (closest < finf()) {
			aimPoint = hitInfo.point;
		}
		bulletStartFrame.lookAt(aimPoint);

		// Non-laser weapon, draw a projectile
		if (!m_config->isLaser()) {
			const shared_ptr<VisibleEntity>& bullet = VisibleEntity::create(format("bullet%03d", ++m_lastBulletId), m_scene.get(), m_bulletModel, bulletStartFrame);
			bullet->setShouldBeSaved(false);
			bullet->setCanCauseCollisions(false);
			bullet->setCastsShadows(false);
			bullet->setVisible(m_config->renderBullets);

			const shared_ptr<Projectile> projectile = Projectile::create(bullet, m_config->bulletSpeed, !m_config->hitScan, m_config->bulletGravity, fmin((closest + 1.0f) / m_config->bulletSpeed, 10.0f));
			m_projectiles.push(projectile);
			m_scene->insert(projectile);
		}
		// Laser weapon (very hacky for now...)
		else {
			// Need to do something better than this, draws for 2 frames and also doesn't work w/ the start frame aligned w/ the camera (see the backfaces)
			//shared_ptr<CylinderShape> beam = std::make_shared<CylinderShape>(CylinderShape(Cylinder(bulletStartFrame.translation, aimPoint, 0.02f)));
			//debugDraw(beam, FLT_EPSILON, Color4(0.2f, 0.8f, 0.0f, 1.0f), Color4::clear());
		}
	}

	// Hit scan specific logic here (immediately do hit/miss determination)
	shared_ptr<TargetEntity> target = nullptr;
	if(m_config->hitScan){
		// Check whether we hit any targets
		int closestIndex = -1;
		for (int t = 0; t < targets.size(); ++t) {
			if (targets[t]->intersect(ray, closest, hitInfo)) {
				closestIndex = t;
			}
		}
		if (closestIndex >= 0) {
			// Hit logic
			target = targets[closestIndex];			// Assign the target pointer here (not null indicates the hit)
			targetIdx = closestIndex;				// Write back the index of the target

			m_hitCallback(target);				// If we did, we are in hitscan mode, apply the damage and manage the target here
			// Offset position slightly along shot direction to avoid Z-fighting the target
			drawDecal(hitInfo.point + 0.01f * -ray.direction(), ray.direction(), true);
		}
		else { 
			m_missCallback(); 
			// Offset position slightly along normal to avoid Z-fighting the wall
			drawDecal(hitInfo.point + 0.01f * hitInfo.normal, hitInfo.normal);
		}
	}

	// If we're not in laser mode play the sounce (once) here
	if (!m_config->isLaser()) {
		m_fireSound->play(m_config->fireSoundVol);
		//m_fireSound->play(activeCamera()->frame().translation, activeCamera()->frame().lookVector() * 2.0f, 0.5f);
	}

	END_PROFILER_EVENT();

	return target;
}

bool Weapon::canFire() const {
	if (isNull(m_config)) return true;
	return timeSinceLastFire() > m_config->firePeriod;
}

float Weapon::cooldownRatio() const {
	if (isNull(m_config) || m_config->firePeriod == 0.0) return 1.0f;
	return min((float)timeSinceLastFire() / m_config->firePeriod, 1.0f);
}