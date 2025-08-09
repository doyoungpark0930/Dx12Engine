#pragma once

class DXUtil
{
public :
	static std::wstring GetAssetFullPath(LPCWSTR assetName);
	static std::wstring m_assetsPath;
};

void GetHardwareAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter);

HRESULT SuballocateFromBuffer(UINT8* m_pDataCur, UINT8* m_pDataEnd, SIZE_T uSize, UINT uAlign);

HRESULT SetDataToUploadBuffer(
	UINT8** m_pDataCur,
	UINT8* m_pDataBegin,
	UINT8* m_pDataEnd,
	const void* pData,
	UINT bytesPerData,
	UINT dataCount,
	UINT alignment,
	UINT& byteOffset
);

SIZE_T Align(SIZE_T uLocation, SIZE_T uAlign);