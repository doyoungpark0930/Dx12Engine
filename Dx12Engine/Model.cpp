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
SrvManager* Model::m_srvManager = nullptr;
CbvManager* Model::m_cbvManager = nullptr;
SRV_CONTAINER* Model::m_srvContainer_CubeMap = nullptr;
Model::Model()
{
	m_device = m_renderer->GetDevice();
	descriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	m_commandQueue = m_renderer->GetCommandQueue();

	CreateCommandList();

	CreateFence();
}


void Model::CreateCommandList()
{
	if (FAILED(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)))) __debugbreak();
	// Create the command list.
	if (FAILED(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator, nullptr, IID_PPV_ARGS(&m_commandList)))) __debugbreak();

	// Command lists are created in the recording state, but there is nothing
	// to record yet. The main loop expects it to be closed, so close it now.
	if (FAILED(m_commandList->Close()))__debugbreak();
}

void Model::CreateFence()
{
	if (FAILED(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)))) __debugbreak();
	m_fenceValue = 0;

	// Create an event handle to use for frame synchronization.
	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (m_fenceEvent == nullptr)
	{
		__debugbreak();
	}
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

	m_srvContainer_CubeMap = new SRV_CONTAINER[4];
	if (meshData.envIBL_Filename)
	{
		CreateCubeTextureFromName(meshData.envIBL_Filename, m_srvContainer_CubeMap[0]);
	}
	if (meshData.specularIBL_Filename)
	{
		CreateCubeTextureFromName(meshData.specularIBL_Filename, m_srvContainer_CubeMap[1]);
	}
	if (meshData.irradianceIBL_Filename)
	{
		CreateCubeTextureFromName(meshData.irradianceIBL_Filename, m_srvContainer_CubeMap[2]);
	}
	if (meshData.brdf_Filename) 
	{
		CreateTextureFromName(meshData.brdf_Filename, m_srvContainer_CubeMap[3]);
	}

	SafeDeleteArray(&meshData.vertices);
	SafeDeleteArray(&meshData.indices);
	SafeDeleteArray(&meshData.envIBL_Filename);
	SafeDeleteArray(&meshData.specularIBL_Filename);
	SafeDeleteArray(&meshData.irradianceIBL_Filename);
	SafeDeleteArray(&meshData.brdf_Filename);
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
	localAABB = meshesInfo.boundingBox;
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

void Model::DrawGeneralMesh(ID3D12GraphicsCommandList* pCommandList, const Matrix* pMatrix, int contextIndex) //일단 materialNum은 1이라고 가정
{
	DescriptorPool* pDescriptorPool = m_renderer->GetDescriptorPool(contextIndex);

	//GlobalConstant
	CBV_CONTAINER globalContainer = m_cbvManager->GetGlobalContainer(contextIndex);

	CD3DX12_CPU_DESCRIPTOR_HANDLE globalCpuHandle(pDescriptorPool->m_descritorHeap->GetCPUDescriptorHandleForHeapStart());
	CD3DX12_GPU_DESCRIPTOR_HANDLE globalGpuHandle(pDescriptorPool->m_descritorHeap->GetGPUDescriptorHandleForHeapStart());
	m_device->CopyDescriptorsSimple(1, globalCpuHandle, globalContainer.CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	pCommandList->SetGraphicsRootDescriptorTable(0, globalGpuHandle);

	//ModelConstant
	CBV_CONTAINER* cbvContainer = m_cbvManager->GetAllocatedContainer(contextIndex);
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
	pDescriptorPool->AllocDescriptorTable(&cpuDescriptorTable, &gpuDescriptorTable, requiredDescriptorCount);

	m_device->CopyDescriptorsSimple(1, cpuDescriptorTable, cbvContainer->CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	pCommandList->SetGraphicsRootDescriptorTable(1, gpuDescriptorTable);
	//animation 빈 공간띄워주기(animation root signature를 공유해서 사용하기 때문)
	cpuDescriptorTable.Offset(2, descriptorSize);
	gpuDescriptorTable.Offset(2, descriptorSize);


	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// shadow map SRV 바인딩 (t8)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE shadowCpu;
		CD3DX12_GPU_DESCRIPTOR_HANDLE shadowGpu;

		// shadow map용 디스크립터 1개만 할당
		pDescriptorPool->AllocDescriptorTable(&shadowCpu, &shadowGpu, 1);

		m_device->CopyDescriptorsSimple(1, shadowCpu, m_srvManager->m_srvContainer[0].srvHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		// root parameter 4 (t8)
		pCommandList->SetGraphicsRootDescriptorTable(4, shadowGpu);
	}

	// cube map SRV 바인딩(t15)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE cubeMapCpu;
		CD3DX12_GPU_DESCRIPTOR_HANDLE cubeMapGpu;

		// shadow map용 디스크립터 1개만 할당
		pDescriptorPool->AllocDescriptorTable(&cubeMapCpu, &cubeMapGpu, 4);

		//envIBL / specularIBL / irradianceIBL / brdfIBL
		for (int i = 0; i < 4; i++)
		{
			if (m_srvContainer_CubeMap[i].pSrvResource)
			{
				m_device->CopyDescriptorsSimple(1, cubeMapCpu, m_srvContainer_CubeMap[i].srvHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				cubeMapCpu.Offset(1, descriptorSize);
			}
			else
			{
				cubeMapGpu.Offset(1, descriptorSize); //resource없으면 Descriptor를 빈공간으로
			}
		}
		// root parameter 4 (t5)
		pCommandList->SetGraphicsRootDescriptorTable(5, cubeMapGpu);
	}

	//MATERIAL_CONSTANT
	{
		MATERIAL_CONSTANT* pMaterialConstant = (MATERIAL_CONSTANT*)(*materialContainer).pSystemMemAddr;

		if (m_srvContainer[ALBEDO_SLOT].pSrvResource != nullptr) pMaterialConstant->useAlbedoTex = true;
		else pMaterialConstant->useAlbedoTex = false;
		if (m_srvContainer[AO_SLOT].pSrvResource != nullptr) pMaterialConstant->useAoTex = true;
		else pMaterialConstant->useAoTex = false;
		if (m_srvContainer[NORMALMAP_SLOT].pSrvResource != nullptr) pMaterialConstant->useNormalTex = true;
		else pMaterialConstant->useNormalTex = false;
		if (m_srvContainer[METALLIC_SLOT].pSrvResource != nullptr) pMaterialConstant->useMetallicTex = true;
		else pMaterialConstant->useMetallicTex = false;
		if (m_srvContainer[ROUGHNESS_SLOT].pSrvResource != nullptr)
		{
			pMaterialConstant->useRoughnessTex = true;
			pMaterialConstant->useGlossinessTex = false;
		}
		else
		{
			pMaterialConstant->useRoughnessTex = false;
			pMaterialConstant->useGlossinessTex = false;
		}


		pMaterialConstant->useShadowMap = true;
		m_device->CopyDescriptorsSimple(1, cpuDescriptorTable, (*materialContainer).CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		pCommandList->SetGraphicsRootDescriptorTable(2, gpuDescriptorTable);
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
	pCommandList->SetGraphicsRootDescriptorTable(3, gpuDescriptorTable);
	gpuDescriptorTable.Offset(MAX_TEXTURE_NUM, descriptorSize);

	pCommandList->IASetVertexBuffers(0, 1, &m_vertexBufferView[0]);
	pCommandList->IASetIndexBuffer(&IndexBufferView);
	pCommandList->DrawIndexedInstanced(indexCnt, 1, 0, 0, 0);
}

void Model::DrawCubeMap(ID3D12GraphicsCommandList* pCommandList, const Matrix* pMatrix,  int contextIndex)
{
	DescriptorPool* pDescriptorPool = m_renderer->GetDescriptorPool(contextIndex);
	//GlobalConstant
	CBV_CONTAINER globalContainer = m_cbvManager->GetGlobalContainer(contextIndex);

	CD3DX12_CPU_DESCRIPTOR_HANDLE globalCpuHandle(pDescriptorPool->m_descritorHeap->GetCPUDescriptorHandleForHeapStart());
	CD3DX12_GPU_DESCRIPTOR_HANDLE globalGpuHandle(pDescriptorPool->m_descritorHeap->GetGPUDescriptorHandleForHeapStart());
	m_device->CopyDescriptorsSimple(1, globalCpuHandle, globalContainer.CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	pCommandList->SetGraphicsRootDescriptorTable(0, globalGpuHandle);

	//ModelConstant
	CBV_CONTAINER* cbvContainer = m_cbvManager->GetAllocatedContainer(contextIndex);
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

	//model행렬(1) + CubeMapTexture(4)
	UINT requiredDescriptorCount = 5;
	pDescriptorPool->AllocDescriptorTable(&cpuDescriptorTable, &gpuDescriptorTable, requiredDescriptorCount);

	// modelCBV
	m_device->CopyDescriptorsSimple(1, cpuDescriptorTable, cbvContainer->CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	cpuDescriptorTable.Offset(1, descriptorSize);
	pCommandList->SetGraphicsRootDescriptorTable(1, gpuDescriptorTable);
	gpuDescriptorTable.Offset(1, descriptorSize);

	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//TEXTURES
	//envIBL / specularIBL / irradianceIBL / brdfIBL
	for (int i = 0; i < 4; i++) 
	{
		if (m_srvContainer_CubeMap[i].pSrvResource)
		{
			m_device->CopyDescriptorsSimple(1, cpuDescriptorTable, m_srvContainer_CubeMap[i].srvHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			cpuDescriptorTable.Offset(1, descriptorSize);
		}
		else
		{
			cpuDescriptorTable.Offset(1, descriptorSize); //resource없으면 Descriptor를 빈공간으로
		}
	}

	pCommandList->SetGraphicsRootDescriptorTable(2, gpuDescriptorTable);
	gpuDescriptorTable.Offset(1, descriptorSize);

	pCommandList->IASetVertexBuffers(0, 1, &m_vertexBufferView[0]);
	pCommandList->IASetIndexBuffer(&IndexBufferView);
	pCommandList->DrawIndexedInstanced(indexCnt, 1, 0, 0, 0);
}

void Model::DrawAnimation(ID3D12GraphicsCommandList* pCommandList, const Matrix* pMatrix, int contextIndex)
{
	DescriptorPool* pDescriptorPool = m_renderer->GetDescriptorPool(contextIndex);
	//GlobalConstant
	CBV_CONTAINER globalContainer = m_cbvManager->GetGlobalContainer(contextIndex);

	CD3DX12_CPU_DESCRIPTOR_HANDLE globalCpuHandle(pDescriptorPool->m_descritorHeap->GetCPUDescriptorHandleForHeapStart());
	CD3DX12_GPU_DESCRIPTOR_HANDLE globalGpuHandle(pDescriptorPool->m_descritorHeap->GetGPUDescriptorHandleForHeapStart());
	m_device->CopyDescriptorsSimple(1, globalCpuHandle, globalContainer.CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	pCommandList->SetGraphicsRootDescriptorTable(0, globalGpuHandle);


	//ModelConstant
	CBV_CONTAINER* cbvContainer = m_cbvManager->GetAllocatedContainer(contextIndex);
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
	pDescriptorPool->AllocDescriptorTable(&cpuDescriptorTable, &gpuDescriptorTable, requiredDescriptorCount);

	// modelCBV
	m_device->CopyDescriptorsSimple(1, cpuDescriptorTable, cbvContainer->CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	cpuDescriptorTable.Offset(1, descriptorSize);
	if (existAnimation)
	{
		m_device->CopyDescriptorsSimple(1, cpuDescriptorTable, boneMatricesContainer->CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	cpuDescriptorTable.Offset(1, descriptorSize);
	pCommandList->SetGraphicsRootDescriptorTable(1, gpuDescriptorTable);
	gpuDescriptorTable.Offset(2, descriptorSize);

	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	// shadow map SRV 바인딩 (t8)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE shadowCpu;
		CD3DX12_GPU_DESCRIPTOR_HANDLE shadowGpu;

		// shadow map용 디스크립터 1개만 할당
		pDescriptorPool->AllocDescriptorTable(&shadowCpu, &shadowGpu, 1);

		m_device->CopyDescriptorsSimple(1, shadowCpu, m_srvManager->m_srvContainer[0].srvHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		// root parameter 4 (t8)
		pCommandList->SetGraphicsRootDescriptorTable(4, shadowGpu);
	}

	// cube map SRV 바인딩(t15)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE cubeMapCpu;
		CD3DX12_GPU_DESCRIPTOR_HANDLE cubeMapGpu;

		// shadow map용 디스크립터 1개만 할당
		pDescriptorPool->AllocDescriptorTable(&cubeMapCpu, &cubeMapGpu, 4);

		//envIBL / specularIBL / irradianceIBL / brdfIBL
		for (int i = 0; i < 4; i++)
		{
			if (m_srvContainer_CubeMap[i].pSrvResource)
			{
				m_device->CopyDescriptorsSimple(1, cubeMapCpu, m_srvContainer_CubeMap[i].srvHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				cubeMapCpu.Offset(1, descriptorSize);
			}
			else
			{
				cubeMapGpu.Offset(1, descriptorSize); //resource없으면 Descriptor를 빈공간으로
			}
		}
		// root parameter 4 (t5)
		pCommandList->SetGraphicsRootDescriptorTable(5, cubeMapGpu);
	}


	//global / model / mtl1 (mtl_constant), (textures...) / mtl2 (mtl_constant), (textures...) / mtl3 ..
	for (int i = 0; i < m_materialNum; i++)
	{
		TRI_GROUP_PER_MTL* pTriGroup = m_TriGroupList + i;

		//MATERIAL_CONSTANT
		{
			MATERIAL_CONSTANT* pMaterialConstant = (MATERIAL_CONSTANT*)materialContainer[i].pSystemMemAddr;

			if (pTriGroup->srvContainer[ALBEDO_SLOT].pSrvResource != nullptr) pMaterialConstant->useAlbedoTex = true;
			else pMaterialConstant->useAlbedoTex = false;
			if (pTriGroup->srvContainer[AO_SLOT].pSrvResource != nullptr) pMaterialConstant->useAoTex = true;
			else pMaterialConstant->useAoTex = false;
			if (pTriGroup->srvContainer[NORMALMAP_SLOT].pSrvResource != nullptr) pMaterialConstant->useNormalTex = true;
			else pMaterialConstant->useNormalTex = false;
			if (pTriGroup->srvContainer[METALLIC_SLOT].pSrvResource != nullptr) pMaterialConstant->useMetallicTex = true;
			else pMaterialConstant->useMetallicTex = false;
			if (pTriGroup->srvContainer[ROUGHNESS_SLOT].pSrvResource != nullptr)
			{
				pMaterialConstant->useRoughnessTex = false;
				pMaterialConstant->useGlossinessTex = true;
			}
			else
			{
				pMaterialConstant->useRoughnessTex = false;
				pMaterialConstant->useGlossinessTex = false;
			}

			pMaterialConstant->useShadowMap = false;
			m_device->CopyDescriptorsSimple(1, cpuDescriptorTable, materialContainer[i].CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			cpuDescriptorTable.Offset(1, descriptorSize);
			pCommandList->SetGraphicsRootDescriptorTable(2, gpuDescriptorTable);
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
		pCommandList->SetGraphicsRootDescriptorTable(3, gpuDescriptorTable);
		gpuDescriptorTable.Offset(MAX_TEXTURE_NUM, descriptorSize);

		//Mesh Draw
		for (int j = 0; j < m_meshNum; j++)
		{
			if (pTriGroup->triNum[j] > 0)
			{
				pCommandList->IASetVertexBuffers(0, 1, &m_vertexBufferView[j]);
				pCommandList->IASetIndexBuffer(&pTriGroup->IndexBufferView[j]);
				pCommandList->DrawIndexedInstanced(pTriGroup->triNum[j] * 3, 1, 0, 0, 0);
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
	m_fenceValue++;
	if (FAILED(m_commandQueue->Signal(m_fence, m_fenceValue))) __debugbreak();

	// Wait until the previous frame is finished.
	if (m_fence->GetCompletedValue() < m_fenceValue)
	{
		if (FAILED(m_fence->SetEventOnCompletion(m_fenceValue, m_fenceEvent))) __debugbreak();
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
		for (int i = 0; i < 4; i++)
		{
			if (m_srvContainer_CubeMap[i].pSrvResource)
			{
				m_srvContainer_CubeMap[i].pSrvResource->Release();
				m_srvContainer_CubeMap[i].pSrvResource = nullptr;
			}
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

	if (m_commandList)
	{
		m_commandList->Release();
		m_commandList = nullptr;
	}

	if (m_commandAllocator)
	{
		m_commandAllocator->Release();
		m_commandAllocator = nullptr;
	}

	if (m_fence)
	{
		m_fence->Release();
		m_fence = nullptr;
	}

}