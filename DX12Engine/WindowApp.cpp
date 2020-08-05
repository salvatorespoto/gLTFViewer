#include "WindowApp.h"
#include "DirectXColors.h"

using Microsoft::WRL::ComPtr;


LRESULT CALLBACK wndMsgCallback(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WindowApp* wndApp = reinterpret_cast<WindowApp*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    if (wndApp) wndApp->wndMsgProc(hWnd, uMsg, wParam, lParam);
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

WindowApp::WindowApp(HINSTANCE hInstance) : m_hInstance(hInstance), m_hWnd(NULL), m_clientWidth(1280), m_clientHeight(800)
{
    setUpViewport();
    setUpScissorRect();
}

WindowApp::~WindowApp()
{}

void WindowApp::init()
{
    // The calling order of the following functions matters to correctly set up the application

    initWindow();
    enableDebugLayer();
    createDefaultDevice();
    createDXGIFactory();
    createCommandObjects();
    createFence();
    createDescriptorHeaps();
    //createFrameContexts();
    setBackBufferFormat();
    //checkMultisampling();
    createSwapChain();
    createRenderTargetViews();
    createDepthStencilBuffer();
    createDepthStencilBufferView();
    flushCmdQueue();
}

int WindowApp::run()
{
    showWindow();

    MSG msg = { WM_NULL };
    while (msg.message != WM_QUIT)  // WM_QUIT indicate that the application should be closed
    {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) // Process window messages without blocking the loop
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);  // Dispatch the message to the window callback procedure
        }
        else
        {
            update();   // Update the app
            draw();
        }
    }

    // Doing cleanup: cannot call onDestroy on WM_CLOSE or WM_DESTROY becouse releasing m_device will result in a null pointer exception
    onDestroy();

    return (int)msg.wParam;
}

void WindowApp::update()
{}

LRESULT WindowApp::wndMsgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_DESTROY:
            PostQuitMessage(0); // End the message looop
            return 0;

        case WM_CLOSE:  // The window has been closed (for example clicking on X)
            return 0;

        case WM_SIZE:   // Window has been resized
            onResize();
            break;

        case WM_LBUTTONDOWN:    // A mouse button has been pressed
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
            // wParam identifies wich button has been pressed, 
            // while the macros GET_#_LPARAM get the coordinates from lParam
            onMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;

        case WM_LBUTTONUP:    // A mouse button has been released:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
            // wParam identifies wich button has been pressed, 
            // while the macros GET_#_LPARAM get the coordinates from lParam
            onMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;

        case WM_KEYDOWN:
            // wParam identifies wich button has been pressed, 
            onKeyDown(wParam);
            return 0;

        default:
            break;
    }

    return 0;
}

void WindowApp::initWindow()
{
    WNDCLASSEX wndClass = {};
    wndClass.cbSize = sizeof(WNDCLASSEX);                   // Size of the struct
    wndClass.style = CS_CLASSDC | CS_HREDRAW | CS_VREDRAW;  // Styles: here set redraw when horizontal or vertical resize occurs
    wndClass.lpfnWndProc = wndMsgCallback;                  // Handle to the procedure that will handle the window messages    
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = m_hInstance;                   // Application instance
    wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);   // Default application icon
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);     // Default cursor
    wndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);   // White background 
    wndClass.lpszMenuName = NULL;
    wndClass.lpszClassName = L"WindowApp";
    wndClass.hIconSm = LoadIcon(NULL, IDI_WINLOGO);     // Default small icon

    // It's mandatory to register the class to create the window associated
    if (!RegisterClassEx(&wndClass)) throw std::exception("Cannot register window class in WindowApp::initWindow()");

    DWORD style = WS_OVERLAPPEDWINDOW;                                      // Window styles
    DWORD exStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;                     // Window extended styles
    RECT wr = { 0, 0, LONG(m_clientWidth), LONG(m_clientHeight) };          // Size of the window client area 
    AdjustWindowRectEx(&wr, style, FALSE, exStyle); // Compute the adjusted size of the window taking styles into account

    // Create the window
    m_hWnd = CreateWindowEx(
        exStyle,                        // Extended styles
        L"WindowApp",		            // Class name
        L"WindowApp",       			// Application name
        style,							// Window styles
        CW_USEDEFAULT,                  // Use defaults x coordinate
        CW_USEDEFAULT,	                // Use defaults y coordinate
        wr.right - wr.left,				// Width
        wr.bottom - wr.top,				// Height
        NULL,							// Handle to parent
        NULL,							// Handle to menu
        m_hInstance,				    // Application instance
        NULL);							// no extra parameters

    if (!m_hWnd) throw std::exception("Cannot create the window");

    // Save the pointer to this class so it's could be used from wndMsgCallback
    SetWindowLongPtr(m_hWnd, GWLP_USERDATA, (LONG_PTR)this);
}

void WindowApp::showWindow()
{
    ShowWindow(m_hWnd, SW_SHOW);
    SetForegroundWindow(m_hWnd);
    SetFocus(m_hWnd);
}

float WindowApp::getScreenAspectRatio()
{
    return static_cast<float>(m_clientWidth) / static_cast<float>(m_clientHeight);
}


//// Event handlers

void WindowApp::onResize() 
{};

void WindowApp::onMouseMove(WPARAM btnState, int x, int y) 
{}; 

void WindowApp::onMouseDown(WPARAM btnState, int x, int y) 
{};

void WindowApp::onMouseUp(WPARAM btnState, int x, int y) 
{};

void WindowApp::onKeyDown(WPARAM wParam)
{
    if (wParam == VK_ESCAPE) DestroyWindow(m_hWnd);
}


//// Direct3D function

void WindowApp::enableDebugLayer()
{
#ifdef ENABLE_DX12_DEBUG_LAYER
    ComPtr<ID3D12Debug> debugController;
    DXUtil::ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)), "Cannot enable debug layer");
    debugController->EnableDebugLayer();
#endif
}

void WindowApp::createDXGIFactory() 
{
    DXUtil::ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&m_dxgiFactory)), "Cannot create DXGI factory");
}

void WindowApp::createDefaultDevice()
{
    ComPtr<IDXGIFactory4> dxgiFactory;
    if (SUCCEEDED(D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_12_1, _uuidof(ID3D12Device), nullptr)))
    {
        // Create the device using the default Direct3D adapter
        DXUtil::ThrowIfFailed(D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_device)), "Cannot create Direct3D device on default adapter");
    }
    else
    {
        // Create the device on the WARP software adapter
        ComPtr<IDXGIAdapter> warpAdapter;
        DXUtil::ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)), "Cannot create Direct3D device on WARP adapter");
        DXUtil::ThrowIfFailed(D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)), "Cannot create WARP adapter");
    }
}

void WindowApp::createCommandObjects()
{
    // Create the command queue
    D3D12_COMMAND_QUEUE_DESC cqDesc = { D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_QUEUE_PRIORITY_NORMAL, D3D12_COMMAND_QUEUE_FLAG_NONE, 1};
    DXUtil::ThrowIfFailed(m_device->CreateCommandQueue(&cqDesc, IID_PPV_ARGS(&m_commandQueue)), "Cannot create command queue");
    DXUtil::ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_cmdListAlloc)), "Cannot create command allocator");
    DXUtil::ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_cmdListAlloc.Get(), nullptr, IID_PPV_ARGS(m_cmdList.GetAddressOf())), "Cannot create the command list");
    m_cmdList->Close();
}

void WindowApp::createFrameContexts()
{
    // CPU will update "m_frameInFlight" frames without wainting for the GPU to complete the previous
    m_frameContexts.resize(m_frameInFlight);

    // Eeach frame context has its own command allocator
    for (FrameContext2 &frameContext : m_frameContexts)
    {
        DXUtil::ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&frameContext.m_cmdListAlloc)), "Cannot create command allocator");
    }
}

void WindowApp::setBackBufferFormat()
{
    m_backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
}

void WindowApp::checkMultisampling()
{
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS multisampleQL;
    multisampleQL.Format = m_backBufferFormat;
    multisampleQL.SampleCount = 4;
    multisampleQL.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    multisampleQL.NumQualityLevels = 0;
    
    DXUtil::ThrowIfFailed(m_device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &multisampleQL, sizeof(multisampleQL)), "Cannot enable multisamppling");
    m_MSAASampleCount = (multisampleQL.SampleCount == 4) ? 4 : 1;
    m_MSAAQualityLevel = (multisampleQL.NumQualityLevels > 0) ? multisampleQL.NumQualityLevels - 1 : 0;
}

void WindowApp::createSwapChain()
{
    // Release the previous swap chain, if any
    m_swapChain.Reset();

    DXGI_SWAP_CHAIN_DESC1 scDesc;
    
    scDesc.Width = m_clientWidth;
    scDesc.Height = m_clientHeight;
    scDesc.Format = m_backBufferFormat;
    scDesc.Stereo = FALSE;
    scDesc.SampleDesc.Count = m_MSAASampleCount;
    scDesc.SampleDesc.Quality = m_MSAAQualityLevel;
    scDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    scDesc.Scaling = DXGI_SCALING_STRETCH;
    scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.BufferCount = 2; // Two buffers in the swap chain for double buffering
    scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        
    DXUtil::ThrowIfFailed(m_dxgiFactory->CreateSwapChainForHwnd(m_commandQueue.Get(), m_hWnd, &scDesc, nullptr, nullptr, m_swapChain.GetAddressOf()), "Cannot create swap chain");
}

void WindowApp::createFence()
{
    DXUtil::ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)), "Cannot create fence");
}

void WindowApp::createDescriptorHeaps()
{
    // Get descriptors sizes
    m_descriptor_CBV_SRV_size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_descriptor_RTV_size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_descriptor_DSV_size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    // Crete descriptor for back buffers
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = 2;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    DXUtil::ThrowIfFailed(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_rtvDescriptorHeap)), "Cannot create back buffer descriptor heap");

    heapDesc = {};
    heapDesc.NumDescriptors = 1;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    DXUtil::ThrowIfFailed(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_dsvDescriptorHeap)), "Cannot create depth stencil buffer descriptor heap");

    heapDesc = {};
    heapDesc.NumDescriptors = 1;
    heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // Constant buffer resources have to be accessed from shaders
    DXUtil::ThrowIfFailed(m_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_cbv_srv_DescriptorHeap)), "Cannot create CBV SRV UAV descriptor heap");
}

void WindowApp::createRenderTargetViews()
{
    // CD3DX12_CPU_DESCRIPTOR_HANDLE is an helper structure that create a D3DX12_CPU_DESCRIPTOR_HANDLE
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle_CPU(m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

    // Create a render target view to buffers
    for (UINT i = 0; i < 2; i++)    // There are two buffers in the swap chain
    {
        // Get the buffer
        m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_swapChainBuffers[i]));

        // Create a view associated to it
        m_device->CreateRenderTargetView(m_swapChainBuffers[i].Get(), nullptr, rtvHeapHandle_CPU); 
    
        // Point to the next descriptor
        rtvHeapHandle_CPU.Offset(1, m_descriptor_RTV_size);
    }
}

void WindowApp::createDepthStencilBuffer() 
{
    D3D12_RESOURCE_DESC depthStencilDesc;
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = m_clientWidth;
    depthStencilDesc.Height = m_clientHeight; 
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.Format = m_depthStencilBufferFormat;
    depthStencilDesc.SampleDesc.Count = m_MSAASampleCount;
    depthStencilDesc.SampleDesc.Quality = m_MSAAQualityLevel;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    
    D3D12_CLEAR_VALUE optClear;
    optClear.Format = m_depthStencilBufferFormat;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;
    
    DXUtil::ThrowIfFailed(m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &depthStencilDesc, D3D12_RESOURCE_STATE_COMMON, &optClear, IID_PPV_ARGS(m_depthStencilBuffer.GetAddressOf())), 
        "Cannot create depth stencile buffer resource");
}

void WindowApp::createDepthStencilBufferView() 
{
    m_device->CreateDepthStencilView(m_depthStencilBuffer.Get(), nullptr, m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}

void WindowApp::setUpViewport()
{
    m_viewPort.TopLeftX = 0.0f;
    m_viewPort.TopLeftY = 0.0f;
    m_viewPort.Width = static_cast<float>(m_clientWidth);
    m_viewPort.Height = static_cast<float>(m_clientHeight);
    m_viewPort.MinDepth = 0.0f;
    m_viewPort.MaxDepth = 1.0f;
}

void WindowApp::setUpScissorRect()
{
    m_scissorRect = { 0, 0, static_cast<long>(m_clientWidth), static_cast<long>(m_clientHeight) };
}

void WindowApp::flushCmdQueue()
{
    m_currentFenceValue++;
    m_commandQueue->Signal(m_fence.Get(), m_currentFenceValue);

    // Wait until the fence has been updated
    if (m_fence->GetCompletedValue() < m_currentFenceValue)
    {
        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
        DXUtil::ThrowIfFailed(m_fence->SetEventOnCompletion(m_currentFenceValue, eventHandle), "Cannot set fence event on completion");
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }
}

ID3D12Resource* WindowApp::getCurrentBackBuffer() const
{
    return m_swapChainBuffers[m_currentBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE WindowApp::getCurrentBackBufferView() const
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_currentBackBuffer, m_descriptor_RTV_size);
} 

D3D12_CPU_DESCRIPTOR_HANDLE WindowApp::getDepthStencilView() const
{
    return m_dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
}

//// Render the scene
void WindowApp::draw() 
{
    DXUtil::ThrowIfFailed(m_cmdListAlloc->Reset(), "Cannot reset allocator");
    DXUtil::ThrowIfFailed(m_cmdList->Reset(m_cmdListAlloc.Get(), nullptr), "Cannot reset command list");

    m_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_depthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));
    m_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(getCurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
    
    m_cmdList->RSSetViewports(1, &m_viewPort);
    m_cmdList->RSSetScissorRects(1, &m_scissorRect);
    m_cmdList->ClearRenderTargetView(getCurrentBackBufferView(), DirectX::Colors::LightSteelBlue, 0, nullptr);
    m_cmdList->ClearDepthStencilView(getDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
    
    m_cmdList->OMSetRenderTargets(1, &getCurrentBackBufferView(), FALSE, &getDepthStencilView());
    
    m_cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(getCurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
    
    DXUtil::ThrowIfFailed(m_cmdList->Close(), "Cannot close command list");
    
    ID3D12CommandList* cmdsLists[] = { m_cmdList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
    
    DXUtil::ThrowIfFailed(m_swapChain->Present(0, 0), "Cannot Present() on swap chain");
    m_currentBackBuffer = (m_currentBackBuffer  + 1) % 2;
    
    flushCmdQueue();
}


//// Cleanup

void WindowApp::onDestroy()
{
    if (m_device) { m_device->Release(); m_device = nullptr; }

#ifdef ENABLE_DX12_DEBUG_LAYER
    ComPtr<IDXGIDebug> debugController = NULL;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debugController))))
    {
        debugController->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
        debugController->Release();
    }
#endif
}