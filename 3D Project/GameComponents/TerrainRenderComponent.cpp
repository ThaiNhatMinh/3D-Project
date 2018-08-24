#include <pch.h>

const char* TerrainRenderComponent::Name = "TerrainRenderComponent";

bool TerrainRenderComponent::VInit(const tinyxml2::XMLElement* pData)
{
	m_fScale = 20.0f;
	if (!pData) return false;
	const tinyxml2::XMLElement* pModelPath = pData->FirstChildElement("Model");

	const char* pFileName = pModelPath->Attribute("File");
	if (pFileName)
	{
		HeightMap* hm = m_Context->GetSystem<Resources>()->GetHeightMap(pFileName);

		const tinyxml2::XMLElement* pTexPath = pData->FirstChildElement("Texture");
		const char* pFileName1 = pTexPath->Attribute("File0");
		//pMesh->Tex = m_Context->GetSystem<Resources>()->GetTexture(pFileName1);
		//m_MeshList.push_back(pMesh);

		GenerateMeshData(hm, m_Context->GetSystem<Resources>()->GetTexture(pFileName1));
	}

	const tinyxml2::XMLElement* pShader = pData->FirstChildElement("Shader");
	if (pShader)
	{
		m_pShader = m_Context->GetSystem<Resources>()->GetShader(pShader->Attribute("name"));
		if (m_pShader == nullptr)
			E_ERROR("Can not find shader name: " + string(pShader->Attribute("name")));
	}

	m_Material.Ks = vec3(0);
	m_Material.Ka = vec3(1.0f);
	m_Material.Kd = vec3(0.8f);
	return true;
}

void TerrainRenderComponent::Render(Scene *pScene)
{
	if (m_MeshList.empty()) return;
	m_pShader->SetupRender(pScene, m_pOwner);

	

	m_pShader->SetUniform("scale", m_fScale);

	m_pShader->SetUniform("gMaterial.Ka", m_Material.Ka);
	m_pShader->SetUniform("gMaterial.Kd", m_Material.Kd);
	m_pShader->SetUniform("gMaterial.Ks", m_Material.Ks);
	m_pShader->SetUniform("gMaterial.exp", m_Material.exp);

	ICamera* pCam = pScene->GetCurrentCamera();
	Frustum* pFrustum = pCam->GetFrustum();
	int numdraw = 0;
	//glPolygonMode(GL_FRONT, GL_LINE);
	for (size_t i = 0; i < m_MeshList.size(); i++)
	{
		SubGrid* pGrid = static_cast<SubGrid*>(m_MeshList[i]);

		if (!pFrustum->Inside(pGrid->box.Min, pGrid->box.Max)) continue;

		m_MeshList[i]->Tex->Bind();

		// ------- Render mesh ----------
		//m_pRenderer->SetVertexArrayBuffer(m_MeshList[i]->VAO);
		m_MeshList[i]->VAO.Bind();
		m_pRenderer->SetDrawMode(m_MeshList[i]->Topology);
		m_pRenderer->DrawElement(m_MeshList[i]->NumIndices, GL_UNSIGNED_INT, 0);
		numdraw++;
	}

	ImGui::Text("Num SubGrid draw: %d in total %d", numdraw, m_MeshList.size());

	//glPolygonMode(GL_FRONT, GL_FILL);
}

TerrainRenderComponent::~TerrainRenderComponent()
{
	
}

void TerrainRenderComponent::GenerateMeshData(HeightMap * hm, Texture* pText)
{
	GLuint numMesh = hm->numSub;			// Num SubMesh device by row and collum
	GLuint numvert = hm->Width / numMesh;	// Num vertices per SubMesh in Row/Collum

	GLint xpos = 0, zpos = 0;
	int pos[2] = { xpos,zpos };

	std::vector<std::vector<DefaultVertex>> vertexList;
	for (int i = 0; i < numMesh; i++)
	{
		for (int j = 0; j < numMesh; j++)
		{
			std::vector<DefaultVertex> vertex;
			vertex = Light::Math::CopySubMatrix(hm->m_Vertexs, pos, numvert);
			vertexList.push_back(vertex);
			pos[0] += numvert - 1;
		}
		pos[0] = 0;
		pos[1] += numvert - 1;
	}

	std::vector<unsigned int> Index;
	GLuint cnt = 0;
	for (GLuint i = 0; i < numvert - 1; i++)
		for (GLuint j = 0; j <numvert - 1; j++)
		{
			Index.push_back(j + (i + 1)*numvert + 1);
			Index.push_back(j + i * numvert + 1);
			Index.push_back(j + i * numvert);

			Index.push_back(j + (i + 1)*numvert);
			Index.push_back(j + (i + 1)*numvert + 1);
			Index.push_back(j + i * numvert);
		}
	for (int i = 0; i < vertexList.size(); i++)
	{
		m_MeshList.push_back(new SubGrid(vertexList[i], Index));
		m_MeshList.back()->Tex = pText;
	}
}



TerrainRenderComponent::SubGrid::SubGrid(const std::vector<DefaultVertex>& vertex, const std::vector<unsigned int> indices) :Mesh(vertex, indices), box()
{
	for (size_t i = 0; i < vertex.size(); i++) box.Test(vertex[i].pos);

}