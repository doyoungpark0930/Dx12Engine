#pragma once

class Object;

class Renderer
{
public :
    void OnInit();
    void OnUpdate();
    void OnRender();


    UINT GetWidth() const { return clientWidth; }
    UINT GetHeight() const { return clientHeight; }

    ID3D12Device* GetDevice() const { return m_device; }
    ID3D12GraphicsCommandList* GetCommandList() const { return m_commandList; }

    Renderer(UINT width, UINT height);
    ~Renderer();

private:
    static const UINT FrameCount = 2;

    UINT clientWidth;
    UINT clientHeight;


    // Pipeline objects.
    D3D12_VIEWPORT m_viewport;
    D3D12_RECT m_scissorRect;
    IDXGISwapChain3* m_swapChain;
    ID3D12Device* m_device;
    ID3D12Resource* m_renderTargets[FrameCount] = {};
    ID3D12CommandAllocator* m_commandAllocator;
    ID3D12CommandQueue* m_commandQueue;
    ID3D12RootSignature* m_rootSignature;
    ID3D12DescriptorHeap* m_rtvHeap;
    ID3D12PipelineState* m_pipelineState;
    ID3D12GraphicsCommandList* m_commandList;
    UINT m_rtvDescriptorSize;


    // Synchronization objects.
    UINT m_frameIndex;
    HANDLE m_fenceEvent;
    ID3D12Fence* m_fence;
    UINT64 m_fenceValue;

    Object* m_Objects;
    UINT ObjectsNum = 2;

    

    void LoadAssets();
    void PopulateCommandList();
    void WaitForPreviousFrame();

};