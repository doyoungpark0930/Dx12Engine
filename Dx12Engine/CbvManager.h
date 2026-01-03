#pragma once

class Renderer;

class CbvManager
{
public:
	CbvManager() = default;
	ID3D12DescriptorHeap* m_descritorHeap[MAX_PENDING_FRAME_COUNT][MAX_RENDER_THREAD_COUNT] = {};
	ID3D12DescriptorHeap* m_materialDescritorHeap = nullptr;
	ID3D12DescriptorHeap* m_animationDescritorHeap = nullptr;
	void OnInit(ID3D12Device* pDevice, Renderer* pRenderer);
	void Reset(int contextIndex, int threadIndex);
	CBV_CONTAINER GetGlobalContainer(int contextIndex, int threadIndex) const { return m_cbvContainer[contextIndex][threadIndex][0]; }
	CBV_CONTAINER* GetAllocatedContainer(int contextIndex, int threadIndex);
	CBV_CONTAINER AllocMaterialCBV();
	CBV_CONTAINER* AllocAnimationMatrices();
	UINT8* GetStartCBV(int contextIndex, int threadIndex) const { return m_constantBegin[contextIndex][threadIndex]; }
	~CbvManager();



private:
	ID3D12Device* m_device = nullptr;
	Renderer* m_renderer = nullptr;
	CBV_CONTAINER* m_cbvContainer[MAX_PENDING_FRAME_COUNT][MAX_RENDER_THREAD_COUNT] = {};

	void CreateGeneralConstantBufferPool(int i, int j);
	void CreateAnimationBufferPool();
	void CreateMaterialBufferPool();

	UINT descriptorSize = 0;
	const UINT max_descriptorNum = 256;

	//global, model Constant
	ID3D12Resource* m_constantUploadBufferPool[MAX_PENDING_FRAME_COUNT][MAX_RENDER_THREAD_COUNT] = {};
	UINT8* m_constantBegin[MAX_PENDING_FRAME_COUNT][MAX_RENDER_THREAD_COUNT] = {};   // starting position of upload buffer
	UINT8* m_constantCur[MAX_PENDING_FRAME_COUNT][MAX_RENDER_THREAD_COUNT] = {};      // current position of upload buffer
	UINT8* m_constantEnd[MAX_PENDING_FRAME_COUNT][MAX_RENDER_THREAD_COUNT] = {};      // ending position of upload buffer
	UINT allocatedCbvNum[MAX_PENDING_FRAME_COUNT][MAX_RENDER_THREAD_COUNT] = {};

	//Material Constant
	ID3D12Resource* m_materialConstantUploadBufferPool = nullptr;
	CD3DX12_CPU_DESCRIPTOR_HANDLE materialHandle;
	const UINT max_materialNum = 128;
	CBV_CONTAINER* m_materialContainer = nullptr;
	UINT m_materialContainerCnt = 0;
	UINT8* m_materialConstantBegin = nullptr;
	UINT8* m_materialConstantCur = nullptr;
	UINT8* m_materialConstantEnd = nullptr;


	//Animation Matrices
	ID3D12Resource* m_animationConstantUploadBufferPool = nullptr;
	CD3DX12_CPU_DESCRIPTOR_HANDLE animationHandle;
	const UINT max_animationNum = 32;
	CBV_CONTAINER* m_animationContainer = nullptr;
	UINT m_animationContainerCnt = 0;
	UINT8* m_animationConstantBegin = nullptr;  
	UINT8* m_animationConstantCur = nullptr;     
	UINT8* m_animationConstantEnd = nullptr;    
};