#include "pch.h"
#include "VertexStruct.h"
#include "Object.h"
#include "Renderer.h"

Renderer* Object::m_renderer = nullptr;
UINT Object::descriptorSize = 0;
Object::Object()
{
	m_device = m_renderer->GetDevice();
	m_commandList = m_renderer->GetCommandList();
	m_commandAllocator = m_renderer->GetCommandAllocator();
	m_cbvHeap = m_renderer->GetDscHeap();
	m_commandQueue = m_renderer->GetCommandQueue();
	m_pipelineState = m_renderer->GetPipeline();
	m_fence = m_renderer->GetFence();
	m_fenceEvent = m_renderer->GetFenceEvent();
	m_fenceValue = m_renderer->GetFenceValue();

}

void Object::OnInit(UINT ObjectCnt, Vertex* vertices, UINT vertexCount)
{
	m_ObjectCnt = ObjectCnt;
	if (m_ObjectCnt == 0)
	{
		descriptorSize= m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	CreateVertexBuffer(vertices, vertexCount);
	m_constantBufferData = {};
	CreateConstantBuffer();
}

void Object::CreateVertexBuffer(Vertex* vertices, UINT vertexCount)
{
	m_vertexCount = vertexCount;
	const UINT vertexBufferSize = sizeof(Vertex) * vertexCount;

	CD3DX12_HEAP_PROPERTIES defaultHeapProps(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);

	if (FAILED(m_device->CreateCommittedResource(
		&defaultHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&m_vertexBuffer)))) __debugbreak();

	ID3D12Resource* vertexBufferUpload = nullptr;
	CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
	if (FAILED(m_device->CreateCommittedResource(
		&uploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertexBufferUpload)))) __debugbreak();


	D3D12_SUBRESOURCE_DATA vertexData = {};
	vertexData.pData = vertices;
	vertexData.RowPitch = vertexBufferSize;
	vertexData.SlicePitch = vertexData.RowPitch;

	if (FAILED(m_commandAllocator->Reset())) __debugbreak();

	// However, when ExecuteCommandList() is called on a particular command 
	// list, that command list can then be reset at any time and must be before 
	// re-recording.
	if (FAILED(m_commandList->Reset(m_commandAllocator, m_pipelineState))) __debugbreak();

	UpdateSubresources(m_commandList, m_vertexBuffer, vertexBufferUpload, 0, 0, 1, &vertexData); //이거 하면 Map/UnMap할 필요없음

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		m_vertexBuffer,
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	m_commandList->ResourceBarrier(1, &barrier);
	if (FAILED(m_commandList->Close())) __debugbreak();

	ID3D12CommandList* ppCommandLists[] = { m_commandList };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	WaitForPreviousFrame();

	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(Vertex);
	m_vertexBufferView.SizeInBytes = vertexBufferSize;

	if (vertexBufferUpload)
	{
		vertexBufferUpload->Release();
		vertexBufferUpload = nullptr;
	}

}

void Object::CreateConstantBuffer()
{
	const UINT constantBufferSize = sizeof(SceneConstantBuffer);    // CB size is required to be 256-byte aligned.

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize);

	if (FAILED(m_device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_constantBuffer))))
	{
		__debugbreak();
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
	cbvHandle.Offset(m_ObjectCnt, descriptorSize);

	// Describe and create a constant buffer view.
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = m_constantBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = constantBufferSize;
	m_device->CreateConstantBufferView(&cbvDesc, cbvHandle);

	// Map and initialize the constant buffer. We don't unmap this until the
	// app closes. Keeping things mapped for the lifetime of the resource is okay.
	CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
	if (FAILED(m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pCbvDataBegin)))) __debugbreak();
	memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));
}
void Object::Render()
{
	CD3DX12_GPU_DESCRIPTOR_HANDLE cbvGpuHandle(m_cbvHeap->GetGPUDescriptorHandleForHeapStart());
	cbvGpuHandle.Offset(m_ObjectCnt, descriptorSize);
	m_commandList->SetGraphicsRootDescriptorTable(0, cbvGpuHandle);
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	m_commandList->DrawInstanced(m_vertexCount, 1, 0, 0);
}
void Object::WaitForPreviousFrame()
{
	// Signal and increment the fence value.
	const UINT64 fence = *m_fenceValue;
	if (FAILED(m_commandQueue->Signal(m_fence, fence))) __debugbreak();
	(*m_fenceValue)++;

	// Wait until the previous frame is finished.
	if (m_fence->GetCompletedValue() < fence)
	{
		if (FAILED(m_fence->SetEventOnCompletion(fence, m_fenceEvent))) __debugbreak();
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

}
Object::~Object()
{
	//WaitForPreviousFrame(); gpu올라오기 전에 종료해버린다면 의미가 잇을수도
	if (m_vertexBuffer)
	{
		m_vertexBuffer->Release();
		m_vertexBuffer = nullptr;
	}
	if (m_constantBuffer)
	{
		m_constantBuffer->Release();
		m_constantBuffer = nullptr;
	}
}