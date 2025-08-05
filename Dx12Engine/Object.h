#pragma once

class Renderer;

class Object
{
public :
	static Renderer* m_renderer;
	void Render();
	void CreateVertexBuffer(Vertex* vertices, UINT vertexCount);
	Object();
	~Object();

private:
	ID3D12Resource* m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	UINT m_vertexCount;

	ID3D12Device* m_device; 
	ID3D12GraphicsCommandList* m_commandList;

};