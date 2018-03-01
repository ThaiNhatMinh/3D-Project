#include "pch.h"

VGUI::VGUI():m_Root(new UIGroup(this))
{
	
}

VGUI::~VGUI()
{
	FTFont::ReleaseFreeTypeFont();
}

void VGUI::Init(Context * c)
{
	m_pWindows = c->GetSystem<Windows>();
	vec2 size = m_pWindows->GetWindowSize();
	m_Proj = glm::ortho(0.0f, size.x,0.0f,size.y);

	m_UIShader = c->GetSystem<Resources>()->GetShader("UI");

	FTFont::InitFreeTypeFont();
	AddFont("Default", "GameAssets\\FONTS\\segoeui.ttf");
	c->AddSystem(this);
}

void VGUI::Render()
{
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	m_UIShader->Use();
	m_UIShader->SetUniformMatrix("MVP", glm::value_ptr(m_Proj));
	m_Root->Render();
	glDisable(GL_BLEND);
	glEnable(GL_CULL_FACE);
}

void VGUI::Update(float dt)
{
	
	m_Root->Update(dt, m_pWindows->GetMousePos());
}

UIGroup * VGUI::GetRoot()
{
	return m_Root.get();
}

bool VGUI::AddFont(const string & fontname, const string & fontfile)
{
	for (auto& el : m_FontLists)
		if (el->GetName() == fontname) return false;

	m_FontLists.push_back(std::unique_ptr<FTFont>(new FTFont(fontname, fontfile)));

	return true;
}

Shader * VGUI::GetShader()
{
	return m_UIShader;
}

const mat4 & VGUI::GetProj()
{
	return m_Proj;
}

FTFont * VGUI::GetFont(const string & fontname)
{
	for (auto& el : m_FontLists)
		if (el->GetName() == fontname) return el.get();

	return nullptr;
}