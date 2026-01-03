#pragma once

class Model;
class DescriptorPool;
class SrvManager;
class CbvManager;
class Animator;
class RenderQueue;
class CommandListPool;
struct RENDER_THREAD_DESC;


class Renderer
{
public:
	void OnInit();
	void Update(float dt);
	void Render();
	void ObjectRender();
	void RenderMeshAnimation(void* pMeshObjHandle, const Matrix* pMatWorld, PASS_STATE passState);
	void RenderMeshGeneral(void* pMeshObjHandle, const Matrix* pMatWorld);

	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;
	D3D12_VIEWPORT m_shadowViewport;
	D3D12_RECT m_shadowScissorRect;


	UINT GetWidth() const { return clientWidth; }
	UINT GetHeight() const { return clientHeight; }
	FLOAT GetAspect() const { return aspect; }
	UINT GetContextIndex() const { return m_curContextIndex; }

	ID3D12Device* GetDevice() const { return m_device; }
	ID3D12CommandQueue* GetCommandQueue() const { return m_commandQueue; }
	DescriptorPool* GetDescriptorPool(UINT threadIndex) const { return m_ppDescriptorPool[m_curContextIndex][threadIndex]; }
	ID3D12RootSignature* GetRootSignatureGeneral() const { return m_rootSignature_General; }
	ID3D12RootSignature* GetRootSignatureCubeMap() const { return m_rootSignature_CubeMap; }
	ID3D12PipelineState* GetPsoAnimation() const { return m_animationPSO; }
	ID3D12PipelineState* GetPsoCubeMap() const { return m_cubeMapPSO; }
	ID3D12PipelineState* GetPsoGeneral() const { return m_GeneralPSO; }
	ID3D12PipelineState* GetPsoDepthOnlyGeneral() const { return m_DepthOnlyGeneralPSO; }
	ID3D12PipelineState* GetPsoDepthOnlyAnimation() const { return m_DepthOnlyAnimationPSO; }

	SRV_CONTAINER GetShadowSrvContainer() const { return m_shadowMapSrvContainer; }

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetDsvHeap() const { 
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(
			m_dsvHeap->GetCPUDescriptorHandleForHeapStart()
		);
	}
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRtvHeap() const { 
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(
			m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
			m_frameIndex,
			m_rtvDescriptorSize
		);
	}

	
	BoundingFrustum* GetFrustum(){ return &frustum; }

	ID3D12Resource* m_vsBufferPool = nullptr;
	ID3D12Resource* m_vsUploadBufferPool = nullptr;
	UINT8* m_vsBegin = nullptr;    // starting position of upload buffer
	UINT8* m_vsCur = nullptr;      // current position of upload buffer
	UINT8* m_vsEnd = nullptr;      // ending position of upload buffer

	ID3D12Resource* m_indexBufferPool = nullptr;
	ID3D12Resource* m_indexUploadBufferPool = nullptr;
	UINT8* m_indexBegin = nullptr;    // starting position of upload buffer
	UINT8* m_indexCur = nullptr;      // current position of upload buffer
	UINT8* m_indexEnd = nullptr;


	void ProcessByThread(UINT threadIndex);
	void SetMainViewport();
	void SetShadowViewport();

	Renderer(UINT width, UINT height);
	~Renderer();

private:

	void CreateRootSignature();
	void CreatePipelineState();
	void CreateDepthStencil();
	void CreateObjects();
	void Create_Vertex_Index();
	void CreateModels();
	void GlobalConstantUpdate(int contextIndex);
	void GlobalShadowFrustumUpdate(int contextIndex);

	UINT clientWidth;
	UINT clientHeight;

	int m_shadowWidth = 1280;
	int m_shadowHeight = 1280;


	// Pipeline objects.
	IDXGISwapChain3* m_swapChain = nullptr;
	ID3D12Device* m_device = nullptr;
	ID3D12Resource* m_depthStencil = nullptr;
	ID3D12CommandQueue* m_commandQueue = nullptr;
	UINT m_frameIndex;
	UINT m_rtvDescriptorSize;

	ID3D12Resource* m_renderTargets[SWAP_CHAIN_FRAME_COUNT] = {};
	CommandListPool* m_ppCommandListPool[MAX_PENDING_FRAME_COUNT][MAX_RENDER_THREAD_COUNT] = {};
	DescriptorPool* m_ppDescriptorPool[MAX_PENDING_FRAME_COUNT][MAX_RENDER_THREAD_COUNT] = {};

	RenderQueue* m_ppRenderQueue[MAX_RENDER_THREAD_COUNT] = {};
	UINT m_renderThreadCount = 0;
	UINT m_curThreadIndex = 0;

	LONG volatile m_lActiveThreadCount = 0;
	HANDLE m_hCompleteEvent = nullptr;
	RENDER_THREAD_DESC* m_pThreadDescList = nullptr;

	ID3D12RootSignature* m_rootSignature_General = nullptr;
	ID3D12RootSignature* m_rootSignature_CubeMap = nullptr;
	ID3D12DescriptorHeap* m_rtvHeap = nullptr;
	ID3D12DescriptorHeap* m_dsvHeap = nullptr;
	ID3D12PipelineState* m_animationPSO = nullptr;
	ID3D12PipelineState* m_cubeMapPSO = nullptr;
	ID3D12PipelineState* m_GeneralPSO = nullptr;
	ID3D12PipelineState* m_DepthOnlyGeneralPSO = nullptr;
	ID3D12PipelineState* m_DepthOnlyAnimationPSO = nullptr;
	SrvManager* m_srvManager = nullptr;
	CbvManager* m_cbvManager = nullptr;
	SRV_CONTAINER m_shadowMapSrvContainer;

	// Synchronization objects.
	HANDLE m_fenceEvent;
	ID3D12Fence* m_fence;
	UINT64	m_pui64LastFenceValue[MAX_PENDING_FRAME_COUNT] = {};
	UINT64 m_fenceValue;
	UINT m_curContextIndex = 0;

	Model* m_Models = nullptr;
	ObjectState* m_ObjectState = nullptr;
	UINT maxObjectsNum = 256; //최대 오브젝트의 수
	UINT ObjectCnt = -1;  //오브젝트가 몇 번째로 생성되었는지

	Matrix View;
	Matrix Proj;
	Matrix invView;

	Matrix shadowView;
	Matrix shadowProj;

	BoundingBox worldAABB;
	BoundingFrustum frustum;

	FLOAT aspect;

	Animator* m_animator = nullptr;
	Animation** m_animations = nullptr;

	Vector4 lightPos = Vector4(-80.0f, 40.0f, 80.0f, 1.0f);

	Vector3 shadowPos;
	Vector3 shadowDirection = Vector3(6.0f, -5.0f, -5.0f);

	Matrix objectMatrix[250] = {};

	bool IsActionKeyDown = false;
	char actionKey[32];

	void LoadAssets();
	void ShadowPass();
	void RenderPass();
	void Fence();
	void WaitForFenceValue(UINT64 ExpectedFenceValue);
	void InitRenderThreadPool(UINT threadCount);

};