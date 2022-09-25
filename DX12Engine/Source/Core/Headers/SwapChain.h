// SPDX-FileCopyrightText: 2022 Salvatore Spoto <salvatore.spoto@gmail.com> 
// SPDX-License-Identifier: MIT

#pragma once

#include "DXUtil.h"

class SwapChain
{
public:
    SwapChain() = default;
    SwapChain(const SwapChain& sc) = delete;
    SwapChain(const SwapChain&& sc) = delete;
    SwapChain& operator=(const SwapChain& sc) = delete;
    SwapChain& operator=(const SwapChain&& sc) = delete;
    ~SwapChain() = default;

    void Create(Microsoft::WRL::ComPtr<ID3D12Device> device, DXGI_SWAP_CHAIN_DESC1 swapChainDesc, DXGI_SWAP_CHAIN_FULLSCREEN_DESC swapChainFullScreenDesc, HWND hWnd, Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue);
    void Resize(const UINT width, const UINT height);
    void SetFullScreen(const bool fullScreen);

    ID3D12Resource* GetCurrentBackBuffer() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentBackBufferView() const;
    
    void Present();

protected:
    
    void CreateRenderTargetViews();
    void CleanupRenderTargetViews();

    HWND m_hWnd;
    Microsoft::WRL::ComPtr<ID3D12Device> m_device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
    DXGI_SWAP_CHAIN_DESC1 m_swapChainDesc;
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC m_swapChainFullScreenDesc;
    Microsoft::WRL::ComPtr<IDXGISwapChain1> m_swapChain;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_swapChainBuffers[2];
    UINT m_currentBackBuffer = 0;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap;
    UINT m_descriptorRTVSize = 0;
    bool m_isFullScreen = false;
};