#pragma once

class DXUtil
{
public :
	static std::wstring GetAssetFullPath(LPCWSTR assetName);
	static std::wstring m_assetsPath;
};

void GetHardwareAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter);