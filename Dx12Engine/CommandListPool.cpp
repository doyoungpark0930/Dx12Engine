#include "pch.h"
#include "LinkedList.h"
#include "CommandListPool.h"


CommandListPool::CommandListPool()
{
}

BOOL CommandListPool::Initialize(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE type, UINT MaxCommandListNum)
{
	if (MaxCommandListNum < 2)
		__debugbreak();

	m_MaxCmdListNum = MaxCommandListNum;

	m_pD3DDevice = pDevice;

	return TRUE;
}
BOOL CommandListPool::AddCmdList()
{
	BOOL	bResult = FALSE;
	COMMAND_LIST* pCmdList = nullptr;

	ID3D12CommandAllocator* pDirectCommandAllocator = nullptr;
	ID3D12GraphicsCommandList* pDirectCommandList = nullptr;

	if (m_TotalCmdNum >= m_MaxCmdListNum)
	{
#ifdef _DEBUG
		__debugbreak();
#endif
		goto lb_return;
	}

	if (FAILED(m_pD3DDevice->CreateCommandAllocator(m_CommnadListType, IID_PPV_ARGS(&pDirectCommandAllocator))))
	{
#ifdef _DEBUG
		__debugbreak();
#endif
		goto lb_return;
	}

	if (FAILED(m_pD3DDevice->CreateCommandList(0, m_CommnadListType, pDirectCommandAllocator, nullptr, IID_PPV_ARGS(&pDirectCommandList))))
	{
#ifdef _DEBUG
		__debugbreak();
#endif
		pDirectCommandAllocator->Release();
		pDirectCommandAllocator = nullptr;
		goto lb_return;

	}
	pCmdList = new COMMAND_LIST;
	memset(pCmdList, 0, sizeof(COMMAND_LIST));
	pCmdList->Link.pItem = pCmdList;
	pCmdList->pDirectCommandList = pDirectCommandList;
	pCmdList->pDirectCommandAllocator = pDirectCommandAllocator;
	m_TotalCmdNum++;

	LinkToLinkedListFIFO(&m_pAvailableCmdLinkHead, &m_pAvailableCmdLinkTail, &pCmdList->Link);
	m_AvailableCmdNum++;
	bResult = TRUE;
lb_return:
	return bResult;
}

COMMAND_LIST* CommandListPool::AllocCmdList()
{
	COMMAND_LIST* pCmdList = nullptr;

	if (!m_pAvailableCmdLinkHead)
	{
		if (!AddCmdList())
		{
			goto lb_return;
		}
	}

	pCmdList = (COMMAND_LIST*)m_pAvailableCmdLinkHead->pItem;

	UnLinkFromLinkedList(&m_pAvailableCmdLinkHead, &m_pAvailableCmdLinkTail, &pCmdList->Link);
	m_AvailableCmdNum--;

	LinkToLinkedListFIFO(&m_pAlloatedCmdLinkHead, &m_pAlloatedCmdLinkTail, &pCmdList->Link);
	m_AllocatedCmdNum++;

lb_return:
	return pCmdList;
}

ID3D12GraphicsCommandList* CommandListPool::GetCurrentCommandList()
{
	ID3D12GraphicsCommandList* pCommandList = nullptr;
	if (!m_pCurCmdList)
	{
		m_pCurCmdList = AllocCmdList();
		if (!m_pCurCmdList)
		{
			__debugbreak();

		}
	}
	return m_pCurCmdList->pDirectCommandList;
}

void CommandListPool::Close()
{
	// 현재 인덱스의 CommandList를 실행
	if (!m_pCurCmdList)
		__debugbreak();

	if (m_pCurCmdList->bClosed)
		__debugbreak();

	if (FAILED(m_pCurCmdList->pDirectCommandList->Close()))
		__debugbreak();

	m_pCurCmdList->bClosed = TRUE;
	m_pCurCmdList = nullptr;
}
void CommandListPool::CloseAndExecute(ID3D12CommandQueue* pCommandQueue)
{
	// 현재 인덱스의 CommandList를 실행
	if (!m_pCurCmdList)
		__debugbreak();

	if (m_pCurCmdList->bClosed)
		__debugbreak();

	if (FAILED(m_pCurCmdList->pDirectCommandList->Close()))
		__debugbreak();

	m_pCurCmdList->bClosed = TRUE;

	pCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList**)&m_pCurCmdList->pDirectCommandList);
	m_pCurCmdList = nullptr;
}

void CommandListPool::Reset()
{
	while (m_pAlloatedCmdLinkHead)
	{
		COMMAND_LIST* pCmdList = (COMMAND_LIST*)m_pAlloatedCmdLinkHead->pItem;

		if (FAILED(pCmdList->pDirectCommandAllocator->Reset()))
			__debugbreak();

		if (FAILED(pCmdList->pDirectCommandList->Reset(pCmdList->pDirectCommandAllocator, nullptr)))
			__debugbreak();

		pCmdList->bClosed = FALSE;

		UnLinkFromLinkedList(&m_pAlloatedCmdLinkHead, &m_pAlloatedCmdLinkTail, &pCmdList->Link);
		m_AllocatedCmdNum--;

		LinkToLinkedListFIFO(&m_pAvailableCmdLinkHead, &m_pAvailableCmdLinkTail, &pCmdList->Link);
		m_AvailableCmdNum++;
	}
}

void CommandListPool::Cleanup()
{
	Reset();

	while (m_pAvailableCmdLinkHead)
	{
		COMMAND_LIST* pCmdList = (COMMAND_LIST*)m_pAvailableCmdLinkHead->pItem;
		pCmdList->pDirectCommandList->Release();
		pCmdList->pDirectCommandList = nullptr;

		pCmdList->pDirectCommandAllocator->Release();
		pCmdList->pDirectCommandAllocator = nullptr;
		m_TotalCmdNum--;

		UnLinkFromLinkedList(&m_pAvailableCmdLinkHead, &m_pAvailableCmdLinkTail, &pCmdList->Link);
		m_AvailableCmdNum--;

		delete pCmdList;
	}
}
CommandListPool::~CommandListPool()
{
	Cleanup();
}