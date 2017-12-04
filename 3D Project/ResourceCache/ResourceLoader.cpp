#include "pch.h"
#include "..\include\IL\il.h"
#include "..\include\IL\ilu.h"
#include "LTBFileLoader.h"



namespace LightEngine
{
	const char* RESOURCES_FILE = "Resources.xml";
}

//vector<Texture*> Resources::m_Textures;
//vector<ModelCache*> Resources::m_ModelCaches;

Texture * Resources::HasTexture(const char * filename)
{
	for (size_t i = 0; i < m_Textures.size(); i++)
		if (!strcmp(m_Textures[i]->szName, filename))
			return m_Textures[i].get();
	
	return nullptr;
}

ModelCache * Resources::HasModel(const char * filename)
{
	for (size_t i = 0; i < m_ModelCaches.size(); i++)
		if (!strcmp(m_ModelCaches[i]->szName, filename))
			return m_ModelCaches[i].get();

	return NULL;
}

HeightMap* Resources::HasHeighMap(const char * filename)
{
	for (size_t i = 0; i < m_HeightMaps.size(); i++)
		if (!strcmp(filename, m_HeightMaps[i]->filename)) return m_HeightMaps[i].get();
	return nullptr;
}


Resources::Resources()
{
	m_ShaderFactory.insert(std::make_pair("SkeShader", [](const char*vs, const char* fs) {return std::make_unique<SkeShader>(vs, fs); }));
	m_ShaderFactory.insert(std::make_pair("PrimShader", [](const char*vs, const char* fs) {return std::make_unique<PrimShader>(vs, fs); }));
	m_ShaderFactory.insert(std::make_pair("Debug", [](const char*vs, const char* fs) {return std::make_unique<Shader>(vs,fs); }));
	m_ShaderFactory.insert(std::make_pair("Shader", [](const char*vs, const char* fs) {return std::make_unique<Shader>(vs, fs); }));
}


Resources::~Resources()
{
	
}

void Resources::Init(Context* c)
{
	ilInit();
	ILenum Error;
	Error = ilGetError();

	if (Error != IL_NO_ERROR)
		Log::Message(Log::LOG_ERROR, "Can't init Devil Lib.");

	// Load default tex
	m_pDefaultTex =  LoadTexture("GameAssets/TEXTURE/Default.png");

	LoadResources("GameAssets/" + string(LightEngine::RESOURCES_FILE));

	c->m_pResources = std::unique_ptr<Resources>(this);
		
}

HeightMap* Resources::LoadHeightMap(const char * filename, int stepsize, int w, int h, float hscale, int sub)
{
	HeightMap* hm=nullptr;
	hm = HasHeighMap(filename);
	if (hm != nullptr) return hm;

	if (filename == nullptr) return nullptr;

	GLint width, height, iType, iBpp;

	string fullpath = m_Path + filename;
	ilLoadImage(fullpath.c_str());

	ILenum Error;
	Error = ilGetError();

	if (Error != IL_NO_ERROR)
	{
		Log::Message(Log::LOG_ERROR, "Can't load terrain " + string(filename));
		return  nullptr;
	}
	width = ilGetInteger(IL_IMAGE_WIDTH);
	height = ilGetInteger(IL_IMAGE_HEIGHT);
	iType = ilGetInteger(IL_IMAGE_FORMAT);
	ILubyte *Data = ilGetData();
	iBpp = ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL);


	if(w!=-1) width = w;
	if(h!=-1) height = h;

	
	//vec2 size = vec2(width*stepsize, height*stepsize);
	
	GLubyte* pRawData = new GLubyte[width*height];
	int c = 0;
	float min = 100000000;
	float max = -min;
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			int b = i*width*iBpp + j*iBpp;


			pRawData[c] = (Data[b] + Data[b + 1] + Data[b + 2]) / 3.0;
			//pRawData[c] = 10;
			if (min > pRawData[c]) min = pRawData[c];
			if (max < pRawData[c]) max = pRawData[c];
			c++;
		}
	}
	

	vec2 size = vec2((width - 1)*stepsize, (height - 1)*stepsize);
	vec2 size2 = vec2((width - 0)*stepsize, (height - 0)*stepsize);
	Mesh* p = new Mesh;

	int x = -size[0] / 2, y = 0, z = -size[1] / 2;
	float t = (min + max) / 2.0f;

	auto heightF = [pRawData, width, height,t,hscale](int x, int z) {
		if (x < 0) x = 0;
		if (z < 0) z = 0;
		if (x >= width) x = width;
		if (z >= height) z = height;
		int b = (int)(z*width + x);
		return (pRawData[b] - t)*hscale;
	};

	// computer vertex xyz
	for (int i = 0; i < height; i++) // z axis
	{

		for (int j = 0; j < width; j++) // x axis
		{
			int b = i*width + j;

			y = (pRawData[b] - t)*hscale;
			vec3 pos(x, y, z);
			vec2 uv((x + size[0] / 2) / size[0], (z + size[1] / 2) / size[1]);
			//computer normal;

			vec2 P(i, j);
			float hL = heightF(j - 1, i);
			float hR = heightF(j + 1, i);
			float hD = heightF(j, i - 1);
			float hU = heightF(j, i + 1);
			vec3 N(0, 1, 0);
			N.x = hL - hR;
			N.y = 2.0f;
			N.z = hD - hU;
			N = normalize(N);
			

			
			DefaultVertex vertex{ pos,N,uv };
			p->m_Vertexs.push_back(vertex);
			x += stepsize;
		}
		x = -size[0] / 2;
		z += stepsize;
	}
	// computer indices
	GLuint cnt = 0;
	for (int i = 0; i < height - 1; i++)
		for (int j = 0; j <width - 1; j++)
		{
			p->m_Indices.push_back(j + (i + 1)*width + 1);
			p->m_Indices.push_back(j + i*width + 1);
			p->m_Indices.push_back(j + i*width);

			p->m_Indices.push_back(j + (i + 1)*width);
			p->m_Indices.push_back(j + (i + 1)*width + 1);
			p->m_Indices.push_back(j + i*width);
		}

	// comput
		
	ilResetMemory();
	hm = new HeightMap;
	hm->Data = pRawData;
	hm->Width = width;
	hm->Height = height;
	hm->stepsize = stepsize;
	hm->maxH = max;
	hm->minH = min;
	hm->hscale = hscale;
	strcpy(hm->filename, filename);
	// [TODO]- Devide large mesh into small mesh

	// Generate Buffer Objet
	p->Init();

	hm->m_Mesh.push_back(std::unique_ptr<IMesh>(p));

	m_HeightMaps.push_back(std::unique_ptr<HeightMap>(hm));

	return hm;
}

Texture * Resources::LoadTexture(const char * filename)
{
	Texture* tex=nullptr;
	if ((tex = HasTexture(filename)) != nullptr) return tex;

	GLint width, height, iType, iBpp;

	string fullpath = m_Path + filename;
	ilLoadImage(fullpath.c_str());
	ILenum Error;
	Error = ilGetError();

	if (Error != IL_NO_ERROR)
	{
		//string error = iluErrorString(Error);
		Log::Message(Log::LOG_ERROR, "Can't load texture " + string(filename));
		//Log::Message(Log::LOG_ERROR, "Devil: " + error);
		return HasTexture("GameAssets/TEXTURE/Default.png");
	}
	
	width = ilGetInteger(IL_IMAGE_WIDTH);
	height = ilGetInteger(IL_IMAGE_HEIGHT);
	iType = ilGetInteger(IL_IMAGE_FORMAT);
	ILubyte *Data = ilGetData();
	iBpp = ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL);

	tex = new Texture;
	tex->Init();

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, iBpp, width, height, 0, iType, GL_UNSIGNED_BYTE, Data);
	glBindTexture(GL_TEXTURE_2D, 0);
	glGenerateMipmap(GL_TEXTURE_2D);

	ilResetMemory();
	
	
	sprintf(tex->szName, "%s", filename);
	tex->iWidth = width;
	tex->iHeight = height;

	m_Textures.push_back(std::unique_ptr<Texture>(tex));


	return tex;
}

Texture * Resources::LoadCubeTex(const vector<string>& filelist)
{
	Texture* tex = NULL;
	GLuint id;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_CUBE_MAP, id);
	GLint width, height, iType, iBpp;
	for (size_t i = 0; i < filelist.size(); i++)
	{
		string fullpath = m_Path + filelist[i];
		ilLoadImage(fullpath.c_str());
		ILenum Error;
		Error = ilGetError();

		if (Error != IL_NO_ERROR)
		{
			//string error = iluErrorString(Error);
			Log::Message(Log::LOG_ERROR, "Can't load texture " + filelist[i]);
			//Log::Message(Log::LOG_ERROR, "Devil: " + error);
			return HasTexture("GameAssets/TEXTURE/Default.png");
		}

		width = ilGetInteger(IL_IMAGE_WIDTH);
		height = ilGetInteger(IL_IMAGE_HEIGHT);
		iType = ilGetInteger(IL_IMAGE_FORMAT);
		ILubyte *Data = ilGetData();
		iBpp = ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL);

		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X +i, 0, iBpp, width, height, 0, iType, GL_UNSIGNED_BYTE, Data);
		
	}
	ilResetMemory();
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	//glGenerateMipmap(GL_TEXTURE_2D);

	

	tex = new Texture;
	sprintf(tex->szName, "%s", filelist[0].c_str());
	tex->iIndex = id;
	tex->iWidth = width;
	tex->iHeight = height;

	m_Textures.push_back(std::unique_ptr<Texture>(tex));


	return tex;

}

Texture * Resources::LoadTexMemory(const char* filename,unsigned char * data, int w, int h)
{
	Texture* tex = NULL;
	if ((tex = HasTexture(filename)) != NULL) return tex;

	GLuint id;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	glBindTexture(GL_TEXTURE_2D, 0);

	tex = new Texture;
	sprintf(tex->szName, "%s", filename);
	tex->iIndex = id;
	tex->iWidth = w;
	tex->iHeight = h;

	m_Textures.push_back(std::unique_ptr<Texture>(tex));


	return tex;

}

Texture * Resources::LoadDTX(const char * filename)
{

	Texture* tex = NULL;
	if ((tex = HasTexture(filename)) != NULL) return tex;

	string fullpath = m_Path + filename;

	FILE* pFile = fopen(fullpath.c_str(), "rb");
	if (!pFile)
	{
		Log::Message(Log::LOG_ERROR, "Can't open file: " + string(filename));
		
		return tex;
	}
	DtxHeader Header;
	memset(&Header, 0, sizeof(DtxHeader));
	fread(&Header, sizeof(DtxHeader), 1, pFile);
	if (Header.iResType != 0 || Header.iVersion != -5 || Header.usMipmaps == 0)
	{
		fclose(pFile);
		return tex;
	}

	int W, H;

	W = Header.usWidth;
	H = Header.usHeight;
	int iBpp = Header.ubExtra[2];
	int iSize;
	int InternalFormat;
	if (iBpp == 3)
	{
		iSize = Header.usWidth * Header.usHeight * 4;
		InternalFormat = GL_RGBA;
	}
	else if (iBpp == 4)
	{
		iSize = (Header.usWidth * Header.usHeight) >> 1;
		InternalFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
	}
	else if (iBpp == 5)
	{
		iSize = Header.usWidth * Header.usHeight;
		InternalFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
	}
	else if (iBpp == 6)
	{
		iSize = Header.usWidth * Header.usHeight;
		InternalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
	}
	else
	{
		iSize = 0;
	}

	int ImageSize = iSize;

	int iBufferSize = 1024 * 1024 * 4;
	if (iSize == 0 || iSize > iBufferSize)
	{
		fclose(pFile);
		return tex;
	}

	unsigned char* ubBuffer = new unsigned char[1024 * 1024 * 4];

	fread(ubBuffer, iSize, 1, pFile);

	if (iBpp == 3)
	{
		for (int i = 0; i < iSize; i += 4)
		{
			ubBuffer[i + 0] ^= ubBuffer[i + 2];
			ubBuffer[i + 2] ^= ubBuffer[i + 0];
			ubBuffer[i + 0] ^= ubBuffer[i + 2];
		}
	}

	GLuint id;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_2D, id);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	

	if (InternalFormat == GL_RGBA)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, W, H, 0, GL_RGBA, GL_UNSIGNED_BYTE, ubBuffer);
	}
	else if (InternalFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT || InternalFormat == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT || InternalFormat == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
	{
		glCompressedTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, W, H, 0, iSize, ubBuffer);
	}


	glBindTexture(GL_TEXTURE_2D, 0);



	tex = new Texture;
	sprintf(tex->szName, "%s", filename);
	tex->iIndex = id;
	tex->iWidth = W;
	tex->iHeight = H;
	
	m_Textures.push_back(std::unique_ptr<Texture>(tex));

	fclose(pFile);
	delete[] ubBuffer;
	return tex;

}

byte * Resources::LoadHeightMap(const char * filename, int& w, int& h)
{
	
	ilLoadImage(filename);
	ILenum Error;
	Error = ilGetError();

	if (Error != IL_NO_ERROR)
	{
		//string error = iluErrorString(Error);
		Log::Message(Log::LOG_ERROR, "Can't load texture " + string(filename));
		//Log::Message(Log::LOG_ERROR, "Devil: " + error);
		return NULL;
	}

	w = ilGetInteger(IL_IMAGE_WIDTH);
	h = ilGetInteger(IL_IMAGE_HEIGHT);
	
	ILubyte *Data = ilGetData();
	
	return Data;

}

ModelCache * Resources::LoadModel(const char * filename)
{
	ModelCache* pModel = nullptr;
	if ((pModel = HasModel(filename)) != nullptr) return pModel;

	string fullpath = m_Path + filename;

	pModel = new ModelCache;
	if (LTBFile::BeginLoad(fullpath.c_str()))
	{
		pModel->Prop = LTBFile::LoadProp();
		pModel->pMeshs = LTBFile::LoadMesh();
		pModel->pSkeNodes = LTBFile::LoadSkeleton();
		pModel->wb = LTBFile::LoadWS();
		pModel->ChildName = LTBFile::LoadChildName();
		pModel->pAnims = LTBFile::LoadAnimation(pModel->pSkeNodes);
		pModel->Sockets = LTBFile::LoadSocket();
		LTBFile::EndLoad();

	}
	else
	{
		E_ERROR("Can't open file: " + string(filename));
		return NULL;
	}

	strcpy(pModel->szName, filename);
	vector<AABB> abb(pModel->pSkeNodes.size());
	for (size_t i = 0; i < pModel->pMeshs.size(); i++)
	{
		SkeMesh* pMesh = pModel->pMeshs[i].get();
		// Generate Buffer Object
		pMesh->Init();

		for (size_t j = 0; j < pMesh->m_Vertexs.size(); j++)
		{
			SkeVertex& vertex = pMesh->m_Vertexs[j];
			for (int k = 0; k < 4; k++)
			{
				if (vertex.weights[k].Bone <100.0f && vertex.weights[k].weight>=0.01)
				{
					vec3 local = pModel->pSkeNodes[vertex.weights[k].Bone]->m_InvBindPose*vec4(vertex.pos, 1.0f);
					local *= vertex.weights[k].weight;
					abb[vertex.weights[k].Bone].Insert(local);
					pModel->pSkeNodes[vertex.weights[k].Bone]->m_Flag = 1;
				}
				else break;
			}
		}
	}

	for (size_t i = 0; i < pModel->pSkeNodes.size(); i++)
	{
		
		pModel->pSkeNodes[i]->m_BoundBox = abb[i];
	}

	m_ModelCaches.push_back(std::unique_ptr<ModelCache>(pModel));
	return pModel;
}

ModelCache * Resources::LoadModelXML(const char * XMLFile)
{
	string fullpath = m_Path + XMLFile;
	tinyxml2::XMLDocument doc;
	int errorID = doc.LoadFile(fullpath.c_str());
	if (errorID)
	{
		E_ERROR("Failed to load file: " + string(XMLFile));
		return nullptr;
	}
	tinyxml2::XMLElement* pData = doc.FirstChildElement("Data");

	// load model
	tinyxml2::XMLElement* pModelNode = pData->FirstChildElement("Model");
	const char* pFileName = pModelNode->Attribute("File");

	ModelCache* pModel = LoadModel(pFileName);
	if (pModel)
	{
		// load texture
		tinyxml2::XMLElement* pTextureNode = pData->FirstChildElement("Texture");
		vector<std::unique_ptr<SkeMesh>>& ve = pModel->pMeshs;
		for (size_t i = 0; i < ve.size(); i++)
		{
			tinyxml2::XMLElement* pTexture = pTextureNode->FirstChildElement(ve[i]->Name.c_str());
			const char* pTextureFile = pTexture->Attribute("File");
			ve[i]->Tex = LoadDTX(pTextureFile);
		}
	}		

	

	// load material
	tinyxml2::XMLElement* pMaterialData = pData->FirstChildElement("Material");
	Material mat;
	tinyxml2::XMLElement* pKa = pMaterialData->FirstChildElement("Ka");
	mat.Ka.x = pKa->FloatAttribute("r", 1.0f);
	mat.Ka.y = pKa->FloatAttribute("g", 1.0f);
	mat.Ka.z = pKa->FloatAttribute("b", 1.0f);
	tinyxml2::XMLElement* pKd = pMaterialData->FirstChildElement("Kd");
	mat.Kd.x = pKd->FloatAttribute("r", 1.0f);
	mat.Kd.y = pKd->FloatAttribute("g", 1.0f);
	mat.Kd.z = pKd->FloatAttribute("b", 1.0f);
	tinyxml2::XMLElement* pKs = pMaterialData->FirstChildElement("Ks");
	mat.Ks.x = pKs->FloatAttribute("r", 1.0f);
	mat.Ks.y = pKs->FloatAttribute("g", 1.0f);
	mat.Ks.z = pKs->FloatAttribute("b", 1.0f);
	mat.exp = vec3(pKs->FloatAttribute("exp", 32.0f));


	// Done return ModelCache


	return pModel;
}

Shader * Resources::LoadShader(string key, const char * vs, const char* fs, bool linkshader)
{
	auto pos = m_ShaderList.find(key);
	if (pos != m_ShaderList.end()) return pos->second.get();

	auto func = m_ShaderFactory.find(key);
	if (func == m_ShaderFactory.end()) return nullptr;
	string fullPathvs = m_Path + vs;
	string fullPathfs = m_Path + fs;
	std::unique_ptr<Shader> p = func->second(fullPathvs.c_str(), fullPathfs.c_str());
	Shader* result = p.get();

	if (linkshader) p->LinkShader();

	m_ShaderList.insert({ key, std::move(p) });

	return result;
}

Shader * Resources::GetShader(string key)
{
	return m_ShaderList[key].get();
}

Texture * Resources::GetTexture(const char * filename)
{
	Texture* tex = nullptr;
	tex = HasTexture(filename);
	if (tex == nullptr) return m_pDefaultTex;
	return tex;
	
}

ModelCache * Resources::GetModel(const char * filename)
{
	ModelCache* pModel = nullptr;
	if (strstr(filename, ".xml") != nullptr) pModel = LoadModelXML(filename);
	else pModel = HasModel(filename);
	return pModel;
}

HeightMap * Resources::GetHeightMap(const char * filename)
{
	HeightMap* hm = nullptr;
	hm = HasHeighMap(filename);
	return hm;
}

IMesh * Resources::CreateShape(ShapeType type)
{

	IMesh* pBox = new CubeMesh();
	pBox->Name = ShapeName[type];
	m_PrimList.push_back(pBox);

	return pBox;
}

void Resources::LoadResources(string path)
{
	tinyxml2::XMLDocument doc;
	doc.LoadFile(path.c_str());
	tinyxml2::XMLElement* p = doc.FirstChildElement("Resources");
	m_Path = p->Attribute("Path");
	// Loop through each child element and load the component
	for (tinyxml2::XMLElement* pNode = p->FirstChildElement(); pNode; pNode = pNode->NextSiblingElement())
	{
		const char* name = pNode->Value();
		if (name == nullptr) continue;
		
		if (!strcmp(name, "Texture"))
		{
			const char* pFile = pNode->Attribute("File");
			if (pFile == nullptr) LoadTexture("TEXTURE//Default.png");
			else LoadTexture(pFile);
		}
		else if (!strcmp(name, "Shader"))
		{
			const char* pTag = pNode->Attribute("Name");
			const char* pFileVS = pNode->Attribute("FileVS");
			const char* pFileFS = pNode->Attribute("FileFS");
			if (!pTag || !pFileVS || !pFileFS) continue;
			LoadShader(pTag, pFileVS, pFileFS);
		}
		else if (!strcmp(name, "ModelXML"))
		{
			const char* pFile = pNode->Attribute("File");
			if (pFile) LoadModelXML(pFile);
		}
		else if (!strcmp(name, "Model"))
		{
			const char* pFile = pNode->Attribute("File");
			if (pFile) LoadModel(pFile);
		}
		else if (!strcmp(name, "HeightMap"))
		{
			const char* pFile = pNode->Attribute("File");
			int size = pNode->DoubleAttribute("Size", 5.0f);
			int w = pNode->DoubleAttribute("Width", 2.0f);
			int h = pNode->DoubleAttribute("Height", 2.0f);
			float hscale = pNode->DoubleAttribute("HeightScale", 1.0);
			int s = pNode->DoubleAttribute("SubDevided", 1.0);
			if (pFile) LoadHeightMap(pFile,size,w,h,hscale,s);
		}
	}
}

void Resources::ShutDown()
{
	for (size_t i = 0; i < m_Textures.size(); i++)
	{
		m_Textures[i]->Shutdown();
	}

	for (size_t i = 0; i < m_ModelCaches.size(); i++)
	{
		
		for (size_t j = 0; j < m_ModelCaches[i]->pMeshs.size(); j++)
		{
			m_ModelCaches[i]->pMeshs[j]->Shutdown();
		}
	}

	for (size_t i = 0; i < m_ShaderList.size(); i++)
	{
		m_ShaderList[i]->Shutdown();
	}

	for (size_t i = 0; i < m_HeightMaps.size(); i++)
	{
		for (size_t j = 0; j < m_HeightMaps[i]->m_Mesh.size(); j++)
		{
			m_HeightMaps[i]->m_Mesh[i]->Shutdown();
		}
	}
}

