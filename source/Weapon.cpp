#include "Weapon.h"


void Weapon::onPose(Array<shared_ptr<Surface> >& surface) {
	if (m_config->renderModel || m_config->renderBullets || m_config->renderMuzzleFlash) {
		// Update the weapon frame for all of these cases
		const float yScale = -0.12f;
		const float zScale = -yScale * 0.5f;
		const float lookY = m_camera->frame().lookVector().y;
		m_frame = m_camera->frame() * CFrame::fromXYZYPRDegrees(0.3f, -0.4f + lookY * yScale, -1.1f + lookY * zScale, 10, 5);
		// Pose the view model (weapon) for render here
		if (m_config->renderModel) {
			const float prevLookY = m_camera->previousFrame().lookVector().y;
			const CFrame prevWeaponPos = CFrame::fromXYZYPRDegrees(0.3f, -0.4f + prevLookY * yScale, -1.1f + prevLookY * zScale, 10, 5);
			m_viewModel->pose(surface, m_frame, m_camera->previousFrame() * prevWeaponPos, nullptr, nullptr, nullptr, Surface::ExpressiveLightScatteringProperties());
		}
	}
}

shared_ptr<TargetEntity> Weapon::fire(
	const Array<shared_ptr<TargetEntity>>& targets,
	int& targetIdx, 
	float& hitDist, 
	Model::HitInfo& hitInfo, 
	Array<shared_ptr<Entity>>& dontHit
){
	static RealTime lastTime;
	shared_ptr<TargetEntity> target = nullptr;

	const Ray& ray = m_camera->frame().lookRay();		// Use the camera lookray for hit detection
	// Check for closest hit (in scene, otherwise this ray hits the skybox)
	float closest = finf();
	dontHit.append(targets);
	m_scene->intersect(ray, closest, false, dontHit, hitInfo);
	if (closest < finf()) { hitDist = closest; }

	// Create the bullet (if we need to draw it or are using non-hitscan behavior)
	if (m_config->renderBullets || !m_config->hitScan) {
		// Create the bullet start frame from the weapon frame plus muzzle offset
		CFrame bulletStartFrame = m_camera->frame();
		
		// Apply bullet offset w/ camera rotation here
		bulletStartFrame.translation += m_camera->frame().rotation * m_config->bulletOffset;

		// Angle the bullet start frame towards the aim point
		Point3 aimPoint = m_camera->frame().translation + m_camera->frame().lookVector() * 1000.0f;
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

			/*	
			const shared_ptr<Entity::Track>& track = Entity::Track::create(bullet.get(), scene().get(),
				Any::parse(format("%s", bulletStartFrame.toXYZYPRDegreesString().c_str())));
			bullet->setTrack(track);
			*/

			m_projectiles->push(Projectile(bullet, m_config->bulletSpeed, !m_config->hitScan, m_config->bulletGravity, fmin((closest+1.0f)/ m_config->bulletSpeed, 10.0f)));
			m_scene->insert(bullet);
		}
		// Laser weapon (very hacky for now...)
		else {
			// Need to do something better than this, draws for 2 frames and also doesn't work w/ the start frame aligned w/ the camera (see the backfaces)
			//shared_ptr<CylinderShape> beam = std::make_shared<CylinderShape>(CylinderShape(Cylinder(bulletStartFrame.translation, aimPoint, 0.02f)));
			//debugDraw(beam, FLT_EPSILON, Color4(0.2f, 0.8f, 0.0f, 1.0f), Color4::clear());
		}
	}

	// Hit scan specific logic here
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
		}
	}
	else {
		// Moving projectile specific code here
		target = nullptr;
	}

	// If we're not in laser mode play the sounce (once) here
	if (!m_config->isLaser()) {
		m_fireSound->play(m_config->fireSoundVol);
		//m_fireSound->play(activeCamera()->frame().translation, activeCamera()->frame().lookVector() * 2.0f, 0.5f);
	}

	END_PROFILER_EVENT();
	return target;
}