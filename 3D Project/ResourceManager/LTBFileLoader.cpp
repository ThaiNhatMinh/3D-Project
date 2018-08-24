#include "LTBFileLoader.h"
#include <pch.h>
#include "LTModel.h"

using namespace Light;

namespace LTBFileLoader
{

	typedef unsigned int        uint; // This is at least 16 bits

	typedef char			int8;
	typedef short int		int16;
	typedef int				int32;
	typedef __int64         int64;

	typedef unsigned char		uint8;
	typedef unsigned short int	uint16;
	typedef unsigned int		uint32;

	typedef unsigned __int64    uint64;
	// File types...
	enum E_LTB_FILE_TYPES {
		LTB_D3D_MODEL_FILE = 1,
		LTB_PS2_MODEL_FILE = 2,
		LTB_XBOX_MODEL_FILE = 3,
		LTB_ABC_MODEL_FILE = 4,
		LTB_D3D_RENDERSTYLE_FILE = 5,
		LTB_PS2_RENDERSTYLE_FILE = 6,
		LTB_D3D_RENDEROBJECT_FILE = 7
	};




	enum    D3DPIECE_TYPE { eRigidMesh = 4, eSkelMesh = 5, eVAMesh = 6, eNullMesh = 7 };
	struct Vector
	{
		float x, y, z;
	};
	// The verts we'll use (for all the possible vertex structures)...
	struct  VSTREAM_XYZ_B0 { float x; float y; float z; };
	struct  VSTREAM_XYZ_B1 { float x; float y; float z; float blend1; };
	struct  VSTREAM_XYZ_B2 { float x; float y; float z; float blend1; float blend2; };
	struct  VSTREAM_XYZ_B3 { float x; float y; float z; float blend1; float blend2; float blend3; };
	struct  VSTREAM_XYZ_B1_INDEX { float x; float y; float z; float blend1; uint8 Index[4]; };
	struct  VSTREAM_XYZ_B2_INDEX { float x; float y; float z; float blend1; float blend2; uint8 Index[4]; };
	struct  VSTREAM_XYZ_B3_INDEX { float x; float y; float z; float blend1; float blend2; float blend3; uint8 Index[4]; };
	struct  VSTREAM_THE_REST { float nx; float ny; float nz; float u1; float v1; float u2, v2; float u3, v3; float u4, v4; uint32 ColorRGBA; Vector S, T, SxT; };


	// VERTEX DATA TYPE FLAGS (Note: Should match those in the d3d structs header)...
#define	VERTDATATYPE_POSITION					0x0001
#define	VERTDATATYPE_NORMAL						0x0002
#define	VERTDATATYPE_COLOR						0x0004
#define	VERTDATATYPE_UVSETS_1					0x0010
#define	VERTDATATYPE_UVSETS_2					0x0020
#define	VERTDATATYPE_UVSETS_3					0x0040
#define	VERTDATATYPE_UVSETS_4					0x0080
#define	VERTDATATYPE_BASISVECTORS				0x0100


	enum ANIMCOMPRESSIONTYPE { NONE = 0, RELEVANT, RELEVANT_16, RELEVANT_ROT16_ONLY };

	struct LTBProp
	{
		uint32		m_iFileVersion;
		uint32		m_nKeyFrames;		// Number of keyframes.
		uint32		m_nParentAnims;		// Number of animations (that come from us).
		uint32		m_nNodes;			// Number of nodes.
		uint32		m_nPieces;			// Number of pieces.
		uint32		m_nChildModels;		// Number of child models (including the self child model).
		uint32		m_nTris;			// Number of triangles.
		uint32		m_nVerts;			// Number of vertices.
		uint32		m_nVertexWeights;	// Number of vertex weights.
		uint32		m_nLODs;			// Number of LODs.
		uint32		m_nSockets;			// Number of sockets.
		uint32		m_nWeightSets;		// Number of weight sets.
		uint32		m_nAnimData;		// Amount of animation data
		uint32		m_nStrings;			// How many strings we're allocating.
		uint32		m_StringLengths;	// Sum of all string lengths (not including null terminator).
		uint32		m_VertAnimDataSize; // byte size of the vertex animation data.
										//file positions for fields that need to be updated after a certain phase has passed
		uint32		m_nAnimDataPos;		// the position of the animation data size in the file
		float		m_GlobalRadius;		// Global radius.. MUST enclose the whole model.
		uint32		m_iNumEnabledOBBs;	// Number of enable OBB
	};


	// LTB header 
	struct LTB_Header {
		LTB_Header() { m_iFileType = 0; m_iVersion = 0; }
		unsigned char  m_iFileType;						// Should be one of E_LTB_FILE_TYPES
		unsigned short m_iVersion;						// Version number...
		unsigned char  m_iReserved1;					// Reserved for stuff we thing of later...
		unsigned int m_iReserved2;
		unsigned int m_iReserved3;
		unsigned int m_iReserved4;
	};
}

using namespace LTBFileLoader;

bool LoadSkeleton(FILE * pFile, Light::SkeNode* pParent, vector<Light::SkeNode>& nodeLists)
{
	Light::SkeNode p;
	if(pParent!=NULL)	p.m_ParentIndex = pParent->m_Index;
	else p.m_ParentIndex = -1;
	uint16 len;
	fread((void*)&len, sizeof(unsigned short), 1, pFile);
	char str[100];
	fread(str, len, 1, pFile);
	str[len] = '\0';

	p.m_Name = str;

	uint8 m_Flags;
	uint16 index;
	fread(&index, sizeof(uint16), 1, pFile);
	fread(&m_Flags, sizeof(uint8), 1, pFile);
	p.m_Index = index;
	float m[16];
	//float* m = glm::value_ptr(p.m_GlobalTransform);


	for (int i = 0; i<16; i++) fread(&m[i], sizeof(float), 1, pFile);
	
	vec4 v1(m[0], m[4], m[8], m[12]);
	vec4 v2(m[1], m[5], m[9], m[13]);
	vec4 v3(m[2], m[6], m[10], m[14]);
	vec4 v4(m[3], m[7], m[11], m[15]);
	p.m_GlobalTransform[0] = v1;
	p.m_GlobalTransform[1] = v2;
	p.m_GlobalTransform[2] = v3;
	p.m_GlobalTransform[3] = v4;

	
	p.m_InvBindPose = glm::inverse(p.m_GlobalTransform);
	uint32 numChild;
	fread(&numChild, sizeof(uint32), 1, pFile);

	nodeLists.push_back(p);
	for (size_t i = 0; i < numChild; i++) LoadSkeleton(pFile, &p, nodeLists);

	return true;
}

void ReadData(FILE * pFile, Light::AnimNode & node, const vector<Light::AnimKeyFrame>& KeyFrames, unsigned int CompressionType)
{
	Light::FrameData frame;
	if (CompressionType == NONE)
	{
		uint8 bIsVertexAnim;
		fread(&bIsVertexAnim, sizeof(uint8), 1, pFile);
		if (bIsVertexAnim)
		{
			cout << "bIsVertexAnim" << endl;
			system("pause");
		}
		else
		{
			float pos[3];
			float ort[4];
			for (uint16 i = 0; i < KeyFrames.size(); i++)
			{
				fread(pos, sizeof(pos), 1, pFile);
				frame.m_Pos = vec3(pos[0], pos[1], pos[2]);
				node.Data.push_back(frame);
			}
			for (uint16 i = 0; i < KeyFrames.size(); i++)
			{
				fread(ort, sizeof(ort), 1, pFile);
				node.Data[i].m_Ort = quat(ort[3], ort[0], ort[1], ort[2]);
			}
		}
	}
	else if (CompressionType == RELEVANT_16)
	{
		uint32 num_keyframes;
		fread(&num_keyframes, sizeof(uint32), 1, pFile);
		int16 compressed_pos[3];
		int16 compressed_rot[4];

		for (uint16 i = 0; i < num_keyframes; i++)
		{
			fread(compressed_pos, sizeof(int16) * 3, 1, pFile);

			frame.m_Pos = vec3(compressed_pos[0] / 16.0f, compressed_pos[1] / 16.0f, compressed_pos[2] / 16.0f);
			node.Data.push_back(frame);

		}
		for (uint16 i = num_keyframes; i < KeyFrames.size(); i++)
		{
			node.Data.push_back(node.Data[0]);
		}
		fread(&num_keyframes, sizeof(uint32), 1, pFile);

		for (uint16 i = 0; i < num_keyframes; i++)
		{
			fread(compressed_rot, sizeof(int16) * 4, 1, pFile);
			node.Data[i].m_Ort.w = (float)compressed_rot[3] / 0x7FFF;
			node.Data[i].m_Ort.x = (float)compressed_rot[0] / 0x7FFF;
			node.Data[i].m_Ort.y = (float)compressed_rot[1] / 0x7FFF;
			node.Data[i].m_Ort.z = (float)compressed_rot[2] / 0x7FFF;
		}
		for (uint16 i = num_keyframes; i < KeyFrames.size(); i++)
		{
			node.Data[i].m_Ort = node.Data[0].m_Ort;
		}
	}
	else
	{
		cout << "Unkown type" << endl;
	}
}




vector<LTRawMesh> LoadMesh(FILE* pFile)
{
	vector<LTRawMesh> meshlist;

	uint32 numPieces;
	fread(&numPieces, sizeof(uint32), 1, pFile);
	meshlist.resize(numPieces);


	unsigned short str_len;
	for (uint32 iPieceCnt = 0; iPieceCnt < numPieces; iPieceCnt++)
	{
		vector<Light::SkeVertex>& vertex = meshlist[iPieceCnt].Vertexs;
		vector<unsigned int>& Index = meshlist[iPieceCnt].Indices;
		std::string meshname;

		fread((void*)&str_len, sizeof(unsigned short), 1, pFile);
		char str[MAX_NAME];
		fread(str, str_len, 1, pFile);
		str[str_len] = '\0';
		//printf("%d %s\n",len,str);

		// name of mesh
		meshlist[iPieceCnt].Name = str;
		uint32 numLOD;
		float LODDists;
		fread(&numLOD, sizeof(uint32), 1, pFile);
		//printf("numLOD: %d\n", numLOD);
		for (uint32 dist_cnt = 0; dist_cnt < numLOD; dist_cnt++)
		{
			fread(&LODDists, sizeof(float), 1, pFile);
			//printf("LODDists: %f\n", LODDists);
		}

		// write out min/max lod offset values.
		uint32 minlodoff, maxlodoff;
		fread(&minlodoff, sizeof(uint32), 1, pFile);
		//("minlodoff: %d\n", minlodoff);
		fread(&maxlodoff, sizeof(uint32), 1, pFile);
		//printf("maxlodoff: %d\n", maxlodoff);
		for (uint32 iLODCnt = 0; iLODCnt < numLOD; iLODCnt++)
		{

			uint32 itmp_RS_use_index = 0;
			uint32 m_nNumTextures;
			int32 m_iTextures[4], m_iRenderStyle;
			uint8 m_nRenderPriority;
			fread(&m_nNumTextures, sizeof(uint32), 1, pFile);
			fread(&m_iTextures, sizeof(int32) * 4, 1, pFile);
			fread(&m_iRenderStyle, sizeof(int32), 1, pFile);
			fread(&m_nRenderPriority, sizeof(uint8), 1, pFile);

			uint PieceType;
			fread(&PieceType, sizeof(uint32), 1, pFile);
			//see if this is a null mesh
			if (PieceType == eNullMesh) fread(&PieceType, sizeof(uint32), 1, pFile);
			else
			{
				uint32 iMaxBonesPerTri, iMaxBonesPerVert;
				uint32 iRendObjectSize = 0;
				uint32 iVertCount;
				uint32 iPolyCount;
				uint32 StreamData[4];
				switch (PieceType)
				{
				case eRigidMesh:
					uint32 Bone;
					//printf("  PieceType: RigidMesh\n");
					fread(&iRendObjectSize, sizeof(uint32), 1, pFile);
					fread(&iVertCount, sizeof(uint32), 1, pFile);
					fread(&iPolyCount, sizeof(uint32), 1, pFile);
					fread(&iMaxBonesPerTri, sizeof(uint32), 1, pFile);
					fread(&iMaxBonesPerVert, sizeof(uint32), 1, pFile);
					fread(StreamData, sizeof(uint32) * 4, 1, pFile);
					fread(&Bone, sizeof(uint32), 1, pFile);
					//uint32 p;
					//fread(&p, sizeof(uint32), 1, pFile);
					Index.resize(iPolyCount *3);
					for (uint32 iStream = 0; iStream < 4; ++iStream)
						for (uint32 i = 0; i < iVertCount; ++i)
						{
							Light::SkeVertex ver;
							if (StreamData[iStream] & VERTDATATYPE_POSITION)
							{
								fread(&ver.pos.x, sizeof(float), 1, pFile);
								fread(&ver.pos.y, sizeof(float), 1, pFile);
								fread(&ver.pos.z, sizeof(float), 1, pFile);
								ver.weights[0] = (Light::Weight(Bone, 1.0f));
								ver.weights[1] = (Light::Weight(255, 0.0f));
								ver.weights[2] = (Light::Weight(255, 0.0f));
								ver.weights[3] = (Light::Weight(255, 0.0f));
								vertex.push_back(ver);
							}
							if (StreamData[iStream] & VERTDATATYPE_NORMAL)
							{
								fread(&vertex[i].normal.x, sizeof(float), 1, pFile);
								fread(&vertex[i].normal.y, sizeof(float), 1, pFile);
								fread(&vertex[i].normal.z, sizeof(float), 1, pFile);
							}
							if (StreamData[iStream] & VERTDATATYPE_COLOR)
							{
								uint32 a;
								fread(&a, sizeof(uint32), 1, pFile);
							}
							if (StreamData[iStream] & VERTDATATYPE_UVSETS_1)
							{
								vec2 uv;
								fread(&uv.x, sizeof(float), 1, pFile);
								fread(&uv.y, sizeof(float), 1, pFile);
								vertex[i].uv = uv;
							}
							if (StreamData[iStream] & VERTDATATYPE_UVSETS_2)
							{
								float temp;
								fread(&temp, sizeof(float), 1, pFile);
								fread(&temp, sizeof(float), 1, pFile);
							}
							if (StreamData[iStream] & VERTDATATYPE_UVSETS_3)
							{
								float temp2;
								fread(&temp2, sizeof(float), 1, pFile);
								fread(&temp2, sizeof(float), 1, pFile);
							}
							if (StreamData[iStream] & VERTDATATYPE_UVSETS_4)
							{
								float temp3;
								fread(&temp3, sizeof(float), 1, pFile);
								fread(&temp3, sizeof(float), 1, pFile);
							}

							if (StreamData[iStream] & VERTDATATYPE_BASISVECTORS)
							{
								float temp4[6];
								fread(&temp4, sizeof(float) * 6, 1, pFile);
							}
						}
					//vector<uint16> IndexList;
					// Write out pIndexList...
					for (uint32 i = 0; i < iPolyCount * 3; ++i)
					{
						uint16 index;
						fread(&index, sizeof(uint16), 1, pFile);
						//IndexList.push_back(index);
						Index[i] = ((uint)index);
					}

					break;
				case eVAMesh:
					printf("  PieceType: VertexAnimatedMesh\n");
					break;
				case eSkelMesh:



					bool bReIndexBones;

					bool UseMatrixPalettes;
					uint32 iMinBone = 256; uint32 iMaxBone = 0;

					fread(&iRendObjectSize, sizeof(uint32), 1, pFile);
					fread(&iVertCount, sizeof(uint32), 1, pFile);
					fread(&iPolyCount, sizeof(uint32), 1, pFile);
					fread(&iMaxBonesPerTri, sizeof(uint32), 1, pFile);
					fread(&iMaxBonesPerVert, sizeof(uint32), 1, pFile);
					fread(&bReIndexBones, sizeof(bool), 1, pFile);
					fread(StreamData, sizeof(uint32) * 4, 1, pFile);
					fread(&UseMatrixPalettes, sizeof(bool), 1, pFile);
					Index.resize(iPolyCount * 3);
					//vector<VSTREAM_XYZ_B0> OutVerts_B0;
					//vector<VSTREAM_XYZ_B1> OutVerts_B1;
					//vector<VSTREAM_XYZ_B2> OutVerts_B2;
					//vector<VSTREAM_XYZ_B3> OutVerts_B3;
					vector<VSTREAM_THE_REST> OutVerts_TheRest;

					for (uint32 iStream = 0; iStream < 4; ++iStream)
					{
						for (uint32 i = 0; i < iVertCount; ++i)
						{
							Light::SkeVertex ver;
							if (StreamData[iStream] & VERTDATATYPE_POSITION)
							{
								switch (iMaxBonesPerTri) {
								case 1:
									VSTREAM_XYZ_B0 b0;
									fread(&b0, sizeof(VSTREAM_XYZ_B0), 1, pFile);
									break;
								case 2:
									VSTREAM_XYZ_B1 tb1;
									fread(&tb1, sizeof(VSTREAM_XYZ_B1), 1, pFile);
									ver.pos = vec3(tb1.x, tb1.y, tb1.z);
									ver.weights[0] = (Light::Weight(255, tb1.blend1));
									ver.weights[1] = (Light::Weight(255, 1.0f - tb1.blend1));
									ver.weights[2] = (Light::Weight(255, 0.0f));
									ver.weights[3] = (Light::Weight(255, 0.0f));
									vertex.push_back(ver);
									break;
								case 3:
									VSTREAM_XYZ_B2 tx;
									fread(&tx, sizeof(tx), 1, pFile);
									ver.pos = vec3(tx.x, tx.y, tx.z);
									ver.weights[0] = (Light::Weight(255, tx.blend1));
									ver.weights[1] = (Light::Weight(255, tx.blend2));
									ver.weights[2] = (Light::Weight(255, 1.0f - tx.blend1 - tx.blend2));
									ver.weights[3] = (Light::Weight(255, 0.0f));
									vertex.push_back(ver);
									break;
								case 4:
									VSTREAM_XYZ_B3 t;
									fread(&t, sizeof(VSTREAM_XYZ_B3), 1, pFile);
									ver.pos = vec3(t.x, t.y, t.z);
									ver.weights[0] = (Light::Weight(255, t.blend1));
									ver.weights[1] = (Light::Weight(255, t.blend2));
									ver.weights[2] = (Light::Weight(255, t.blend3));
									ver.weights[3] = (Light::Weight(255, 1.0f - t.blend3 - t.blend2 - t.blend1));

									vertex.push_back(ver);
									break;
								}
							}

							if (StreamData[iStream] & VERTDATATYPE_NORMAL)
							{
								VSTREAM_THE_REST t;
								fread(&t.nx, sizeof(float), 1, pFile);
								fread(&t.ny, sizeof(float), 1, pFile);
								fread(&t.nz, sizeof(float), 1, pFile);
								vertex[i].normal = vec3(t.nx, t.ny, t.nz);
							}
							if (StreamData[iStream] & VERTDATATYPE_COLOR)
							{
								uint32 tem;
								fread(&tem, sizeof(uint32), 1, pFile);
							}
							if (StreamData[iStream] & VERTDATATYPE_UVSETS_1)
							{
								vec2 uv;
								fread(&uv, sizeof(vec2), 1, pFile);
								vertex[i].uv = uv;
							}
							if (StreamData[iStream] & VERTDATATYPE_UVSETS_2)
							{
								fread(&OutVerts_TheRest[i].u2, sizeof(float), 1, pFile);
								fread(&OutVerts_TheRest[i].v2, sizeof(float), 1, pFile);
							}
							if (StreamData[iStream] & VERTDATATYPE_UVSETS_3)
							{
								fread(&OutVerts_TheRest[i].u3, sizeof(float), 1, pFile);
								fread(&OutVerts_TheRest[i].v3, sizeof(float), 1, pFile);
							}
							if (StreamData[iStream] & VERTDATATYPE_UVSETS_4)
							{
								fread(&OutVerts_TheRest[i].u4, sizeof(float), 1, pFile);
								fread(&OutVerts_TheRest[i].v4, sizeof(float), 1, pFile);
							}

							if (StreamData[iStream] & VERTDATATYPE_BASISVECTORS)
							{
								fread(&OutVerts_TheRest[i].S.x, sizeof(float), 1, pFile);
								fread(&OutVerts_TheRest[i].S.y, sizeof(float), 1, pFile);
								fread(&OutVerts_TheRest[i].S.z, sizeof(float), 1, pFile);
								fread(&OutVerts_TheRest[i].T.x, sizeof(float), 1, pFile);
								fread(&OutVerts_TheRest[i].T.y, sizeof(float), 1, pFile);
								fread(&OutVerts_TheRest[i].T.z, sizeof(float), 1, pFile);
							}
						}
					}


					// Write out pIndexList...
					for (uint32 i = 0; i < iPolyCount * 3; ++i)
					{
						uint16 index;
						fread(&index, sizeof(uint16), 1, pFile);
						Index[i] = ((uint)index);
					}

					uint BoneComboListSize;
					fread(&BoneComboListSize, sizeof(uint32), 1, pFile);
					//printf("BoneComboListSize: %d\n", BoneComboListSize);
					for (uint32 i = 0; i < BoneComboListSize; i++)
					{
						uint16 m_BoneIndex_Start, m_BoneIndex_End;
						uint8 m_BoneList[4];

						fread(&m_BoneIndex_Start, sizeof(uint16), 1, pFile);
						fread(&m_BoneIndex_End, sizeof(uint16), 1, pFile);


						fread(&m_BoneList[0], sizeof(uint8) * 4, 1, pFile);

						for (uint16 k = m_BoneIndex_Start; k < m_BoneIndex_Start + m_BoneIndex_End; k++)
						{
							unsigned int wid;
							for (wid = 0; wid < 4; wid++) vertex[k].weights[wid].Bone = m_BoneList[wid];


						}

						uint32 m_iIndexIndex;
						fread(&m_iIndexIndex, sizeof(uint32), 1, pFile);
						//printf("m_iIndexIndex: %d\n",m_iIndexIndex);
					}

					break;

				}

			}



			uint8 m_UsedNodeListSize;
			fread(&m_UsedNodeListSize, sizeof(uint8), 1, pFile);
			vector<uint32>			m_UsedNodeList;
			for (uint32 iUsedNodeCnt = 0; iUsedNodeCnt < m_UsedNodeListSize; iUsedNodeCnt++)
			{
				uint8 t;
				fread(&t, sizeof(uint8), 1, pFile);
				//printf("m_UsedNodeList[%d]: %d\n", iUsedNodeCnt, t);
				m_UsedNodeList.push_back(t);
			}
		}
	}

	return meshlist;
}

vector<Light::SkeNode> LoadSkeleton(FILE *pFile)
{
	vector<Light::SkeNode> nodelist;

	//m_pSkeleton = new SkeletonNode;
	//m_pSkeleton->m_pParent = NULL;
	//m_pSkeleton->LoadSkeleton(pFile);
	LoadSkeleton(pFile, NULL, nodelist);

	return nodelist;
}

vector<Light::WeightBlend> LoadWS(FILE* pFile)
{
	vector<Light::WeightBlend> ws;

	uint32 valid_ws_indexsize;
	fread(&valid_ws_indexsize, sizeof(uint32), 1, pFile);
	ws.resize(valid_ws_indexsize);
	for (uint16 i = 0; i < valid_ws_indexsize; i++)
	{
		uint32 k;
		uint16 len;
		fread((void*)&len, sizeof(unsigned short), 1, pFile);
		char str[100];
		fread(str, len, 1, pFile);
		str[len] = '\0';
		strcpy(ws[i].Name, str);
		uint32 m_WeightsGetSize;
		fread(&m_WeightsGetSize, sizeof(uint32), 1, pFile);
		for (k = 0; k < m_WeightsGetSize; k++)
		{
			float t;
			fread(&t, sizeof(float), 1, pFile);
			ws[i].Blend[k] = t;
			//printf("m_Weights[%d]: %.3f\n",k,t);
		}
	}

	return ws;
}

vector<string> LoadChildName(FILE* pFile)
 {
	vector<string> childs;
	uint32 NumChildModels;
	fread(&NumChildModels, sizeof(uint32), 1, pFile);
	unsigned short str_len;
	for (size_t i = 1; i < NumChildModels; ++i)
	{
		fread((void*)&str_len, sizeof(unsigned short), 1, pFile);
		char str[100];
		fread(str, str_len, 1, pFile);
		str[str_len] = '\0';
		childs.push_back(str);
		//printf("%d %s\n", str_len, str);
	}

	return childs;
}

vector<Light::Animation> LoadAnimation(FILE* pFile,const vector<Light::SkeNode>& skenode)
 {
	 uint32 m_CompressionType = 0;
	 uint32 m_InterpolationMS = 0;
	 vector<Light::Animation>m_pAnimList;
	
	 // Save animations.
	 uint32 nAnimDataSize = 0;
	
	 uint32 CalcNumParentAnims;
	 unsigned short str_len = 0;
	 fread(&CalcNumParentAnims, sizeof(uint32), 1, pFile);
	 m_pAnimList.resize(CalcNumParentAnims);
	 for (uint32 i = 0; i < CalcNumParentAnims; i++)
	 {
		 Light::Animation p;
		 vec3 D;
		 fread(&D, sizeof(float) * 3, 1, pFile);
		 //printf("Dimension: %.2f, %.2f, %.2f\n",D[0],D[1],D[2]);
		 p.BV.SetMax(D);
		 p.BV.SetMin(-D);
		 // Read name of animation
		 fread((void*)&str_len, sizeof(unsigned short), 1, pFile);
		 char str[100];
		 fread(str, str_len, 1, pFile);
		 str[str_len] = '\0';

		 p.Name = str;

		 fread(&m_CompressionType, sizeof(uint32), 1, pFile);
		 fread(&m_InterpolationMS, sizeof(uint32), 1, pFile);



		 uint32 m_KeyFramesGetSize;
		 fread(&m_KeyFramesGetSize, sizeof(uint32), 1, pFile);
		 //std::vector<AnimKeyFrame> KeyFrame;
		 for (uint16 j = 0; j<m_KeyFramesGetSize; j++)
		 {
			 Light::AnimKeyFrame key;
			 fread(&key.m_Time, sizeof(uint32), 1, pFile);
			 fread((void*)&str_len, sizeof(unsigned short), 1, pFile);
			 char str1[100];
			 fread(str1, str_len, 1, pFile);
			 str1[str_len] = '\0';

			 key.m_pString = str1;
			 p.KeyFrames.push_back(key);
		 }
		 //m_pSkeleton->LoadAnimation(pFile, str, KeyFrame, m_CompressionType);
		 //printf("Finish animation: %s\n",str);

		 for (GLuint j = 0; j < skenode.size(); j++)
		 {
			 //p.AnimNodeLists.resize(pNodeList.size());
			 Light::AnimNode node;
			 node.Parent = skenode[j].m_ParentIndex;
			 ReadData(pFile, node, p.KeyFrames,m_CompressionType);
			 p.AnimNodeLists.push_back(node);
		 }
		 m_pAnimList[i] = p;
	 }


	 return m_pAnimList;

 }

 vector<Light::LTBSocket> LoadSocket(FILE* pFile)
{
	vector<Light::LTBSocket> socketlist;
	uint32 NumSockets;
	uint16 str_len;
	fread(&NumSockets, sizeof(uint32), 1, pFile);
	for (uint32 i = 0; i < NumSockets; i++)
	{
		Light::LTBSocket socket;
		fread(&socket.m_iNode, sizeof(uint32), 1, pFile);
		fread((void*)&str_len, sizeof(unsigned short), 1, pFile);
		char str1[100];
		fread(str1, str_len, 1, pFile);
		str1[str_len] = '\0';
		socket.name = str1;
		float ort[4];
		fread(&ort, sizeof(float) * 4, 1, pFile);
		vec3 pos;
		fread(&pos, sizeof(float) * 3, 1, pFile);
		vec3 scale;
		fread(&scale, sizeof(float) * 3, 1, pFile);
		
		quat q(ort[0], ort[1], ort[2], ort[3]);
		q = glm::pow(q, 0.5f);
		mat4 rotate = glm::toMat4(q);
		mat4 trf = glm::translate(mat4(),pos);
		socket.Transform = trf;
		socketlist.push_back(socket);
	}

	return socketlist;
	
}

LTBProp LoadProp(FILE* pFile)
 {
	 LTBProp prop;
	 LTB_Header Header;

	 uint32		m_iFileVersion;
	 uint32		m_nKeyFrames;		// Number of keyframes.
	 uint32		m_nParentAnims;		// Number of animations (that come from us).
	 uint32		m_nNodes;			// Number of nodes.
	 uint32		m_nPieces;			// Number of pieces.
	 uint32		m_nChildModels;		// Number of child models (including the self child model).
	 uint32		m_nTris;			// Number of triangles.
	 uint32		m_nVerts;			// Number of vertices.
	 uint32		m_nVertexWeights;	// Number of vertex weights.
	 uint32		m_nLODs;			// Number of LODs.
	 uint32		m_nSockets;			// Number of sockets.
	 uint32		m_nWeightSets;		// Number of weight sets.
	 uint32		m_nAnimData;		// Amount of animation data
	 uint32		m_nStrings;			// How many strings we're allocating.
	 uint32		m_StringLengths;	// Sum of all string lengths (not including null terminator).
	 uint32		m_VertAnimDataSize; // byte size of the vertex animation data.
									//file positions for fields that need to be updated after a certain phase has passed

	 float		m_GlobalRadius;		// Global radius.. MUST enclose the whole model.
	 uint32		m_iNumEnabledOBBs;	// Number of enable OBB


	 fread((void*)&Header, sizeof(LTB_Header), 1, pFile);
	 fread((void*)&m_iFileVersion, sizeof(unsigned int), 1, pFile);

	 fread(&m_nKeyFrames, sizeof(uint32), 1, pFile);
	 fread(&m_nParentAnims, sizeof(uint32), 1, pFile);
	 fread(&m_nNodes, sizeof(uint32), 1, pFile);
	 fread(&m_nPieces, sizeof(uint32), 1, pFile);
	 fread(&m_nChildModels, sizeof(uint32), 1, pFile);
	 fread(&m_nTris, sizeof(uint32), 1, pFile);
	 fread(&m_nVerts, sizeof(uint32), 1, pFile);
	 fread(&m_nVertexWeights, sizeof(uint32), 1, pFile);
	 fread(&m_nLODs, sizeof(uint32), 1, pFile);
	 fread(&m_nSockets, sizeof(uint32), 1, pFile);
	 fread(&m_nWeightSets, sizeof(uint32), 1, pFile);
	 fread(&m_nStrings, sizeof(uint32), 1, pFile);
	 fread(&m_StringLengths, sizeof(uint32), 1, pFile);
	 fread(&m_VertAnimDataSize, sizeof(uint32), 1, pFile);
	 fread(&m_nAnimData, sizeof(uint32), 1, pFile);


	 // read command string
	 unsigned short str_len;
	 fread((void*)&str_len, sizeof(unsigned short), 1, pFile);
	 char str[100];
	 fread(str, str_len, 1, pFile);

	 fread(&m_GlobalRadius, sizeof(float), 1, pFile);
	 fread(&m_iNumEnabledOBBs, sizeof(uint32), 1, pFile);

	 prop.m_iFileVersion = m_iFileVersion;
	 prop.m_nKeyFrames = m_nKeyFrames;
	 prop.m_nParentAnims = m_nParentAnims;
	 prop.m_nNodes = m_nNodes;
	 prop.m_nPieces = m_nPieces;
	 prop.m_nChildModels = m_nChildModels;
	 prop.m_nTris = m_nTris;
	 prop.m_nVerts = m_nVerts;
	 prop.m_nVertexWeights = m_nVertexWeights;
	 prop.m_nLODs = m_nLODs;
	 prop.m_nSockets = m_nSockets;
	 prop.m_nWeightSets = m_nWeightSets;
	 prop.m_nAnimData = m_nAnimData;
	 prop.m_nStrings = m_nStrings;
	 prop.m_StringLengths = m_StringLengths;
	 prop.m_VertAnimDataSize = m_VertAnimDataSize;

	 //prop.m_nAnimDataPos= m_nAnimDataPos;	
	 prop.m_GlobalRadius = m_GlobalRadius;
	 prop.m_iNumEnabledOBBs = m_iNumEnabledOBBs;
	 return prop;
 }


LTRawData * LTBFileLoader::LoadModel(const char * pFileName)
{
	FILE* pFile = fopen(pFileName, "rb");
	if (!pFile)
	{
		E_ERROR("Can't load file: %s",pFileName);
		return nullptr;
	}

	LTRawData* pModel = new LTRawData;
	
	LoadProp(pFile);
	pModel->Meshs = LoadMesh(pFile);
	pModel->SkeNodes = LoadSkeleton(pFile);
	pModel->wb = LoadWS(pFile);
	pModel->ChildName = LoadChildName(pFile);
	pModel->Anims = LoadAnimation(pFile, pModel->SkeNodes);
	pModel->Sockets = LoadSocket(pFile);
	
	
	fclose(pFile);

	return pModel;

}


