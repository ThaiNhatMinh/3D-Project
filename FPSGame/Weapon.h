#pragma once

class Weapon :public Actor
{
public:
	
	Weapon(ActorId id);
	virtual bool			Init(const tinyxml2::XMLElement* pData);
	virtual void			PostInit(void);
	virtual HRESULT			VRender(Scene* pScene) override;
	virtual HRESULT			VOnUpdate(Scene *, float elapsedMs);
	virtual mat4			VGetGlobalTransform();
	
	
	mat4&					GetSocketTransform();
	const string&			GetPVFileName();;
	int						GetWeaponSlot();
	int						GetWeaponIndex();
	const WeaponResource&	GetWeaponInfo()const;

	
private:

	
	std::unique_ptr<MeshRenderComponent> m_MeshRender;
	// using to get bone transform;
	AnimationComponent*		m_ParentAnim;
	mat4					m_BoneTransform;
	mat4					m_SocketTransform;
	int						m_BoneID;
	WeaponResource			WeaponInfo;
	
};