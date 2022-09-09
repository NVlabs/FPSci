#include "PlayerEntity.h"
#include "PhysicsScene.h"

// Disable collisions
// #define NO_COLLISIONS

// Print lots of debugging info
//#define TRACE_COLLISIONS

// Show collision geometry
//#define SHOW_COLLISIONS


shared_ptr<Entity> PlayerEntity::create 
    (const String&                  name,
     Scene*                         scene,
     AnyTableReader&                propertyTable,
     const ModelTable&              modelTable,
     const Scene::LoadOptions&      loadOptions) {

    // Don't initialize in the constructor, where it is unsafe to throw Any parse exceptions
    shared_ptr<PlayerEntity> playerEntity(new PlayerEntity());

    // Initialize each base class, which parses its own fields
    playerEntity->Entity::init(name, scene, propertyTable);
    playerEntity->VisibleEntity::init(propertyTable, modelTable);
    playerEntity->PlayerEntity::init(propertyTable);

    // Verify that all fields were read by the base classes
    propertyTable.verifyDone();

    return playerEntity;
}


shared_ptr<Entity> PlayerEntity::create 
(const String&                           name,
 Scene*                                  scene,
 const CFrame&                           position,
 const shared_ptr<Model>&                model) {

    // Don't initialize in the constructor, where it is unsafe to throw Any parse exceptions
    shared_ptr<PlayerEntity> playerEntity(new PlayerEntity());

    // Initialize each base class, which parses its own fields
    playerEntity->Entity::init(name, scene, position, shared_ptr<Entity::Track>(), true, true);
    playerEntity->VisibleEntity::init(model, true, Surface::ExpressiveLightScatteringProperties(), ArticulatedModel::PoseSpline());
    playerEntity->PlayerEntity::init(Sphere(1.0f));
 
    return playerEntity;
}


void PlayerEntity::init(AnyTableReader& propertyTable) {
   // Get values from Any
    Sphere collisionSphere(1.5f);
    propertyTable.getIfPresent("collisionSphere", collisionSphere);
    // Create the player
    init(collisionSphere);
}

float PlayerEntity::heightOffset(float height) const {
	return height - m_collisionProxySphere.radius;
}

void PlayerEntity::init(const Sphere& collisionProxy) {
    m_collisionProxySphere = collisionProxy;
    m_desiredOSVelocity     = Vector3::zero();
    m_desiredYawVelocity    = 0;
    m_desiredPitchVelocity  = 0;
	m_spawnHeadingRadians   = frame().getHeading();
    m_headingRadians        = frame().getHeading();
    m_headTilt              = 0;
}

bool PlayerEntity::doDamage(float damage) {
	m_health -= damage;
	return m_health <= 0;
}

Any PlayerEntity::toAny(const bool forceAll) const {
    Any a = VisibleEntity::toAny(forceAll);
    a.setName("PlayerEntity");
    a["collisionSphere"] = m_collisionProxySphere;
    return a;
}
    
void PlayerEntity::onPose(Array<shared_ptr<Surface> >& surfaceArray) {
    VisibleEntity::onPose(surfaceArray);
}

void PlayerEntity::updateFromInput(UserInput* ui) {
	
	m_walkSpeed = 0;

	// Check if player is sprinting or not
	if (!m_sprinting) {
		m_walkSpeed = *moveRate * units::meters() / units::seconds();
	}
	else {
		m_walkSpeed = *moveRate * *sprintMultiplier * units::meters() / units::seconds();
	}

	// Get walking speed here (and normalize if necessary)
	m_linearVector = Vector3(ui->getX()*moveScale->x, 0, -ui->getY()*moveScale->y);
	if (m_linearVector.magnitude() > 0) {
		m_gettingMovementInput = true;
	}

	// Add jump here (if needed)
	RealTime timeSinceLastJump = System::time() - m_lastJumpTime;
	if (m_jumpPressed && timeSinceLastJump > *jumpInterval) {
		// Allow jumping if jumpTouch = False or if jumpTouch = True and the player is in contact w/ the map
		if (!(*jumpTouch) || m_inContact) {
			const Vector3 jv(0, *jumpVelocity * units::meters() / units::seconds(), 0);
			m_linearVector += jv;
			m_lastJumpTime = System::time();
		}
	}
	m_jumpPressed = false;

	// Get the mouse rotation here
	Vector2 mouseRotate = ui->mouseDXY() * turnScale * (float)m_cameraRadiansPerMouseDot;
	float yaw = mouseRotate.x;
	float pitch = mouseRotate.y;

	// Set the player view velocity
	setDesiredAngularVelocity(yaw, pitch);
}

/** Maximum coordinate values for the player ship */
void PlayerEntity::onSimulation(SimTime absoluteTime, SimTime deltaTime) {
	// Do not call Entity::onSimulation; that will override with spline animation
    if (! isNaN(deltaTime) && (deltaTime > 0)) {
        m_previousFrame = m_frame;
    }
    simulatePose(absoluteTime, deltaTime);

	if (!isNaN(deltaTime)) {
		// Apply rotation first
		m_headingRadians += m_desiredYawVelocity;												// Integrate the yaw change into heading
		m_headingRadians = mod1((m_headingRadians) / (2 * pif())) * 2 * pif();					// Keep the user's heading value in the [0,2pi) range		
		m_headTilt -= m_desiredPitchVelocity;													// Integrate the pitch change into head tilt
		m_headTilt = clamp(m_headTilt, -89.9f * units::degrees(), 89.9f * units::degrees());	// Keep the user's head tilt to <90°
		// Set player frame rotation based on the heading and tilt
		m_frame.rotation = Matrix3::fromAxisAngle(Vector3::unitY(), -m_headingRadians) * Matrix3::fromAxisAngle(Vector3::unitX(), m_headTilt);
		
		// Translation update - in direction after rotating
		if (m_motionEnable) {
			m_inContact = slideMove(deltaTime);
		}
		
		// Check for "off map" condition and reset position here...
		if (!isNaN(m_respawnHeight) && m_frame.translation.y < m_respawnHeight) {
			respawn();
		}
	}

	if (!m_gettingMovementInput) {

		if (accelerationEnabled!= nullptr && *accelerationEnabled) {
			m_acceleratedVelocity = max(m_acceleratedVelocity - *movementDeceleration * deltaTime, 0.0f);
		}
		else {
			m_acceleratedVelocity = 0;
		}

		if (m_acceleratedVelocity > 0) {
			m_linearVector = m_acceleratedVelocity * m_lastDirection;
		}
	}
	else {

		if (*accelerationEnabled) {
			m_acceleratedVelocity = min(m_acceleratedVelocity + *movementAcceleration * deltaTime, m_walkSpeed);
		}
		else {
			m_acceleratedVelocity = m_walkSpeed;
		}

		m_linearVector = m_linearVector.direction() * m_acceleratedVelocity;
		m_lastDirection = m_linearVector.direction();
	}
	/** The HeadBob uses lerp to achieve the "Bobbing" effect. The lerp function tries to reach
		the designated amplitude, but as Lerp can never reach final value, we change the polarity
		when it reaches half of the designated amplitude. The lerp function will then try to reach
		to the negative(-) of amplitude value. The polarity will again be changed when it reaches
		half of (-amplitude), thus achieving the effect. 
	 
		The frequency (how fast the HeadBob will happen) is controlled by the headBobFrequency value
		as well as the players current movement speed. So the faster the player moves, faster the
		effect will be. Its also tied to deltaTime, so FPS will not effect how fast/slow HeadBob 
		happens. 
	*/
	if (headBobEnabled != nullptr && *headBobEnabled && m_acceleratedVelocity > 0) {
		if (!m_headBobPolarity) {
			m_headBobCurrentHeight = lerp(m_headBobCurrentHeight, *headBobAmplitude, *headBobFrequency * m_acceleratedVelocity * deltaTime);

			if (m_headBobCurrentHeight >= *headBobAmplitude / 2) {
				m_headBobPolarity = !m_headBobPolarity;
			}
		}
		else {
			m_headBobCurrentHeight = lerp(m_headBobCurrentHeight, -*headBobAmplitude, *headBobFrequency * m_acceleratedVelocity * deltaTime);

			if (m_headBobCurrentHeight <= -*headBobAmplitude / 2) {
				m_headBobPolarity = !m_headBobPolarity;
			}
		}
	}
	// Height of the camera will reset to the original position once the player has stopped moving
	else if(headBobEnabled != nullptr && *headBobEnabled && m_acceleratedVelocity <= 0){
		m_headBobCurrentHeight = lerp(m_headBobCurrentHeight, 0, *headBobFrequency * deltaTime);
	}

	//Set Players Translation velocity
	setDesiredOSVelocity(m_linearVector);

	m_gettingMovementInput = false;
}

void PlayerEntity::getConservativeCollisionTris(Array<Tri>& triArray, const Vector3& velocity, float deltaTime) const {
    Sphere nearby = collisionProxy();
    nearby.radius += velocity.length() * deltaTime;
    ((PhysicsScene*)m_scene)->staticIntersectSphere(nearby, triArray);
}


bool PlayerEntity::findFirstCollision
(const Array<Tri>&    triArray,
	const Vector3&       velocity,
	float&               stepTime,
	Vector3&             collisionNormal,
	Point3&              collisionPoint) const {

	bool collision = false;
	const Sphere& startSphere = collisionProxy();
	for (int t = 0; t < triArray.size(); ++t) {

		const Tri& tri = triArray[t];
		const CPUVertexArray& cpuVertexArray = ((PhysicsScene*)m_scene)->vertexArrayOfCollisionTree();
		Triangle triangle(tri.position(cpuVertexArray, 0), tri.position(cpuVertexArray, 1), tri.position(cpuVertexArray, 2));
		Vector3 C;
		const float d =
			CollisionDetection::collisionTimeForMovingSphereFixedTriangle
			(startSphere, velocity, triangle, C);

		if (d < stepTime) {
			// Found a new collision sooner than the previous one.
			const Vector3& centerAtCollisionTime = startSphere.center + velocity * d;

			const Vector3& delta = centerAtCollisionTime - C;

			// Distance from sphere to collision point; if this is less than the sphere radius,
			// the collision was interpenetarat
			const float r = delta.length();
			const Vector3& n = delta / r;

			static const float epsilon = 0.000001f;
			const bool interpenetration = (r < startSphere.radius - epsilon);
			const bool rightDirection   = (dot(n, velocity) < -epsilon);

			if (interpenetration || rightDirection) {
				// Normal to the sphere at the collision point
				collisionNormal = n;
				collisionPoint  = C;
				stepTime        = d;
				collision       = true;
			}
		}
	}
	
#   ifdef SHOW_COLLISIONS
	if (collision) {
		if (collisionNormal.y < 0.99f) {
			//runSimulation = false;
			const float duration = 1.0f;
			ArrowShape arrow = ArrowShape(collisionPoint, collisionNormal);
			debugDraw(Sphere(collisionPoint, 0.1f), duration, Color3::red(), Color4::clear());
			debugDraw(arrow.vertexArray(), arrow.indexArray(), duration, Color3::red(), Color4::clear());
		}
		else {
			debugDraw(Sphere(collisionPoint, 0.2f), 1.0f, Color3::cyan(), Color4::clear());
		}
	}
#   endif

	return collision;
}

bool PlayerEntity::slideMove(SimTime deltaTime) { 
	if (deltaTime == 0.0f) return false;
	static const float epsilon = 0.0001f;
	Point3 loc;

	// Only allow y-axis gravity for now
    alwaysAssertM(((PhysicsScene*)m_scene)->gravity().x == 0.0f && ((PhysicsScene*)m_scene)->gravity().z == 0.0f, 
                            "We assume gravity points along the y axis to simplify implementation");
	alwaysAssertM(axisLock->size() == 3, "Player axis lock must have length 3!");
	float ygrav = ((PhysicsScene*)m_scene)->gravity().y;
	Vector3 velocity = frame().vectorToWorldSpace(m_desiredOSVelocity);
	velocity.x = (*axisLock)[0] ? 0.0f : velocity.x;
	velocity.y = (*axisLock)[1] ? 0.0f : velocity.y;
	velocity.z = (*axisLock)[2] ? 0.0f : velocity.z;

	// Apply the velocity using a terminal velocity of about 5.4s of acceleration (human is ~53m/s w/ 9.8m/s^2)
	if (m_desiredOSVelocity.y > 0) {
		m_inAir = true;
		// Jump occurring, need to track this
		m_lastJumpVelocity = m_desiredOSVelocity.y;
	}
	else if (m_inAir) {
		// Already in a jump, apply gravity and enforce terminal velocity
		m_lastJumpVelocity += ygrav * deltaTime;
		if (abs(m_lastJumpVelocity) > 0) {
			// Enforce terminal velocity here
			m_lastJumpVelocity = m_lastJumpVelocity / abs(m_lastJumpVelocity) * min(abs(m_lastJumpVelocity), abs(5.4f * ygrav));
			velocity.y = m_lastJumpVelocity;
		}
	}
	else {
		// Walking on the ground, just set a small negative veloicity to keep us in contact (this is a hack)
		m_lastJumpVelocity = 0.0f;
		velocity.y = -epsilon;
	}
	
    Array<Tri> triArray;
    getConservativeCollisionTris(triArray, velocity, (float)deltaTime);
    
    // Trivial implementation that ignores collisions:
#   if NO_COLLISIONS
		loc = m_frame.translation + velocity  * timeLeft;
		m_frame.translation = loc;
        return;
#   endif

    // Keep simulating until we run out of time or velocity, at which point
    // no further movement is possible.
#   ifdef TRACE_COLLISIONS
        debugPrintf("================================\n");
        debugPrintf("Initial velocity = %s; position = %s\n", velocity.toString().c_str(),  m_frame.translation.toString().c_str());
#   endif
    int iterations = 0;

    // Handle case where there is no motion here (return last collision state)
    if (velocity.length() <= epsilon) {
        return m_inContact;
    }

	bool collided = false;
    while ((deltaTime > epsilon) && (velocity.length() > epsilon)) {
        float stepTime = float(deltaTime);
        Vector3 collisionNormal;
        Point3 collisionPoint;

		bool collision = findFirstCollision(triArray, velocity, stepTime, collisionNormal, collisionPoint);
		collided |= collision;

#       ifdef TRACE_COLLISIONS
            debugPrintf("  stepTime = %f\n", stepTime);
#       endif

        // Advance to just before the collision
        stepTime = max(0.0f, stepTime - epsilon * 0.5f);
		loc = m_frame.translation + velocity * stepTime;
		m_frame.translation = loc;

        // Early out of loop when debugging
        //if (! runSimulation) { return; }
		m_inAir = !collision;
        if (collision) {
			m_inAir = collisionNormal.y < 0.99;
#           ifdef TRACE_COLLISIONS
                debugPrintf("  Collision C=%s, n=%s; position after=%s)\n", 
                            collisionPoint.toString().c_str(),
                            collisionNormal.toString().c_str(),
							loc.toString().c_str());
#           endif
            if (collisionProxy().contains(collisionPoint)) {
                // Interpenetration. This is bad because the
                // rest of the code assumes no interpenetration and
                // uses that to rise up steps.  Place the sphere
                // adjacent to the triangle and eliminate all velocity
                // towards the triangle.
               loc = collisionPoint + collisionNormal * (m_collisionProxySphere.radius + epsilon * 2.0f);
               m_frame.translation = loc;
                
#               ifdef TRACE_COLLISIONS
                    debugPrintf("  Interpenetration detected.  Position after = %s\n",
                        loc.toString().c_str());
#               endif
            }
                
            // Constrain the velocity by subtracting the component
            // into the collision normal
            const Vector3& vPerp = collisionNormal * collisionNormal.dot(velocity);
            const Vector3& vPar  = velocity - vPerp;
#           ifdef SHOW_COLLISIONS
            if (collisionNormal.y < 0.95f) {
                float duration = 1.0f;
				ArrowShape a1 = ArrowShape(collisionPoint, velocity);
				ArrowShape a2 = ArrowShape(collisionPoint, vPerp);
				ArrowShape a3 = ArrowShape(collisionPoint, vPar);
				debugDraw(a1.vertexArray(), a1.indexArray(), duration, Color3::green());
                debugDraw(a2.vertexArray(), a2.indexArray(), duration, Color3::yellow());
                debugDraw(a3.vertexArray(), a3.indexArray(), duration, Color3::blue());
                if (duration == finf()) {
                    // Pause so we can see the result
                    //runSimulation = false;
                }
            }
#           endif
            velocity = vPar;

#           ifdef TRACE_COLLISIONS
                debugPrintf("  velocity after collision = %s\n", velocity.toString().c_str());
#           endif
        }
#       ifdef TRACE_COLLISIONS
            debugPrintf("  --------------\n");
#       endif

        ++iterations;
        deltaTime -= stepTime;
    }
	
	return collided;
    //screenPrintf("%d collision iterations", iterations);
}
