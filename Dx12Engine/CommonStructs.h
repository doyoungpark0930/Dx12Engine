#pragma once
#include "DXHelper.h"
using namespace DirectX;
using namespace DirectX::SimpleMath;

#define MAX_BONE_INFLUENCE 4
#define MAX_TEXTURE_NUM 5

#define TicksPerSecond 4800
#define ModelMatrixNum 128

#define ALBEDO_SLOT 0
#define AO_SLOT 1
#define NORMALMAP_SLOT 2
#define METALLIC_SLOT 3
#define	ROUGHNESS_SLOT 4

struct SkinnedVertex
{
	Vector3 Pos;
	Vector3 Normal;
	Vector2 Tex;
	Vector3 tangent;

	FLOAT m_Weights[MAX_BONE_INFLUENCE];
	uint8_t m_BoneIDs[MAX_BONE_INFLUENCE];

};

struct Vertex
{
	Vector3 Pos;
	Vector3 Normal;
	Vector2 Tex;
	Vector3 tangent;
};

struct BoneInfo
{
	int id;
	Matrix offset;
};

struct NormalVertex
{
	Vector3 Pos;
	Vector3 Normal;
	Vector3 StartPos;  //vertex시작 지점
};

struct ShadowFrustum {
	Vector4 radiance = Vector4(5.0f, 5.0f, 5.0f, 1.0f); // strength
	Vector4 direction = Vector4(0.0f, 0.0f, 1.0f, 1.0f);
	Vector4 position = Vector4(0.0f, 0.0f, -2.0f, 1.0f);

	Matrix viewProj; // 그림자 렌더링에 필요
};

struct GLOBAL_CONSTANT
{
	Matrix View;
	Matrix Proj;
	Vector4 eyePos;
	Vector4 lightPos;

	ShadowFrustum shadowFrustum;
	float strengthIBL;
};

struct MODEL_CONSTANT
{
	Matrix Model;
	Matrix NormalModel;
};

struct MATERIAL_CONSTANT
{
	int useAlbedoTex;
	int useAoTex;
	int useNormalTex;
	int useMetallicTex;
	int useRoughnessTex;
	int useGlossinessTex;

	int useShadowMap = 0;
};

struct SkinnedConstants
{
	Matrix boneTransforms[ModelMatrixNum];
};

struct ObjectState
{
	Vector3 scale = Vector3(1.0f, 1.0f, 1.0f);
	Vector3 rotation = Vector3(0.0f, 0.0f, 0.0f);
	Vector3 pos = Vector3(0.0f, 0.0f, 0.0f);
};

struct SRV_CONTAINER
{
	ID3D12Resource* pSrvResource = nullptr;
	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle;
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle;
};

struct CBV_CONTAINER
{
	UINT8* pSystemMemAddr;
	D3D12_CPU_DESCRIPTOR_HANDLE	CBVHandle;
};

struct meshNode
{
	std::string name;
	Matrix offset;
	int mNumChildren = 0;
	meshNode* mChildren[20]; //최대 자식 수 20개
	meshNode* mParent = nullptr;
};

struct aniNode
{
	std::string name;
	int mNumChildren = 0;
	aniNode* mChildren[20]; //최대 자식 수 20개
	aniNode* mParent = nullptr;

	Vector3 decomp_t;
	Vector4 decomp_q;
	Vector4 decomp_u;
	Vector3 decomp_k;

	int mNumPositionKeys = 0;
	Vector3 mPositionKeysValue[30];
	float mPositionKeysTime[30];

	int mNumRotationKeys = 0;
	Vector4 mRotationKeysValue[30];
	float mRotationKeysTime[30];

	int mNumScaleKeys = 0;
	Vector3 mScaleKeysValue[30];
	float mScaleKeysTime[30];
};

struct TRI_GROUP_PER_MTL
{
	D3D12_INDEX_BUFFER_VIEW* IndexBufferView = nullptr;
	UINT* triNum = nullptr;

	SRV_CONTAINER* srvContainer = nullptr;
};

struct material
{
	char* albedoTexFilename = nullptr;
	char* aoTexFilename = nullptr;
	char* normalTexFilename = nullptr;
	char* metallicTexFilename = nullptr;
	char* roughnessTexFilename = nullptr;

	UINT meshNum = 0;
	UINT** index = nullptr; //첫번째 괄호는 mesh순서에 해당
	UINT* face_cnt = nullptr; //mesh당 face가 몇개 그려졌는지

	~material()
	{
		SafeDeleteArray(&albedoTexFilename);
		SafeDeleteArray(&aoTexFilename);
		SafeDeleteArray(&normalTexFilename);
		SafeDeleteArray(&metallicTexFilename);
		SafeDeleteArray(&roughnessTexFilename);

		SafeDeleteArray(&face_cnt);
		for (int i = 0; i < meshNum; i++)
		{
			SafeDeleteArray(&index[i]);
		}
		SafeDeleteArray(&index);
	}
};

struct SkinnedMesh //일단 이렇게 확실히 mesh로 나누어야함. 이렇게 안하면 각 mesh의 index가 vertex를 참조못함
{
	SkinnedVertex* vertices = nullptr;
	UINT verticesNum = 0;

	~SkinnedMesh()
	{
		SafeDeleteArray(&vertices);
	}
};

struct JustMeshData
{
	Vertex* vertices = nullptr;
	UINT verticesNum = 0;
	UINT* indices = nullptr;
	UINT indicesNum = 0;

	char* albedoTexFilename = nullptr;
	char* aoTexFilename = nullptr;
	char* normalTexFilename = nullptr;
	char* metallicTexFilename = nullptr;
	char* roughnessTexFilename = nullptr;

	char* envIBL_Filename = nullptr;
	char* specularIBL_Filename = nullptr;
	char* irradianceIBL_Filename = nullptr;
	char* brdf_Filename = nullptr;

};

class Animation;
struct MeshDataInfo {
	//helperNode
	meshNode* rootNode = nullptr;

	//MaterialArray
	material* Materials = nullptr;
	UINT materialNum = 0;

	//MeshArray
	SkinnedMesh* meshes = nullptr;
	UINT meshNum = 0;

	//animation
	Animation* m_animations = nullptr;
	UINT animationCnt = 0;
	Matrix m_defaultTransform;
	Matrix* finalBoneMatrices = nullptr;

	//boundingBox
	BoundingBox boundingBox;
};