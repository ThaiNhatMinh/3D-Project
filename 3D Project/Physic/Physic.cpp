#include "pch.h"

// helpers for conversion to and from Bullet's data types
static btVector3 Vec3_to_btVector3(vec3 const & v)
{
	return btVector3(v.x, v.y, v.z);
}

static vec3 btVector3_to_Vec3(btVector3 const & btvec)
{
	return vec3(btvec.x(), btvec.y(), btvec.z());
}

/////////////////////////////////////////////////////////////////////////////
// struct ActorMotionState						
//
// Interface that Bullet uses to communicate position and orientation changes
//   back to the game.  note:  this assumes that the actor's center of mass
//   and world position are the same point.  If that was not the case,
//   an additional transformation would need to be stored here to represent
//   that difference.
//

BulletPhysics::BulletPhysics()
{
	REGISTER_EVENT(EvtData_PhysTrigger_Enter);
	REGISTER_EVENT(EvtData_PhysTrigger_Leave);
	REGISTER_EVENT(EvtData_PhysCollisionStart);
	REGISTER_EVENT(EvtData_PhysOnCollision);
	REGISTER_EVENT(EvtData_PhysCollisionEnd);
}


/////////////////////////////////////////////////////////////////////////////
// BulletPhysics::~BulletPhysics				
//
BulletPhysics::~BulletPhysics()
{
	
}

void BulletPhysics::onStartUp()
{
	E_DEBUG("Physic Engine Initialize...");
	this->VInitialize();
}

void BulletPhysics::onShutDown()
{
	E_DEBUG("Physic Engine Shutdown...");
	// delete any physics objects which are still in the world

	// iterate backwards because removing the last object doesn't affect the
	//  other objects stored in a vector-type array
	for (int ii = m_dynamicsWorld->getNumCollisionObjects() - 1; ii >= 0; --ii)
	{
		btCollisionObject * const obj = m_dynamicsWorld->getCollisionObjectArray()[ii];

		RemoveCollisionObject(obj);
	}

	m_rigidBodyToActorId.clear();

	delete m_debugDrawer;
	delete m_dynamicsWorld;
	delete m_solver;
	delete m_broadphase;
	delete m_dispatcher;
	delete m_collisionConfiguration;
}

/////////////////////////////////////////////////////////////////////////////
// BulletPhysics::LoadXml						
//
//    Loads the physics materials from an XML file
//
void BulletPhysics::LoadXml()
{
	// Load the physics config file and grab the root XML node
	tinyxml2::XMLDocument doc;// = new tinyxml2::XMLDocument;
	doc.LoadFile("GameAssets\\Physics.xml");

	tinyxml2::XMLElement* pRoot = doc.FirstChildElement("Physics");

	// load all materials
	tinyxml2::XMLElement* pParentNode = pRoot->FirstChildElement("PhysicsMaterials");
	//GCC_ASSERT(pParentNode);
	for (tinyxml2::XMLElement* pNode = pParentNode->FirstChildElement(); pNode; pNode = pNode->NextSiblingElement())
	{
		double restitution = pNode->DoubleAttribute("restitution");
		double friction = pNode->DoubleAttribute("friction");
		
		
		m_materialTable.insert(std::make_pair(pNode->Value(), MaterialData((float)restitution, (float)friction)));
	}

	// load all densities
	pParentNode = pRoot->FirstChildElement("DensityTable");
	//GCC_ASSERT(pParentNode);
	for (tinyxml2::XMLElement* pNode = pParentNode->FirstChildElement(); pNode; pNode = pNode->NextSiblingElement())
	{
		m_densityTable.insert(std::make_pair(pNode->Value(), (float)atof(pNode->FirstChild()->Value())));
	}
	
}

/////////////////////////////////////////////////////////////////////////////
// BulletPhysics::VInitialize				
//
bool BulletPhysics::VInitialize()
{
	LoadXml();

	// this controls how Bullet does internal memory management during the collision pass
	m_collisionConfiguration = new btDefaultCollisionConfiguration();

	// this manages how Bullet detects precise collisions between pairs of objects
	m_dispatcher = new btCollisionDispatcher(m_collisionConfiguration);

	// Bullet uses this to quickly (imprecisely) detect collisions between objects.
	//   Once a possible collision passes the broad phase, it will be passed to the
	//   slower but more precise narrow-phase collision detection (btCollisionDispatcher).
	m_broadphase = new btDbvtBroadphase();

	// Manages constraints which apply forces to the physics simulation.  Used
	//  for e.g. springs, motors.  We don't use any constraints right now.
	m_solver = new btSequentialImpulseConstraintSolver;

	// This is the main Bullet interface point.  Pass in all these components to customize its behavior.
	m_dynamicsWorld = new btDiscreteDynamicsWorld(m_dispatcher,
		m_broadphase,
		m_solver,
		m_collisionConfiguration);

	m_dynamicsWorld->setGravity(btVector3(0, -10, 0));

	m_debugDrawer = new BulletDebugDrawer;
	//m_debugDrawer->ReadOptions();

	if (!m_collisionConfiguration || !m_dispatcher || !m_broadphase ||
		!m_solver || !m_dynamicsWorld || !m_debugDrawer)
	{
		E_ERROR("BulletPhysics::VInitialize failed!");
		return false;
	}

	m_dynamicsWorld->setDebugDrawer(m_debugDrawer);


	// and set the internal tick callback to our own method "BulletInternalTickCallback"
	m_dynamicsWorld->setInternalTickCallback(BulletInternalTickCallback, static_cast<void*>(this),false);
	m_dynamicsWorld->setInternalTickCallback(BulletInternalPreTickCallback, static_cast<void*>(this), true);
	m_dynamicsWorld->setWorldUserInfo(this);

	return true;
}

/////////////////////////////////////////////////////////////////////////////
// BulletPhysics::VOnUpdate						
//
void BulletPhysics::VOnUpdate(float const deltaSeconds)
{
	// Bullet uses an internal fixed timestep (default 1/60th of a second)
	//   We pass in 4 as a max number of sub steps.  Bullet will run the simulation
	//   in increments of the fixed timestep until "deltaSeconds" amount of time has
	//   passed, but will only run a maximum of 4 steps this way.
	m_dynamicsWorld->stepSimulation(deltaSeconds, 4);
	//E_DEBUG("Physuic Update()");
	

}

void BulletPhysics::VPostStep(float timeStep)
{


	SendCollisionEvents();


	IEvent* pEvent = new EvtData_PhysPostStep(timeStep);
	gEventManager()->VTriggerEvent(pEvent);
}

void BulletPhysics::VPreStep(float timeStep)
{
	IEvent* pEvent = new EvtData_PhysPreStep(timeStep);
	gEventManager()->VTriggerEvent(pEvent);
}

/////////////////////////////////////////////////////////////////////////////
// BulletPhysics::VSyncVisibleScene				
//
void BulletPhysics::VSyncVisibleScene()
{
	// Keep physics & graphics in sync

	// check all the existing actor's bodies for changes. 
	//  If there is a change, send the appropriate event for the game system.
	for (ActorIDToRigidBodyMap::const_iterator it = m_actorIdToRigidBody.begin(); it != m_actorIdToRigidBody.end();	++it)
	{
		ActorId const id = it->first;

		// get the MotionState.  this object is updated by Bullet.
		// it's safe to cast the btMotionState to ActorMotionState, because all the bodies in m_actorIdToRigidBody
		//   were created through AddShape()E
		ActorMotionState const * const actorMotionState = static_cast<ActorMotionState*>(it->second->GetMotionState());
		//GCC_ASSERT(actorMotionState);
		Actor* pGameActor = 0;
		RigidBodyComponent *pRb = FindBulletRigidBody(id);
		pGameActor = pRb->GetOwner();
		TransformComponent* pTransformComponent = pGameActor->GetComponent<TransformComponent>(TransformComponent::Name);
		if (pTransformComponent)
		{
			if (pTransformComponent->GetTransform() != actorMotionState->m_worldToPositionTransform)
			{
				// Bullet has moved the actor's physics object.  Sync the transform and inform the game an actor has moved
				pTransformComponent->SetTransform(actorMotionState->m_worldToPositionTransform);
				//shared_ptr<EvtData_Move_Actor> pEvent(GCC_NEW EvtData_Move_Actor(id, actorMotionState->m_worldToPositionTransform));
				//IEventManager::Get()->VQueueEvent(pEvent);
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// BulletPhysics::AddShape						
//
/*
void BulletPhysics::AddShape(Actor* pGameActor, btCollisionShape* shape, float mass, const std::string& physicsMaterial)
{
	//GCC_ASSERT(pGameActor);

	ActorId actorID = pGameActor->GetId();
	//GCC_ASSERT(m_actorIdToRigidBody.find(actorID) == m_actorIdToRigidBody.end() && "Actor with more than one physics body?");

	// lookup the material
	MaterialData material(LookupMaterialData(physicsMaterial));

	// localInertia defines how the object's mass is distributed
	btVector3 localInertia(0.f, 0.f, 0.f);
	if (mass > 0.001f)
		shape->calculateLocalInertia(mass, localInertia);


	mat4 transform;
	TransformComponent* pTransformComponent = pGameActor->GetComponent<TransformComponent>(TransformComponent::Name);
	//GCC_ASSERT(pTransformComponent);
	if (pTransformComponent)
	{
		transform = pTransformComponent->GetTransform();
	}
	else
	{
		// Physics can't work on an actor that doesn't have a TransformComponent!
		E_WARNING("Physics can't work on an actor that doesn't have a TransformComponent!");
		return;
	}

	// set the initial transform of the body from the actor
	ActorMotionState * myMotionState = new ActorMotionState(transform);

	btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, shape, localInertia);

	// set up the materal properties
	rbInfo.m_restitution = material.m_restitution;
	rbInfo.m_friction = material.m_friction;

	btRigidBody * body = new btRigidBody(rbInfo);
	
	m_dynamicsWorld->addRigidBody(body);

	// add it to the collection to be checked for changes in VSyncVisibleScene
	m_actorIdToRigidBody[actorID] = body;
	m_rigidBodyToActorId[body] = actorID;
}
*/
void BulletPhysics::AddRigidBody(ActorId id, RigidBodyComponent * rb)
{
	m_dynamicsWorld->addRigidBody(rb->GetRigidBody());
	m_actorIdToRigidBody[id] = rb;
	m_rigidBodyToActorId[rb] = id;
}
/////////////////////////////////////////////////////////////////////////////
// BulletPhysics::RemoveCollisionObject			
//
//    Removes a collision object from the game world
//
void BulletPhysics::RemoveCollisionObject(btCollisionObject * const removeMe)
{
	// first remove the object from the physics sim
	m_dynamicsWorld->removeCollisionObject(removeMe);

	// then remove the pointer from the ongoing contacts list
	for (CollisionPairs::iterator it = m_previousTickCollisionPairs.begin();
	it != m_previousTickCollisionPairs.end(); )
	{
		CollisionPairs::iterator next = it;
		++next;

		if (it->first == removeMe || it->second == removeMe)
		{
			//SendCollisionPairRemoveEvent(it->first, it->second);
			m_previousTickCollisionPairs.erase(it);
		}

		it = next;
	}

	// if the object is a RigidBody (all of ours are RigidBodies, but it's good to be safe)
	if (btRigidBody * const body = btRigidBody::upcast(removeMe))
	{
		// delete the components of the object
		if(body->getMotionState()) delete body->getMotionState();
		if(body->getCollisionShape()) delete body->getCollisionShape();
		//delete body->getUserPointer();
		//delete body->getUserPointer();

		for (int ii = body->getNumConstraintRefs() - 1; ii >= 0; --ii)
		{
			btTypedConstraint * const constraint = body->getConstraintRef(ii);
			m_dynamicsWorld->removeConstraint(constraint);
			delete constraint;
		}
	}

	delete removeMe;
}

/////////////////////////////////////////////////////////////////////////////
// BulletPhysics::FindBulletRigidBody			
//    Finds a Bullet rigid body given an actor ID
//
RigidBodyComponent* BulletPhysics::FindBulletRigidBody(ActorId const id) const
{
	ActorIDToRigidBodyMap::const_iterator found = m_actorIdToRigidBody.find(id);
	if (found != m_actorIdToRigidBody.end())
		return found->second;

	return NULL;
}

/////////////////////////////////////////////////////////////////////////////
// BulletPhysics::FindActorID				- not described in the book
//    Finds an Actor ID given a Bullet rigid body 
//
ActorId BulletPhysics::FindActorID(RigidBodyComponent const * const body) const
{
	RigidBodyToActorIDMap::const_iterator found = m_rigidBodyToActorId.find(body);
	if (found != m_rigidBodyToActorId.end())
		return found->second;

	return ActorId();
}


/*
/////////////////////////////////////////////////////////////////////////////
// BulletPhysics::VAddSphere					- Chapter 17, page 599
//
void BulletPhysics::VAddSphere(float const radius, Actor* pGameActor, const std::string& densityStr, const std::string& physicsMaterial)
{
	//StrongActorPtr pStrongActor = MakeStrongPtr(pGameActor);
	//if (!pStrongActor)
	//	return;  // FUTURE WORK - Add a call to the error log here

				 // create the collision body, which specifies the shape of the object
	btSphereShape * const collisionShape = new btSphereShape(radius);

	// calculate absolute mass from specificGravity
	float specificGravity = LookupSpecificGravity(densityStr);
	float const volume = (4.f / 3.f) * glm::pi<float>()* radius * radius * radius;
	btScalar const mass = volume * specificGravity;

	AddShape(pGameActor, /*initialTransform, collisionShape, mass, physicsMaterial);
}

/////////////////////////////////////////////////////////////////////////////
// BulletPhysics::VAddBox
//
void BulletPhysics::VAddBox(const vec3& dimensions, Actor* pGameActor, const std::string& densityStr, const std::string& physicsMaterial)
{
	//StrongActorPtr pStrongActor = MakeStrongPtr(pGameActor);
	//if (!pStrongActor)
	//	return;  // FUTURE WORK: Add a call to the error log here

				 // create the collision body, which specifies the shape of the object
	btBoxShape * const boxShape = new btBoxShape(Vec3_to_btVector3(dimensions));

	// calculate absolute mass from specificGravity
	float specificGravity = LookupSpecificGravity(densityStr);
	float const volume = dimensions.x * dimensions.y * dimensions.z;
	btScalar const mass = volume * specificGravity;

	AddShape(pGameActor,/* initialTransform, boxShape, mass, physicsMaterial);
}

/////////////////////////////////////////////////////////////////////////////
// BulletPhysics::VAddPointCloud				- Chapter 17, page 601
//
void BulletPhysics::VAddPointCloud(vec3 *verts, int numPoints, Actor* pGameActor, /*const Mat4x4& initialTransform, const std::string& densityStr, const std::string& physicsMaterial)
{
	//StrongActorPtr pStrongActor = MakeStrongPtr(pGameActor);
	//if (!pStrongActor)
	//	return;  // FUTURE WORK: Add a call to the error log here

	btConvexHullShape * const shape = new btConvexHullShape();

	// add the points to the shape one at a time
	for (int ii = 0; ii<numPoints; ++ii)
		shape->addPoint(Vec3_to_btVector3(verts[ii]));

	// approximate absolute mass using bounding box
	btVector3 aabbMin(0, 0, 0), aabbMax(0, 0, 0);
	shape->getAabb(btTransform::getIdentity(), aabbMin, aabbMax);

	btVector3 const aabbExtents = aabbMax - aabbMin;

	float specificGravity = LookupSpecificGravity(densityStr);
	float const volume = aabbExtents.x() * aabbExtents.y() * aabbExtents.z();
	btScalar const mass = volume * specificGravity;

	AddShape(pGameActor, shape, mass, physicsMaterial);
}
*/
/////////////////////////////////////////////////////////////////////////////
// BulletPhysics::VRemoveActor					
//
//    Implements the method to remove actors from the physics simulation
//
void BulletPhysics::VRemoveActor(ActorId id)
{
	if (RigidBodyComponent * const body = FindBulletRigidBody(id))
	{
		// destroy the body and all its components
		RemoveCollisionObject(body->GetRigidBody());
		m_actorIdToRigidBody.erase(id);
		m_rigidBodyToActorId.erase(body);
	}
}

/*
void BulletPhysics::VAddCharacter(const vec3 & dimensions, Actor * pGameActor)
{
	btBoxShape * const boxShape = new btBoxShape(Vec3_to_btVector3(dimensions));

	ActorId actorID = pGameActor->GetId();
	mat4 transform;
	TransformComponent* pTransformComponent = pGameActor->GetComponent<TransformComponent>(TransformComponent::Name);
	//GCC_ASSERT(pTransformComponent);
	if (pTransformComponent)
	{
		transform = pTransformComponent->GetTransform();
	}
	else
	{
		// Physics can't work on an actor that doesn't have a TransformComponent!
		E_WARNING("Physics can't work on an actor that doesn't have a TransformComponent!");
		return;
	}

	// set the initial transform of the body from the actor
	ActorMotionState * myMotionState = new ActorMotionState(transform);

	btRigidBody * body = new btRigidBody(0.5f,myMotionState,boxShape,btVector3(0.0f,0.0f,0.0f));
	m_dynamicsWorld->addRigidBody(body);

	// add it to the collection to be checked for changes in VSyncVisibleScene
	m_actorIdToRigidBody[actorID] = body;
	m_rigidBodyToActorId[body] = actorID;


	//btGeneric6DofSpringConstraint* joint = new btGeneric6DofSpringConstraint();
}
*/
/////////////////////////////////////////////////////////////////////////////
// BulletPhysics::VRenderDiagnostics			- Chapter 17, page 604
//
void BulletPhysics::VRenderDiagnostics()
{
	m_dynamicsWorld->debugDrawWorld();
}

/////////////////////////////////////////////////////////////////////////////
// BulletPhysics::VCreateTrigger				
//
// FUTURE WORK: Mike create a trigger actor archetype that can be instantiated in the editor!!!!!
//
/*void BulletPhysics::VCreateTrigger(Actor* pGameActor, const vec3 &pos, const float dim)
{
	//StrongActorPtr pStrongActor = MakeStrongPtr(pGameActor);
	//if (!pStrongActor)
	//	return;  // FUTURE WORK: Add a call to the error log here

				 // create the collision body, which specifies the shape of the object
	btBoxShape * const boxShape = new btBoxShape(Vec3_to_btVector3(vec3(dim, dim, dim)));

	// triggers are immoveable.  0 mass signals this to Bullet.
	btScalar const mass = 0;

	// set the initial position of the body from the actor
	mat4 triggerTrans;
	triggerTrans = glm::translate(triggerTrans,pos);
	ActorMotionState * const myMotionState = new ActorMotionState(triggerTrans);

	btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, boxShape, btVector3(0, 0, 0));
	btRigidBody * const body = new btRigidBody(rbInfo);

	m_dynamicsWorld->addRigidBody(body);

	// a trigger is just a box that doesn't collide with anything.  That's what "CF_NO_CONTACT_RESPONSE" indicates.
	body->setCollisionFlags(body->getCollisionFlags() | btRigidBody::CF_NO_CONTACT_RESPONSE);

	m_actorIdToRigidBody[pGameActor->GetId()] = body;
	m_rigidBodyToActorId[body] = pGameActor->GetId();
}

/////////////////////////////////////////////////////////////////////////////
// BulletPhysics::VApplyForce					- Chapter 17, page 603
//
void BulletPhysics::VApplyForce(const vec3 &dir, float newtons, ActorId aid)
{
	if (RigidBodyComponent * const body = FindBulletRigidBody(aid))
	{
		vec3 const force(dir.x * newtons,
			dir.y * newtons,
			dir.z * newtons);

		body->ApplyForce(force);
	}
}

/////////////////////////////////////////////////////////////////////////////
// BulletPhysics::VApplyTorque					- Chapter 17, page 603
//
void BulletPhysics::VApplyTorque(const vec3 &dir, float magnitude, ActorId aid)
{
	if (RigidBodyComponent * const body = FindBulletRigidBody(aid))
	{
		//body->setActivationState(DISABLE_DEACTIVATION);

		vec3 const torque(dir.x * magnitude,
			dir.y * magnitude,
			dir.z * magnitude);

		body->ApplyTorqueImpulse(torque);
	}
}

/////////////////////////////////////////////////////////////////////////////
// BulletPhysics::VKinematicMove				- not described in the book
//
//    Forces a phyics object to a new location/orientation
//
bool BulletPhysics::VKinematicMove(const mat4 &mat, ActorId aid)
{
	if (RigidBodyComponent * const body = FindBulletRigidBody(aid))
	{

		// warp the body to the new position
		//body->setWorldTransform(Mat4x4_to_btTransform(mat));
		return true;
	}

	return false;
}

/////////////////////////////////////////////////////////////////////////////
// BulletPhysics::VGetTransform			
//
//   Returns the current transform of the phyics object
//
mat4 BulletPhysics::VGetTransform(const ActorId id)
{
	RigidBodyComponent * pRigidBody = FindBulletRigidBody(id);
	//GCC_ASSERT(pRigidBody);

	const btTransform& actorTransform = pRigidBody->GetTransform();
	return btTransform_to_Mat4x4(actorTransform);
}

void BulletPhysics::VClearForce(ActorId id)
{
	RigidBodyComponent * pRigidBody = FindBulletRigidBody(id);
	//GCC_ASSERT(pRigidBody);
	if (!pRigidBody)
		return;
	pRigidBody->ResetForces();
}

/////////////////////////////////////////////////////////////////////////////
// BulletPhysics::VSetTransform					
//
//   Sets the current transform of the phyics object
//
void BulletPhysics::VSetTransform(ActorId actorId, const mat4& mat)
{
	VKinematicMove(mat, actorId);
}

/////////////////////////////////////////////////////////////////////////////
// BulletPhysics::VRotateY						- not described in the book
//
//   A helper function used to turn objects to a new heading
//
/*
void BulletPhysics::VRotateY(ActorId const actorId, float const deltaAngleRadians, float const time)
{
	btRigidBody * pRigidBody = FindBulletRigidBody(actorId);
	//GCC_ASSERT(pRigidBody);

	// create a transform to represent the additional turning this frame
	btTransform angleTransform;
	angleTransform.setIdentity();
	angleTransform.getBasis().setEulerYPR(0, deltaAngleRadians, 0); // rotation about body Y-axis

																	// concatenate the transform onto the body's transform
	pRigidBody->setCenterOfMassTransform(pRigidBody->getCenterOfMassTransform() * angleTransform);
}



/////////////////////////////////////////////////////////////////////////////
// BulletPhysics::VGetOrientationY				- not described in the book
//
//   A helper functions use to access the current heading of a physics object
//
float BulletPhysics::VGetOrientationY(ActorId actorId)
{
	btRigidBody * pRigidBody = FindBulletRigidBody(actorId);
	//GCC_ASSERT(pRigidBody);

	const btTransform& actorTransform = pRigidBody->getCenterOfMassTransform();
	btMatrix3x3 actorRotationMat(actorTransform.getBasis());  // should be just the rotation information

	btVector3 startingVec(0, 0, 1);
	btVector3 endingVec = actorRotationMat * startingVec; // transform the vector

	endingVec.setY(0);  // we only care about rotation on the XZ plane

	float const endingVecLength = endingVec.length();
	if (endingVecLength < 0.001)
	{
		// gimbal lock (orientation is straight up or down)
		return 0;
	}

	else
	{
		btVector3 cross = startingVec.cross(endingVec);
		float sign = cross.getY() > 0 ? 1.0f : -1.0f;
		return (acosf(startingVec.dot(endingVec) / endingVecLength) * sign);
	}

	return FLT_MAX;  // fail...
}

/////////////////////////////////////////////////////////////////////////////
// BulletPhysics::VStopActor					- Chapter 17, page 604
//
void BulletPhysics::VStopActor(ActorId actorId)
{
	VSetVelocity(actorId, vec3(0.f, 0.f, 0.f));
}

/////////////////////////////////////////////////////////////////////////////
// BulletPhysics::VGetVelocity					- Chapter 17, page 604
//
vec3 BulletPhysics::VGetVelocity(ActorId actorId)
{
	btRigidBody* pRigidBody = FindBulletRigidBody(actorId);
	//GCC_ASSERT(pRigidBody);
	if (!pRigidBody)
		return vec3();
	btVector3 btVel = pRigidBody->getLinearVelocity();
	return btVector3_to_Vec3(btVel);
}

/////////////////////////////////////////////////////////////////////////////
void BulletPhysics::VSetVelocity(ActorId actorId, const vec3& vel)
{
	btRigidBody * pRigidBody = FindBulletRigidBody(actorId);
	//GCC_ASSERT(pRigidBody);
	if (!pRigidBody)
		return;
	btVector3 btVel = Vec3_to_btVector3(vel);
	pRigidBody->setLinearVelocity(btVel);
	
}

/////////////////////////////////////////////////////////////////////////////
vec3 BulletPhysics::VGetAngularVelocity(ActorId actorId)
{
	btRigidBody* pRigidBody = FindBulletRigidBody(actorId);
	//GCC_ASSERT(pRigidBody);
	if (!pRigidBody)
		return vec3();
	btVector3 btVel = pRigidBody->getAngularVelocity();
	return btVector3_to_Vec3(btVel);
}

/////////////////////////////////////////////////////////////////////////////
void BulletPhysics::VSetAngularVelocity(ActorId actorId, const vec3& vel)
{
	btRigidBody * pRigidBody = FindBulletRigidBody(actorId);
	//GCC_ASSERT(pRigidBody);
	if (!pRigidBody)
		return;
	btVector3 btVel = Vec3_to_btVector3(vel);
	pRigidBody->setAngularVelocity(btVel);
}

/////////////////////////////////////////////////////////////////////////////
void BulletPhysics::VTranslate(ActorId actorId, const vec3& vec)
{
	btRigidBody * pRigidBody = FindBulletRigidBody(actorId);
	//GCC_ASSERT(pRigidBody);
	btVector3 btVec = Vec3_to_btVector3(vec);
	pRigidBody->translate(btVec);
}

*/
/////////////////////////////////////////////////////////////////////////////
// BulletPhysics::BulletInternalTickCallback
//
// This function is called after bullet performs its internal update.  We
//   use it to detect collisions between objects for Game code.
//
void BulletPhysics::BulletInternalTickCallback(btDynamicsWorld * const world, btScalar const timeStep)
{
	//E_DEBUG("BulletInternalTickCallback()");
	assert(world);

	assert(world->getWorldUserInfo());
	BulletPhysics * const bulletPhysics = static_cast<BulletPhysics*>(world->getWorldUserInfo());

	bulletPhysics->VPostStep(timeStep);

}

void BulletPhysics::BulletInternalPreTickCallback(btDynamicsWorld * const world, btScalar const timeStep)
{
	//E_DEBUG("BulletInternalPreTickCallback()");
	assert(world);

	assert(world->getWorldUserInfo());
	BulletPhysics * const bulletPhysics = static_cast<BulletPhysics*>(world->getWorldUserInfo());
	bulletPhysics->VPreStep(timeStep);
}

void BulletPhysics::SendCollisionEvents()
{
	CollisionPairs currentTickCollisionPairs;

	// look at all existing contacts
	btDispatcher * const dispatcher = m_dynamicsWorld->getDispatcher();
	for (int manifoldIdx = 0; manifoldIdx < dispatcher->getNumManifolds(); ++manifoldIdx)
	{
		// get the "manifold", which is the set of data corresponding to a contact point
		//   between two physics objects
		btPersistentManifold const * const manifold = dispatcher->getManifoldByIndexInternal(manifoldIdx);
		assert(manifold);

		// get the two bodies used in the manifold.  Bullet stores them as void*, so we must cast
		//  them back to btRigidBody*s.  Manipulating void* pointers is usually a bad
		//  idea, but we have to work with the environment that we're given.  We know this
		//  is safe because we only ever add btRigidBodys to the simulation
		btRigidBody const * const body0 = static_cast<btRigidBody const *>(manifold->getBody0());
		btRigidBody const * const body1 = static_cast<btRigidBody const *>(manifold->getBody1());

		// always create the pair in a predictable order
		bool const swapped = body0 > body1;

		btRigidBody const * const sortedBodyA = swapped ? body1 : body0;
		btRigidBody const * const sortedBodyB = swapped ? body0 : body1;

		CollisionPair const thisPair = std::make_pair(sortedBodyA, sortedBodyB);
		currentTickCollisionPairs.insert(thisPair);

		RigidBodyComponent* bodyC0 = static_cast<RigidBodyComponent*>(body0->getUserPointer());
		RigidBodyComponent* bodyC1 = static_cast<RigidBodyComponent*>(body1->getUserPointer());
		// this is a new contact, which wasn't in our list before.  send an event to the game.
		ActorId const id0 = FindActorID(bodyC0);
		ActorId const id1 = FindActorID(bodyC1);

		if (id0 == 0 || id1 == 0)
		{
			E_WARNING("something is colliding with a non-actor.  we currently don't send events for that");
			return;
		}

		// this pair of colliding objects is new.  send a collision-begun event
		std::list<vec3> collisionPoints;
		vec3 sumNormalForce;
		vec3 sumFrictionForce;

		for (int pointIdx = 0; pointIdx < manifold->getNumContacts(); ++pointIdx)
		{
			btManifoldPoint const & point = manifold->getContactPoint(pointIdx);

			collisionPoints.push_back(btVector3_to_Vec3(point.getPositionWorldOnB()));

			sumNormalForce += btVector3_to_Vec3(point.m_combinedRestitution * point.m_normalWorldOnB);
			sumFrictionForce += btVector3_to_Vec3(point.m_combinedFriction * point.m_lateralFrictionDir1);
		}

		if (m_previousTickCollisionPairs.find(thisPair) == m_previousTickCollisionPairs.end())
		{
			// this is new collision
			// send the event for the game
			EvtData_PhysCollisionStart* pEvent = new EvtData_PhysCollisionStart(id0, id1, sumNormalForce, sumFrictionForce, collisionPoints);
			gEventManager()->VQueueEvent(pEvent);
		}
		
		// send the event for the game
		EvtData_PhysOnCollision* pEvent = new EvtData_PhysOnCollision(id0, id1, sumNormalForce, sumFrictionForce, collisionPoints);
		gEventManager()->VQueueEvent(pEvent);
		
	}

	CollisionPairs removedCollisionPairs;

	// use the STL set difference function to find collision pairs that existed during the previous tick but not any more
	std::set_difference(m_previousTickCollisionPairs.begin(), m_previousTickCollisionPairs.end(),
		currentTickCollisionPairs.begin(), currentTickCollisionPairs.end(),
		std::inserter(removedCollisionPairs, removedCollisionPairs.begin()));

	for (CollisionPairs::const_iterator it = removedCollisionPairs.begin(),
		end = removedCollisionPairs.end(); it != end; ++it)
	{
		btRigidBody const * const body0 = it->first;
		btRigidBody const * const body1 = it->second;

		RigidBodyComponent* bodyA = static_cast<RigidBodyComponent*>(body0->getUserPointer());
		RigidBodyComponent* bodyB = static_cast<RigidBodyComponent*>(body1->getUserPointer());

		ActorId const id0 = FindActorID(bodyA);
		ActorId const id1 = FindActorID(bodyB);

		if (id0 == 0 || id1 == 0)
		{
			// collision is ending between some object(s) that don't have actors.  we don't send events for that.
			return;
		}

		EvtData_PhysCollisionEnd* pEvent(new EvtData_PhysCollisionEnd(id0, id1));
		gEventManager()->VQueueEvent(pEvent);
	}

	// the current tick becomes the previous tick.  this is the way of all things.
	m_previousTickCollisionPairs = currentTickCollisionPairs;
}


float BulletPhysics::LookupSpecificGravity(const std::string& densityStr)
{
	float density = 0;
	auto densityIt = m_densityTable.find(densityStr);
	if (densityIt != m_densityTable.end())
		density = densityIt->second;
	// else: dump error

	return density;
}

MaterialData BulletPhysics::LookupMaterialData(const std::string& materialStr)
{
	auto materialIt = m_materialTable.find(materialStr);
	if (materialIt != m_materialTable.end())
		return materialIt->second;
	else
		return MaterialData(0, 0);
}

IGamePhysics *gPhysic()
{
	return BulletPhysics::InstancePtr();
}