#include "pch.h"
#include "DXUtil.h"

std::wstring DXUtil::m_assetsPath = L"";

// Helper function for resolving the full path of assets.
std::wstring DXUtil::GetAssetFullPath(LPCWSTR assetName)
{
    return m_assetsPath + assetName;
}

void GetHardwareAdapter(
	IDXGIFactory1* pFactory,
	IDXGIAdapter1** ppAdapter)
{
	*ppAdapter = nullptr;

	IDXGIAdapter1* adapter = nullptr;


	IDXGIFactory6* factory6;
	//window 10�̻� factory6����
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

	//window 10 �̸��� ��
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


//https://learn.microsoft.com/en-us/windows/win32/direct3d12/uploading-resources
HRESULT SuballocateFromBuffer(UINT8** m_pDataCur, UINT8* m_pDataEnd, SIZE_T uSize, UINT uAlign)
{
	*m_pDataCur = reinterpret_cast<UINT8*>(
		Align(reinterpret_cast<SIZE_T>(*m_pDataCur), uAlign)
		);

	return (*m_pDataCur + uSize > m_pDataEnd) ? E_INVALIDARG : S_OK;
}

//
// Place and copy data to the upload buffer.
//

HRESULT SetDataToUploadBuffer(
	UINT8** m_pDataCur, 
	UINT8* m_pDataBegin,
	UINT8* m_pDataEnd,
	const void* pData,
	UINT bytesPerData,
	UINT dataCount,
	UINT alignment,
	UINT& byteOffset
)
{
	SIZE_T byteSize = bytesPerData * dataCount;
	HRESULT hr = SuballocateFromBuffer(m_pDataCur, m_pDataEnd, byteSize, alignment);
	if (SUCCEEDED(hr))
	{
		byteOffset = UINT(*m_pDataCur - m_pDataBegin);
		memcpy(*m_pDataCur, pData, byteSize);
		*m_pDataCur += byteSize;
	}

	return hr;
}

//
// Align uLocation to the next multiple of uAlign.
//

SIZE_T Align(SIZE_T uLocation, SIZE_T uAlign)
{
	if ((0 == uAlign) || (uAlign & (uAlign - 1))) // uAlign�� 0�̰ų� 2�� �ŵ�����(1,2,4,8...)�� �ƴϸ� ���� �߻�
	{
		throw std::exception();
	}

	// uLocation�� uAlign ����� �ø� (����)
   // ��: uAlign = 256, uLocation = 8 �� 256 ��ȯ
   // ��Ʈ ����ũ�� �̿��Ͽ� ���� ����� uAlign ����� �����.
	return ((uLocation + (uAlign - 1)) & ~(uAlign - 1)); 
}