#include "pch.h"
#include "Renderer.h"
#include "winApp.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

using namespace DirectX;

Renderer::Renderer(UINT width, UINT height)
{
	clientWidth = width;
	clientHeight = height;
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

	if(FAILED(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)))) __debugbreak();
	
	
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

	if (debugController)
	{
		debugController->Release();

		debugController = nullptr;
	}



}

void Renderer::GetHardwareAdapter(
	IDXGIFactory1* pFactory,
	IDXGIAdapter1** ppAdapter)
{
	*ppAdapter = nullptr;

	IDXGIAdapter1* adapter = nullptr;


	IDXGIFactory6* factory6;
	//window 10이상만 factory6지원
	if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6))))
	{
		for (
			UINT adapterIndex = 0;
			SUCCEEDED(factory6->EnumAdapterByGpuPreference(
				adapterIndex,
				DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
				IID_PPV_ARGS(&adapter)));
				adapterIndex++)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);
			wprintf(L"Adapter %d: %s\n", adapterIndex, desc.Description);
			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				// Don't select the Basic Render Driver adapter.
				// If you want a software adapter, pass in "/warp" on the command line.
				continue;
			}
			// Check to see whether the adapter supports Direct3D 12, but don't create the
			// actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
			{
				break;
			}
		}
	}

	//window 10 미만일 때
	if (adapter == nullptr)
	{
		for (UINT adapterIndex = 0; SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &adapter)); adapterIndex++)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);
			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				// Don't select the Basic Render Driver adapter.
				// If you want a software adapter, pass in "/warp" on the command line.
				continue;
			}

			// Check to see whether the adapter supports Direct3D 12, but don't create the
			// actual device yet.
			if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
			{
				break;
			}
		}
	}

	*ppAdapter = adapter;

	if (factory6)
	{
		factory6->Release();

		factory6 = nullptr;
	}
}


Renderer::~Renderer()
{
	if (m_commandQueue)
	{
		m_commandQueue->Release();
		m_commandQueue = nullptr;
	}

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