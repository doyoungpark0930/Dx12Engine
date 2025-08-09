#pragma once
using namespace DirectX;

struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};

struct SceneConstantBuffer
{
	XMFLOAT4 offset;
	float padding[60]; // Padding so the constant buffer is 256-byte aligned.
};
static_assert((sizeof(SceneConstantBuffer) % D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) == 0, "Constant Buffer size must be 256-byte aligned");

struct Vertex1 //�̰� ��¥�� �����Ű� �׽�Ʈ
{
	XMFLOAT3 Pos;
	XMFLOAT3 Normal;
	XMFLOAT2 Tex;   // �ؽ�ó ��ǥ (U, V)
};

struct NormalVertex
{
	XMFLOAT3 Pos;
	XMFLOAT3 Normal;
	XMFLOAT3 StartPos;  //vertex���� ����
};



struct MODEL_CONSTANT
{
	XMFLOAT4X4 Model;
	XMFLOAT4X4 NormalModel;
};

struct VIEWPROJ_CONSTANT
{
	XMFLOAT4X4 ViewProj;
	XMFLOAT4 eyePos;
	XMFLOAT4 lightPos;
};