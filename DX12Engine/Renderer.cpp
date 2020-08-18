#include "Renderer.h"
#include "Buffers.h"
#include "FrameContext.h"
#include "Mesh.h"

using Microsoft::WRL::ComPtr;
using DXUtil::ThrowIfFailed;

void Renderer::Init(HWND hWnd, unsigned int width, unsigned int height)
{
    m_hWnd = hWnd;
    m_width = width; m_height = height;
    SetViewport({ 0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height), 0.0f, 1.0f });
    SetScissorRect({ 0, 0, static_cast<long>(m_width), static_cast<long>(m_height) });
    EnableDebugLayer();
    CreateDefaultDevice();
    CreateCommandQueue();
    CreateFence();
    CreateSwapChain();
    CreateDepthStencilBuffer();
    CreateConstantBuffer();
    CompileShaders();
    CreateRootSignature();
    CreatePipelineState();
}

void Renderer::SetSize(unsigned int width, unsigned int height)
{
    FlushCommandQueue();
    SetViewport({ 0.0f, 0.0f, static_cast<float>(m_width), static_cast<float>(m_height), 0.0f, 1.0f });
    SetScissorRect({ 0, 0, static_cast<long>(m_width), static_cast<long>(m_height) });
    m_swapChain.Resize(m_width, m_height);
    CreateDepthStencilBuffer();
}

void Renderer::SetViewport(D3D12_VIEWPORT viewPort)
{
    m_viewPort = viewPort;
}

void Renderer::SetScissorRect(D3D12_RECT scissorRect)
{
    m_scissorRect = scissorRect;
}

void Renderer::SetFullScreen(bool fullScreen)
{
    m_swapChain.SetFullScreen(fullScreen);
}

HWND Renderer::GetWindowHandle()
{
    return m_hWnd;
}

Microsoft::WRL::ComPtr<ID3D12Device> Renderer::GetDevice()
{
    return m_device;
}

void Renderer::EnableDebugLayer()
{
#ifdef ENABLE_DX12_DEBUG_LAYER
    ComPtr<ID3D12Debug> debugController;
    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)), "Cannot enable debug layer");
    debugController->EnableDebugLayer();
#endif
}

void Renderer::CreateDefaultDevice()
{
    // Create the device using the default Direct3D adapter
    ComPtr<IDXGIFactory4> dxgiFactory;
    if (SUCCEEDED(D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_12_1, _uuidof(ID3D12Device), nullptr)))
    {
        ThrowIfFailed(D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_device)), "Cannot create Direct3D device on default adapter");
    }
    else
    {
        // Fallback on the WARP software adapter
        ComPtr<IDXGIAdapter> warpAdapter;
        ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)), "Cannot create Direct3D device on WARP adapter");
        ThrowIfFailed(D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)), "Cannot create WARP adapter");
    }
}

void Renderer::GetDisplayModes()
{
    DXGI_ADAPTER_DESC aDesc;
    ComPtr<IDXGIAdapter1> adapter = DXUtil::enumerateAdapters()[0];
    adapter->GetDesc(&aDesc);
    ComPtr<IDXGIOutput> output = DXUtil::enumerateAdaptersOutputs(adapter.Get())[0];
    std::vector<DXGI_MODE_DESC> m_displayModes = DXUtil::enumerateAdapterOutputDisplayModes(output.Get(), DXGI_FORMAT_B8G8R8A8_UNORM);
    m_fullScreenMode = m_displayModes[m_displayModes.size() - 1];
}

void Renderer::CreateSwapChain()
{
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
    swapChainDesc.Width = m_width;
    swapChainDesc.Height = m_height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullScreenSwapChainDesc;
    fullScreenSwapChainDesc.RefreshRate = m_fullScreenMode.RefreshRate;
    fullScreenSwapChainDesc.Scaling = m_fullScreenMode.Scaling;
    fullScreenSwapChainDesc.ScanlineOrdering = m_fullScreenMode.ScanlineOrdering;
    fullScreenSwapChainDesc.Windowed = !m_isFullScreen;

    m_swapChain.Create(m_device, swapChainDesc, fullScreenSwapChainDesc, m_hWnd, m_commandQueue);
}


void Renderer::CreateCommandQueue()
{
    D3D12_COMMAND_QUEUE_DESC cqDesc = { D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_QUEUE_PRIORITY_NORMAL, D3D12_COMMAND_QUEUE_FLAG_NONE, 1 };
    ThrowIfFailed(m_device->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&m_commandQueue)), "Cannot create command queue");
    ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandListAlloc)), "Cannot create command allocator");
    ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandListAlloc.Get(), nullptr, IID_PPV_ARGS(&m_commandList)), "Cannot create the command list");
    m_commandList->Close();
}

void Renderer::CreateFence()
{
    ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)), "Cannot create fence");
}

void Renderer::FlushCommandQueue()
{
    m_currentFenceValue++;
    m_commandQueue->Signal(m_fence.Get(), m_currentFenceValue);

    if (m_fence->GetCompletedValue() < m_currentFenceValue)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
        ThrowIfFailed(m_fence->SetEventOnCompletion(m_currentFenceValue, eventHandle), "Cannot set fence event on completion");
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }
}

void Renderer::CreateDepthStencilBuffer()
{
    // Create depth stencil resource
    D3D12_RESOURCE_DESC depthStencilDesc;
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = m_width;
    depthStencilDesc.Height = m_height;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.SampleDesc.Quality = 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE optClear;
    optClear.Format = DXGI_FORMAT_D32_FLOAT;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;

    ThrowIfFailed(m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &depthStencilDesc, D3D12_RESOURCE_STATE_COMMON, &optClear, IID_PPV_ARGS(m_depthStencilBuffer.GetAddressOf())),
        "Cannot create depth stencile buffer resource");

    // Create the descriptor heap for the depth stencil view
    m_DSV_DescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    dsvHeapDesc = {};
    dsvHeapDesc.NumDescriptors = 1;
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_DSV_DescriptorHeap)),
        "Cannot create depth stencil buffer descriptor heap");

    // Create the depth stencil view
    m_device->CreateDepthStencilView(m_depthStencilBuffer.Get(), nullptr, GetDepthStencilView());
}

D3D12_CPU_DESCRIPTOR_HANDLE Renderer::GetCurrentBackBufferView() const
{
    return m_swapChain.GetCurrentBackBufferView();
}

D3D12_CPU_DESCRIPTOR_HANDLE Renderer::GetDepthStencilView() const
{
    return m_DSV_DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
}

void Renderer::CreateConstantBuffer()
{
    m_passConstantBuffer = std::make_unique<UploadBuffer<PassConstants>>(m_device.Get(), 1, true);

    m_CBV_SRV_DescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    D3D12_DESCRIPTOR_HEAP_DESC CBV_SRV_heapDesc = {};
    CBV_SRV_heapDesc = {};
    CBV_SRV_heapDesc.NumDescriptors = 1;
    CBV_SRV_heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    CBV_SRV_heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; 
    ThrowIfFailed(m_device->CreateDescriptorHeap(&CBV_SRV_heapDesc, IID_PPV_ARGS(&m_CBV_SRV_DescriptorHeap)),
        "Cannot create CBV SRV UAV descriptor heap");

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
    D3D12_GPU_VIRTUAL_ADDRESS cbAddress = m_passConstantBuffer->getResource()->GetGPUVirtualAddress();
    cbvDesc.BufferLocation = cbAddress;
    cbvDesc.SizeInBytes = DXUtil::padByteSizeTo256Mul(sizeof(PassConstants));

    CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHeapHandle_CPU(m_CBV_SRV_DescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    m_device->CreateConstantBufferView(&cbvDesc, cbvHeapHandle_CPU);
}

void Renderer::CompileShaders()
{

#if defined(_DEBUG)
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compileFlags = 0;
#endif

    ThrowIfFailed(D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "VSMain", "vs_5_1", compileFlags, 0, &m_vertexShader, nullptr),
        "Cannot compile vertex shader");

    ThrowIfFailed(D3DCompileFromFile(L"shaders.hlsl", nullptr, nullptr, "PSMain", "ps_5_1", compileFlags, 0, &m_pixelShader, nullptr),
        "Cannot compile pixel shader");
}

void Renderer::CreateRootSignature()
{
    CD3DX12_ROOT_PARAMETER rootParameters[1];

    CD3DX12_DESCRIPTOR_RANGE cbvTable;
    cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
    rootParameters[0].InitAsDescriptorTable(1, &cbvTable);

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    ThrowIfFailed(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf()), "Cannot serialize root signature");
    ThrowIfFailed(m_device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)), "Cannot create root signature");
}

void Renderer::CreatePipelineState()
{
    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
    inputLayoutDesc.pInputElementDescs = vertexElementsDesc;
    inputLayoutDesc.NumElements = 1;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { vertexElementsDesc, 1 };
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = { reinterpret_cast<UINT8*>(m_vertexShader->GetBufferPointer()), m_vertexShader->GetBufferSize() };
    psoDesc.PS = { reinterpret_cast<UINT8*>(m_pixelShader->GetBufferPointer()), m_pixelShader->GetBufferSize() };
    D3D12_RASTERIZER_DESC rasterDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    rasterDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;
    psoDesc.RasterizerState = rasterDesc;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;
    ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)), "Cannot create the pipeline state");
}

Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> Renderer::GetCommandList() 
{
    return m_commandList;
}

void Renderer::ExecuteCommandList(ID3D12GraphicsCommandList* commandList)
{
    ID3D12CommandList* commandLists[] = { commandList };
    m_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
}

void Renderer::UpdatePassConstants(const PassConstants& passConstants)
{
    DirectX::XMFLOAT4X4 projViewMtx;
    XMStoreFloat4x4(&projViewMtx, DirectX::XMMatrixTranspose(XMMatrixMultiply(XMLoadFloat4x4(&passConstants.viewMtx), XMLoadFloat4x4(&passConstants.projMtx))));
    m_passConstantBuffer->copyData(0, passConstants);
}

void Renderer::SetPipelineState(ID3D12GraphicsCommandList* commandList)
{
    commandList->SetPipelineState(m_pipelineState.Get());
}

void Renderer::SetRootSignature(ID3D12GraphicsCommandList* commandList)
{
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    ID3D12DescriptorHeap* descriptorHeaps[] = { m_CBV_SRV_DescriptorHeap.Get() };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    CD3DX12_GPU_DESCRIPTOR_HANDLE cbv(m_CBV_SRV_DescriptorHeap->GetGPUDescriptorHandleForHeapStart());
    cbv.Offset(0, m_CBV_SRV_DescriptorSize);
    commandList->SetGraphicsRootDescriptorTable(0, cbv);
}

void Renderer::ResetCommandList() 
{
    ThrowIfFailed(m_commandListAlloc->Reset(), "Cannot reset allocator");
    ThrowIfFailed(m_commandList->Reset(m_commandListAlloc.Get(), nullptr), "Cannot reset command list");
}

void Renderer::NewFrame() 
{
    ThrowIfFailed(m_commandListAlloc->Reset(), "Cannot reset allocator");
    ThrowIfFailed(m_commandList->Reset(m_commandListAlloc.Get(), nullptr), "Cannot reset command list");
    SetPipelineState(m_commandList.Get());
    SetRootSignature(m_commandList.Get());
    m_commandList->RSSetViewports(1, &m_viewPort);
    m_commandList->RSSetScissorRects(1, &m_scissorRect);
    //m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_depthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));
    m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_swapChain.GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
    m_commandList->ClearRenderTargetView(m_swapChain.GetCurrentBackBufferView(), DirectX::Colors::LightSteelBlue, 0, nullptr);
    m_commandList->OMSetRenderTargets(1, &m_swapChain.GetCurrentBackBufferView(), FALSE, &GetDepthStencilView());
}

void Renderer::EndFrame()
{
    m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_swapChain.GetCurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
    //m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_depthStencilBuffer.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_COMMON));
    m_commandList->Close();
    ExecuteCommandList(m_commandList.Get());
    FlushCommandQueue();
    m_swapChain.Present();
}