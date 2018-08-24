#include <pch.h>
#include "SkeMesh.h"

namespace Light 
{
	SkeMesh::SkeMesh(render::RenderDevice* pRenderDevice, LTRawMesh* pData) //:m_Vertexs(vertex), m_Indices(indices)
	{
		auto& vertex = pData->Vertexs;
		auto& indices = pData->Indices;
		m_pVBO = std::unique_ptr<render::VertexBuffer>(pRenderDevice->CreateVertexBuffer(vertex.size() * sizeof(SkeVertex), &vertex[0]));
		m_pIBO = std::unique_ptr<render::IndexBuffer>(pRenderDevice->CreateIndexBuffer(indices.size() * sizeof(unsigned int), &indices[0]));
		
		size_t stride = sizeof(SkeVertex);
		render::VertexElement elements[] = 
		{
			{ render::SHADER_POSITION_ATTRIBUTE, render::VERTEXELEMENTTYPE_FLOAT, 3, stride, 0 },
			{ render::SHADER_NORMAL_ATTRIBUTE, render::VERTEXELEMENTTYPE_FLOAT, 3, stride, 3 * sizeof(float) },
			{ render::SHADER_TEXCOORD_ATTRIBUTE, render::VERTEXELEMENTTYPE_FLOAT, 2, stride, 6 * sizeof(float) },
			{ render::SHADER_BLEND1_ATTRIBUTE, render::VERTEXELEMENTTYPE_FLOAT, 2, stride,8 * sizeof(float) },
			{ render::SHADER_BLEND2_ATTRIBUTE, render::VERTEXELEMENTTYPE_FLOAT, 2, stride, 10 * sizeof(float) },
			{ render::SHADER_BLEND3_ATTRIBUTE, render::VERTEXELEMENTTYPE_FLOAT, 2, stride, 12 * sizeof(float) },
			{ render::SHADER_BLEND4_ATTRIBUTE, render::VERTEXELEMENTTYPE_FLOAT, 2, stride, 14 * sizeof(float) },
		};

		render::VertexDescription* pVertexDes = pRenderDevice->CreateVertexDescription(7, elements);
		render::VertexBuffer* ptemp = m_pVBO.get();
		m_pVAO = std::unique_ptr<render::VertexArray>(pRenderDevice->CreateVertexArray(1, &ptemp, &pVertexDes));

		delete pVertexDes;

		m_Name = pData->Name;
	}
}