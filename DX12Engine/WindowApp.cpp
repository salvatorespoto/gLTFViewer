#include "WindowApp.h"
#include "Renderer.h"
#include "Mesh.h"
#include "World.h"
#include "Camera.h"
#include "FrameContext.h"
#include "Buffers.h"
#include "GUI.h"

using Microsoft::WRL::ComPtr;

using DirectX::XMStoreFloat4x4;
using DirectX::XMMatrixMultiply;
using DirectX::XMLoadFloat4x4;
using DirectX::XMConvertToRadians;
using DXUtil::ThrowIfFailed;

LRESULT CALLBACK wndMsgCallback(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam)) return true;

    WindowApp* wndApp = reinterpret_cast<WindowApp*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    if (wndApp) wndApp->wndMsgProc(hWnd, uMsg, wParam, lParam);
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

WindowApp::WindowApp(HINSTANCE hInstance) : m_hInstance(hInstance), m_hWnd(NULL), m_clientWidth(1280), m_clientHeight(800)
{
    //setUpViewport();
    //setUpScissorRect();
    m_renderer = std::make_shared<Renderer>();
    m_camera = std::make_unique<Camera>(m_clientWidth, m_clientHeight, DirectX::XM_PIDIV4, 1.0f, 1000.0f);
    m_camera->setPosition({ 0.0f, 0.0f, -5.0f });
    InitWIC();
}

WindowApp::~WindowApp()
{}

void WindowApp::init()
{
    InitWindow(); 
    m_renderer->Init(m_hWnd, m_clientWidth, m_clientHeight);
    //GUI.Init(m_renderer);
    LoadWorld();

   // m_swapChain->SetFullscreenState(true, nullptr);
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
            DispatchMessage(&msg);                  // Dispatch the message to the window callback procedure
        }
        else
        {
            if (m_isAppPaused || m_isAppMinimized) continue;
            Update();
            draw();
        }
    }

    // Doing cleanup: cannot call onDestroy on WM_CLOSE or WM_DESTROY becouse releasing m_device will result in a null pointer exception
    onDestroy();

    return (int)msg.wParam;
}

LRESULT WindowApp::wndMsgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_DESTROY:
            PostQuitMessage(0); // End the message looop
            return 0;

        case WM_CLOSE:          // The window has been closed (for example clicking on X)
            return 0;

        case WM_ENTERSIZEMOVE:  // The user is dragging the window borders
            OnEnterSizeMove();
            return 0;
    
        case WM_EXITSIZEMOVE:  // The user finished dragging the window borders
            OnExitSizeMove();
            return 0;

        case WM_SIZE:
            if (wParam == SIZE_MINIMIZED) OnAppMinimized(); 
            else OnResize((UINT)LOWORD(lParam), (UINT)HIWORD(lParam));
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

        case WM_MOUSEMOVE:
            onMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;

        case WM_KEYDOWN:
            onKeyDown(wParam);
            return 0;

        case WM_WINDOWPOSCHANGED:
            /*if(m_swapChain) 
            {
                m_swapChain->GetFullscreenState(&fullscreen, nullptr);
                if (fullscreen != m_fullScreen)
                {
                    m_clientWidth = m_fullScreenMode.Width;
                    m_clientHeight = m_fullScreenMode.Height;
                    onFullScreenSwitch();
                }
            }*/
            return 0;
        default:
            break;
    }

    return 0;
}

void WindowApp::InitWIC() 
{
//#if (_WIN32_WINNT >= 0x0A00 /*_WIN32_WINNT_WIN10*/)
//    Microsoft::WRL::Wrappers::RoInitializeWrapper initialize(RO_INIT_MULTITHREADED);
//    if (FAILED(initialize))
//        return;
//#else
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
        return;
        // error
//#endif
}
/*
void WindowApp::SetFullScreen(bool fullScreen)
{
    m_swapChain.SetFullScreen(fullScreen);


    / *
    if (m_device != NULL)
    {
    
        setUpViewport();
        setUpScissorRect();
        //m_camera = std::make_unique<Camera>(m_clientWidth, m_clientHeight, DirectX::XM_PIDIV4, 1.0f, 1000.0f);
        //m_camera->setPosition({ 0.0f, 0.0f, -5.0f });

        flushCmdQueue();
        ImGui_ImplDX12_InvalidateDeviceObjects();
        createSwapChain();
        //resizeSwapChain((UINT)LOWORD(lParam), (UINT)HIWORD(lParam));
        createDepthStencilBuffer();
        createDepthStencilBufferView();
        ImGui_ImplDX12_CreateDeviceObjects();
    }* /
}*/

void WindowApp::InitWindow()
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
    //wndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);   // White background 
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
void WindowApp::OnEnterSizeMove() 
{
    m_isAppPaused = true;
    m_isResizingWindow = true;
}

void WindowApp::OnExitSizeMove()
{
    m_isAppPaused = false;
    m_isResizingWindow = false;
    OnResize(m_clientWidth, m_clientHeight);
}

void WindowApp::OnAppMinimized() 
{
    m_isAppMinimized = true;
}

void WindowApp::OnResize(UINT width, UINT height)
{
    m_clientWidth = width;
    m_clientHeight = height;
    m_isAppMinimized = false;

    if (m_isAppPaused || m_isResizingWindow) return;
    m_renderer->SetSize(width, height);
   
    
    //if (m_device != NULL)
    //{
    //    flushCmdQueue();
        m_camera->setLens(DirectX::XM_PIDIV4, static_cast<float>(m_clientWidth) / static_cast<float>(m_clientHeight), 1.0f, 1000.f);
       // setUpViewport();
       // setUpScissorRect();
        //m_swapChain.Resize(m_clientWidth, m_clientHeight);
        //createDepthStencilBuffer();
        //createDepthStencilBufferView();
        GUI.RecreateDeviceObjects();

    //}
};

void WindowApp::onMouseMove(WPARAM btnState, int x, int y) 
{
    // Rotate camera on mouse left button
    if (btnState == MK_RBUTTON) 
    {
        float rotWorldUp= m_mouseSensitivity * XMConvertToRadians(static_cast<float>(x - m_lastMousePosX));
        float  pitch = m_mouseSensitivity* XMConvertToRadians(static_cast<float>(y - m_lastMousePosY));
        m_camera->rotate(pitch, rotWorldUp);
    }
 
    // Update cached mouse position
    m_lastMousePosX = x;
    m_lastMousePosY = y;
}; 

void WindowApp::onMouseDown(WPARAM btnState, int x, int y) 
{};

void WindowApp::onMouseUp(WPARAM btnState, int x, int y) 
{};

void WindowApp::onKeyDown(WPARAM wParam)
{
    if (wParam == VK_ESCAPE) DestroyWindow(m_hWnd);
    
    // Handle camera movements
    if (wParam == VK_KEY_W) m_camera->moveForward(0.2f);
    if (wParam == VK_KEY_S) m_camera->moveForward(-0.2f);
    if (wParam == VK_KEY_A) m_camera->strafe(-0.2f);
    if (wParam == VK_KEY_D) m_camera->strafe(0.2f);
}


/*
void WindowApp::createFrameContexts()
{
    // CPU will update "m_frameInFlight" frames without wainting for the GPU to complete the previous
    m_frameContexts.resize(m_frameInFlight);

    // Eeach frame context has its own command allocator
    for (FrameContext2 &frameContext : m_frameContexts)
    {
        ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&frameContext.m_cmdListAlloc)), "Cannot create command allocator");
    }
}
*/

void WindowApp::LoadWorld()
{
    m_world.Init(m_renderer);
    m_world.LoadGLTF("models/DamagedHelmet.glb");  
}

void WindowApp::UpdateCamera()
{
    m_camera->update();
    PassConstants passConstants;
    passConstants.projMtx = m_camera->getProjMtx();
    passConstants.viewMtx = m_camera->getViewMtx();
    XMStoreFloat4x4(&passConstants.projViewMtx, DirectX::XMMatrixTranspose(XMMatrixMultiply(XMLoadFloat4x4(&passConstants.viewMtx), XMLoadFloat4x4(&passConstants.projMtx))));
    m_renderer->UpdatePassConstants(passConstants);
}

void WindowApp::Update()
{
    //m_swapChain.SetFullScreen(m_state.m_isAppFullscreen);
    UpdateCamera();
}

//// Render the scene
void WindowApp::draw()
{
    m_renderer->NewFrame();
    m_world.draw();
    //GUI.Draw(&m_state);
    m_renderer->EndFrame();
}

//// Cleanup

void WindowApp::onDestroy()
{
    // Assigning nullptr to ComPtr smar pointer release the interface and set the holded pointer to null
    /*
    if (m_device != nullptr) flushCmdQueue();
    GUI.ShutDown();

    m_passConstantBuffer->release();
    if (m_cmdList) { m_cmdList = nullptr; }
    m_world.release();
    if (m_depthStencilBuffer) { m_depthStencilBuffer = nullptr; }
    if (m_commandQueue) { m_commandQueue = nullptr; }
    if (m_cbv_srv_DescriptorHeap) { m_cbv_srv_DescriptorHeap = nullptr; }
    if (m_dsvDescriptorHeap) { m_dsvDescriptorHeap = nullptr; }
    if (m_fence) { m_fence = nullptr; }
    if (m_device) { m_device = nullptr; }
    if (m_rootSignature) { m_rootSignature = nullptr; }
    if (m_pipelineState) { m_pipelineState = nullptr; }

    */
    /*
#ifdef ENABLE_DX12_DEBUG_LAYER
    ComPtr<IDXGIDebug> debugController = NULL;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debugController))))
    {
        debugController->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
        debugController->Release();
    }
#endif
*/
}