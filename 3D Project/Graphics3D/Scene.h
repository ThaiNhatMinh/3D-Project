#pragma once
#include "pch.h"
#include "RenderAPI.h"

#include "Camera.h"
class Scene
{
private:
	// Simple Object with only transform component
	// This store every thing in scene
	std::unique_ptr<Actor>	m_pRoot;
	std::list<Actor*>		m_ActorLast;
	Light					m_DirectionLight; // only one direction light


	
	
	Context*			m_Context;
public:
	Scene(Context* c);
	~Scene();

	bool LoadScene(const string& filename);
	bool OnRender();

	bool OnUpdate(float dt);
	bool PostUpdate();
	
	void PushLastActor(Actor*);
	Actor* GetRoot() { return m_pRoot.get(); };
	Light GetDirLight() { return m_DirectionLight; };
};