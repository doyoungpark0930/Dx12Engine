#pragma once

class Renderer;
class DescriptorPool;
class SrvManager;
class CbvManger;
class Animation;


class Model
{
public:
	static Renderer* m_renderer;
	static DescriptorPool* m_descriptorPool;
	static SrvManager* m_srvManager;
	static CbvManager* m_cbvManager;
	void CreateCubeMap(JustMeshData& meshData);
	void CreateGeneralModel(JustMeshData& meshData);
	void CreateModelFromFile(MeshDataInfo& meshesInfo);
	void CreateTextureFromName(char* textureFilename, SRV_CONTAINER& srvContainer);
	void CreateCubeTextureFromName(char* textureFilename, SRV_CONTAINER& srvContainer);
	template <typename T>
	D3D12_VERTEX_BUFFER_VIEW CreateVertexBuffer(T* vertices, UINT vertexCount);
	D3D12_INDEX_BUFFER_VIEW CreateIndexBuffer(UINT* indices, UINT indiceCount);
	void DrawGeneralMesh(const Matrix* pMatrix);
	void DrawCubeMap(const Matrix* pMatrix);
	void DrawAnimation(const Matrix* pMatrix);
	Model();
	~Model();

private:
	ID3D12Device* m_device = nullptr;
	ID3D12GraphicsCommandList* m_commandList = nullptr;
	ID3D12CommandAllocator* m_commandAllocator = nullptr;
	ID3D12CommandQueue* m_commandQueue = nullptr;
	HANDLE m_fenceEvent;
	ID3D12Fence* m_fence = nullptr;
	UINT64* m_fenceValue = nullptr;

	UINT descriptorSize;

	//TriGroup
	D3D12_VERTEX_BUFFER_VIEW* m_vertexBufferView = nullptr;
	TRI_GROUP_PER_MTL* m_TriGroupList = nullptr;
	UINT m_materialNum = 0;
	UINT m_meshNum = 0;

	//MultiMaterial이 아닐 때, TriGroup을 사용하는게 아닌 IndexBuffer쓰임
	D3D12_INDEX_BUFFER_VIEW IndexBufferView;
	int indexCnt = 0;
	SRV_CONTAINER* m_srvContainer = nullptr;

	//MaterialConstant
	CBV_CONTAINER* materialContainer = nullptr;

	//animation
	bool existAnimation = false;
	CBV_CONTAINER* boneMatricesContainer = nullptr;
	Matrix* m_FinalBoneMatrices = nullptr;
	Animation* m_animations = nullptr;


	void UpdateMembers();
	void WaitForPreviousFrame();

	bool m_useNormalMap = false;

	//boneNode
	meshNode* rootNode = nullptr;


};