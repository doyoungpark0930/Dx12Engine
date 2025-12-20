#include "pch.h"
#include "DXUtil.h"
#include "Renderer.h"
#include "CbvManager.h"
#include "DXHelper.h"


void CbvManager::OnInit(ID3D12Device* pDevice, Renderer* pRenderer)
{
	m_device = pDevice;
	m_renderer = pRenderer;
	descriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	for(int i = 0 ; i < MAX_PENDING_FRAME_COUNT ; i++) //반드시 정리하기
	{
		CreateGeneralConstantBufferPool(i);
	}

	//MaterialCBV Init
	{
		m_materialContainer = new CBV_CONTAINER[max_materialNum];

		D3D12_DESCRIPTOR_HEAP_DESC materialHeapDesc = {};
		materialHeapDesc.NumDescriptors = max_materialNum;
		materialHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		materialHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		if (FAILED(m_device->CreateDescriptorHeap(&materialHeapDesc, IID_PPV_ARGS(&m_materialDescritorHeap)))) __debugbreak();

		materialHandle = m_materialDescritorHeap->GetCPUDescriptorHandleForHeapStart();

		CreateMaterialBufferPool();
	}

	//Animation Init
	{
		m_animationContainer = new CBV_CONTAINER[max_animationNum];

		D3D12_DESCRIPTOR_HEAP_DESC aniHeapDesc = {};
		aniHeapDesc.NumDescriptors = max_animationNum;
		aniHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		aniHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		if (FAILED(m_device->CreateDescriptorHeap(&aniHeapDesc, IID_PPV_ARGS(&m_animationDescritorHeap)))) __debugbreak();

		animationHandle = m_animationDescritorHeap->GetCPUDescriptorHandleForHeapStart();

		CreateAnimationBufferPool();
	}
}

void CbvManager::CreateGeneralConstantBufferPool(int i)
{
	m_cbvContainer[i] = new CBV_CONTAINER[max_descriptorNum + 1];

	//globalConstant / constant1 / constant2 / ..
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
	cbvHeapDesc.NumDescriptors = 1 + max_descriptorNum; //1은 globalConstant자리
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	if (FAILED(m_device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_descritorHeap[i])))) __debugbreak();

	UINT constantMemory = Align(sizeof(GLOBAL_CONSTANT), 256) + Align(sizeof(MODEL_CONSTANT), 256) * max_descriptorNum;
	CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(constantMemory);

	if (FAILED(m_device->CreateCommittedResource(
		&uploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_constantUploadBufferPool[i]))))__debugbreak();

	void* pData;
	CD3DX12_RANGE readRange(0, 0);
	m_constantUploadBufferPool[i]->Map(0, &readRange, &pData);
	m_constantCur[i] = m_constantBegin[i] = reinterpret_cast<UINT8*>(pData);
	m_constantEnd[i] = m_constantBegin[i] + constantMemory;

	UINT CbvCnt = 0;
	//첫 디스크립터 자리는 globalConstant자리
	UINT globalOffset = 0;
	if (FAILED(
		SetDataToUploadBuffer(
			&m_constantCur[i],
			m_constantBegin[i],
			m_constantEnd[i],
			nullptr, sizeof(GLOBAL_CONSTANT), 1,
			256,
			globalOffset
		)))__debugbreak();

	CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(m_descritorHeap[i]->GetCPUDescriptorHandleForHeapStart());

	D3D12_CONSTANT_BUFFER_VIEW_DESC globalDesc = {};
	globalDesc.BufferLocation = m_constantUploadBufferPool[i]->GetGPUVirtualAddress() + globalOffset;
	globalDesc.SizeInBytes = Align(sizeof(GLOBAL_CONSTANT), 256);
	m_device->CreateConstantBufferView(&globalDesc, cbvHandle);

	m_cbvContainer[i][0].pSystemMemAddr = m_constantBegin[i] + globalOffset;
	m_cbvContainer[i][0].CBVHandle = cbvHandle;

	cbvHandle.Offset(1, descriptorSize);
	CbvCnt = 1;

	//modelConstant
	for (int j = 0; j < max_descriptorNum; j++)
	{
		UINT constantOffset = 0;
		if (FAILED(
			SetDataToUploadBuffer(
				&m_constantCur[i],
				m_constantBegin[i],
				m_constantEnd[i],
				nullptr, sizeof(MODEL_CONSTANT), 1,
				256,
				constantOffset
			)))__debugbreak();

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = m_constantUploadBufferPool[i]->GetGPUVirtualAddress() + constantOffset;
		cbvDesc.SizeInBytes = Align(sizeof(MODEL_CONSTANT), 256);
		m_device->CreateConstantBufferView(&cbvDesc, cbvHandle);

		m_cbvContainer[i][j + 1].pSystemMemAddr = m_constantBegin[i] + constantOffset;
		m_cbvContainer[i][j + 1].CBVHandle = cbvHandle;

		cbvHandle.Offset(1, descriptorSize);
		CbvCnt++;
	}
}

CBV_CONTAINER* CbvManager::GetAllocatedContainer(int contextIndex)
{
	return &m_cbvContainer[contextIndex][allocatedCbvNum[contextIndex]++];
}


void CbvManager::Reset(int contextIndex)
{
	allocatedCbvNum[contextIndex] = 1; //첫 디스크립터 자리 globalConstant
}

void CbvManager::CreateMaterialBufferPool()
{
	UINT constantMemory = Align(sizeof(MATERIAL_CONSTANT), 256) * max_materialNum;

	CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(constantMemory);

	if (FAILED(m_device->CreateCommittedResource(
		&uploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_materialConstantUploadBufferPool)))) __debugbreak();

	void* pData;
	CD3DX12_RANGE readRange(0, 0);
	m_materialConstantUploadBufferPool->Map(0, &readRange, &pData);
	m_materialConstantCur = m_materialConstantBegin = reinterpret_cast<UINT8*>(pData);
	m_materialConstantEnd = m_materialConstantBegin + constantMemory;
}

CBV_CONTAINER CbvManager::AllocMaterialCBV()
{
	UINT constantOffset = 0;
	if (FAILED(
		SetDataToUploadBuffer(
			&m_materialConstantCur,
			m_materialConstantBegin,
			m_materialConstantEnd,
			nullptr, sizeof(MATERIAL_CONSTANT), 1,
			256,
			constantOffset
		)))__debugbreak();

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = m_materialConstantUploadBufferPool->GetGPUVirtualAddress() + constantOffset;
	cbvDesc.SizeInBytes = Align(sizeof(MATERIAL_CONSTANT), 256);

	m_device->CreateConstantBufferView(&cbvDesc, materialHandle);

	m_materialContainer[m_materialContainerCnt].pSystemMemAddr = m_materialConstantBegin + constantOffset;
	m_materialContainer[m_materialContainerCnt].CBVHandle = materialHandle;

	materialHandle.Offset(1, descriptorSize);

	return m_materialContainer[m_materialContainerCnt++];

}


void CbvManager::CreateAnimationBufferPool()
{
	UINT constantMemory = Align(sizeof(SkinnedConstants), 256) * max_animationNum;

	CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(constantMemory);

	if (FAILED(m_device->CreateCommittedResource(
		&uploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_animationConstantUploadBufferPool)))) __debugbreak();

	void* pData;
	CD3DX12_RANGE readRange(0, 0);
	m_animationConstantUploadBufferPool->Map(0, &readRange, &pData);
	m_animationConstantCur = m_animationConstantBegin = reinterpret_cast<UINT8*>(pData);
	m_animationConstantEnd = m_animationConstantBegin + constantMemory;

}

CBV_CONTAINER* CbvManager::AllocAnimationMatrices()
{
	UINT constantOffset = 0;
	if (FAILED(
		SetDataToUploadBuffer(
			&m_animationConstantCur,
			m_animationConstantBegin,
			m_animationConstantEnd,
			nullptr, sizeof(SkinnedConstants), 1,
			256,
			constantOffset
		)))__debugbreak();

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = m_animationConstantUploadBufferPool->GetGPUVirtualAddress() + constantOffset;
	cbvDesc.SizeInBytes = Align(sizeof(SkinnedConstants), 256);

	m_device->CreateConstantBufferView(&cbvDesc, animationHandle);

	m_animationContainer[m_animationContainerCnt].pSystemMemAddr = m_animationConstantBegin + constantOffset;
	m_animationContainer[m_animationContainerCnt].CBVHandle = animationHandle;

	animationHandle.Offset(1, descriptorSize);

	return &m_animationContainer[m_animationContainerCnt++];

}


CbvManager::~CbvManager()
{
	for (int i = 0; i < MAX_PENDING_FRAME_COUNT; i++)
	{
		if (m_constantUploadBufferPool[i])
		{
			m_constantUploadBufferPool[i]->Release();
			m_constantUploadBufferPool[i] = nullptr;
		}
		if (m_descritorHeap[i])
		{
			m_descritorHeap[i]->Release();
			m_descritorHeap[i] = nullptr;
		}
		SafeDeleteArray(&m_cbvContainer[i]);
	}

	if (m_materialConstantUploadBufferPool)
	{
		m_materialConstantUploadBufferPool->Release();
		m_materialConstantUploadBufferPool = nullptr;
	}

	if (m_materialDescritorHeap)
	{
		m_materialDescritorHeap->Release();
		m_materialDescritorHeap = nullptr;
	}

	if (m_animationConstantUploadBufferPool)
	{
		m_animationConstantUploadBufferPool->Release();
		m_animationConstantUploadBufferPool = nullptr;
	}
	if (m_animationDescritorHeap)
	{
		m_animationDescritorHeap->Release();
		m_animationDescritorHeap = nullptr;
	}
	SafeDeleteArray(&m_animationContainer);
	SafeDeleteArray(&m_materialContainer);
}