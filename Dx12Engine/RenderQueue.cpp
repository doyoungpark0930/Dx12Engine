#include "pch.h"
#include "Model.h"
#include "Renderer.h"
#include "CommandListPool.h"
#include "RenderQueue.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

RenderQueue::RenderQueue()
{
}
BOOL RenderQueue::Initialize(Renderer* pRenderer, UINT MaxItemNum)
{
	m_pRenderer = pRenderer;
	m_MaxBufferSize = sizeof(RENDER_ITEM) * MaxItemNum;
	m_pBuffer = (UINT8*)malloc(m_MaxBufferSize);
	memset(m_pBuffer, 0, m_MaxBufferSize);

	return TRUE;
}
BOOL RenderQueue::Add(const RENDER_ITEM* pItem)
{
	BOOL bResult = FALSE;
	if (m_AllocatedSize + sizeof(RENDER_ITEM) > m_MaxBufferSize)
	{
		return false;
	}
	else
	{
		UINT8* pDest = m_pBuffer + m_AllocatedSize;
		memcpy(pDest, pItem, sizeof(RENDER_ITEM));
		m_AllocatedSize += sizeof(RENDER_ITEM);
		return true;
	}

}
const RENDER_ITEM* RenderQueue::Dispatch()
{
	const RENDER_ITEM* pItem = nullptr;
	if (m_ReadBufferPos + sizeof(RENDER_ITEM) > m_AllocatedSize)
	{
		return pItem;
	}
	else
	{
		pItem = (const RENDER_ITEM*)(m_pBuffer + m_ReadBufferPos);
		m_ReadBufferPos += sizeof(RENDER_ITEM);
		return pItem;
	}

}
void RenderQueue::Process(UINT threadIndex, CommandListPool* pCommandListPool, ID3D12CommandQueue* pCommandQueue, UINT processCountPerCommandList)
{
	ID3D12Device* pD3DDevice = m_pRenderer->GetDevice();
	frustum = m_pRenderer->GetFrustum();
	ID3D12GraphicsCommandList* ppCommandList[64] = {};
	UINT	commandListCount = 0;

	ID3D12GraphicsCommandList* pCommandList = nullptr;
	UINT processedCountPerCommandList = 0;
	BOOL isShadowPass = false;

	const RENDER_ITEM* pItem = nullptr;
	bool preBind = false;
	PASS_STATE prePassState = SHADOW_PASS;
	while (pItem = Dispatch())
	{
		if (prePassState != pItem->passState || processedCountPerCommandList > processCountPerCommandList)
		{
			pCommandListPool->Close();
			ppCommandList[commandListCount] = pCommandList;
			commandListCount++;
			pCommandList = nullptr;
			processedCountPerCommandList = 0;
			preBind = false;
		}
		pCommandList = pCommandListPool->GetCurrentCommandList();

		Model* pMeshObj = (Model*)pItem->pObjHandle;
		if (!preBind)
		{
			if (pItem->passState == RENDER_PASS) pMeshObj->PreBinding(threadIndex, pCommandList, RENDER_PASS);
			else pMeshObj->PreBinding(threadIndex, pCommandList, SHADOW_PASS);
			preBind = true;
		}

		if (pItem->passState == RENDER_PASS)
		{
			pCommandList->SetPipelineState(m_pRenderer->GetPsoAnimation());
			pCommandList->SetGraphicsRootSignature(m_pRenderer->GetRootSignatureGeneral());
			pCommandList->RSSetViewports(1, &m_pRenderer->m_viewport);
			pCommandList->RSSetScissorRects(1, &m_pRenderer->m_scissorRect);
			auto rtv = m_pRenderer->GetRtvHeap();
			auto dsv = m_pRenderer->GetDsvHeap();

			pCommandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
		}
		else
		{
			pCommandList->SetPipelineState(m_pRenderer->GetPsoDepthOnlyAnimation());
			pCommandList->SetGraphicsRootSignature(m_pRenderer->GetRootSignatureGeneral());
			pCommandList->RSSetViewports(1, &m_pRenderer->m_shadowViewport);
			pCommandList->RSSetScissorRects(1, &m_pRenderer->m_shadowScissorRect);
			auto dsv = m_pRenderer->GetShadowSrvContainer().dsvHandle;
			pCommandList->OMSetRenderTargets(0, nullptr, FALSE, &dsv);
		}
		switch (pItem->Type)
		{
		case RENDER_ITEM_TYPE_MESH_ANIMATION:
		{
			pMeshObj->localAABB.Transform(worldAABB, pItem->matWorld);
			if ((*frustum).Intersects(worldAABB))
			{
				pMeshObj->DrawAnimation(threadIndex, pCommandList, &pItem->matWorld);
			}
		}
		break;
		case RENDER_ITEM_TYPE_MESH_GENERAL:
		{
			pMeshObj->DrawGeneralMesh(threadIndex, pCommandList, &pItem->matWorld);
		}
		break;

		default:
			__debugbreak();
		}

		processedCountPerCommandList++;

		prePassState = pItem->passState;

	}

	// 남은 렌더링아이템 처리
	if (processedCountPerCommandList)
	{
		pCommandListPool->Close();
		ppCommandList[commandListCount] = pCommandList;
		commandListCount++;
		pCommandList = nullptr;
		processedCountPerCommandList = 0;
	}

	if (commandListCount)
	{
		pCommandQueue->ExecuteCommandLists(commandListCount, (ID3D12CommandList**)ppCommandList);
	}


}
void RenderQueue::Reset()
{
	m_AllocatedSize = 0;
	m_ReadBufferPos = 0;
}
void RenderQueue::Cleanup()
{
	if (m_pBuffer)
	{
		free(m_pBuffer);
		m_pBuffer = nullptr;
	}
}
RenderQueue::~RenderQueue()
{
	Cleanup();
}
