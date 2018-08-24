#include <pch.h>


#pragma region BaseAnimComponent



void BaseAnimComponent::DrawSkeleton(const mat4& m)
{


	for (size_t i = 0; i < m_pSkeNodes.size(); i++)
	{
		/*int parentID = m_pSkeNodes[i]->m_ParentIndex;
		if (parentID != -1)
		{
			vec3 pos1 = m_DbTransform[i][3];
			vec3 pos2 = m_DbTransform[parentID][3];
			m_Context->m_pDebuger->DrawLine(pos1, pos2, vec3(1.0f, 1.0, 1.0f), m);
		}*/
		
		//if (m_pSkeNodes[i]->m_Name.find("Spine") == string::npos|| m_pSkeNodes[i]->m_Name.find("Finger") != string::npos) continue;
		
		if (m_pSkeNodes[i]->m_Flag != 1) continue;
		vec3 v[8];
		m_pSkeNodes[i]->m_BoundBox.GenPoint(v);
		//mat4 mm = temp;
		mat4 temp = m*m_DbTransform[i];
		vec3 color = vec3(1,1,1);
		
		//m_pDebuger->DrawLine(m_pSkeNodes[i]->m_BoundBox.Min, m_pSkeNodes[i]->m_BoundBox.Max, vec3(0, 1, 0), temp);

		m_pDebuger->DrawLine(v[0], v[1], color, temp);
		m_pDebuger->DrawLine(v[1], v[2], color, temp);
		m_pDebuger->DrawLine(v[2], v[3], color, temp);
		m_pDebuger->DrawLine(v[3], v[0], color, temp);

		m_pDebuger->DrawLine(v[4], v[5], color, temp);
		m_pDebuger->DrawLine(v[5], v[6], color, temp);
		m_pDebuger->DrawLine(v[6], v[7], color, temp);
		m_pDebuger->DrawLine(v[7], v[4], color, temp);

		m_pDebuger->DrawLine(v[0], v[4], color, temp);
		m_pDebuger->DrawLine(v[1], v[5], color, temp);
		m_pDebuger->DrawLine(v[2], v[6], color, temp);
		m_pDebuger->DrawLine(v[3], v[7], color, temp);

		//vec3 pos1 = m_DbTransform[i][3];

		//vec3 front = vec3(m_DbTransform[i][0]) + pos1;
		//m_Context->m_pDebuger->DrawLine(pos1, front, vec3(1, 0, 0), m);
		//front = 2.0f*vec3(m_DbTransform[i][1]) + pos1;
		//m_Context->m_pDebuger->DrawLine(pos1, front, vec3(0, 1, 0), m);
		//front = 4.0f*vec3(m_DbTransform[i][2]) + pos1;
		//m_Context->m_pDebuger->DrawLine(pos1, front, vec3(0, 0, 1), m);
		
	}

}

const vector<mat4>& BaseAnimComponent::GetBoneTransform()
{
	return m_DbTransform;
}

void BaseAnimComponent::SetData(ModelCache * pModel)
{
	// We cannot assign so just coppy pointer
	for (size_t i = 0; i < pModel->pSkeNodes.size(); i++)
		m_pSkeNodes.push_back(pModel->pSkeNodes[i].get());
	for (size_t i = 0; i < pModel->pAnims.size(); i++)
		m_pAnimList.push_back(pModel->pAnims[i].get());

	m_WB = pModel->wb;
	m_SkeTransform.resize(m_pSkeNodes.size());
	m_CurrentFrames.resize(m_pSkeNodes.size());
	m_DbTransform.resize(m_pSkeNodes.size());

	//m_iDefaultAnimation = FindAnimation(pNameAnim);
}


const vector<mat4>& BaseAnimComponent::GetVertexTransform()
{
	return m_SkeTransform;
}



bool BaseAnimComponent::VInit(Context* pContext,const tinyxml2::XMLElement* pData)
{
	
	//m_Context->m_pConsole->RegisterVar("draw_skeleton", &m_bDrawSkeleton, 1, sizeof(int), TYPE_INT);
	if (!pData) return false;
	// load model
	const tinyxml2::XMLElement* pModelNode = pData->FirstChildElement("Model");
	const char* pFileName = pModelNode->Attribute("File");

	const tinyxml2::XMLElement* pAnimNode = pData->FirstChildElement("DefaultAnim");
	const char* pNameAnim = pAnimNode->Attribute("Anim");

	if (strlen(pFileName) > 1)
	{
		ModelCache* pModel = static_cast<ModelCache*>(pContext->GetSystem<Resources>()->GetModel(pFileName));

		if (!pModel)
		{
			E_ERROR("AnimationComponent can't load data.");
			return 0;
		}

		// We cannot assign so just coppy pointer
		for (size_t i = 0; i < pModel->pSkeNodes.size(); i++)
			m_pSkeNodes.push_back(pModel->pSkeNodes[i].get());
		for (size_t i = 0; i < pModel->pAnims.size(); i++)
			m_pAnimList.push_back(pModel->pAnims[i].get());

		m_WB = pModel->wb;
		m_SkeTransform.resize(m_pSkeNodes.size());
		m_CurrentFrames.resize(m_pSkeNodes.size());
		m_DbTransform.resize(m_pSkeNodes.size());

		
	}

	m_pDebuger = pContext->GetSystem<Debug>();
	return true;
}


FrameData BaseAnimComponent::InterpolateFrame(AnimControl & control, const AnimNode & Anim, const vector<AnimKeyFrame>& KeyFrames)
{
	int frame0 = -1;
	int frame1 = -1;
	float t = 0.0f;

	if (control.m_iCurrentFrame == KeyFrames[control.KeyFrameID].m_Time)
	{
		frame1 = control.KeyFrameID;
		if (KeyFrames[frame1].m_pString.size() > 0)	
			AnimEvent(KeyFrames[frame1].m_pString);
		return	Anim.Data[frame1];
	}
	else if (control.m_iCurrentFrame > KeyFrames[control.KeyFrameID].m_Time)
	{
		control.KeyFrameID++;
		if (control.KeyFrameID == KeyFrames.size())
		{
			control.m_iCurrentFrame = 0;
			control.KeyFrameID = 0;
			control.m_bFinished = 1;
			if (KeyFrames[control.KeyFrameID].m_pString.size() > 0)	AnimEvent(KeyFrames[control.KeyFrameID].m_pString);
			return	Anim.Data[control.KeyFrameID];
		}
		frame0 = control.KeyFrameID - 1;
		frame1 = control.KeyFrameID;
	}
	else
	{
		frame0 = control.KeyFrameID - 1;
		frame1 = control.KeyFrameID;
	}

	t = (float)(control.m_iCurrentFrame - KeyFrames[frame0].m_Time) / (float)(KeyFrames[frame1].m_Time - KeyFrames[frame0].m_Time);

	if (t>1.0f || t<0.0f) return Anim.Data[control.KeyFrameID];
	FrameData frame;
	frame.m_Pos = glm::lerp(Anim.Data[frame0].m_Pos, Anim.Data[frame1].m_Pos, t);
	frame.m_Ort = glm::slerp(Anim.Data[frame0].m_Ort, Anim.Data[frame1].m_Ort, t);

	return frame;
}

GLint BaseAnimComponent::FindAnimation(string name)
{
	for (size_t i = 0; i<m_pAnimList.size(); i++)
		if (m_pAnimList[i]->Name == name)
		{
			return i;
		}


	return 0;
}

#pragma endregion

#pragma region AnimationComponent
AnimationComponent::blendset AnimationComponent::GetBlendSet(GLuint id)
{
	Animation* pAnim = m_pAnimList.at(id);
	
	if (pAnim->Name.find("walk") != string::npos || pAnim->Name.find("run") != string::npos || pAnim->Name.find("jump") != string::npos) return fullbody;

	if (pAnim->Name.find("hit") != string::npos || pAnim->Name.find("shoot") != string::npos || pAnim->Name.find("combo") != string::npos || pAnim->Name.find("bigshot") != string::npos || pAnim->Name.find("reload") != string::npos) return upper;

	return fullbody;

}

void AnimationComponent::ResetControl(CharAnimControl& control, GLuint anim, AnimState state)
{
	control.m_fTime = 0;
	control.m_iCurrentAnim = anim;
	control.m_iCurrentFrame = 0;
	control.KeyFrameID = 0;
	control.m_State = state;
	control.m_bFinished = 0;
}

void AnimationComponent::AnimEvent(const string& data)
{
	std::shared_ptr<IEvent> pEvent(new EvtData_AnimationString(m_pOwner->VGetId(), data));
	m_pEventManager->VQueueEvent(pEvent);
}

void AnimationComponent::Play(blendset layer, int anim, bool loop)
{
	CharAnimControl cac;
	cac.m_layer = layer;
	
	if (layer == blendset::fullbody)
	{
		if(m_Controls.size()==0) m_Controls.push_back(cac);

		CharAnimControl& fullbody = m_Controls.front();
		ResetControl(fullbody, anim, ANIM_TRANSITION);
	}
	else
	{
		ResetControl(cac, anim, ANIM_PLAYING);
		cac.m_loop = loop;
		m_Controls.push_back(cac);
	}
	//if (m_Control.size() > 0) ResetControl(cac, anim, ANIM_TRANSITION);
	//else ResetControl(cac, anim, ANIM_PLAYING);

	
}

/*void AnimationComponent::Play(blendset part, int anim, bool fromBaseAnim)
{
	GLint animID = 0;

	if (fromBaseAnim) animID = anim + m_iDefaultAnimation;
	else animID = anim;

	if (m_Control[part].m_iCurrentAnim == animID) return;

	ResetControl(part, animID, ANIM_TRANSITION);
}*/

void AnimationComponent::SetBoneEdit(float yaw, float pitch)
{
	m_Yaw = yaw;
	m_Pitch = pitch;
}

bool AnimationComponent::IsFinish()
{
	return m_Controls.front().m_bFinished;
}




void AnimationComponent::ComputerFrame(CharAnimControl & control,int i)
{
	Animation* anim = m_pAnimList[control.m_iCurrentAnim];

	if (control.m_State == ANIM_TRANSITION)
	{
		float t = control.m_fTime / m_fBlendTime;

		m_CurrentFrames[i].m_Pos = glm::lerp(m_CurrentFrames[i].m_Pos, anim->AnimNodeLists[i].Data[0].m_Pos, t);
		m_CurrentFrames[i].m_Ort = glm::slerp(m_CurrentFrames[i].m_Ort, anim->AnimNodeLists[i].Data[0].m_Ort, t);
	}
	else if (control.m_State == ANIM_PLAYING)
	{
		m_CurrentFrames[i] = InterpolateFrame(control, anim->AnimNodeLists[i], anim->KeyFrames);
	}
}

AnimationComponent::AnimationComponent(void)
{
	/*m_Control[upper].m_fTime = 0;
	m_Control[upper].m_iCurrentAnim = 0;
	m_Control[upper].m_iCurrentFrame = 0;
	m_Control[upper].KeyFrameID = 0;
	m_Control[upper].m_State = ANIM_PLAYING;

	m_Control[lower].m_fTime = 0;
	m_Control[lower].m_iCurrentAnim = 0;
	m_Control[lower].m_iCurrentFrame = 0;
	m_Control[lower].KeyFrameID = 0;
	m_Control[lower].m_State = ANIM_PLAYING;*/

	m_fBlendTime = 0.2f;
	m_Yaw = 0;
	m_Pitch = 0;
}

AnimationComponent::~AnimationComponent(void)
{
	m_pEventManager->VRemoveListener(MakeDelegate(this, &AnimationComponent::SetAnimationEvent), EvtData_SetAnimation::sk_EventType);
}


bool AnimationComponent::VInit(Context* pContext,const tinyxml2::XMLElement * pData)
{
	bool result = BaseAnimComponent::VInit(pContext,pData);

	const tinyxml2::XMLElement* pAnimNode = pData->FirstChildElement("DefaultAnim");
	const char* pNameAnim = pAnimNode->Attribute("Anim");

	if (m_pSkeNodes.size())
	{
		int anim = FindAnimation(pNameAnim);
		// Play idle animation
		Play(blendset::fullbody, anim, true);
	}

	m_pEventManager = pContext->GetSystem<EventManager>();
	return result;
}

void AnimationComponent::VPostInit(void)
{
	m_pEventManager->VAddListener(MakeDelegate(this, &AnimationComponent::SetAnimationEvent), EvtData_SetAnimation::sk_EventType);
	
	//ResetControl(upper, m_iDefaultAnimation, ANIM_PLAYING);
	//ResetControl(lower, m_iDefaultAnimation, ANIM_PLAYING);
}

void AnimationComponent::VUpdate(float deltaMs)
{
	//if (m_Context->DrawSkeleton) DrawSkeleton(m_pOwner->VGetGlobalTransform());

	if (!m_pAnimList.size()) return;
	if (!m_Controls.size()) return;

	CharAnimControl& fullbody = m_Controls.front();

	for (auto& el : m_Controls)
	{
		el.m_fTime += deltaMs;

		if (el.m_State == ANIM_PLAYING) el.m_iCurrentFrame = (GLuint)(el.m_fTime * 1000);
		else if (el.m_State == ANIM_TRANSITION)
		{
			if (el.m_fTime > m_fBlendTime)
			{
				el.m_State = ANIM_PLAYING;
				el.m_fTime = 0.0f;
				el.m_iCurrentFrame = 0;
			}
		}
	}
	/*m_Control[lower].m_fTime += deltaMs;
	m_Control[upper].m_fTime += deltaMs;

	if (m_Control[upper].m_State == ANIM_PLAYING)
	{
		
		m_Control[upper].m_iCurrentFrame = (GLuint)(m_Control[upper].m_fTime * 1000);
	}
	else if(m_Control[upper].m_State == ANIM_TRANSITION)
	{
		if (m_Control[upper].m_fTime > m_fBlendTime)
		{
			m_Control[upper].m_State = ANIM_PLAYING;
			m_Control[upper].m_fTime = 0.0f;
			m_Control[upper].m_iCurrentFrame = 0;
		}
	}

	if (m_Control[lower].m_State == ANIM_PLAYING)
	{
		m_Control[lower].m_iCurrentFrame = (GLuint)(m_Control[lower].m_fTime * 1000);
	}
	else if(m_Control[lower].m_State == ANIM_TRANSITION)
	{
		
		if (m_Control[lower].m_fTime > m_fBlendTime)
		{
			m_Control[lower].m_State = ANIM_PLAYING;
			m_Control[lower].m_fTime = 0.0f;
			m_Control[lower].m_iCurrentFrame = 0;
		}
	}

	
	Animation* animUpper = m_pAnimList[m_Control[upper].m_iCurrentAnim];
	Animation* animLower = m_pAnimList[m_Control[lower].m_iCurrentAnim];
	*/
	
	for (GLuint i = 0; i < m_pSkeNodes.size(); i++)
	{
		// process upper 
		/*if (m_Control[upper].m_State == ANIM_TRANSITION && m_WB[upper].Blend[i])
		{
			float t = m_Control[upper].m_fTime / m_fBlendTime;
			m_CurrentFrames[i].m_Pos = glm::lerp(m_CurrentFrames[i].m_Pos, animUpper->AnimNodeLists[i].Data[0].m_Pos, t);
			m_CurrentFrames[i].m_Ort = glm::slerp(m_CurrentFrames[i].m_Ort, animUpper->AnimNodeLists[i].Data[0].m_Ort, t);
		}
		else if (m_Control[upper].m_State == ANIM_PLAYING && m_WB[upper].Blend[i])
		{
			m_CurrentFrames[i] = InterpolateFrame(m_Control[upper], animUpper->AnimNodeLists[i], animUpper->KeyFrames);
		}

		// process lower 
		else if (m_Control[lower].m_State == ANIM_TRANSITION && m_WB[lower].Blend[i])
		{
			float t = m_Control[lower].m_fTime / m_fBlendTime;
			m_CurrentFrames[i].m_Pos = glm::lerp(m_CurrentFrames[i].m_Pos, animLower->AnimNodeLists[i].Data[0].m_Pos, t);
			m_CurrentFrames[i].m_Ort = glm::slerp(m_CurrentFrames[i].m_Ort, animLower->AnimNodeLists[i].Data[0].m_Ort, t);
		}
		else if (m_Control[lower].m_State == ANIM_PLAYING && m_WB[lower].Blend[i])
		{
			
			m_CurrentFrames[i] = InterpolateFrame(m_Control[lower], animLower->AnimNodeLists[i], animLower->KeyFrames);
		}
		*/

		CharAnimControl& fullbody = m_Controls.front();
		
		if (m_Controls.size() > 1 && m_WB[upper].Blend[i]) ComputerFrame(m_Controls.back(), i);
		else ComputerFrame(fullbody, i);


		mat4 m_TransformLocal;
		mat4 rotate = glm::toMat4(m_CurrentFrames[i].m_Ort);
		mat4 translate = glm::translate(mat4(), m_CurrentFrames[i].m_Pos);
		mat4 transform = translate*rotate;

		
		
		if (m_Pitch != 0)
		{
			if (m_pSkeNodes[i]->m_Name == "M-bone Neck")
			{
				mat4 rotate;
				rotate = glm::rotate(rotate, glm::radians(m_Pitch / 3), vec3(0, 1, 0));
				transform = rotate*transform;
			}
			else if (m_pSkeNodes[i]->m_Name == "M-bone Spine")
			{
				mat4 rotate;
				rotate = glm::rotate(rotate, glm::radians(m_Pitch / 3), vec3(0, 1, 0));
				transform = rotate*transform;
			}
			else if (m_pSkeNodes[i]->m_Name == "M-bone Spine1")
			{
				mat4 rotate;
				rotate = glm::rotate(rotate, glm::radians(m_Pitch / 3), vec3(0, 1, 0));
				transform = rotate*transform;
			}
		}

		if (m_pSkeNodes[i]->m_ParentIndex != -1) m_TransformLocal = m_DbTransform[m_pSkeNodes[i]->m_ParentIndex] * transform;
		else m_TransformLocal = transform;
		
		m_SkeTransform[i] = m_TransformLocal;
		m_DbTransform[i] = m_TransformLocal;
		m_SkeTransform[i] = m_SkeTransform[i] * m_pSkeNodes[i]->m_InvBindPose;


	}

	
	
}

void AnimationComponent::VPostUpdate()
{
	/*for (auto it = m_Controls.begin(); it!=m_Controls.end(); it++)
	{
		if (it->m_bFinished)
		{
			if (it->m_loop) ResetControl(*it, it->m_iCurrentAnim, ANIM_PLAYING);
			else m_Control.erase(it);
		}
	}*/
	CharAnimControl& fullbody = m_Controls.front();
	if (fullbody.m_bFinished)
	{
		ResetControl(fullbody, fullbody.m_iCurrentAnim, ANIM_PLAYING);
	}

	if (m_Controls.size() > 1)
	{
		CharAnimControl& upper = m_Controls.back();
		if (upper.m_bFinished)
		{
			if (upper.m_loop) ResetControl(upper, upper.m_iCurrentAnim, ANIM_PLAYING);
			else m_Controls.pop_back();
		}
	}
}


void AnimationComponent::SetAnimationEvent(std::shared_ptr<IEvent> pEvent)
{
	/*const EvtData_SetAnimation* p = dynamic_cast<const EvtData_SetAnimation*>(pEvent.get());

	if (p->GetId() != m_pOwner->GetId()) return;

	GLuint animID = p->GetAnimation();

	if (animID >= m_pAnimList.size()) return;
	blendset bs = GetBlendSet(animID);
	
	m_Control[bs].KeyFrameID = 0;
	m_Control[bs].m_iCurrentFrame = 0;
	m_Control[bs].m_iCurrentAnim = animID;
	m_Control[bs].m_fTime = 0.0f;				// restart time to zero
	if(bs==lower) m_Control[bs].m_State = ANIM_TRANSITION;    // Set state to transition to blend current frame of character to frame 0 of m_iCurrentAnim
	else m_Control[bs].m_State = ANIM_PLAYING;
	if (p->isDefault()) m_iDefaultAnimation = animID;*/

}
/*
void AnimationComponent::PlayAnimation(int anim, bool fromBaseAnim)
{
	GLint animID = 0;
	

	//if (animID == -1) return;
	if(fromBaseAnim) animID = anim + m_iDefaultAnimation;
	else animID = anim;

	blendset bs = GetBlendSet(animID);
	
	if (m_Control[bs].m_iCurrentAnim == animID) return;
	
	ResetControl(bs, animID, ANIM_TRANSITION);
}

void AnimationComponent::PlayAnimation(const string & anim, bool v)
{
	int id = FindAnimation(anim);
	PlayAnimation(id, v);
}

void AnimationComponent::PlayDefaultAnimation()
{
	//blendset bs = GetBlendSet(m_iDefaultAnimation);

	if (m_Control[lower].m_iCurrentAnim == m_iDefaultAnimation) return;
	ResetControl(lower, m_iDefaultAnimation, ANIM_TRANSITION);

	//if (m_Control[upper].m_iCurrentAnim == m_iDefaultAnimation) return;
	//ResetControl(upper, m_iDefaultAnimation, ANIM_TRANSITION);
}
*/
AABB AnimationComponent::GetUserDimesion()
{
	assert(m_Controls.size() > 0);

	return m_pAnimList[m_Controls.front().m_iCurrentAnim]->m_BV;
}


#pragma endregion

#pragma region PlayerViewAnimation
// Using to control PV Model


void PVAnimationComponent::ResetControl(GLuint anim, AnimState state)
{
	m_Control.m_fTime = 0;
	m_Control.m_iCurrentAnim = anim;
	m_Control.m_iCurrentFrame = 0;
	m_Control.KeyFrameID = 0;
	m_Control.m_State = state;
	m_Control.m_bFinished = 0;
}


PVAnimationComponent::PVAnimationComponent(void)
{
	m_iDefaultAnimation = 0;
	m_Control.m_fTime = 0;
	m_Control.m_iCurrentAnim = 0;
	m_Control.m_iCurrentFrame = 0;
	m_Control.KeyFrameID = 0;
	m_Control.m_State = ANIM_PLAYING;
	m_fBlendTime = 0.3f;
}


void PVAnimationComponent::VPostInit(void)
{
	m_Control.m_loop = 1;
}

void PVAnimationComponent::VUpdate(float deltaMs)
{
	if (!m_pAnimList.size()) return;
	if (m_Control.m_bFinished && !m_Control.m_loop) return;

	if (m_Control.m_State != ANIM_STOP)
	{
		m_Control.m_fTime += deltaMs;
		m_Control.m_iCurrentFrame = (GLuint)(m_Control.m_fTime * 1000);
	}

	Animation* anim = m_pAnimList[m_Control.m_iCurrentAnim];
	for (GLuint i = 0; i < anim->AnimNodeLists.size(); i++)
	{
		if (m_Control.m_State == ANIM_TRANSITION)
		{
			if (m_Control.m_fTime > m_fBlendTime)
			{
				m_Control.m_State = ANIM_PLAYING;
				m_Control.m_fTime = 0.0f;
			}
			else
			{
				float t = m_Control.m_fTime / m_fBlendTime;
				m_CurrentFrames[i].m_Pos = glm::lerp(m_CurrentFrames[i].m_Pos, anim->AnimNodeLists[i].Data[0].m_Pos, t);
				m_CurrentFrames[i].m_Ort = glm::slerp(m_CurrentFrames[i].m_Ort, anim->AnimNodeLists[i].Data[0].m_Ort, t);
			}
		}
		else if (m_Control.m_State == ANIM_PLAYING)
		{
			m_CurrentFrames[i] = InterpolateFrame(m_Control, anim->AnimNodeLists[i], anim->KeyFrames);
		}




		mat4 m_TransformLocal;
		mat4 rotate = glm::toMat4(m_CurrentFrames[i].m_Ort);
		mat4 translate = glm::translate(mat4(), m_CurrentFrames[i].m_Pos);
		mat4 transform = translate*rotate;

		if (anim->AnimNodeLists[i].Parent != -1) m_TransformLocal = m_DbTransform[anim->AnimNodeLists[i].Parent] * transform;
		else m_TransformLocal = transform;

		m_SkeTransform[i] = m_TransformLocal;
		m_DbTransform[i] = m_SkeTransform[i];
		m_SkeTransform[i] = m_SkeTransform[i] * m_pSkeNodes[i]->m_InvBindPose;


	}


	

}

void PVAnimationComponent::VPostUpdate()
{
	if (m_Control.m_bFinished)
	{
		if (m_Control.m_loop) ResetControl(m_Control.m_iCurrentAnim, ANIM_PLAYING);
		else
		{
			ResetControl(m_iDefaultAnimation, ANIM_PLAYING);
			m_Control.m_loop = true;
		}
	}
}

void PVAnimationComponent::SetAnimationEvent(std::shared_ptr<IEvent> pEvent)
{
	const EvtData_SetAnimation* p = dynamic_cast<const EvtData_SetAnimation*>(pEvent.get());

	if (p->GetId() != m_pOwner->VGetId()) return;

	GLuint animID = p->GetAnimation();

	if (animID >= m_pAnimList.size()) return;

	m_Control.KeyFrameID = 0;
	m_Control.m_iCurrentFrame = 0;
	m_Control.m_iCurrentAnim = animID;
	m_Control.m_fTime = 0.0f;				// restart time to zero
	m_Control.m_State = ANIM_PLAYING;
	if (p->isDefault()) m_iDefaultAnimation = animID;
}

void PVAnimationComponent::PlayAnimation(int anim, bool loop)
{
	if (anim == m_Control.m_iCurrentAnim) return;

	ResetControl(anim, ANIM_PLAYING);
	m_Control.m_loop = loop;
}

AABB PVAnimationComponent::GetUserDimesion()
{
	if (m_Control.m_iCurrentAnim<0 || m_Control.m_iCurrentAnim >= m_pAnimList.size()) return AABB();
	return m_pAnimList[m_Control.m_iCurrentAnim]->m_BV;
}

void PVAnimationComponent::AnimEvent(const string&)
{
}

mat4 PVAnimationComponent::GetRootTransform()
{
	return m_DbTransform[0];
}
bool PVAnimationComponent::IsFinish()
{
	return m_Control.m_bFinished;
}
#pragma endregion


#pragma region AnimationState
AnimationState::AnimationState(Animation * p):KeyFrameID(0), m_fTime(0),m_pAnim(p), m_iCurrentFrame(0), m_loop(0),m_speed(1.0f)
{
	
}

void AnimationState::Update(float dt)
{

}

#pragma endregion