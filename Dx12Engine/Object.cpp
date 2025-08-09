#include "pch.h"
#include "VertexStruct.h"
#include "Object.h"
#include "Renderer.h"
#include "DXUtil.h"

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
//, Vertex* vertices, UINT vertexCount
void Object::OnInit(UINT ObjectCnt)
{
	m_ObjectCnt = ObjectCnt;
	if (m_ObjectCnt == 0)
	{
		descriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	m_constantBufferData = {};
}

void Object::CreateVertexBuffer(Vertex* vertices, UINT vertexCount)
{
	m_vertexCount = vertexCount;
	const UINT vertexBufferSize = sizeof(Vertex) * m_vertexCount;

	UINT verticesOffset = 0;
	if (FAILED(
		SetDataToUploadBuffer(
			&(m_renderer->m_vsCur),
			m_renderer->m_vsBegin,
			m_renderer->m_vsEnd,
			vertices, sizeof(Vertex), vertexCount,
			sizeof(float),
			verticesOffset
		)))__debugbreak();

	if (FAILED(m_commandAllocator->Reset())) __debugbreak();

	// However, when ExecuteCommandList() is called on a particular command 
	// list, that command list can then be reset at any time and must be before 
	// re-recording.
	if (FAILED(m_commandList->Reset(m_commandAllocator, m_pipelineState))) __debugbreak();

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderer->m_vsBufferPool, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);
	m_commandList->ResourceBarrier(1, &barrier);

	m_commandList->CopyBufferRegion(m_renderer->m_vsBufferPool, verticesOffset, m_renderer->m_vsUploadBufferPool, verticesOffset, vertexBufferSize);

	barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderer->m_vsBufferPool, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	m_commandList->ResourceBarrier(1, &barrier);

	if (FAILED(m_commandList->Close())) __debugbreak();

	ID3D12CommandList* ppCommandLists[] = { m_commandList };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	WaitForPreviousFrame();

	m_vertexBufferView.BufferLocation = m_renderer->m_vsBufferPool->GetGPUVirtualAddress() + verticesOffset;
	m_vertexBufferView.StrideInBytes = sizeof(Vertex);
	m_vertexBufferView.SizeInBytes = vertexBufferSize;

}

void Object::CreateConstantBuffer()
{
	const UINT constantBufferSize = sizeof(SceneConstantBuffer);    // CB size is required to be 256-byte aligned.

	if (FAILED(
		SetDataToUploadBuffer(
			&(m_renderer->m_constantCur),
			m_renderer->m_constantBegin,
			m_renderer->m_constantEnd,
			&m_constantBufferData, sizeof(SceneConstantBuffer), 1,
			D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT,
			constantOffset
		)))__debugbreak();

	CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
	cbvHandle.Offset(m_ObjectCnt, descriptorSize);

	// Describe and create a constant buffer view.
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = m_renderer->m_constantUploadBufferPool->GetGPUVirtualAddress() + constantOffset;
	cbvDesc.SizeInBytes = constantBufferSize;
	m_device->CreateConstantBufferView(&cbvDesc, cbvHandle);

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
}