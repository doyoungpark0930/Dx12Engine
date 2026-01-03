#pragma once

enum RENDER_ITEM_TYPE
{
	RENDER_ITEM_TYPE_MESH_ANIMATION,
	RENDER_ITEM_TYPE_MESH_GENERAL
};



struct RENDER_ITEM
{
	RENDER_ITEM_TYPE Type;
	void* pObjHandle;
	Matrix matWorld;
	PASS_STATE passState;
};

class Renderer;
class CommandListPool;
class RenderQueue
{
	Renderer* m_pRenderer = nullptr;
	UINT8* m_pBuffer = nullptr;

	UINT m_MaxBufferSize = 0;
	UINT m_AllocatedSize = 0;
	UINT m_ReadBufferPos = 0;

	BoundingFrustum* frustum;
	BoundingBox worldAABB;

	const RENDER_ITEM* Dispatch();
	void	Cleanup();

public:
	BOOL Initialize(Renderer* pRenderer, UINT dwMaxItemNum);
	BOOL Add(const RENDER_ITEM* pItem);
	void Process(UINT threadIndex, CommandListPool* pCommandListPool, ID3D12CommandQueue* pCommandQueue, UINT processCountPerCommandList);
	void Reset();

	RenderQueue();
	~RenderQueue();
};