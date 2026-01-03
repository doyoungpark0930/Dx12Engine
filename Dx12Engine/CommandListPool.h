#pragma once
#include "LinkedList.h"

struct COMMAND_LIST
{
	ID3D12CommandAllocator* pDirectCommandAllocator;
	ID3D12GraphicsCommandList* pDirectCommandList;
	SORT_LINK	Link;
	BOOL	bClosed;
};


class CommandListPool
{
	ID3D12Device* m_pD3DDevice = nullptr;
	D3D12_COMMAND_LIST_TYPE	m_CommnadListType = D3D12_COMMAND_LIST_TYPE_DIRECT;
	UINT	m_AllocatedCmdNum = 0;
	UINT	m_AvailableCmdNum = 0;
	UINT	m_TotalCmdNum = 0;
	UINT	m_MaxCmdListNum = 0;
	COMMAND_LIST* m_pCurCmdList = nullptr;
	SORT_LINK* m_pAlloatedCmdLinkHead = nullptr;
	SORT_LINK* m_pAlloatedCmdLinkTail = nullptr;
	SORT_LINK* m_pAvailableCmdLinkHead = nullptr;
	SORT_LINK* m_pAvailableCmdLinkTail = nullptr;

	BOOL	AddCmdList();
	COMMAND_LIST* AllocCmdList();
	void	Cleanup();
public:
	BOOL	Initialize(ID3D12Device* pDevice, D3D12_COMMAND_LIST_TYPE type, UINT MaxCommandListNum);
	ID3D12GraphicsCommandList* GetCurrentCommandList();
	void	Close();
	void	CloseAndExecute(ID3D12CommandQueue* pCommandQueue);
	void	Reset();

	UINT	GetTotalCmdListNum() const { return m_TotalCmdNum; }
	UINT	GetAllocatedCmdListNum() const { return m_AllocatedCmdNum; }
	UINT	GetAvailableCmdListNum() const { return m_AvailableCmdNum; }
	ID3D12Device* INL_GetD3DDevice() { return m_pD3DDevice; }


	CommandListPool();
	~CommandListPool();
};

