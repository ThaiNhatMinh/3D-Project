#pragma once

class GunPlayerView;
class LocalPlayerComponent : public ActorComponent
{
private:
	vec3			m_MoveDirection;
	vec3			m_JumpDirection;
	float			m_fMaxSpeed;
	float			m_fJumpForce;
	float			m_fMoveForce;
	float			m_fBrakeForce;
	float			m_fInAirTime;
	float			m_Yaw;
	float			m_Pitch;
	int				m_Shooting;
	RigidBodyComponent	*m_pRBC;
	TransformComponent	*m_pTC;
	AnimationComponent	*m_pBAC;
	Actor*			m_PVGroup;
	GunPlayerView*	m_Weapon;
	bool			m_bOnGround;

public:
	LocalPlayerComponent();
	~LocalPlayerComponent();
	static const char*	Name;

	virtual bool VInit(const tinyxml2::XMLElement* pData);
	virtual tinyxml2::XMLElement* VGenerateXml(tinyxml2::XMLDocument*p);
	virtual void VPostInit(void);
	virtual void VUpdate(float dt);
	virtual void VPostUpdate();
	virtual void VOnChanged(void);

	virtual const char *VGetName() const;

	// Event
	void PhysicCollisionEvent(std::shared_ptr<const IEvent> pEvent);
	void PhysicPreStepEvent(std::shared_ptr<const IEvent> pEvent);
	void PhysicPostStepEvent(std::shared_ptr<const IEvent> pEvent);
};