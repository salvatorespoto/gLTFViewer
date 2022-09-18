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
    int32_t renderMode; DirectX::XMFLOAT3 _pad0;
    Light lights[MAX_LIGHT_NUMBER];
};

class Scene;
class Grid;
class SkyBox;

class Renderer
{

public:
    Renderer() = default;
    Renderer(const Renderer&) = delete;
    Renderer(const Renderer&&) = delete;
    Renderer operator=(const Renderer&) = delete;
    Renderer operator=(const Renderer&&) = delete;

    void Init(HWND hWnd, const unsigned int width, const unsigned int height);
    void SetSize(const unsigned int width, const unsigned int height);
    void SetViewport(const D3D12_VIEWPORT viewPort);
    void SetScissorRect(const D3D12_RECT scissorRect);
    void SetFullScreen(const bool fullScreen);
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
    void Draw(Scene& scene, bool wireFrame);
    void Draw(SkyBox& skyBox);
    void Draw(Grid& grid);
    void EndDraw();

    std::vector<DXGI_MODE_DESC> GetDisplayModes();
    bool CompileShaders(const std::wstring& fileName, std::string& errorMsg);
    bool CompileVertexShader(const std::wstring& vsFileName, std::string& errorMsg);
    bool CompileGeometryShader(const std::wstring& vsFileName, std::string& errorMsg);
    bool CompilePixelShader(const std::wstring& vsFileName, std::string& errorMsg);

    void CreatePipelineState(SkyBox* skyBox);
    void CreatePipelineState(Grid* grid);
    void CreatePipelineState(Scene* scene, const bool wireFrame);

private:
    void EnableDebugLayer();
    void CreateDefaultDevice();
    void CreateSwapChain();
    void CreateCommandQueue();
    void CreateFence();
    void CreateDepthStencilBuffer();
    void CreateConstantBuffer();
    void SetPipelineState(ID3D12GraphicsCommandList* commandList);
    
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
    Microsoft::WRL::ComPtr<ID3DBlob> m_vertexShader;
    Microsoft::WRL::ComPtr<ID3DBlob> m_pixelShader;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineStateSky;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineStateGrid;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineStateScene;
};

