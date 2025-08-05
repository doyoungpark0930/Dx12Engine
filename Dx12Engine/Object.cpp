#include "pch.h"
#include "VertexStruct.h"
#include "Object.h"
#include "Renderer.h"

Renderer* Object::m_renderer = nullptr;
Object::Object()
{
	m_device = m_renderer->GetDevice();
	m_commandList = m_renderer->GetCommandList();
}
void Object::CreateVertexBuffer(Vertex* vertices, UINT vertexCount)
{
	m_vertexCount = vertexCount;
	const UINT vertexBufferSize = sizeof(Vertex) * vertexCount;

	// Note: using upload heaps to transfer static data like vert buffers is not
	// recommended. Every time the GPU needs it, the upload heap will be marshalled
	// over. Please read up on Default Heap usage. An upload heap is used here for
	// code simplicity and because there are very few verts to actually transfer.
	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
	if (FAILED(m_device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_vertexBuffer)))) __debugbreak();

	// Copy the triangle data to the vertex buffer.
	UINT8* pVertexDataBegin;
	CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
	if (FAILED(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)))) __debugbreak();
	memcpy(pVertexDataBegin, vertices, vertexBufferSize);
	m_vertexBuffer->Unmap(0, nullptr);

	// Initialize the vertex buffer view.
	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes = sizeof(Vertex);
	m_vertexBufferView.SizeInBytes = vertexBufferSize;

}
void Object::Render()
{
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	m_commandList->DrawInstanced(m_vertexCount, 1, 0, 0);
}

Object::~Object()
{
	if (m_vertexBuffer)
	{
		m_vertexBuffer->Release();
		m_vertexBuffer = nullptr;
	}
}