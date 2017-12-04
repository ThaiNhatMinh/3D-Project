#include <pch.h>
#include "GameHeader.h"
#include "Game.h"

void Game::Init(Context *c)
{
	LoadWeapon();
	LoadCharacter();

	m_Scene = std::unique_ptr<Scene>(new Scene(c));
	ActorFactory& factory = m_Scene->GetActorFactory();

	Actor* p4 = factory.CreateActor("GameAssets\\ACTOR\\Player.xml", nullptr, nullptr);
	m_Scene->GetRoot()->VAddChild(std::unique_ptr<Actor>(p4));

	Actor*pp = factory.CreateActor("GameAssets\\ACTOR\\Terrain.xml", nullptr, nullptr);
	m_Scene->GetRoot()->VAddChild(std::unique_ptr<Actor>(pp));
	pp = factory.CreateActor("GameAssets\\ACTOR\\PV.xml", nullptr, nullptr);
	m_Scene->GetRoot()->VAddChild(std::unique_ptr<Actor>(pp));

	// send wp data;
	std::shared_ptr<const IEvent> p(new EvtData_PlayerWpData(m_WeaponResources));
	c->m_pEventManager->VTriggerEvent(p);

}

void Game::Update(float dt)
{
	m_Scene->OnUpdate(dt);
}

void Game::Render()
{
	m_Scene->OnRender();
}

void Game::LoadWeapon()
{
	string file = "GameAssets\\XML\\Weapon.xml";

	tinyxml2::XMLDocument doc;
	doc.LoadFile(file.c_str());
	tinyxml2::XMLElement* pWeapon = doc.FirstChildElement("Weapon");
	
	// Loop through each child element and load the component
	for (tinyxml2::XMLElement* pNode = pWeapon->FirstChildElement(); pNode; pNode = pNode->NextSiblingElement())
	{
		WeaponResource wr;
		wr.Class = pNode->FirstChildElement("Class")->DoubleAttribute("Class");
		wr.ModelFile = pNode->FirstChildElement("ModelFileName")->Attribute("File");
		wr.ModelTex = pNode->FirstChildElement("SkinFileName")->Attribute("File");
		wr.PVModelFile = pNode->FirstChildElement("PViewModelFileName")->Attribute("File");
		wr.PVTexFile = pNode->FirstChildElement("PViewSkinFileName")->Attribute("File");
		wr.AnimName = pNode->FirstChildElement("GViewAnimName")->Attribute("File");

		wr.Range = pNode->FirstChildElement("Info")->DoubleAttribute("Range");
		wr.MaxAmmo = pNode->FirstChildElement("Info")->DoubleAttribute("MaxAmmo");
		wr.AmmoPerMagazine = pNode->FirstChildElement("Info")->DoubleAttribute("AmmoPerMagazine");
		wr.AmmoDamage = pNode->FirstChildElement("Info")->DoubleAttribute("AmmoDamage");
		wr.TargetSlot = pNode->FirstChildElement("Info")->DoubleAttribute("TargetSlot");

		m_WeaponResources.push_back(wr);
	}

}

void Game::LoadCharacter()
{
	string file = "GameAssets\\XML\\Character.xml";

	tinyxml2::XMLDocument doc;
	doc.LoadFile(file.c_str());
	tinyxml2::XMLElement* pCharacter = doc.FirstChildElement("Character");
	for (tinyxml2::XMLElement* pNode = pCharacter->FirstChildElement(); pNode; pNode = pNode->NextSiblingElement())
	{
		CharacterResource cr;
		cr.Name = pNode->Value();
		tinyxml2::XMLElement* pBL = pNode->FirstChildElement("BL");
		cr.ModelFile[TEAM_BL] = pBL->FirstChildElement("Model")->Attribute("File");
		cr.ModelFile[TEAM_BL] = pBL->FirstChildElement("Skin")->Attribute("File");
		cr.ModelFile[TEAM_BL] = pBL->FirstChildElement("Arm")->Attribute("File");
		cr.ModelFile[TEAM_BL] = pBL->FirstChildElement("Hand")->Attribute("File");

		tinyxml2::XMLElement* pBL = pNode->FirstChildElement("GR");
		cr.ModelFile[TEAM_GR] = pBL->FirstChildElement("Model")->Attribute("File");
		cr.ModelFile[TEAM_GR] = pBL->FirstChildElement("Skin")->Attribute("File");
		cr.ModelFile[TEAM_GR] = pBL->FirstChildElement("Arm")->Attribute("File");
		cr.ModelFile[TEAM_GR] = pBL->FirstChildElement("Hand")->Attribute("File");

		cr.AnimFile = pNode->FirstChildElement("Animation")->Attribute("File");
		m_CharacterResources.push_back(cr);
	}


}
