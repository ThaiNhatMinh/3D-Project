#include "stdafx.h"
#include "PlayerView.h"

PlayerView::PlayerView(ActorId id) :Actor(id)
{
	
}

bool PlayerView::Init(const tinyxml2::XMLElement * pData)
{
	bool r = Actor::Init(pData);

	/*const tinyxml2::XMLElement * pNode = pData->FirstChildElement("PVTransform");
	
	const tinyxml2::XMLElement* pPositionElement = pNode->FirstChildElement("Position");
	vec3 position, yawPitchRoll;
	if (pPositionElement)
	{
		float x = 0;
		float y = 0;
		float z = 0;
		x = pPositionElement->FloatAttribute("x");
		y = pPositionElement->FloatAttribute("y");
		z = pPositionElement->FloatAttribute("z");
		position = vec3(x, y, z);
	}

	const tinyxml2::XMLElement* pOrientationElement = pNode->FirstChildElement("YawPitchRoll");
	if (pOrientationElement)
	{
		float yaw = 0;
		float pitch = 0;
		float roll = 0;
		pitch = pOrientationElement->FloatAttribute("x");
		yaw = pOrientationElement->FloatAttribute("y");
		roll = pOrientationElement->FloatAttribute("z");
		yawPitchRoll = vec3(pitch, yaw, roll);
	}


	mat4 translation;
	translation = glm::translate(translation, position);

	mat4 rotation = glm::eulerAngleYXZ(yawPitchRoll.y, yawPitchRoll.x, yawPitchRoll.z);

	m_PVTransform = translation*rotation;

	pNode = pNode->FirstChildElement("FOV");

	m_fFOV = pNode->DoubleAttribute("fov", 45.0);*/

	return 1;
}

void PlayerView::PostInit(void)
{
	Actor::PostInit();

}

HRESULT PlayerView::VRender(Scene * pScene)
{
	//ICamera* pCam = pScene->GetCurrentCamera();
	pScene->PushLastActor(this);
	static auto* pDebug = m_Context->GetSystem<Debug>();
	pDebug->DrawLine(InvView[3], -InvView[2] * 2000.0f, vec3(1.0, 1.0, 1.0));
	return S_OK;
}

HRESULT PlayerView::VOnUpdate(Scene *pScene, float elapsedMs)
{
	Actor::VOnUpdate(pScene, elapsedMs);

	InvView = pScene->GetCurrentCamera()->GetViewMatrix();
	InvView = glm::inverse(InvView);
	

	return S_OK;
}

mat4 PlayerView::VGetGlobalTransform()
{
	mat4 trans = InvView* m_TransformComponent->GetTransform();
	return trans;
}