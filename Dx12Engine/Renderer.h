#pragma once

class Object;

class Renderer
{
public:
	void OnInit();
	void OnUpdate();
	void OnRender();


	UINT GetWidth() const { return clientWidth; }
	UINT GetHeight() const { return clientHeight; }

	ID3D12Device* GetDevice() const { return m_device; }
	ID3D12GraphicsCommandList* GetCommandList() const { return m_commandList; }
	ID3D12CommandAllocator* GetCommandAllocator() const { return m_commandAllocator; }
	ID3D12CommandQueue* GetCommandQueue() const { return m_commandQueue; }
	ID3D12PipelineState* GetPipeline() const { return m_pipelineState; }
	ID3D12DescriptorHeap* GetDscHeap() const { return m_cbvHeap; }
	ID3D12Fence* GetFence() const { return m_fence; }
	HANDLE GetFenceEvent() const { return m_fenceEvent; }
	UINT64* GetFenceValue() { return &m_fenceValue; }
	UINT GetObjectNum() const { return ObjectsNum; }

	ID3D12Resource* m_vsBufferPool;
	ID3D12Resource* m_vsUploadBufferPool;
	UINT8* m_vsBegin = nullptr;    // starting position of upload buffer
	UINT8* m_vsCur = nullptr;      // current position of upload buffer
	UINT8* m_vsEnd = nullptr;      // ending position of upload buffer

	ID3D12Resource* m_constantUploadBufferPool;
	UINT8* m_constantBegin = nullptr;    // starting position of upload buffer
	UINT8* m_constantCur = nullptr;      // current position of upload buffer
	UINT8* m_constantEnd = nullptr;      // ending position of upload buffer

	Renderer(UINT width, UINT height);
	~Renderer();

private:
	static const UINT FrameCount = 2;

	UINT clientWidth;
	UINT clientHeight;


	// Pipeline objects.
	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;
	IDXGISwapChain3* m_swapChain;
	ID3D12Device* m_device;
	ID3D12Resource* m_renderTargets[FrameCount] = {};
	ID3D12CommandAllocator* m_commandAllocator;
	ID3D12CommandQueue* m_commandQueue;
	ID3D12RootSignature* m_rootSignature;
	ID3D12DescriptorHeap* m_rtvHeap;
	ID3D12DescriptorHeap* m_cbvHeap;
	ID3D12PipelineState* m_pipelineState;
	ID3D12GraphicsCommandList* m_commandList;
	UINT m_rtvDescriptorSize;


	// Synchronization objects.
	UINT m_frameIndex;
	HANDLE m_fenceEvent;
	ID3D12Fence* m_fence;
	UINT64 m_fenceValue;

	Object* m_Objects;
	UINT ObjectsNum = 2; //오브젝트의 수
	UINT ObjectCnt = -1;  //오브젝트가 몇 번째로 생성되었는지



	void LoadAssets();
	void PopulateCommandList();
	void WaitForPreviousFrame();

};