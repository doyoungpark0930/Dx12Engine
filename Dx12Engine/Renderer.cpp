#include "pch.h"
#include "Renderer.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

using namespace DirectX;

Renderer::Renderer()
{

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
			if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), (void**)(&m_device))))
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

void Renderer::OnInit()
{
	ID3D12Debug* debugController = nullptr;

#if defined(_DEBUG)

		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();

		}
		else
		{
			__debugbreak();
		}
		
#endif

	IDXGIAdapter1* adapter;
	IDXGIFactory1* factory;

	//DXGI_CREATE_FACTORY_DEBUG이거 해야 Com객체 메모리 누수 잡음
	if (FAILED(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&factory)))) __debugbreak();

	GetHardwareAdapter(factory, &adapter);

	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0
	};


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



Renderer::~Renderer()
{
	if (m_device)
	{
		m_device->Release();
		m_device = nullptr;
	}
}