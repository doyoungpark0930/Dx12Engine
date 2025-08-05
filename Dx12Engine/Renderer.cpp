#include "pch.h"
#include "Renderer.h"
#include "winApp.h"
#include "VertexStruct.h"

#include "DXUtil.h"
#include "DXHelper.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")


#include "Object.h"
#include "mesh.h"

using namespace DirectX;

Renderer::Renderer(UINT width, UINT height)
{
	clientWidth = width;
	clientHeight = height;

	m_viewport.TopLeftX = 0.0f;
	m_viewport.TopLeftY = 0.0f;
	m_viewport.Width = static_cast<float>(clientWidth);
	m_viewport.Height = static_cast<float>(clientHeight);

	m_scissorRect.left = 0;
	m_scissorRect.top = 0;
	m_scissorRect.right = clientWidth;   
	m_scissorRect.bottom = clientHeight;
	

	WCHAR assetsPath[512];
	GetAssetsPath(assetsPath, _countof(assetsPath));
	DXUtil::m_assetsPath = assetsPath;
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

		m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
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
	// Create an empty root signature.
	{
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ID3DBlob* signature;
		ID3DBlob* error;
		if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error))) __debugbreak();
		if (FAILED(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)))) __debugbreak();

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

	// Create the pipeline state, which includes compiling and loading shaders.
	{
		ID3DBlob* vertexShader = nullptr;
		ID3DBlob* pixelShader = nullptr;

#if defined(_DEBUG)
		// Enable better shader debugging with the graphics debugging tools.
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif

		if (FAILED(D3DCompileFromFile(L"./shaders.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr))) __debugbreak();
		if (FAILED(D3DCompileFromFile(L"./shaders.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr))) __debugbreak();

		// Define the vertex input layout.
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};
		
		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = m_rootSignature;
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader);
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader);
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;
		if (FAILED(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)))) __debugbreak();
	}

	// Create the command list.
	if (FAILED(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator, m_pipelineState, IID_PPV_ARGS(&m_commandList)))) __debugbreak();

	// Command lists are created in the recording state, but there is nothing
	// to record yet. The main loop expects it to be closed, so close it now.
	if (FAILED(m_commandList->Close()))__debugbreak();

	Object::m_renderer = this;

	Vertex* triangleVertices = CreateTriangleVertex();
	Vertex* squareVertices = CreateSquareVertex();

	m_Objects = new Object[ObjectsNum];

	m_Objects[0].CreateVertexBuffer(triangleVertices, 3);
	m_Objects[1].CreateVertexBuffer(squareVertices, 6);



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

	if (triangleVertices)
	{
		delete triangleVertices;
		triangleVertices = nullptr;
	}

	if (squareVertices)
	{
		delete squareVertices;
		squareVertices = nullptr;
	}

	
}


// Update frame-based values.
void Renderer::OnUpdate()
{
}


// Render the scene.
void Renderer::OnRender()
{
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
	if (FAILED(m_commandList->Reset(m_commandAllocator, m_pipelineState))) __debugbreak();

	// Set necessary state.
	m_commandList->SetGraphicsRootSignature(m_rootSignature);
	m_commandList->RSSetViewports(1, &m_viewport);
	m_commandList->RSSetScissorRects(1, &m_scissorRect);

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	// Indicate that the back buffer will be used as a render target.
	m_commandList->ResourceBarrier(1, &barrier);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);
	m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// Record commands.
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	for (int i = 0; i < ObjectsNum; i++)
	{
		m_Objects[i].Render();
	}

	barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET ,D3D12_RESOURCE_STATE_PRESENT);
	// Indicate that the back buffer will now be used to present.
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

	if (m_commandAllocator)
	{
		m_commandAllocator->Release();
		m_commandAllocator = nullptr;
	}

	if (m_pipelineState)
	{
		m_pipelineState->Release();
		m_pipelineState = nullptr;
	}
	for (int i = 0; i < FrameCount; i++)
	{
		if (m_renderTargets[i])
		{
			m_renderTargets[i]->Release();
			m_renderTargets[i] = nullptr;
		}
	}

	if (m_rootSignature)
	{
		m_rootSignature->Release();
		m_rootSignature = nullptr;
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
	
	delete[] m_Objects;
	m_Objects = nullptr;

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