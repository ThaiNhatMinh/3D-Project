#pragma once
enum TeamClass
{
	TEAM_BL,
	TEAM_GR,
	TEAM_NONE,
	TEAM_MAX
};
class CharacterResource
{
public:
	string Name;
	string ModelFile[2];
	string TexFile[2];
	string ArmTex[2];
	string HandTex[2];
	string AnimFile;
};