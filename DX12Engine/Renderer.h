#pragma once

#include "DXUtil.h"
#include "Buffers.h"
#include "SwapChain.h"
#include "Material.h"
#include "Light.h"

constexpr unsigned int MAX_LIGHT_NUMBER = 7;
const DirectX::XMVECTORF32 backgroundColor = DirectX::Colors::DarkGray;

/** All the common constants used from shaders for a single frame rendering */
struct FrameConstants
{
    DirectX::XMFLOAT4X4 viewMtx;
    DirectX::XMFLOAT4X4 projMtx;
    DirectX::XMFLOAT4X4 projViewMtx;
    DirectX::XMFLOAT4 eyePosition;
    Light lights[MAX_LIGHT_NUMBER];
};

class Scene;

class Renderer
{

public:
    Renderer() = default;
    Renderer(const Renderer&) = delete;
    Renderer(const Renderer&&) = delete;
    Renderer operator=(const Renderer&) = delete;
    Renderer operator=(const Renderer&&) = delete;

    void Init(HWND hWnd, unsigned int width, unsigned int height);
    void SetSize(unsigned int width, unsigned int height);
    void SetViewport(D3D12_VIEWPORT viewPort);
    void SetScissorRect(D3D12_RECT scissorRect);
    void SetFullScreen(bool fullScreen);
    HWND GetWindowHandle();
    Microsoft::WRL::ComPtr<ID3D12Device> GetDevice();
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> GetCommandList();
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> GetCommandQueue();
    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferView() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const;
    void ExecuteCommandList(ID3D12GraphicsCommandList* commandList);
    void ResetCommandList();
    void FlushCommandQueue();
    
    void BeginDraw();
    void Draw(Scene& scene);
    void EndDraw();

    std::vector<DXGI_MODE_DESC> GetDisplayModes();
    bool CompileShaders(std::wstring fileName, std::string& errorMsg);
    void CreatePipelineState(Scene* scene);

private:
    void EnableDebugLayer();
    void CreateDefaultDevice();
    void CreateSwapChain();
    void CreateCommandQueue();
    void CreateFence();
    void CreateDepthStencilBuffer();
    void CreateConstantBuffer();
    //void CreateRootSignature();
    void SetPipelineState(ID3D12GraphicsCommandList* commandList);
    //void SetRootSignature(ID3D12GraphicsCommandList* commandList);
    
    HWND m_hWnd;
    unsigned int m_width, m_height;
    D3D12_VIEWPORT m_viewPort;
    D3D12_RECT m_scissorRect;
    
    Microsoft::WRL::ComPtr<ID3D12Device> m_device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandListAlloc;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
    SwapChain m_swapChain;

    DXGI_MODE_DESC m_fullScreenMode;
    BOOL m_isFullScreen = false;
    std::vector<DXGI_MODE_DESC> m_displayModes;

    
    UINT m_currentFenceValue = 0;
    Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
    
    UINT m_DSV_DescriptorSize = 0;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DSV_DescriptorHeap;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_depthStencilBuffer;

    //std::unique_ptr<UploadBuffer<PassConstants>> m_passConstantBuffer;
    //std::unique_ptr<UploadBuffer<DirectX::XMFLOAT4X4>> m_meshConstantBuffer;
    //std::unique_ptr<UploadBuffer<RoughMetallicMaterial>> m_materialConstantBuffer;
    
    //UINT m_CBV_SRV_DescriptorSize = 0;

    Microsoft::WRL::ComPtr<ID3DBlob> m_vertexShader;
    Microsoft::WRL::ComPtr<ID3DBlob> m_pixelShader;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
};

