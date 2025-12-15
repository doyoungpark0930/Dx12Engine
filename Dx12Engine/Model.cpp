#include "pch.h"
#include "CommonStructs.h"
#include "DescriptorPool.h"
#include "Renderer.h"
#include "DXUtil.h"
#include "DXHelper.h"
#include "SrvManager.h"
#include "CbvManager.h"
#include "GeometryGenerator.h"
#include "Animation.h"
#include "Model.h"
#include <iostream>
using namespace DirectX::SimpleMath;

Renderer* Model::m_renderer = nullptr;
DescriptorPool* Model::m_descriptorPool = nullptr;
SrvManager* Model::m_srvManager = nullptr;
CbvManager* Model::m_cbvManager = nullptr;

Model::Model()
{
	m_device = m_renderer->GetDevice();
	UpdateMembers();
	descriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

}

void Model::UpdateMembers()
{
	m_commandList = m_renderer->GetCommandList();
	m_commandAllocator = m_renderer->GetCommandAllocator();
	m_commandQueue = m_renderer->GetCommandQueue();
	m_fence = m_renderer->GetFence();
	m_fenceEvent = m_renderer->GetFenceEvent();
	m_fenceValue = m_renderer->GetFenceValue();
}

template <typename T>
D3D12_VERTEX_BUFFER_VIEW Model::CreateVertexBuffer(T* vertices, UINT vertexCount)
{
	const UINT vertexBufferSize = sizeof(T) * vertexCount;

	UINT verticesOffset = 0;
	if (FAILED(
		SetDataToUploadBuffer(
			&(m_renderer->m_vsCur),
			m_renderer->m_vsBegin,
			m_renderer->m_vsEnd,
			vertices, sizeof(T), vertexCount,
			sizeof(float),
			verticesOffset
		)))__debugbreak();

	if (FAILED(m_commandAllocator->Reset())) __debugbreak();

	if (FAILED(m_commandList->Reset(m_commandAllocator, nullptr))) __debugbreak();

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderer->m_vsBufferPool, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);
	m_commandList->ResourceBarrier(1, &barrier);

	m_commandList->CopyBufferRegion(m_renderer->m_vsBufferPool, verticesOffset, m_renderer->m_vsUploadBufferPool, verticesOffset, vertexBufferSize);

	barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderer->m_vsBufferPool, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	m_commandList->ResourceBarrier(1, &barrier);

	if (FAILED(m_commandList->Close())) __debugbreak();

	ID3D12CommandList* ppCommandLists[] = { m_commandList };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	WaitForPreviousFrame();

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

	vertexBufferView.BufferLocation = m_renderer->m_vsBufferPool->GetGPUVirtualAddress() + verticesOffset;
	vertexBufferView.StrideInBytes = sizeof(T);
	vertexBufferView.SizeInBytes = vertexBufferSize;

	return vertexBufferView;
}

D3D12_INDEX_BUFFER_VIEW Model::CreateIndexBuffer(UINT* indices, UINT indiceCount)
{
	const UINT indicesBufferSize = sizeof(UINT) * indiceCount;

	UINT indicesOffset = 0;
	if (FAILED(
		SetDataToUploadBuffer(
			&(m_renderer->m_indexCur),
			m_renderer->m_indexBegin,
			m_renderer->m_indexEnd,
			indices, sizeof(UINT), indiceCount,
			sizeof(UINT),
			indicesOffset
		)))__debugbreak();

	if (FAILED(m_commandAllocator->Reset())) __debugbreak();

	if (FAILED(m_commandList->Reset(m_commandAllocator, nullptr))) __debugbreak();

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderer->m_indexBufferPool, D3D12_RESOURCE_STATE_INDEX_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);
	m_commandList->ResourceBarrier(1, &barrier);

	m_commandList->CopyBufferRegion(m_renderer->m_indexBufferPool, indicesOffset, m_renderer->m_indexUploadBufferPool, indicesOffset, indicesBufferSize);

	barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderer->m_indexBufferPool, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
	m_commandList->ResourceBarrier(1, &barrier);

	if (FAILED(m_commandList->Close())) __debugbreak();

	ID3D12CommandList* ppCommandLists[] = { m_commandList };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	WaitForPreviousFrame();

	D3D12_INDEX_BUFFER_VIEW indexBufferView;

	indexBufferView.BufferLocation = m_renderer->m_indexBufferPool->GetGPUVirtualAddress() + indicesOffset;
	indexBufferView.SizeInBytes = indicesBufferSize;
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;

	return indexBufferView;
}

void Model::CreateCubeMap(JustMeshData& meshData)
{
	m_vertexBufferView = new D3D12_VERTEX_BUFFER_VIEW[1];
	m_vertexBufferView[0] = CreateVertexBuffer(meshData.vertices, meshData.verticesNum);
	IndexBufferView = CreateIndexBuffer(meshData.indices, meshData.indicesNum);
	indexCnt = meshData.indicesNum;

	if (meshData.albedoTexFilename)
	{
		m_srvContainer_CubeMap = new SRV_CONTAINER;
		CreateCubeTextureFromName(meshData.albedoTexFilename, *m_srvContainer_CubeMap);
	}

	SafeDeleteArray(&meshData.vertices);
	SafeDeleteArray(&meshData.indices);
	SafeDeleteArray(&meshData.albedoTexFilename);

}

void Model::CreateGeneralModel(JustMeshData& meshData)
{
	m_vertexBufferView = new D3D12_VERTEX_BUFFER_VIEW[1];
	m_vertexBufferView[0] = CreateVertexBuffer(meshData.vertices, meshData.verticesNum);
	IndexBufferView = CreateIndexBuffer(meshData.indices, meshData.indicesNum);
	indexCnt = meshData.indicesNum;

	m_srvContainer = new SRV_CONTAINER[MAX_TEXTURE_NUM];
	if (meshData.albedoTexFilename)
		CreateTextureFromName(meshData.albedoTexFilename, m_srvContainer[0]);
	if (meshData.aoTexFilename)
		CreateTextureFromName(meshData.aoTexFilename, m_srvContainer[1]);
	if (meshData.normalTexFilename)
		CreateTextureFromName(meshData.normalTexFilename, m_srvContainer[2]);
	if (meshData.metallicTexFilename)
		CreateTextureFromName(meshData.metallicTexFilename, m_srvContainer[3]);
	if (meshData.roughnessTexFilename)
		CreateTextureFromName(meshData.roughnessTexFilename, m_srvContainer[4]);

	materialContainer = new CBV_CONTAINER;
	*materialContainer = m_cbvManager->AllocMaterialCBV();


	SafeDeleteArray(&meshData.vertices);
	SafeDeleteArray(&meshData.indices);
	SafeDeleteArray(&meshData.albedoTexFilename);
	SafeDeleteArray(&meshData.aoTexFilename);
	SafeDeleteArray(&meshData.normalTexFilename);
	SafeDeleteArray(&meshData.metallicTexFilename);
	SafeDeleteArray(&meshData.roughnessTexFilename);

}
void Model::CreateModelFromFile(MeshDataInfo& meshesInfo)
{
	rootNode = meshesInfo.rootNode;
	m_materialNum = meshesInfo.materialNum;
	m_meshNum = meshesInfo.meshNum;
	m_TriGroupList = new TRI_GROUP_PER_MTL[m_materialNum];
	for (int i = 0; i < m_materialNum; i++)
	{
		m_TriGroupList[i].srvContainer = new SRV_CONTAINER[MAX_TEXTURE_NUM];
		if (meshesInfo.Materials[i].albedoTexFilename)
			CreateTextureFromName(meshesInfo.Materials[i].albedoTexFilename, m_TriGroupList[i].srvContainer[0]);
		if (meshesInfo.Materials[i].aoTexFilename)
			CreateTextureFromName(meshesInfo.Materials[i].aoTexFilename, m_TriGroupList[i].srvContainer[1]);
		if (meshesInfo.Materials[i].normalTexFilename)
			CreateTextureFromName(meshesInfo.Materials[i].normalTexFilename, m_TriGroupList[i].srvContainer[2]);
		if (meshesInfo.Materials[i].metallicTexFilename)
			CreateTextureFromName(meshesInfo.Materials[i].metallicTexFilename, m_TriGroupList[i].srvContainer[3]);
		if (meshesInfo.Materials[i].roughnessTexFilename)
			CreateTextureFromName(meshesInfo.Materials[i].roughnessTexFilename, m_TriGroupList[i].srvContainer[4]);

		m_TriGroupList[i].IndexBufferView = new D3D12_INDEX_BUFFER_VIEW[m_meshNum];
		m_TriGroupList[i].triNum = new UINT[m_meshNum];
		for (int j = 0; j < m_meshNum; j++)
		{
			m_TriGroupList[i].triNum[j] = meshesInfo.Materials[i].face_cnt[j]; //이걸로 그릴지안그릴지 판단
			if (meshesInfo.Materials[i].face_cnt[j] > 0)
			{
				m_TriGroupList[i].IndexBufferView[j] = CreateIndexBuffer(meshesInfo.Materials[i].index[j], m_TriGroupList[i].triNum[j] * 3);
			}
		}
	}

	m_vertexBufferView = new D3D12_VERTEX_BUFFER_VIEW[m_meshNum];
	for (int i = 0; i < m_meshNum; i++)
	{
		m_vertexBufferView[i] = CreateVertexBuffer(meshesInfo.meshes[i].vertices, meshesInfo.meshes[i].verticesNum);
	}


	//Alloc Material Constant 
	materialContainer = new CBV_CONTAINER[m_materialNum];
	for (int i = 0; i < m_materialNum; i++)
	{
		materialContainer[i] = m_cbvManager->AllocMaterialCBV();
	}


	if (&meshesInfo.m_animations[0]) //일단 첫번째 애니메이션만
	{
		existAnimation = true;
		boneMatricesContainer = m_cbvManager->AllocAnimationMatrices();
		m_FinalBoneMatrices = meshesInfo.finalBoneMatrices;
		m_animations = &meshesInfo.m_animations[0]; //Model에서 animation데이터 해제하기 위함
	}


	SafeDeleteArray(&meshesInfo.meshes);
	SafeDeleteArray(&meshesInfo.Materials);

}

void Model::DrawGeneralMesh(const Matrix* pMatrix) //일단 materialNum은 1이라고 가정
{
	//GlobalConstant
	CBV_CONTAINER globalContainer = m_cbvManager->GetGlobalContainer();

	CD3DX12_CPU_DESCRIPTOR_HANDLE globalCpuHandle(m_descriptorPool->m_descritorHeap->GetCPUDescriptorHandleForHeapStart());
	CD3DX12_GPU_DESCRIPTOR_HANDLE globalGpuHandle(m_descriptorPool->m_descritorHeap->GetGPUDescriptorHandleForHeapStart());
	m_device->CopyDescriptorsSimple(1, globalCpuHandle, globalContainer.CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_commandList->SetGraphicsRootDescriptorTable(0, globalGpuHandle);

	//ModelConstant
	CBV_CONTAINER* cbvContainer = m_cbvManager->GetAllocatedContainer();
	MODEL_CONSTANT* pModelConstant = (MODEL_CONSTANT*)cbvContainer->pSystemMemAddr;

	//Model Update
	{
		pModelConstant->Model = pMatrix->Transpose();

		Matrix NormalMatrix = *pMatrix;
		NormalMatrix.Translation(Vector3(0.0f));
		NormalMatrix = NormalMatrix.Invert().Transpose();
		pModelConstant->NormalModel = NormalMatrix.Transpose();
	}

	//AllocTable 및 디스크립터 복사
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorTable = {};
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescriptorTable = {};

	//model행렬(1) + animation(1) + mtlCbv + textures
	UINT requiredDescriptorCount = 1 + 1 + 1 + MAX_TEXTURE_NUM;
	m_descriptorPool->AllocDescriptorTable(&cpuDescriptorTable, &gpuDescriptorTable, requiredDescriptorCount);

	m_device->CopyDescriptorsSimple(1, cpuDescriptorTable, cbvContainer->CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_commandList->SetGraphicsRootDescriptorTable(1, gpuDescriptorTable);
	//animation 빈 공간띄워주기(animation root signature를 공유해서 사용하기 때문)
	cpuDescriptorTable.Offset(2, descriptorSize);
	gpuDescriptorTable.Offset(2, descriptorSize);


	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// shadow map SRV 바인딩 (t8)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE shadowCpu;
		CD3DX12_GPU_DESCRIPTOR_HANDLE shadowGpu;

		// shadow map용 디스크립터 1개만 할당
		m_descriptorPool->AllocDescriptorTable(&shadowCpu, &shadowGpu, 1);

		m_device->CopyDescriptorsSimple(1, shadowCpu, m_srvManager->m_srvContainer[0].srvHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		// root parameter 4 (t8)
		m_commandList->SetGraphicsRootDescriptorTable(4, shadowGpu);
	}

	//MATERIAL_CONSTANT
	{
		MATERIAL_CONSTANT* pMaterialConstant = (MATERIAL_CONSTANT*)(*materialContainer).pSystemMemAddr;
		if (m_srvContainer[NORMALMAP_SLOT].pSrvResource != nullptr) pMaterialConstant->useNormalMap = true;
		else pMaterialConstant->useNormalMap = false;
		m_device->CopyDescriptorsSimple(1, cpuDescriptorTable, (*materialContainer).CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_commandList->SetGraphicsRootDescriptorTable(2, gpuDescriptorTable);
		cpuDescriptorTable.Offset(1, descriptorSize);
		gpuDescriptorTable.Offset(1, descriptorSize);
	}

	//TEXTURES
	for (int j = 0; j < MAX_TEXTURE_NUM; j++)
	{
		if (m_srvContainer[j].pSrvResource != nullptr)
		{
			m_device->CopyDescriptorsSimple(1, cpuDescriptorTable, m_srvContainer[j].srvHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
		cpuDescriptorTable.Offset(1, descriptorSize);
	}
	m_commandList->SetGraphicsRootDescriptorTable(3, gpuDescriptorTable);
	gpuDescriptorTable.Offset(MAX_TEXTURE_NUM, descriptorSize);

	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView[0]);
	m_commandList->IASetIndexBuffer(&IndexBufferView);
	m_commandList->DrawIndexedInstanced(indexCnt, 1, 0, 0, 0);
}

void Model::DrawCubeMap(const Matrix* pMatrix)
{
	//GlobalConstant
	CBV_CONTAINER globalContainer = m_cbvManager->GetGlobalContainer();

	CD3DX12_CPU_DESCRIPTOR_HANDLE globalCpuHandle(m_descriptorPool->m_descritorHeap->GetCPUDescriptorHandleForHeapStart());
	CD3DX12_GPU_DESCRIPTOR_HANDLE globalGpuHandle(m_descriptorPool->m_descritorHeap->GetGPUDescriptorHandleForHeapStart());
	m_device->CopyDescriptorsSimple(1, globalCpuHandle, globalContainer.CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_commandList->SetGraphicsRootDescriptorTable(0, globalGpuHandle);

	//ModelConstant
	CBV_CONTAINER* cbvContainer = m_cbvManager->GetAllocatedContainer();
	MODEL_CONSTANT* pModelConstant = (MODEL_CONSTANT*)cbvContainer->pSystemMemAddr;

	//Model Update
	{
		pModelConstant->Model = pMatrix->Transpose();

		Matrix NormalMatrix = *pMatrix;
		NormalMatrix.Translation(Vector3(0.0f));
		NormalMatrix = NormalMatrix.Invert().Transpose();
		pModelConstant->NormalModel = NormalMatrix.Transpose();
	}

	//AllocTable 및 디스크립터 복사
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorTable = {};
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescriptorTable = {};

	//model행렬(1) + CubeMapTexture
	UINT requiredDescriptorCount = 2;
	m_descriptorPool->AllocDescriptorTable(&cpuDescriptorTable, &gpuDescriptorTable, requiredDescriptorCount);

	// modelCBV
	m_device->CopyDescriptorsSimple(1, cpuDescriptorTable, cbvContainer->CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	cpuDescriptorTable.Offset(1, descriptorSize);
	m_commandList->SetGraphicsRootDescriptorTable(1, gpuDescriptorTable);
	gpuDescriptorTable.Offset(1, descriptorSize);

	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//TEXTURES
	if (m_srvContainer_CubeMap != nullptr)
	{
		m_device->CopyDescriptorsSimple(1, cpuDescriptorTable, m_srvContainer_CubeMap->srvHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		cpuDescriptorTable.Offset(1, descriptorSize);
	}
	else
	{
		cpuDescriptorTable.Offset(1, descriptorSize); //resource없으면 Descriptor를 빈공간으로
	}

	m_commandList->SetGraphicsRootDescriptorTable(2, gpuDescriptorTable);
	gpuDescriptorTable.Offset(1, descriptorSize);

	m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView[0]);
	m_commandList->IASetIndexBuffer(&IndexBufferView);
	m_commandList->DrawIndexedInstanced(indexCnt, 1, 0, 0, 0);
}

void Model::DrawAnimation(const Matrix* pMatrix)
{
	//GlobalConstant
	CBV_CONTAINER globalContainer = m_cbvManager->GetGlobalContainer();

	CD3DX12_CPU_DESCRIPTOR_HANDLE globalCpuHandle(m_descriptorPool->m_descritorHeap->GetCPUDescriptorHandleForHeapStart());
	CD3DX12_GPU_DESCRIPTOR_HANDLE globalGpuHandle(m_descriptorPool->m_descritorHeap->GetGPUDescriptorHandleForHeapStart());
	m_device->CopyDescriptorsSimple(1, globalCpuHandle, globalContainer.CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_commandList->SetGraphicsRootDescriptorTable(0, globalGpuHandle);


	//ModelConstant
	CBV_CONTAINER* cbvContainer = m_cbvManager->GetAllocatedContainer();
	MODEL_CONSTANT* pModelConstant = (MODEL_CONSTANT*)cbvContainer->pSystemMemAddr;

	//Model Update
	{
		pModelConstant->Model = pMatrix->Transpose();

		Matrix NormalMatrix = *pMatrix;
		NormalMatrix.Translation(Vector3(0.0f));
		NormalMatrix = NormalMatrix.Invert().Transpose();
		pModelConstant->NormalModel = NormalMatrix.Transpose();
	}

	//AnimationUpdate
	if (existAnimation)
	{
		SkinnedConstants* pSkinnedConstant = (SkinnedConstants*)boneMatricesContainer->pSystemMemAddr;
		memcpy(pSkinnedConstant->boneTransforms, m_FinalBoneMatrices, sizeof(Matrix) * ModelMatrixNum);

	}


	//AllocTable 및 디스크립터 복사
	CD3DX12_GPU_DESCRIPTOR_HANDLE gpuDescriptorTable = {};
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuDescriptorTable = {};

	//model행렬(1)+ animationMatrices(1) + (mtlCbv + textures) * mtlNum;
	UINT requiredDescriptorCount = 1 + 1 + m_materialNum * (1 + MAX_TEXTURE_NUM);
	m_descriptorPool->AllocDescriptorTable(&cpuDescriptorTable, &gpuDescriptorTable, requiredDescriptorCount);

	// modelCBV
	m_device->CopyDescriptorsSimple(1, cpuDescriptorTable, cbvContainer->CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	cpuDescriptorTable.Offset(1, descriptorSize);
	if (existAnimation)
	{
		m_device->CopyDescriptorsSimple(1, cpuDescriptorTable, boneMatricesContainer->CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	cpuDescriptorTable.Offset(1, descriptorSize);
	m_commandList->SetGraphicsRootDescriptorTable(1, gpuDescriptorTable);
	gpuDescriptorTable.Offset(2, descriptorSize);

	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	// shadow map SRV 바인딩 (t8)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE shadowCpu;
		CD3DX12_GPU_DESCRIPTOR_HANDLE shadowGpu;

		// shadow map용 디스크립터 1개만 할당
		m_descriptorPool->AllocDescriptorTable(&shadowCpu, &shadowGpu, 1);

		m_device->CopyDescriptorsSimple(1, shadowCpu, m_srvManager->m_srvContainer[0].srvHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		// root parameter 4 (t8)
		m_commandList->SetGraphicsRootDescriptorTable(4, shadowGpu);
	}

	//global / model / mtl1 (mtl_constant), (textures...) / mtl2 (mtl_constant), (textures...) / mtl3 ..

	for (int i = 0; i < m_materialNum; i++)
	{
		TRI_GROUP_PER_MTL* pTriGroup = m_TriGroupList + i;

		//MATERIAL_CONSTANT
		{
			MATERIAL_CONSTANT* pMaterialConstant = (MATERIAL_CONSTANT*)materialContainer[i].pSystemMemAddr;
			if (pTriGroup->srvContainer[NORMALMAP_SLOT].pSrvResource != nullptr)  pMaterialConstant->useNormalMap = true;
			else  pMaterialConstant->useNormalMap = false;
			m_device->CopyDescriptorsSimple(1, cpuDescriptorTable, materialContainer[i].CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			cpuDescriptorTable.Offset(1, descriptorSize);
			m_commandList->SetGraphicsRootDescriptorTable(2, gpuDescriptorTable);
			gpuDescriptorTable.Offset(1, descriptorSize);
		}

		//TEXTURES
		for (int j = 0; j < MAX_TEXTURE_NUM; j++)
		{
			if (pTriGroup->srvContainer[j].pSrvResource != nullptr)
			{
				m_device->CopyDescriptorsSimple(1, cpuDescriptorTable, pTriGroup->srvContainer[j].srvHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				cpuDescriptorTable.Offset(1, descriptorSize);
			}
			else
			{
				cpuDescriptorTable.Offset(1, descriptorSize); //resource없으면 Descriptor를 빈공간으로
			}
		}
		m_commandList->SetGraphicsRootDescriptorTable(3, gpuDescriptorTable);
		gpuDescriptorTable.Offset(MAX_TEXTURE_NUM, descriptorSize);

		//Mesh Draw
		for (int j = 0; j < m_meshNum; j++)
		{
			if (pTriGroup->triNum[j] > 0)
			{
				m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView[j]);
				m_commandList->IASetIndexBuffer(&pTriGroup->IndexBufferView[j]);
				m_commandList->DrawIndexedInstanced(pTriGroup->triNum[j] * 3, 1, 0, 0, 0);
			}
		}
	}




}

void Model::CreateTextureFromName(char* textureFilename, SRV_CONTAINER& srvContainer)
{
	if (textureFilename)
	{
		wchar_t PathName[512];
		MultiByteToWideChar(CP_UTF8, 0, textureFilename, -1, PathName, 512);

		srvContainer = m_srvManager->CreateTexture(PathName);
	}
}

void Model::CreateCubeTextureFromName(char* textureFilename, SRV_CONTAINER& srvContainer)
{
	if (textureFilename)
	{
		wchar_t PathName[512];
		MultiByteToWideChar(CP_UTF8, 0, textureFilename, -1, PathName, 512);

		srvContainer = m_srvManager->CreateCubemapTexture(PathName);
	}
}

void Model::WaitForPreviousFrame()
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

void deleteNode(meshNode* node)
{
	if (node == nullptr)
		return;

	for (int i = 0; i < node->mNumChildren; ++i)
	{
		deleteNode(node->mChildren[i]);
		node->mChildren[i] = nullptr;
	}
	delete node;
}
Model::~Model()
{

	deleteNode(rootNode);
	SafeDeleteArray(&materialContainer);
	SafeDeleteArray(&m_animations);
	SafeDeleteArray(&m_vertexBufferView);

	for (int i = 0; i < m_materialNum; i++)
	{
		SafeDeleteArray(&m_TriGroupList[i].IndexBufferView);
		SafeDeleteArray(&m_TriGroupList[i].triNum);
		for (int j = 0; j < MAX_TEXTURE_NUM; j++)
		{
			ID3D12Resource* pSrvResource = m_TriGroupList[i].srvContainer[j].pSrvResource;
			if (pSrvResource)
			{
				pSrvResource->Release();
				pSrvResource = nullptr;
			}
		}
		SafeDeleteArray(&m_TriGroupList[i].srvContainer);
	}
	SafeDeleteArray(&m_TriGroupList);


	if (m_srvContainer_CubeMap)
	{
		if (m_srvContainer_CubeMap->pSrvResource)
		{
			m_srvContainer_CubeMap->pSrvResource->Release();
			m_srvContainer_CubeMap->pSrvResource = nullptr;
		}
		SafeDeleteArray(&m_srvContainer_CubeMap);
	}

	if (m_srvContainer)
	{
		for (int i = 0; i < MAX_TEXTURE_NUM; i++)
		{
			ID3D12Resource* pSrvResource = m_srvContainer[i].pSrvResource;
			if (pSrvResource)
			{
				pSrvResource->Release();
				pSrvResource = nullptr;
			}
		}
		SafeDeleteArray(&m_srvContainer);
	}

}