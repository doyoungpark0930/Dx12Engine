#pragma once

class Renderer;

class SrvManager
{
public :
	SrvManager() = default;
	ID3D12DescriptorHeap* m_descritorHeap = nullptr;
	void OnInit(ID3D12Device* pDevice, Renderer* pRenderer);
	SRV_CONTAINER CreateShadowMapTexture();
	SRV_CONTAINER CreateCubemapTexture(const wchar_t* szFileName);
	SRV_CONTAINER CreateTexture(const wchar_t* szFileName);
	UINT GetAllocatedNum() const { return allocatedNum; }
	SRV_CONTAINER* GetSRVContainer() const { return m_srvContainer; }
	~SrvManager();
	SRV_CONTAINER* m_srvContainer = nullptr; //´Ù½Ã private·Î
private:
	ID3D12Device* m_device = nullptr;
	Renderer* m_renderer = nullptr;
	ID3D12GraphicsCommandList* m_commandList = nullptr;
	ID3D12CommandAllocator* m_commandAllocator = nullptr;
	ID3D12CommandQueue* m_commandQueue = nullptr;
	ID3D12DescriptorHeap* m_dsvHeap = nullptr;

	HANDLE m_fenceEvent;
	ID3D12Fence* m_fence = nullptr;
	UINT64 m_fenceValue;

	UINT descriptorSize = 0;
	ID3D12Resource* m_textureUploadHeap = nullptr;
	bool uploadHeapCreated = false;
	const UINT max_descriptorNum = 256;
	ID3D12Resource** m_textures = nullptr;

	UINT allocatedNum = 0;
	//SRV_CONTAINER* m_srvContainer = nullptr;

	int m_shadowWidth = 1280;
	int m_shadowHeight = 1280;

	void CreateCommandList();
	void CreateFence();
	void Fence();
	void WaitForFenceValue();

};