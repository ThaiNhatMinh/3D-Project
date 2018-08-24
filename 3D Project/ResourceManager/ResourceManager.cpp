#include <pch.h>
#include <IL/il.h>
#include "LTBFileLoader.h"
#include <fmod_errors.h>
#include <assimp\Importer.hpp>
#include <assimp\scene.h>
#include <assimp\postprocess.h>
#include "ResourceManager.h"

#include "..\Graphics3D\OpenGL\OpenGLTexture.h"
#include "..\Core\OpenGLWindows.h"
#include "..\Graphics3D\DefaultMesh.h"
namespace Light
{
	namespace resources
	{
		struct DtxHeader
		{
			unsigned int iResType;
			int iVersion;
			unsigned short usWidth;
			unsigned short usHeight;
			unsigned short usMipmaps;
			unsigned short usSections;
			int iFlags;
			int iUserFlags;
			unsigned char ubExtra[12];
			char szCommandString[128];
		};

		ResourceManager::ResourceManager(IContext* c) :m_pContext(c)
		{
			ilInit();
			ILenum Error;
			Error = ilGetError();

			if (Error != IL_NO_ERROR)
				E_ERROR("Can't init Devil Lib.");

			LoadSystemResources();
			CheckResourceFunc a = [](const std::string&a, const std::string& b)
			{
				return (a.find(b) != string::npos);
			};
			// Load default tex
			m_pDefaultTex = HasResource(m_Textures, "Default.png",a );

			//LoadResources("GameAssets/" + string(Light::RESOURCES_FILE));

			c->VAddSystem(this);
			m_pRenderDevice = c->GetSystem<render::RenderDevice>();
			m_pFactory = c->GetSystem<IFactory>();
			//m_FMOD = c->GetSystem<SoundEngine>()->GetFMODSystem();

		}


		ResourceManager::~ResourceManager()
		{
		
		}

		const char * ResourceManager::VGetName()
		{
			static const char * pName = typeid(IResourceManager).name();
			return pName;
		}



		SpriteData * ResourceManager::LoadSpriteAnimation(const string& filename)
		{
			

			SpriteData* s = nullptr;
			s = HasResource(m_Sprites,filename);
			if (s) return s;

			if (filename.size() == 0) return nullptr;

			

			FILE* pFile = fopen(filename.c_str(), "rb");
			if (!pFile)
			{
				E_ERROR("Can't load sprite: %s",filename.c_str());
				return nullptr;
			}

			uint32 nFrames, nFrameRate, bTransparent, bTranslucent, colourKey;
			char filetex[1024];
			uint16 strLen;
			fread(&nFrames, sizeof(uint32), 1, pFile);
			fread(&nFrameRate, sizeof(uint32), 1, pFile);
			fread(&bTransparent, sizeof(uint32), 1, pFile);
			fread(&bTranslucent, sizeof(uint32), 1, pFile);
			fread(&colourKey, sizeof(uint32), 1, pFile);

			s = new SpriteData;

			//s->m_FrameLists.resize(nFrames);
			s->m_MsFrameRate = nFrameRate;
			s->m_MsAnimLength = (1000 / nFrameRate) * nFrames;

			s->m_bKeyed = (uint8)bTransparent;
			s->m_bTranslucent = (uint8)bTranslucent;
			s->m_ColourKey = colourKey;

			for (size_t i = 0; i < nFrames; i++)
			{
				// Read in frame file name
				fread(&strLen, sizeof(strLen), 1, pFile);
				if (strLen > 1000)
				{
					delete s;
					return nullptr;
				}

				fread(filetex, strLen, 1, pFile);
				filetex[strLen] = 0;

				for (int j = 0; j < strLen; j++)
					if (filetex[j] > 'a' && filetex[j] < 'z')
						filetex[j] += 'A' - 'a';

				std::string fullfile = "TEXTURES\\";
				fullfile += filetex;
				s->m_FrameLists.push_back(fullfile);
				LoadTexture(fullfile);

			}

			m_Sprites.push_back(ResourceHandle<SpriteData>(filename,s));
			return s;
		}

		HeightMap* ResourceManager::LoadHeightMap(const string& filename, int stepsize, int w, int h, float hscale, int sub)
		{
			HeightMap* hm = nullptr;
			hm = HasResource(m_HeightMaps,filename);
			if (hm != nullptr) return hm;

			GLint width, height, iType, iBpp;

			
			ilLoadImage(filename.c_str());

			ILenum Error;
			Error = ilGetError();

			if (Error != IL_NO_ERROR)
			{
				E_ERROR("Can't load heightmap %",filename.c_str());
				return  nullptr;
			}
			width = ilGetInteger(IL_IMAGE_WIDTH);
			height = ilGetInteger(IL_IMAGE_HEIGHT);
			iType = ilGetInteger(IL_IMAGE_FORMAT);
			ILubyte *Data = ilGetData();
			iBpp = ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL);


			if (w != -1) width = w;
			if (h != -1) height = h;


			//vec2 size = vec2(width*stepsize, height*stepsize);

			GLubyte* pRawData = new GLubyte[width*height];
			int c = 0;
			float min = 100000000;
			float max = -min;
			for (int i = 0; i < height; i++)
			{
				for (int j = 0; j < width; j++)
				{
					int b = i * width*iBpp + j * iBpp;


					pRawData[c] = (Data[b] + Data[b + 1] + Data[b + 2]) / 3.0;
					//pRawData[c] = 10;
					if (min > pRawData[c]) min = pRawData[c];
					if (max < pRawData[c]) max = pRawData[c];
					c++;
				}
			}


			vec2 size = vec2((width - 1)*stepsize, (height - 1)*stepsize);
			vec2 size2 = vec2((width - 0)*stepsize, (height - 0)*stepsize);
			//Mesh* p = new Mesh;

			std::vector<DefaultVertex> vertex;
			std::vector<unsigned int> Index;

			int x = -size[0] / 2, y = 0, z = -size[1] / 2;
			float t = (min + max) / 2.0f;

			auto heightF = [pRawData, width, height, t, hscale](int x, int z) {
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
					int b = i * width + j;

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



					DefaultVertex v{ pos,N,uv };
					vertex.push_back(v);
					x += stepsize;
				}
				x = -size[0] / 2;
				z += stepsize;
			}
			// computer indices
			GLuint cnt = 0;
			for (int i = 0; i < height - 1; i++)
				for (int j = 0; j < width - 1; j++)
				{
					Index.push_back(j + (i + 1)*width + 1);
					Index.push_back(j + i * width + 1);
					Index.push_back(j + i * width);

					Index.push_back(j + (i + 1)*width);
					Index.push_back(j + (i + 1)*width + 1);
					Index.push_back(j + i * width);
				}

			// comput

			ilResetMemory();
			hm = new HeightMap();
			hm->Data = std::unique_ptr<GLubyte[]>(pRawData);
			hm->Width = width;
			hm->Height = height;
			hm->stepsize = stepsize;
			hm->maxH = max;
			hm->minH = min;
			hm->hscale = hscale;
			hm->numSub = sub;
			// [TODO]- Devide large mesh into small mesh

			hm->m_Vertexs = vertex;
			hm->m_Indices = Index;
			m_HeightMaps.push_back(ResourceHandle<HeightMap>(filename,hm));

			return hm;
		}

		render::Texture * ResourceManager::LoadTexture(const string& filename)
		{
			render::Texture* tex = nullptr;
			if ((tex = HasResource(m_Textures,filename)) != nullptr) return tex;

			if (filename.find(".DTX") != string::npos) return LoadDTX(filename);
			GLint width, height, iType, iBpp;

			
			ilLoadImage(filename.c_str());
			ILenum Error;
			Error = ilGetError();

			if (Error != IL_NO_ERROR)
			{
				//string error = iluErrorString(Error);
				E_ERROR("Can't load texture: %s",filename.c_str());
				
				//Log::Message(Log::LOG_ERROR, "Devil: " + error);
				return m_pDefaultTex;//HasTexture("GameAssets/TEXTURE/Default.png");
			}

			width = ilGetInteger(IL_IMAGE_WIDTH);
			height = ilGetInteger(IL_IMAGE_HEIGHT);
			iType = ilGetInteger(IL_IMAGE_FORMAT);
			ILubyte *Data = ilGetData();
			iBpp = ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL);

				
			render::TextureCreateInfo texinfo;
			texinfo.eTarget = GL_TEXTURE_2D;
			texinfo.iLevel = 0;
			texinfo.iInternalFormat = iBpp==3?GL_RGB:GL_RGBA;
			texinfo.uiWidth = width;
			texinfo.uiHeight = height;
			texinfo.pData = Data;
			texinfo.eType = GL_UNSIGNED_BYTE;
			texinfo.eFormat = texinfo.iInternalFormat;
			
			tex = new render::OpenGLTexture(texinfo);


			ilResetMemory();



			m_Textures.push_back(ResourceHandle<render::Texture>(filename,tex));


			return tex;
		}

		render::Texture * ResourceManager::LoadCubeTex(const vector<string>& filelist)
		{
			render::Texture* tex = nullptr;
			unsigned char* pData[6];
			GLint width, height, iType, iBpp;
			for (size_t i = 0; i < filelist.size(); i++)
			{
				
				ilLoadImage(filelist[i].c_str());
				ILenum Error;
				Error = ilGetError();

				if (Error != IL_NO_ERROR)
				{
					E_ERROR("Can't load texture: %s",filelist[i].c_str());
					return nullptr;
				}

				width = ilGetInteger(IL_IMAGE_WIDTH);
				height = ilGetInteger(IL_IMAGE_HEIGHT);
				iType = ilGetInteger(IL_IMAGE_FORMAT);
				ILubyte *Data = ilGetData();
				iBpp = ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL);

				pData[i] = new unsigned char[width*height*iBpp];
				memcpy(pData[i], Data, width*height*iBpp);

			}


			render::TextureCreateInfo texinfo;
			texinfo.eTarget = GL_TEXTURE_CUBE_MAP;
			texinfo.iLevel = 0;
			texinfo.iInternalFormat = iBpp == 3 ? GL_RGB : GL_RGBA;
			texinfo.uiWidth = width;
			texinfo.uiHeight = height;
			texinfo.pData = pData;
			texinfo.eType = GL_UNSIGNED_BYTE;
			texinfo.eFormat = texinfo.iInternalFormat;


			tex = m_pRenderDevice->CreateTexture(texinfo);
			m_Textures.push_back(ResourceHandle<render::Texture>(filelist[0],tex));

			ilResetMemory();

			for (int i = 0; i < 6; i++) delete[] pData[i];

			return tex;

		}

		//Texture * ResourceManager::LoadTexMemory(const string& filename, unsigned char * data, int w, int h)
		//{
		//	Texture* tex = NULL;
		//	if ((tex = HasTexture(filename)) != NULL) return tex;

		//	//GLuint id;
		//	//glGenTextures(1, &id);
		//	//glBindTexture(GL_TEXTURE_2D, id);
		//	//glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		//	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		//	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		//	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		//	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		//	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		//	//glBindTexture(GL_TEXTURE_2D, 0);
		//	E_ERROR("ResourceManager::LoadTexMemory");
		//	//tex = new Texture(w,h,24,data);

		//	//m_Textures.push_back(std::unique_ptr<Texture>(tex));


		//	return tex;

		//}

		render::Texture * ResourceManager::LoadDTX(const string& filename)
		{

			render::Texture* tex = nullptr;
			if ((tex = HasResource(m_Textures,filename)) != nullptr) return tex;

			

			FILE* pFile = fopen(filename.c_str(), "rb");
			if (!pFile)
			{
				E_ERROR("Can't open file: %s",filename.c_str());

				return m_pDefaultTex;
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

	

			if (InternalFormat == GL_RGBA)
			{
				//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, W, H, 0, GL_RGBA, GL_UNSIGNED_BYTE, ubBuffer);
				//tex = new Texture(W, H,32,ubBuffer);
				render::TextureCreateInfo texinfo;
				texinfo.eTarget = GL_TEXTURE_2D;
				texinfo.pData = ubBuffer;
				texinfo.eType = GL_UNSIGNED_BYTE;
				texinfo.uiWidth = W;
				texinfo.uiHeight = H;
				texinfo.iInternalFormat = InternalFormat;
				texinfo.eFormat = GL_RGBA;
				tex = m_pRenderDevice->CreateTexture(texinfo);
			}
			else if (InternalFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT || InternalFormat == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT || InternalFormat == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT)
			{
				render::TextureCreateInfo texinfo;
				texinfo.pData = ubBuffer;
				texinfo.uiWidth = W;
				texinfo.uiHeight = H;
				texinfo.iInternalFormat = InternalFormat;
				texinfo.iLevel = iSize;
				tex = m_pRenderDevice->CreateTexture(texinfo,true);
			}


			//glBindTexture(GL_TEXTURE_2D, 0);



			//tex = new Texture(id,filename,W,H);

			m_Textures.push_back(ResourceHandle<render::Texture>(filename,tex));

			fclose(pFile);
			delete[] ubBuffer;
			return tex;

		}


		render::Model * ResourceManager::LoadModel(const string& filename)
		{
			render::Model* pModel = nullptr;


			m_ModelCaches.push_back(ResourceHandle<render::Model>(filename, pModel));
			return pModel;
		}

		render::Model * ResourceManager::LoadModelXML(const string& XMLFile)
		{
			tinyxml2::XMLDocument doc;
			LTModel* pModelRender = nullptr;

			int errorID = doc.LoadFile(XMLFile.c_str());
			if (errorID)
			{
				E_ERROR("Failed to load XML file: %s",XMLFile);
				return nullptr;
			}
			tinyxml2::XMLElement* pData = doc.FirstChildElement();

			if (strcmp(pData->Value(), "Data") == 0)
			{
				// load model
				tinyxml2::XMLElement* pModelNode = pData->FirstChildElement("Model");
				const char* pFileName = pModelNode->Attribute("File");

				LTRawData* pLTBData = LoadLTBModel(pFileName);
				if (pLTBData)
				{

					pModelRender = new LTModel;
				
					tinyxml2::XMLElement* pTextureNode = pData->FirstChildElement("Texture");
					vector<LTRawMesh>& ve = pLTBData->Meshs;
					for (size_t i = 0; i < ve.size(); i++)
					{
						pModelRender->m_pMesh.push_back(std::unique_ptr<Mesh>(new SkeMesh(m_pRenderDevice, &ve[i])));

						tinyxml2::XMLElement* pTextureElement = pTextureNode->FirstChildElement(ve[i].Name.c_str());
						if (pTextureElement)
						{
							const char* pTextureFile = pTextureElement->Attribute("File");
							pModelRender->m_Textures.push_back(HasResource(m_Textures, pTextureFile));
							const char* pMaterialFile = pTextureElement->Attribute("Material");
							
						}
						else pModelRender->m_Textures.push_back(m_pDefaultTex);

						
					}
				}



				//// load material
				//tinyxml2::XMLElement* pMaterialData = pData->FirstChildElement("Material");
				//render::Material* mat;
				//tinyxml2::XMLElement* pKa = pMaterialData->FirstChildElement("Ka");
				//mat.Ka.x = pKa->FloatAttribute("r", 1.0f);
				//mat.Ka.y = pKa->FloatAttribute("g", 1.0f);
				//mat.Ka.z = pKa->FloatAttribute("b", 1.0f);
				//tinyxml2::XMLElement* pKd = pMaterialData->FirstChildElement("Kd");
				//mat.Kd.x = pKd->FloatAttribute("r", 1.0f);
				//mat.Kd.y = pKd->FloatAttribute("g", 1.0f);
				//mat.Kd.z = pKd->FloatAttribute("b", 1.0f);
				//tinyxml2::XMLElement* pKs = pMaterialData->FirstChildElement("Ks");
				//mat.Ks.x = pKs->FloatAttribute("r", 1.0f);
				//mat.Ks.y = pKs->FloatAttribute("g", 1.0f);
				//mat.Ks.z = pKs->FloatAttribute("b", 1.0f);
				//mat.exp = vec3(pKs->FloatAttribute("exp", 32.0f));

				//pModelRender->m_Material.resize(pModelRender->m_pMesh.size(), mat);
				// Done return LTModel
				return pModelRender;
			}
			else if (strcmp(pData->Value(), "PVModel") == 0)
			{
				return nullptr;
			}

			return nullptr;
		}

		//ResourceManager::SoundRAAI * ResourceManager::LoadSound(const string & filename, const string& tag, int mode)
		//{
		//	SoundRAAI* pSound = nullptr;
		//	if ((pSound = HasSound(tag)))
		//	{
		//		E_ERROR("Sound: " + tag + "has been exits.");
		//		return nullptr;
		//	}

		//	FMOD::Sound* pFMODSound = nullptr;
		//	FMOD::System* pSystem = m_pContext->GetSystem<SoundEngine>()->GetFMODSystem();
		//	FMOD_RESULT result;

		//	string fullpath = m_Path + filename;
		//	if ((result = pSystem->createSound(fullpath.c_str(), mode, 0, &pFMODSound)) != FMOD_OK)
		//	{
		//		E_ERROR("Can't create sound: " + fullpath);
		//		printf("FMOD error! (%d) %s\n", result, FMOD_ErrorString(result));
		//		return nullptr;
		//	}
		//	pFMODSound->set3DMinMaxDistance(0.5f, 10000.0f);
		//	pSound = new SoundRAAI(pFMODSound);
		//	pSound->FilePath = filename;

		//	return pSound;
		//}

		//Shader * ResourceManager::LoadShader(string key, const char* type, const char * vs, const char* fs, bool linkshader)
		//{
		//	auto pos = m_ShaderList.find(key);
		//	if (pos != m_ShaderList.end()) return pos->second.get();


		//	string fullPathvs = m_Path + vs;
		//	string fullPathfs = m_Path + fs;
		//	std::unique_ptr<Shader> p(m_pContext->GetSystem<ActorFactory>()->VCreateShader(type, fullPathvs.c_str(), fullPathfs.c_str()));
		//	Shader* result = p.get();

		//	if (linkshader) p->LinkShader();

		//	m_ShaderList.insert({ key, std::move(p) });

		//	return result;
		//}
		//
		ObjModel * ResourceManager::LoadObjModel(const std::string filename)
		{
			
			string localPath;
			int pos = filename.find_last_of("/\\");
			localPath = filename.substr(0, pos) + '\\';
			Assimp::Importer importer;
			const aiScene* scene = importer.ReadFile(filename, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);
			
			if (!scene)
			{
				E_ERROR("Assimp: %s ",importer.GetErrorString());
				getchar();
				return false;
			}
			ObjModel* pModel = new ObjModel;
		
			std::vector<DefaultVertex> vertexs;
			std::vector<unsigned int> Indices;
			for (size_t i = 0; i < scene->mNumMeshes; i++)
			{
				vertexs.clear();
				Indices.clear();
		
				const aiMesh* mesh = scene->mMeshes[i];
				
				for (size_t j = 0; j < mesh->mNumVertices;j++)
				{
					aiVector3D pos = mesh->mVertices[j];
					aiVector3D UVW = mesh->mTextureCoords[0][j];
					aiVector3D n = mesh->mNormals[j];
					DefaultVertex dv;
					dv.pos = vec3(pos.x, pos.y, pos.z);
					dv.normal = vec3(n.x, n.y, n.z);
					dv.uv = vec2(UVW.x, UVW.y);
					vertexs.push_back(dv);
				}
				for (size_t j = 0; j < mesh->mNumFaces; j++)
				{
					const aiFace& Face = mesh->mFaces[j];
					if (Face.mNumIndices == 3) {
						Indices.push_back(Face.mIndices[0]);
						Indices.push_back(Face.mIndices[1]);
						Indices.push_back(Face.mIndices[2]);
		
					}
				}
				
				uint32 a = mesh->mMaterialIndex;
				aiMaterial* mat = scene->mMaterials[a];
				
				auto m = m_pFactory->VGetMaterial("Default");

				aiString name;
				mat->Get<aiString>(AI_MATKEY_NAME, name);
				m->Name = name.C_Str();
				aiColor3D color(0.f, 0.f, 0.f);
				mat->Get(AI_MATKEY_COLOR_AMBIENT,color);
				m->Ka = vec3(color.r, color.g, color.b);
				mat->Get(AI_MATKEY_COLOR_DIFFUSE, color);
				m->Kd = vec3(color.r, color.g, color.b);
				mat->Get(AI_MATKEY_COLOR_SPECULAR, color);
				m->Ks = vec3(color.r, color.g, color.b);
				float exp;
				mat->Get(AI_MATKEY_SHININESS, exp);
				m->exp = vec3(exp);
				aiString Path;
				mat->GetTexture(aiTextureType_DIFFUSE, 0, &Path, NULL, NULL, NULL, NULL, NULL);



				DefaultMesh* pMesh = new DefaultMesh(m_pRenderDevice,vertexs, Indices, mesh->mName.C_Str());
			
				//pMesh->Tex = LoadTexture(localPath + Path.C_Str());
				//pMesh->mat = m;
				pModel->Textures.push_back(VGetTexture(localPath + Path.C_Str()));
				pModel->Materials.push_back(m);
				pModel->Meshs.push_back(std::unique_ptr<Mesh>(pMesh));
				
				
			}
		
			m_ModelCaches.push_back(ResourceHandle<render::Model>(filename,pModel));
		
			return pModel;
		}

		/*Shader * ResourceManager::VGetShader(string key)
		{
			return m_ShaderList[key].get();
		}*/

		render::Texture * ResourceManager::VGetTexture(const string& filename)
		{
			render::Texture* tex = nullptr;
			tex = HasResource(m_Textures,filename);
			if (tex == nullptr)
			{
				if((tex = LoadTexture(filename))==nullptr)
					E_ERROR("Cound not find texture: %s" ,filename);
				return m_pDefaultTex;
			}
			return tex;

		}

		render::VertexShader * ResourceManager::VGetVertexShader(const std::string & filename)
		{
			render::VertexShader* Vshader = nullptr;
			CheckResourceFunc a = [](const std::string&a, const std::string& b)
			{
				return (a.find(b) != string::npos);
			};
			Vshader = HasResource(m_VertexShaders, filename,a);
			if (Vshader == nullptr)
			{
				if((Vshader=LoadVertexShader(filename))==nullptr)
					E_ERROR("Cound not find vertex shader: %s", filename);
				
			}
			return Vshader;
		}

		render::PixelShader * ResourceManager::VGetPixelShader(const std::string & filename)
		{
			render::PixelShader* Pshader = nullptr;
			Pshader = HasResource(m_PixelShaders, filename);
			if (Pshader == nullptr)
			{
				if((Pshader=LoadPixelShader(filename))==nullptr)
					E_ERROR("Cound not find pixel shader: %s", filename);

			}
			return Pshader;
		}

		render::Model * ResourceManager::VGetModel(const string& filename)
		{
			render::Model* pModel = nullptr;
			if (filename.find(".xml") != string::npos) pModel = LoadModelXML(filename);
			/*else if (filename.find(".obj") != string::npos|| filename.find(".3ds") != string::npos)
			{
				for (size_t i = 0; i < m_ObjLists.size(); i++)
					if (m_ObjLists[i]->GetName() == filename) pModel = m_ObjLists[i].get();
			}*/
			else
			{
				pModel = HasResource(m_ModelCaches, filename);
				if (pModel == nullptr)
				{
					if ((pModel = LoadModel(filename)) == nullptr)
						E_ERROR("Cound not find model: %s", filename.c_str());
				}
			}
			
			return pModel;
		}

		HeightMap * ResourceManager::VGetHeightMap(const string& filename)
		{
			HeightMap* hm = nullptr;
			hm = HasResource(m_HeightMaps,filename);
			if (hm == nullptr)
			{
				E_ERROR("Could not find heightmap: %s", filename.c_str());
			}
			return hm;
		}

		/*LoadStatus * ResourceManager::VLoadResource(const std::string & resourcePath, bool async)
		{
			if (async)
			{
				if (resourcePath == m_LoadStatus.path) return &m_LoadStatus;
				m_LoadThread = std::thread(&ResourceManager::LoadThreadResource, this, &m_ThreadContext, resourcePath);

				return  &m_LoadStatus;
			}
			else LoadResources(resourcePath);
			return nullptr;
		}*/

		//FMOD::Sound * ResourceManager::VGetSound(const string & tag)
		//{
		//	SoundRAAI* pSound = HasSound(tag);
		//	if (pSound) return pSound->GetSound();
		//	return nullptr;
		//}

		//IMesh * ResourceManager::CreateShape(ShapeType type, float* size)
		//{
		//	if (type == SHAPE_BOX)
		//	{
		//		IMesh* pBox = new CubeMesh(size[0], size[1], size[2]);
		//		pBox->Name = ShapeName[type];
		//		m_PrimList.push_back(std::unique_ptr<IMesh>(pBox));
		//		return pBox;
		//		//E_ERROR("Error ResourceManager::CreateShape doesn't empl yet");
		//		//return nullptr;
		//	}

		//	return nullptr;

		//}

		//SpriteAnim * ResourceManager::VGetSpriteAnimation(const string& filename)
		//{
		//	SpriteAnim* s = nullptr;
		//	s = HasSprite(filename);
		//	return s;
		//}

		render::VertexShader * ResourceManager::LoadVertexShader(const std::string & filepath)
		{
			render::VertexShader* vertexShader = nullptr;

			if ((vertexShader = HasResource(m_VertexShaders, filepath)) != nullptr) return vertexShader;

			char sourceVertex[10000];

			FILE* pvFile = fopen(filepath.c_str(), "rt");

			if (!pvFile)
			{
				E_ERROR("Can't open File: %s", filepath.c_str());
				return nullptr;
			}
			if (fseek(pvFile, 0L, SEEK_END) == 0)
			{
				long bufsize = ftell(pvFile);
				fseek(pvFile, 0L, SEEK_SET);

				size_t newLen = fread(sourceVertex, sizeof(char), bufsize, pvFile);
				sourceVertex[newLen] = '\0'; /* Just to be safe. */
			}
			fclose(pvFile);


			vertexShader = m_pRenderDevice->CreateVertexShader(sourceVertex);

			m_VertexShaders.push_back(ResourceHandle<render::VertexShader>(filepath,vertexShader));
			return vertexShader;
		}

		render::PixelShader * ResourceManager::LoadPixelShader(const std::string & filepath)
		{
			render::PixelShader* pixelShader = nullptr;

			if ((pixelShader = HasResource(m_PixelShaders, filepath)) != nullptr) return pixelShader;

			char sourceVertex[10000];

			FILE* pvFile = fopen(filepath.c_str(), "rt");

			if (!pvFile)
			{
				E_ERROR("Can't open File: %s", filepath.c_str());
				return nullptr;
			}
			if (fseek(pvFile, 0L, SEEK_END) == 0)
			{
				long bufsize = ftell(pvFile);
				fseek(pvFile, 0L, SEEK_SET);

				size_t newLen = fread(sourceVertex, sizeof(char), bufsize, pvFile);
				sourceVertex[newLen] = '\0'; /* Just to be safe. */
			}
			fclose(pvFile);


			pixelShader = m_pRenderDevice->CreatePixelShader(sourceVertex);

			m_PixelShaders.push_back(ResourceHandle<render::PixelShader>(filepath, pixelShader));
			return pixelShader;
		}

		void ResourceManager::LoadResources(string path)
		{

			tinyxml2::XMLDocument doc;
			doc.LoadFile(path.c_str());
			tinyxml2::XMLElement* p = doc.FirstChildElement("Resources");


			//int totalResource = 0;
			//for (tinyxml2::XMLElement* pNode = p->FirstChildElement(); pNode; pNode = pNode->NextSiblingElement()) totalResource++;
			//int sumResource = 0;



			std::string Resourcepath = p->Attribute("Path");
			// Loop through each child element and load the component
			for (tinyxml2::XMLElement* pNode = p->FirstChildElement(); pNode; pNode = pNode->NextSiblingElement())
			{
				const char* name = pNode->Value();
				if (name == nullptr) continue;

				if (!strcmp(name, "Texture"))
				{
					const char* pFile = pNode->Attribute("File");
					LoadTexture(Resourcepath + pFile);
				}
				else if (!strcmp(name, "VertexShader"))
				{
					const char* pFile = pNode->Attribute("File");
					LoadVertexShader(Resourcepath + pFile);
				}
				else if (!strcmp(name, "PixelShader"))
				{
					const char* pFile = pNode->Attribute("File");
					LoadPixelShader(Resourcepath + pFile);
				}
				else if (!strcmp(name, "CubeTexture"))
				{
					std::vector<std::string> filelist;

					for (tinyxml2::XMLElement* pNodeTex = pNode->FirstChildElement(); pNodeTex; pNodeTex = pNodeTex->NextSiblingElement())
					{
						const char* pFile = pNodeTex->Attribute("File");
						filelist.push_back(Resourcepath + pFile);
					}

					LoadCubeTex(filelist);
				}
				else if (!strcmp(name, "Model"))
				{
					const char* pFile = pNode->Attribute("File");
					LoadModel(Resourcepath+pFile);
				}
				
				
				else if (!strcmp(name, "HeightMap"))
				{
					const char* pFile = pNode->Attribute("File");
					int size = pNode->DoubleAttribute("Size", 5.0f);
					int w = pNode->DoubleAttribute("Width", 2.0f);
					int h = pNode->DoubleAttribute("Height", 2.0f);
					float hscale = pNode->DoubleAttribute("HeightScale", 1.0);
					int s = pNode->DoubleAttribute("SubDevided", 1.0);
					if (pFile) LoadHeightMap(pFile, size, w, h, hscale, s);
				}
				else if (!strcmp(name, "Sprite"))
				{
					const char* pFile = pNode->Attribute("File");
					if (pFile) LoadSpriteAnimation(pFile);
				}
				/*else if (!strcmp(name, "Sound"))
				{

					const char* pFile = pNode->Attribute("File");
					const char* pTag = pNode->Attribute("Tag");
					unsigned int mode = pNode->Int64Attribute("Sound3D", 0);
					SoundRAAI* pSound = LoadSound(pFile, pTag, mode ? FMOD_3D : FMOD_2D);


					if (pSound)
					{
						if (mode)
						{
							float InnerRadius = pNode->FloatAttribute("InnerRadius", 1.0f);
							float OuterRadius = pNode->FloatAttribute("OuterRadius", 512.0f);
							pSound->GetSound()->set3DMinMaxDistance(InnerRadius, OuterRadius);
							pSound->GetSound()->setMode(FMOD_3D_LINEARROLLOFF);
						}

						m_SoundList.insert({ pTag,std::unique_ptr<SoundRAAI>(pSound) });
					}
				}*/
				
				//sumResource++;
				
				//m_LoadStatus.percent = float(sumResource) / totalResource;
			}


			
		}

		/*void ResourceManager::LoadThreadResource(OpenGLContext* ThreadContext, const std::string & path)
		{
			ThreadContext->MakeContext();

			m_LoadLock.lock();
			m_LoadStatus.path = path;
			m_LoadStatus.status = LoadStatus::LOADING;
			m_LoadLock.unlock();

			LoadResources(path);
			
			m_LoadLock.lock();
			m_LoadStatus.status = LoadStatus::FINISH;
			m_LoadStatus.percent = 1.f;
			m_LoadLock.unlock();
			m_LoadThread.detach();

		}*/

		void ResourceManager::LoadSystemResources()
		{
			LoadResources(SYSTEM_RESOURCES_CONFIG);
		}

		LTRawData* ResourceManager::LoadLTBModel(const std::string & filename)
		{
			LTRawData* pTemp = LTBFileLoader::LoadModel(filename.c_str());

			vector<math::AABB> abb(pTemp->SkeNodes.size());
			for (size_t i = 0; i < pTemp->Meshs.size(); i++)
			{
				LTRawMesh* pMesh = &pTemp->Meshs[i];


				for (size_t j = 0; j < pMesh->Vertexs.size(); j++)
				{
					const SkeVertex& vertex = pMesh->Vertexs[j];
					vec3 local;
					for (int k = 0; k < 4; k++)
					{
						if (vertex.weights[k].Bone < 100.0f && vertex.weights[k].weight >= 0.0f)
						{
							local = pTemp->SkeNodes[vertex.weights[k].Bone].m_InvBindPose*vec4(vertex.pos, 1.0f);
							local *= vertex.weights[k].weight;
							abb[vertex.weights[k].Bone].Test(local);
							pTemp->SkeNodes[vertex.weights[k].Bone].m_Flag = 1;
						}
						else
						{
							//assert(0);
						}
					}
				}
			}

			for (size_t i = 0; i < pTemp->SkeNodes.size(); i++)
			{
				//vec3 size = abb[i].Max - abb[i].Min;
				//vec3 pos = size / 2.0f + abb[i].Min;
				//pos.x = 0;
				//pos.y = 0;


				//if(size<vec3(20)) pTemp->pSkeNodes[i]->m_Flag = 0;
				//cout << size.x <<" " << size.y << " " << size.z << endl;
				pTemp->SkeNodes[i].m_BoundBox.Min = abb[i].Min;
				pTemp->SkeNodes[i].m_BoundBox.Max = abb[i].Max;
			}

			return pTemp;
		}


		ResourceManager::OpenGLContext::OpenGLContext(IContext * pContext)
		{
			OpenGLWindows* pWindow = static_cast<OpenGLWindows*>(pContext->GetSystem<IWindow>());
			pthread = glfwCreateWindow(1, 1, "Load", nullptr, pWindow->GetGLFW());
			if (pthread == nullptr) E_ERROR("Failed to create thread loading...");
			glfwHideWindow((GLFWwindow*)pthread);
			
		}

		void ResourceManager::OpenGLContext::MakeContext()
		{
			glfwMakeContextCurrent((GLFWwindow*)pthread);
		}

		ResourceManager::OpenGLContext::~OpenGLContext()
		{
			glfwDestroyWindow((GLFWwindow*)pthread);
		}

}
}