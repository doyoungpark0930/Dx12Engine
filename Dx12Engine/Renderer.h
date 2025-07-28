#pragma once

class Renderer
{
public :
    void OnInit();
    void OnUpdate();
    void OnRender();
    void OnDestroy();

    void GetHardwareAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter);

    Renderer();
    ~Renderer();

private:
    static const UINT FrameCount = 2;


    // Pipeline objects.
    D3D12_VIEWPORT m_viewport;
    D3D12_RECT m_scissorRect;
    IDXGISwapChain3* m_swapChain;
    ID3D12Device* m_device;
    ID3D12Resource* m_renderTargets[FrameCount];
    ID3D12CommandAllocator* m_commandAllocator;
    ID3D12CommandQueue* m_commandQueue;
    ID3D12RootSignature* m_rootSignature;
    ID3D12DescriptorHeap* m_rtvHeap;
    ID3D12PipelineState* m_pipelineState;
    ID3D12GraphicsCommandList* m_commandList;
    UINT m_rtvDescriptorSize;

    // App resources.
    ID3D12Resource* m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

    // Synchronization objects.
    UINT m_frameIndex;
    HANDLE m_fenceEvent;
    ID3D12Fence* m_fence;
    UINT64 m_fenceValue;

    void LoadAssets();
    void PopulateCommandList();
    void WaitForPreviousFrame();
};