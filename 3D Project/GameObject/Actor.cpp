#include "pch.h"

Actor::Actor(ActorId id) :m_id(id), m_pParent(nullptr)
{

}

Actor::~Actor()
{
	/*
	// release children
	for (size_t i = 0; i < m_Children.size(); i++)
	{
		deletem_Children[i];
	}
	
	// release component
	for (ActorComponents::iterator it = m_components.begin(); it != m_components.end(); ++it)
	{
		deleteit->second;
	}*/
}

bool Actor::Init(const tinyxml2::XMLElement* pData)
{
	const char* tag = pData->Attribute("type");
	const char* name = pData->Attribute("name");


	m_Tag = tag;
	m_Name = name;

	return 1;
}

void Actor::PostInit(void)
{
	for (ActorComponents::iterator it = m_components.begin(); it != m_components.end(); ++it)
	{
		it->second->VPostInit();
	}
}

void Actor::Destroy(void)
{
}

HRESULT Actor::VOnUpdate(Scene *pScene, float deltaMs)
{
	for (ActorComponents::iterator it = m_components.begin(); it != m_components.end(); ++it)
	{
		it->second->VUpdate(deltaMs);
	}

	ActorList::iterator i = m_Children.begin();
	ActorList::iterator end = m_Children.end();

	while (i != end)
	{
		(*i)->VOnUpdate(pScene, deltaMs);
		++i;
	}
	return S_OK;

	
}

void Actor::VSetTransform(const mat4 * toWorld)
{
	TransformComponent* Tc = GetComponent<TransformComponent>("TransformComponent");
	Tc->SetTransform(*toWorld);
}

mat4 Actor::VGetTransform()
{
	mat4 transform = GetComponent<TransformComponent>("TransformComponent")->GetTransform();
	return transform;
}

mat4 Actor::VGetGlobalTransform()
{
	
	mat4 transform = GetComponent<TransformComponent>("TransformComponent")->GetTransform();
	if (m_pParent)
		transform = transform*m_pParent->VGetGlobalTransform();

	return transform;
}

HRESULT Actor::VPreRender(Scene * pScene)
{
	return S_OK;
}

void Actor::AddComponent(ActorComponent * pComponent)
{
	std::pair<ActorComponents::iterator, bool> success = m_components.insert(std::make_pair(pComponent->VGetId(), pComponent));
	//GCC_ASSERT(success.second);
}

HRESULT Actor::VRender(Scene * pScene)
{
	//m_pShader->SetupRender(pScene, this);

	// this only using on derived class of Actor. This only a test for Component Archir
	MeshRenderComponent* mrc = GetComponent<MeshRenderComponent>("MeshRenderComponent");
	if (mrc) mrc->Render(pScene);
	return S_OK;
	
}

HRESULT Actor::VRenderChildren(Scene * pScene)
{
	// Iterate through the children....
	ActorList::iterator i = m_Children.begin();
	ActorList::iterator end = m_Children.end();
	
	while (i != end)
	{
		if ((*i)->VPreRender(pScene) == S_OK)
		{
			// You could short-circuit rendering
			// if an object returns E_FAIL from
			// VPreRender()

			// Don't render this node if you can't see it
			if ((*i)->VIsVisible(pScene))
			{
				(*i)->VRender(pScene);
				// [mrmike] see comment just below...
				(*i)->VRenderChildren(pScene);
			}

			// [mrmike] post-press fix - if the parent is not visible, the childrend
			//           shouldn't be visible either.
			//(*i)->VRenderChildren(pScene);
		}
		(*i)->VPostRender(pScene);
		++i;
	}	
	return S_OK;
}

HRESULT Actor::VPostRender(Scene * pScene)
{
	return E_NOTIMPL;
}

bool Actor::VAddChild(std::unique_ptr<Actor> kid)
{
	m_Children.push_back(std::move(kid));
	kid->m_pParent = this;
	return true;
}

bool Actor::VRemoveChild(ActorId id)
{
	for (ActorList::iterator it = m_Children.begin(); it != m_Children.end(); it++)
	{
		if ((*it)->GetId() == id)
		{
			m_Children.erase(it);
			return true;
		}
	}
	return false;
}

Actor * Actor::VGetParent()
{
	return m_pParent;
}
