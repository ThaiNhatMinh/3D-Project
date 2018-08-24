#pragma once
#include "..\Graphics3D\ModelRender.h"
namespace Light
{
	class ObjModel : public render::Model
	{
	public:
		std::vector<std::unique_ptr<Mesh>>  Meshs;
		std::vector<render::Texture*> Textures;
		std::vector<std::shared_ptr<render::Material>>	Materials;
	};
}