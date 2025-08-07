#pragma once

class Renderer;

class Object
{
public:
	static Renderer* m_renderer;
	void Render();
	void CreateVertexBuffer(Vertex* vertices, UINT vertexCount);
	void CreateConstantBuffer();
	void OnInit(UINT ObjectCnt, Vertex* vertices, UINT vertexCount);
	Object();
	~Object();

private:
	//Object Resource
	ID3D12Resource* m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	UINT m_vertexCount;

	ID3D12Resource* m_constantBuffer;

	ID3D12Device* m_device = nullptr;
	ID3D12GraphicsCommandList* m_commandList;
	ID3D12DescriptorHeap* m_cbvHeap;
	ID3D12CommandAllocator* m_commandAllocator;
	ID3D12CommandQueue* m_commandQueue;
	ID3D12PipelineState* m_pipelineState;
	HANDLE m_fenceEvent;
	ID3D12Fence* m_fence;
	UINT64* m_fenceValue;



	void WaitForPreviousFrame();

public:
	UINT m_ObjectCnt = -1;
	UINT8* m_pCbvDataBegin;
	SceneConstantBuffer m_constantBufferData;

	static UINT descriptorSize;


};