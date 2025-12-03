#include "pch.h"
#include "DescriptorPool.h"
#include "SrvManager.h"
#include "CbvManager.h"
#include "Camera.h"
#include "WinApp.h"
#include "CommonStructs.h"

#include "DXUtil.h"
#include "DXHelper.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")


#include "Model.h"
#include "Mesh.h"
#include "GeometryGenerator.h"
#include "Animation.h"
#include "Animator.h"
#include "Renderer.h"

using namespace DirectX;
using namespace DirectX::SimpleMath;

Renderer::Renderer(UINT width, UINT height)
{
	clientWidth = width;
	clientHeight = height;

	m_viewport.TopLeftX = 0.0f;
	m_viewport.TopLeftY = 0.0f;
	m_viewport.Width = static_cast<float>(clientWidth);
	m_viewport.Height = static_cast<float>(clientHeight);
	m_viewport.MinDepth = 0.0f; //이거 안하면 depth값 판정 안됨
	m_viewport.MaxDepth = 1.0f;

	m_scissorRect.left = 0;
	m_scissorRect.top = 0;
	m_scissorRect.right = clientWidth;
	m_scissorRect.bottom = clientHeight;

	aspect = m_viewport.Width / m_viewport.Height;

	wchar_t assetsPath[512];
	GetResourcesPath(assetsPath, _countof(assetsPath));
	wcscpy_s(DXUtil::m_assetsResourcesPath, assetsPath);

}

void Renderer::OnInit()
{
	ID3D12Debug* debugController = nullptr;
	UINT dxgiFactoryFlags = 0;
#if defined(_DEBUG)

	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;

	}
	else
	{
		__debugbreak();
	}

#endif

	IDXGIAdapter1* adapter;
	IDXGIFactory4* factory;

	//DXGI_CREATE_FACTORY_DEBUG이거 해야 Com객체 메모리 누수 잡음
	if (FAILED(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory))))
		__debugbreak();

	GetHardwareAdapter(factory, &adapter);

	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	bool deviceCreated = false;
	for (int i = 0; i < _countof(featureLevels); i++)
	{
		if (SUCCEEDED(D3D12CreateDevice(adapter, featureLevels[i], IID_PPV_ARGS(&m_device))))
		{
			deviceCreated = true;
			break;
		}
	}
	if (!deviceCreated) __debugbreak();

	// Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	if (FAILED(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)))) __debugbreak();


	// Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.Width = clientWidth;
	swapChainDesc.Height = clientHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;
	IDXGISwapChain1* swapChain;
	if (FAILED(factory->CreateSwapChainForHwnd(
		m_commandQueue,        // Swap chain needs the queue so that it can force a flush on it.
		WinApp::GetHwnd(),
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain
	))) __debugbreak();

	if (FAILED(swapChain->QueryInterface(IID_PPV_ARGS(&m_swapChain)))) __debugbreak();

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// Create descriptor heaps.
	{
		// Describe and create a render target view (RTV) descriptor heap.
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = FrameCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		if (FAILED(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)))) __debugbreak();

		// Describe and create a depth stencil view (DSV) descriptor heap.
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		if (FAILED(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)))) __debugbreak();

		m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		//cbv,srv는 descriptorPool에서 관리
		m_descriptorPool = new DescriptorPool();
		m_descriptorPool->OnInit(m_device, maxObjectsNum);

		m_srvManager = new SrvManager();
		m_srvManager->OnInit(m_device, this);

		m_cbvManager = new CbvManager();
		m_cbvManager->OnInit(m_device, this);

	}

	// Create frame resources.
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

		// Create a RTV for each frame.
		for (UINT n = 0; n < FrameCount; n++)
		{
			//renderTarget에 백버퍼의 리소스를 할당
			if (FAILED(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n]))));
			m_device->CreateRenderTargetView(m_renderTargets[n], nullptr, rtvHandle);
			rtvHandle.Offset(1, m_rtvDescriptorSize);
		}
	}


	if (FAILED(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)))) __debugbreak();

	LoadAssets();


	if (adapter)
	{
		adapter->Release();

		adapter = nullptr;
	}

	if (factory)
	{
		factory->Release();

		factory = nullptr;
	}

	if (swapChain)
	{
		swapChain->Release();
		swapChain = nullptr;
	}

	if (debugController)
	{
		debugController->Release();

		debugController = nullptr;
	}



}

void Renderer::LoadAssets()
{
	CreateRootSignature();
	CreatePipelineState();

	// Create the command list.
	if (FAILED(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator, nullptr, IID_PPV_ARGS(&m_commandList)))) __debugbreak();

	// Command lists are created in the recording state, but there is nothing
	// to record yet. The main loop expects it to be closed, so close it now.
	if (FAILED(m_commandList->Close()))__debugbreak();

	// Create synchronization objects.
	{
		if (FAILED(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)))) __debugbreak();
		m_fenceValue = 1;

		// Create an event handle to use for frame synchronization.
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_fenceEvent == nullptr)
		{
			__debugbreak();
		}
	}

	CreateDepthStencil();

	CreateObjects();

	Create_Vertex_Index();

	OnInitGlobalConstant();





}

void Renderer::CreateRootSignature()
{
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

	// This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

	if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// Allow input layout and deny uneccessary access to certain pipeline stages.
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	D3D12_STATIC_SAMPLER_DESC sampler = {};
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	sampler.MipLODBias = 0;
	sampler.MaxAnisotropy = 0;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;
	sampler.ShaderRegister = 0;
	sampler.RegisterSpace = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;


	//General and Animation rootsignature
	{
		CD3DX12_DESCRIPTOR_RANGE1 ranges[4];
		CD3DX12_ROOT_PARAMETER1 rootParameters[4];

		// b0: GLOBAL_CONSTANT (1개 고정)
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0);
		rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);

		// b1 : Model_Constant (오브젝트 개수만큼)
		// b2 : m_FinalBoneMatrices
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 2, 1, 0);
		rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_VERTEX);

		// b3 : Material_Constant
		ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 3, 0);
		rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_PIXEL);

		// t0~t7 : srv
		UINT maxSrvNum = 8;
		ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, maxSrvNum, 0, 0);
		rootParameters[3].InitAsDescriptorTable(1, &ranges[3], D3D12_SHADER_VISIBILITY_PIXEL);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 1, &sampler, rootSignatureFlags);

		ID3DBlob* signature = nullptr;
		ID3DBlob* error = nullptr;

		if (FAILED(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error))) __debugbreak();
		if (FAILED(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature_General)))) __debugbreak();

		if (signature)
		{
			signature->Release();
			signature = nullptr;
		}
		if (error)
		{
			error->Release();
			error = nullptr;
		}
	}

	//cubemap rootsignature
	{
		CD3DX12_DESCRIPTOR_RANGE1 ranges[3];
		CD3DX12_ROOT_PARAMETER1 rootParameters[3];

		// b0: GLOBAL_CONSTANT (1개 고정)
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0);
		rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL);

		// b1 : Model_Constant (오브젝트 개수만큼)
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1, 0);
		rootParameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_VERTEX);

		// t0~t7 : srv
		ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
		rootParameters[2].InitAsDescriptorTable(1, &ranges[2], D3D12_SHADER_VISIBILITY_PIXEL);

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 1, &sampler, rootSignatureFlags);

		ID3DBlob* signature = nullptr;
		ID3DBlob* error = nullptr;
		
		if (FAILED(D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error))) __debugbreak();
		if (FAILED(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature_CubeMap)))) __debugbreak();

		if (signature)
		{
			signature->Release();
			signature = nullptr;
		}
		if (error)
		{
			error->Release();
			error = nullptr;
		}
	}

}
void Renderer::CreatePipelineState()
{

#if defined(_DEBUG) 
	// Enable better shader debugging with the graphics debugging tools.
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else 
	UINT compileFlags = 0;
#endif

	//AnimationPSO
	ID3DBlob* animationVertexShader = nullptr;
	ID3DBlob* pixelShader = nullptr;

	// 디버그모드는 현재디렉터리가 vcxproj있는 디렉터리, exe누르면 exe파일 있는곳이 directory.
	// exe와 f5의 실행이 다르다면, exe있는 디렉터리에 hlsl파일 복사해서 넣기
	// 혹은 hlsl파일의 property에서 itemtype을 copyFile로 바꾸기
	if (FAILED(D3DCompileFromFile(L"./AnimationVSShader.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &animationVertexShader, nullptr)))
	{
		printf("Cannot find shaderfile\n");
		throw std::exception();
	}
	if (FAILED(D3DCompileFromFile(L"./PSShader.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr)))
	{
		printf("Cannot find shaderfile\n");
		throw std::exception();
	}

	// Define the vertex input layout.
	D3D12_INPUT_ELEMENT_DESC inputElementDescs_Animation[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

		{"BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 44,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"BLENDINDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT, 0, 60,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	// Describe and create the graphics pipeline state object (PSO).
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs_Animation, _countof(inputElementDescs_Animation) };
	psoDesc.pRootSignature = m_rootSignature_General;
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(animationVertexShader);
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader);
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleDesc.Count = 1;
	if (FAILED(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_animationPSO)))) __debugbreak();


	//CubePSO
	ID3DBlob* CubeVertexShader = nullptr;
	ID3DBlob* CubePixelShader = nullptr;

	if (FAILED(D3DCompileFromFile(L"./CubeVSShader.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &CubeVertexShader, nullptr)))
	{
		printf("Cannot find shaderfile\n");
		throw std::exception();
	}
	if (FAILED(D3DCompileFromFile(L"./CubePSShader.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &CubePixelShader, nullptr)))
	{
		printf("Cannot find shaderfile\n");
		throw std::exception();
	}

	D3D12_INPUT_ELEMENT_DESC inputElementDescs_JustMesh[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	psoDesc.InputLayout = { inputElementDescs_JustMesh, _countof(inputElementDescs_JustMesh) };
	psoDesc.pRootSignature = m_rootSignature_CubeMap;
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(CubeVertexShader);
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(CubePixelShader);

	if (FAILED(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_cubeMapPSO)))) __debugbreak();


	//GeneralPSO
	ID3DBlob* vertexShader = nullptr;

	if (FAILED(D3DCompileFromFile(L"./VSShader.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr)))
	{
		printf("Cannot find shaderfile\n");
		throw std::exception();
	}

	psoDesc.InputLayout = { inputElementDescs_JustMesh, _countof(inputElementDescs_JustMesh) };
	psoDesc.pRootSignature = m_rootSignature_General;
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader);
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader);

	if (FAILED(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_GeneralPSO)))) __debugbreak();

	if (animationVertexShader)
	{
		animationVertexShader->Release();
		animationVertexShader = nullptr;
	}
	if (pixelShader)
	{
		pixelShader->Release();
		pixelShader = nullptr;
	}
	if (CubeVertexShader)
	{
		CubeVertexShader->Release();
		CubeVertexShader = nullptr;
	}
	if (CubePixelShader)
	{
		CubePixelShader->Release();
		CubePixelShader = nullptr;
	}
	if (vertexShader)
	{
		vertexShader->Release();
		vertexShader = nullptr;
	}
}

void Renderer::CreateDepthStencil()
{
	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
	depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, clientWidth, clientHeight, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);


	if (FAILED(m_device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&depthDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthOptimizedClearValue,
		IID_PPV_ARGS(&m_depthStencil)
	)))__debugbreak();


	m_device->CreateDepthStencilView(m_depthStencil, &depthStencilDesc, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
}

void Renderer::CreateObjects()
{
	Model::m_renderer = this;
	Model::m_descriptorPool = m_descriptorPool;
	Model::m_srvManager = m_srvManager;
	Model::m_cbvManager = m_cbvManager;

	m_Models = new Model[3];

}

void Renderer::Create_Vertex_Index()
{
	//Create VertexPool
	UINT64 vertexMemory = 1024 * 1024 * 32;
	CD3DX12_HEAP_PROPERTIES defaultHeapProps(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexMemory);
	if (FAILED(m_device->CreateCommittedResource(
		&defaultHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
		nullptr,
		IID_PPV_ARGS(&m_vsBufferPool)))) __debugbreak();

	CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);
	if (FAILED(m_device->CreateCommittedResource(
		&uploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_vsUploadBufferPool)))) __debugbreak();

	void* pData;
	CD3DX12_RANGE readRange(0, 0);
	m_vsUploadBufferPool->Map(0, &readRange, &pData);
	m_vsCur = m_vsBegin = reinterpret_cast<UINT8*>(pData);
	m_vsEnd = m_vsBegin + vertexMemory;


	//Create IndexPool
	UINT64 indexMemory = 1024 * 1024;
	CD3DX12_RESOURCE_DESC indexResDesc = CD3DX12_RESOURCE_DESC::Buffer(indexMemory);
	if (FAILED(m_device->CreateCommittedResource(
		&defaultHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&indexResDesc,
		D3D12_RESOURCE_STATE_INDEX_BUFFER,
		nullptr,
		IID_PPV_ARGS(&m_indexBufferPool)))) __debugbreak();

	if (FAILED(m_device->CreateCommittedResource(
		&uploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&indexResDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_indexUploadBufferPool)))) __debugbreak();

	m_indexUploadBufferPool->Map(0, &readRange, &pData);
	m_indexCur = m_indexBegin = reinterpret_cast<UINT8*>(pData);
	m_indexEnd = m_indexBegin + indexMemory;

	CreateModels();


	if (m_vsUploadBufferPool)
	{
		m_vsUploadBufferPool->Release();
		m_vsUploadBufferPool = nullptr;
	}

	if (m_indexUploadBufferPool)
	{
		m_indexUploadBufferPool->Release();
		m_indexUploadBufferPool = nullptr;
	}

}

void Renderer::CreateModels()
{
	char basePath[512];
	WideCharToMultiByte(CP_UTF8, 0, DXUtil::m_assetsResourcesPath, -1, basePath, sizeof(basePath), NULL, NULL); // wchar → MultiByte 변환 (UTF-8 기준)
	strcat_s(basePath, sizeof(basePath), "Pete\\");

	const char* fileName = "PeteT.dy";
	const char* animationFileNames[] =
	{ "BoxingIdle.ani","Boxing.ani","JabCross.ani",
		"HeadHit.ani","Martelo2.ani","KidneyHit.ani",
		"RecevingUppercut.ani","Walking.ani","Running.ani",
		"Running Forward Flip.ani"
	};

	m_animator = new Animator;

	//Animation
	MeshDataInfo meshesInfo = GeometryGenerator::ReadMeshFromFile(basePath, fileName, animationFileNames, _countof(animationFileNames));
	m_animations = meshesInfo.m_animations;
	m_animator->OnInit(&m_animations[0], 1, meshesInfo.m_defaultTransform);
	meshesInfo.finalBoneMatrices = m_animator->GetFinalBoneMatrices();
	m_Models[0].CreateModelFromFile(meshesInfo);


	//CubeMap
	char* cubeMapPath = MakeFilePath(DXUtil::m_assetsResourcesPath, "Cubemap\\HDRI\\Mountain\\",
		"mountainEnvHDR.dds");
	JustMeshData cubeMapping = GeometryGenerator::MakeBox(20.0f);
	ReverseIndices(cubeMapping.indices, cubeMapping.indicesNum);
	cubeMapping.albedoTexFilename = cubeMapPath;
	m_Models[1].CreateCubeMap(cubeMapping);

	//Ground
	char* groundAlbedoPath = MakeFilePath(DXUtil::m_assetsResourcesPath, "PBR\\TCom_Ground_Soil18_2.5x2.5_2K\\",
		"TCom_Ground_Soil18_2.5x2.5_2K_albedo.tif");
	char* groundAoPath = MakeFilePath(DXUtil::m_assetsResourcesPath, "PBR\\TCom_Ground_Soil18_2.5x2.5_2K\\",
		"TCom_Ground_Soil18_2.5x2.5_2K_ao.tif");
	char* groundNormalPath = MakeFilePath(DXUtil::m_assetsResourcesPath, "PBR\\TCom_Ground_Soil18_2.5x2.5_2K\\",
		"TCom_Ground_Soil18_2.5x2.5_2K_normal.tif");
	char* groundRoughnessPath = MakeFilePath(DXUtil::m_assetsResourcesPath, "PBR\\TCom_Ground_Soil18_2.5x2.5_2K\\",
		"TCom_Ground_Soil18_2.5x2.5_2K_roughness.tif");

	JustMeshData ground = GeometryGenerator::MakeSquare(1.0f);
	ground.albedoTexFilename = groundAlbedoPath;
	ground.aoTexFilename = groundAoPath;
	ground.normalTexFilename = groundNormalPath;
	ground.roughnessTexFilename = groundRoughnessPath;
	m_Models[2].CreateGeneralModel(ground);

	m_ObjectState = new ObjectState[maxObjectsNum];
}

void Renderer::OnInitGlobalConstant()
{
	GLOBAL_CONSTANT* globalConstant = (GLOBAL_CONSTANT*)(m_cbvManager->GetStartCBV() + 0);
	View = camera.GetViewRow();
	Proj = camera.GetProjRow();
	globalConstant->ViewProj = (View * Proj).Transpose();
	globalConstant->eyePos = Vector4(camera.m_eyePos.x, camera.m_eyePos.y, camera.m_eyePos.z, 1.0f);
	globalConstant->lightPos = Vector4(lightPos.x, lightPos.y, lightPos.z, 1.0f);

}

// Update frame-based values.
void Renderer::Update(float dt)
{ 
	camera.UpdateKeyboard(dt, m_keyPressed);

	//globalConstant Update
	GLOBAL_CONSTANT* globalConstant = (GLOBAL_CONSTANT*)(m_cbvManager->GetStartCBV() + 0);
	View = camera.GetViewRow();
	globalConstant->ViewProj = (View * Proj).Transpose();
	globalConstant->eyePos = Vector4(camera.m_eyePos.x, camera.m_eyePos.y, camera.m_eyePos.z, 1.0f);
	globalConstant->lightPos = Vector4(lightPos.x, lightPos.y, lightPos.z, 1.0f);

	int animIndex = GetAnimationIndexFromKey();
	m_animator->RequestAnimation(&m_animations[animIndex]);

	m_animator->UpdateAnimation(dt);

	m_ObjectState[0].scale.x = 2.0f;
	m_ObjectState[0].scale.y = 2.0f;
	m_ObjectState[0].scale.z = 2.0f;
	m_ObjectState[0].pos.x = -2.0f;

	m_ObjectState[1].scale.x = 2.0f;
	m_ObjectState[1].scale.y = 2.0f;
	m_ObjectState[1].scale.z = 2.0f;
	m_ObjectState[1].pos.x = 0.0f;

	m_ObjectState[2].scale.x = 2.0f;
	m_ObjectState[2].scale.y = 2.0f;
	m_ObjectState[2].scale.z = 2.0f;
	m_ObjectState[2].pos.x = 2.0f;

	//cubemap
	m_ObjectState[3].scale.x = 1.0f;
	m_ObjectState[3].scale.y = 1.0f;
	m_ObjectState[3].scale.z = 1.0f;

	//ground
	m_ObjectState[4].scale.x = 20.0f;
	m_ObjectState[4].scale.y = 20.0f;
	m_ObjectState[4].scale.z = 1.0f;
	m_ObjectState[4].rotation.x = pi / 2.0f;
	m_ObjectState[4].pos.y = -1.0f;

}
void Renderer::ObjectRender()
{
	Matrix object0_Matrix = GetObjectWorldMatrix(m_ObjectState[0]);

	Matrix object1_Matrix = GetObjectWorldMatrix(m_ObjectState[1]);

	Matrix object2_Matrix = GetObjectWorldMatrix(m_ObjectState[2]);

	//cubeMap
	Matrix object3_Matrix = GetObjectWorldMatrix(m_ObjectState[3]);

	Matrix object4_Matrix = GetObjectWorldMatrix(m_ObjectState[4]);

	m_commandList->SetPipelineState(m_animationPSO);
	m_commandList->SetGraphicsRootSignature(m_rootSignature_General);

	//animation object
	m_Models[0].DrawAnimation(&object0_Matrix);
	m_Models[0].DrawAnimation(&object1_Matrix);
	m_Models[0].DrawAnimation(&object2_Matrix);

	//cubemap
	m_commandList->SetPipelineState(m_cubeMapPSO);
	m_commandList->SetGraphicsRootSignature(m_rootSignature_CubeMap);
	m_Models[1].DrawCubeMap(&object3_Matrix);

	//ground
	m_commandList->SetPipelineState(m_GeneralPSO);
	m_commandList->SetGraphicsRootSignature(m_rootSignature_General);
	m_Models[2].DrawGeneralMesh(&object4_Matrix);
}

// Render the scene.
void Renderer::Render()
{
	m_descriptorPool->Reset();
	m_cbvManager->Reset();

	// Record all the commands we need to render the scene into the command list.
	PopulateCommandList();

	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { m_commandList };
	m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);


	// Present the frame.
	m_swapChain->Present(1, 0);
	WaitForPreviousFrame();
}

void Renderer::PopulateCommandList()
{
	// Command list allocators can only be reset when the associated 
	// command lists have finished execution on the GPU; apps should use 
	// fences to determine GPU execution progress.
	if (FAILED(m_commandAllocator->Reset())) __debugbreak();

	// However, when ExecuteCommandList() is called on a particular command 
	// list, that command list can then be reset at any time and must be before 
	// re-recording.
	if (FAILED(m_commandList->Reset(m_commandAllocator, nullptr))) __debugbreak();

	// Set necessary state.
	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	ID3D12DescriptorHeap* ppHeaps[] = { m_descriptorPool->m_descritorHeap };
	m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	// Indicate that the back buffer will be used as a render target.
	m_commandList->ResourceBarrier(1, &barrier);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	// Record commands.
	const float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	ObjectRender();

	barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	m_commandList->ResourceBarrier(1, &barrier);

	if (FAILED(m_commandList->Close())) __debugbreak();
}

void Renderer::WaitForPreviousFrame()
{
	// WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
	// This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
	// sample illustrates how to use fences for efficient resource usage and to
	// maximize GPU utilization.

	// Signal and increment the fence value.
	const UINT64 fence = m_fenceValue;
	if (FAILED(m_commandQueue->Signal(m_fence, fence))) __debugbreak();
	m_fenceValue++;

	// Wait until the previous frame is finished.
	if (m_fence->GetCompletedValue() < fence)
	{
		if (FAILED(m_fence->SetEventOnCompletion(fence, m_fenceEvent))) __debugbreak();
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}


Renderer::~Renderer()
{
	WaitForPreviousFrame();
	CloseHandle(m_fenceEvent);
	if (m_commandQueue)
	{
		m_commandQueue->Release();
		m_commandQueue = nullptr;
	}

	if (m_swapChain)
	{
		m_swapChain->Release();
		m_swapChain = nullptr;
	}

	if (m_rtvHeap)
	{
		m_rtvHeap->Release();
		m_rtvHeap = nullptr;
	}

	if (m_dsvHeap)
	{
		m_dsvHeap->Release();
		m_dsvHeap = nullptr;
	}

	if (m_descriptorPool)
	{
		delete m_descriptorPool;
		m_descriptorPool = nullptr;
	}

	if (m_srvManager)
	{
		delete m_srvManager;
		m_srvManager = nullptr;
	}

	if (m_cbvManager)
	{
		delete m_cbvManager;
		m_cbvManager = nullptr;
	}

	if (m_commandAllocator)
	{
		m_commandAllocator->Release();
		m_commandAllocator = nullptr;
	}

	if (m_animationPSO)
	{
		m_animationPSO->Release();
		m_animationPSO = nullptr;
	}

	if (m_GeneralPSO)
	{
		m_GeneralPSO->Release();
		m_GeneralPSO = nullptr;
	}

	if (m_cubeMapPSO)
	{
		m_cubeMapPSO->Release();
		m_cubeMapPSO = nullptr;
	}
	for (int i = 0; i < FrameCount; i++)
	{
		if (m_renderTargets[i])
		{
			m_renderTargets[i]->Release();
			m_renderTargets[i] = nullptr;
		}
	}

	if (m_depthStencil)
	{
		m_depthStencil->Release();
		m_depthStencil = nullptr;
	}

	if (m_rootSignature_General)
	{
		m_rootSignature_General->Release();
		m_rootSignature_General = nullptr;
	}

	if (m_rootSignature_CubeMap)
	{
		m_rootSignature_CubeMap->Release();
		m_rootSignature_CubeMap = nullptr;
	}

	if (m_commandList)
	{
		m_commandList->Release();
		m_commandList = nullptr;
	}

	if (m_fence)
	{
		m_fence->Release();
		m_fence = nullptr;
	}

	if (m_vsBufferPool)
	{
		m_vsBufferPool->Release();
		m_vsBufferPool = nullptr;
	}

	if (m_indexBufferPool)
	{
		m_indexBufferPool->Release();
		m_indexBufferPool = nullptr;
	}
	SafeDeleteArray(&m_Models);
	if (m_animator)
	{
		delete m_animator;
		m_animator = nullptr;
	}
	SafeDeleteArray(&m_ObjectState);

	if (m_device)
	{
		int restCount = m_device->Release();
		if (restCount)
		{
			IDXGIDebug1* dxgiDebug;
			if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
			{
				OutputDebugStringW(L"Com Object Dump:\r\n");
				dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);

				dxgiDebug->Release();
			}
			__debugbreak();
		}
		m_device = nullptr;
	}
}