#pragma once
#include <vector>
#include <memory>
#include "..\Graphics3D\Mesh.h"
#include "..\Core\Log.h"
#include "RenderPass.h"
#include "Material.h"
#include "..\Math\AABB.h"
namespace Light
{
	namespace render
	{
		
		class Model
		{
		public:
			typedef std::vector<std::unique_ptr<Mesh>> MeshList;

		public:
			
			virtual ~Model(){};
			virtual void Draw(render::RenderDevice* pRenderer,  Material::MatrixParam& matrixParam) = 0;
			virtual MeshList& GetMeshs() = 0;
			virtual math::AABB GetBox() = 0;
		};
	}
}