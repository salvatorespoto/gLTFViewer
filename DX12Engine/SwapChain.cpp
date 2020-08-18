#include "SwapChain.h"

using DXUtil::ThrowIfFailed;

void SwapChain::Create(Microsoft::WRL::ComPtr<ID3D12Device> device, DXGI_SWAP_CHAIN_DESC1 swapChainDesc, DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullScreenDesc, HWND hWnd, Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue)
{
    m_hWnd = hWnd;
    m_device = device;
    m_swapChainDesc = swapChainDesc;
    m_swapChainFullScreenDesc = swapChainFullScreenDesc;
    m_commandQueue = commandQueue;

    IDXGIFactory6* dxgiFactory;
    ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&dxgiFactory)), "Cannot create DXGI factory");

    m_swapChain.Reset();
    ThrowIfFailed(dxgiFactory->CreateSwapChainForHwnd(m_commandQueue.Get(), m_hWnd, &swapChainDesc, &m_swapChainFullScreenDesc, nullptr, m_swapChain.GetAddressOf()), "Cannot create swap chain");

    m_descriptorRTVSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 2;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_rtvDescriptorHeap)), "Cannot create RTV descriptor heap");
}

void SwapChain::Resize(UINT width, UINT height)
{
    m_swapChainDesc.Width = width;
    m_swapChainDesc.Height = height;
    CleanupRenderTargetViews();
    m_swapChain->ResizeBuffers(m_swapChainDesc.BufferCount, m_swapChainDesc.Width, m_swapChainDesc.Height, m_swapChainDesc.Format, m_swapChainDesc.Flags);
    CreateRenderTargetViews();
}

void SwapChain::SetFullScreen(bool fullScreen)
{
    if (m_isFullScreen == fullScreen) return;
    m_isFullScreen = fullScreen;
    m_swapChain->SetFullscreenState(m_isFullScreen, nullptr);
    Resize(m_swapChainDesc.Width, m_swapChainDesc.Height);
}

ID3D12Resource* SwapChain::GetCurrentBackBuffer() const
{
    return m_swapChainBuffers[m_currentBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE SwapChain::GetCurrentBackBufferView() const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_currentBackBuffer, m_descriptorRTVSize);
}

void SwapChain::CreateRenderTargetViews()
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle_CPU(m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    for (UINT i = 0; i < 2; i++)
    {
        m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_swapChainBuffers[i]));
        m_device->CreateRenderTargetView(m_swapChainBuffers[i].Get(), nullptr, rtvHeapHandle_CPU);
        rtvHeapHandle_CPU.Offset(1, m_descriptorRTVSize); // Point to the next descriptor
    }
}

void SwapChain::CleanupRenderTargetViews()
{
    for (UINT i = 0; i < 2; i++)
    {
        if (m_swapChainBuffers[i]) m_swapChainBuffers[i] = nullptr; // Setting nullptr release also the associated COM interfaces
    }
    m_currentBackBuffer = 0;
}

void SwapChain::Present()
{
    ThrowIfFailed(m_swapChain->Present(0, 0), "Cannot Present() on swap chain");
    m_currentBackBuffer = (m_currentBackBuffer + 1) % 2;
}