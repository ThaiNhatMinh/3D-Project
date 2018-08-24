#pragma once
#include <pch.h>

enum ObjState
{
	STAND = 1,
	JUMPING = 2,
	FALLING = 4,
	WALKING = 8
};
// WASD controller for player
class CharacterControllerComponent : public ActorComponent
{
private:
	vec3			m_MoveDirection;
	vec3			m_JumpDirection;
	float			m_fMaxSpeed;
	float			m_fJumpForce;
	float			m_fMoveForce;
	float			m_fBrakeForce;
	float			m_fInAirTime;
	RigidBodyComponent	*m_pRBC;
	TransformComponent	*m_pTC;
	BaseAnimComponent	*m_pBAC;
	bool			m_bOnGround;
public:
	CharacterControllerComponent();
	~CharacterControllerComponent();
	static const char*	Name;

	virtual bool VInit(const tinyxml2::XMLElement* pData);
	virtual tinyxml2::XMLElement* VGenerateXml(tinyxml2::XMLDocument*p);
	virtual void VPostInit(void);
	virtual void VUpdate(float dt);
	virtual void VOnChanged(void);

	virtual const char *VGetName() const ;

	// Event
	void PhysicCollisionEvent(std::shared_ptr<IEvent> pEvent);
	void PhysicPreStepEvent(std::shared_ptr<IEvent> pEvent);
	void PhysicPostStepEvent(std::shared_ptr<IEvent> pEvent);
};