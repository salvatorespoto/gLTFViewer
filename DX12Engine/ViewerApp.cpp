#include "ViewerApp.h"

#include "Buffers.h"
#include "Texture.h"
#include "Renderer.h"
#include "Camera.h"
#include "Mesh.h"
#include "Scene.h"
#include "SkyBox.h"
#include "Grid.h"
#include "GUI.h"
#include "GLTFSceneLoader.h"

#include "using_directives.h"


LRESULT CALLBACK wndMsgCallback(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam)) return true;
    ViewerApp* wndApp = reinterpret_cast<ViewerApp*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    if (wndApp) wndApp->WndMsgProc(hWnd, uMsg, wParam, lParam);
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

ViewerApp::ViewerApp(const HINSTANCE& hInstance) : m_hInstance(hInstance), m_hWnd(NULL), m_clientWidth(1280), m_clientHeight(1024)
{
    DEBUG_LOG("Initializing Viewer application");
    InitWindow();
    InitCamera();
    InitWIC();
    InitRenderer();
    InitScene();
    InitGui();
}

ViewerApp::~ViewerApp() {}

void ViewerApp::InitWindow()
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
    wndClass.lpszClassName = L"ViewerApp";
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
        L"ViewerApp",		            // Class name
        L"GLTF Viewer",       			// Application name
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

    m_appState.isAppWindowed = true;
    DEBUG_LOG("Main window initialized");
}

void ViewerApp::InitRenderer() 
{
    m_renderer = std::make_shared<Renderer>();
    m_renderer->Init(m_hWnd, m_clientWidth, m_clientHeight);
    DEBUG_LOG("Renderer object initialized");
}

void ViewerApp::InitCamera()
{
    m_camera = std::make_unique<Camera>(m_clientWidth, m_clientHeight, DirectX::XM_PIDIV4, 0.1f, 1000.0f);
    m_camera->setPosition({ 0.0f, 0.0f, -5.0f });
    DEBUG_LOG("Camera initialized");
}

void ViewerApp::InitScene()
{
    CreateTextureFromDDSFile(m_renderer->GetDevice().Get(), m_renderer->GetCommandQueue().Get(), "assets/wood-cubemap.dds", &m_cubeMapTexture);

    m_gltfLoader = std::make_unique<GLTFSceneLoader>(m_renderer->GetDevice(), m_renderer->GetCommandQueue());
    m_scene = std::make_shared<Scene>(m_renderer->GetDevice());
    m_scene->SetCubeMapTexture(m_cubeMapTexture);

    DEBUG_LOG("Initializing SkyBox")
    m_skyBox = std::make_unique<SkyBox>();
    m_skyBox->Init(m_renderer->GetDevice(), m_renderer->GetCommandQueue());
    m_skyBox->SetCubeMapTexture(m_cubeMapTexture);
    
    m_grid = std::make_unique<Grid>();
    m_grid->Init(m_renderer->GetDevice(), m_renderer->GetCommandQueue(), 100.0f);
    DEBUG_LOG("Scene initalized");
}

void ViewerApp::InitGui()
{
    m_gui = std::make_unique<Gui>();
    m_gui->Init(m_renderer, &m_appState);
    DEBUG_LOG("Gui initalized");
}

int ViewerApp::Run()
{
    DEBUG_LOG("Starting application main loop");
    ShowWindow(m_hWnd, SW_MAXIMIZE);
    SetForegroundWindow(m_hWnd);
    SetFocus(m_hWnd);

    MSG msg = { WM_NULL };
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) // Process window messages without blocking the loop
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);                 // Dispatch the message to the window callback procedure
        }
        else
        {
            if (m_appState.isAppPaused || m_appState.isAppMinimized) continue;
            OnUpdate();
            OnDraw();
        }
    }
    OnDestroy();
    
    return (int)msg.wParam;
}

LRESULT ViewerApp::WndMsgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_DESTROY:
            PostQuitMessage(0); // End the message looop -> quit application
            return 0;

        case WM_CLOSE:          // The window has been closed
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

        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
            if (m_gui->WantCaptureMouse()) return 0;
            // wParam identifies wich button has been pressed, while the macros GET_#_LPARAM get the coordinates from lParam
            OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;

        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
            if (m_gui->WantCaptureMouse()) return 0;
            OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;

        case WM_MOUSEMOVE:
            if (m_gui->WantCaptureMouse()) return 0;
            OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;

        case WM_KEYDOWN:
            if (m_gui->WantCaptureKeyboard()) return 0;
            OnKeyDown(wParam);
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

void ViewerApp::InitWIC() 
{
    DXUtil::ThrowIfFailed(CoInitializeEx(nullptr, COINIT_MULTITHREADED), "Cannot initialize WIC");
    DEBUG_LOG("WIC initialized");
}

void ViewerApp::SetFullScreen(bool fullScreen)
{
    m_renderer->SetFullScreen(fullScreen);
}

float ViewerApp::GetScreenAspectRatio()
{
    return static_cast<float>(m_clientWidth) / static_cast<float>(m_clientHeight);
}

void ViewerApp::OnEnterSizeMove() 
{
    m_appState.isAppPaused = true;
    m_appState.isResizingWindow = true;
}

void ViewerApp::OnExitSizeMove()
{
    m_appState.isAppPaused = false;
    m_appState.isResizingWindow = false;
    OnResize(m_clientWidth, m_clientHeight);
}

void ViewerApp::OnAppMinimized() 
{
    m_appState.isAppMinimized = true;
}

void ViewerApp::OnResize(UINT width, UINT height)
{
    float scaleGUI = static_cast<float>(m_appState.screenWidth) / static_cast<float>(m_clientWidth);

    m_clientWidth = width;
    m_clientHeight = height;
    m_appState.isAppMinimized = false;
    m_appState.screenWidth = width;
    m_appState.screenHeight = height;
    if (m_appState.isAppPaused || m_appState.isResizingWindow) return;
    
    //ImGui_ImplDX12_InvalidateDeviceObjects();
    m_renderer->SetSize(width, height);
    m_camera->setLens(DirectX::XM_PIDIV4, static_cast<float>(m_clientWidth) / static_cast<float>(m_clientHeight), 0.1f, 1000.f);
    //ImGui_ImplDX12_CreateDeviceObjects();
    //m_gui.RecreateDeviceObjects();
};

void ViewerApp::OnMouseMove(WPARAM btnState, int x, int y)
{
    if (btnState == MK_LBUTTON) 
    {
        // Rotate camera
        float rotWorldUp= m_mouseSensitivity * XMConvertToRadians(static_cast<float>(x - m_lastMousePosX));
        float  pitch = m_mouseSensitivity* XMConvertToRadians(static_cast<float>(y - m_lastMousePosY));
        m_camera->rotate(pitch, rotWorldUp);
    }

    if (btnState == MK_RBUTTON)
    {
        // Rotate mesh
        m_appState.modelConstants[0].rotXYZ.z += m_mouseSensitivity * XMConvertToRadians(static_cast<float>(x - m_lastMousePosX));
        m_appState.modelConstants[0].rotXYZ.x += XMConvertToRadians(static_cast<float>(y - m_lastMousePosY));
    }
 
    // Update cached mouse position
    m_lastMousePosX = x;
    m_lastMousePosY = y;
}; 

void ViewerApp::OnMouseDown(WPARAM btnState, int x, int y)
{};

void ViewerApp::OnMouseUp(WPARAM btnState, int x, int y)
{};

void ViewerApp::OnKeyDown(WPARAM wParam)
{    
    // Handle camera movements
    if (wParam == VK_KEY_W) m_camera->moveForward(m_cameraStep);
    if (wParam == VK_KEY_S) m_camera->moveForward(-m_cameraStep);
    if (wParam == VK_KEY_A) m_camera->strafe(-m_cameraStep);
    if (wParam == VK_KEY_D) m_camera->strafe(m_cameraStep);
}

void ViewerApp::UpdateScene()
{    
    // Update camera 
    m_camera->update();
    m_scene->SetCamera(*m_camera);

    // Update lights
    for (auto light : m_appState.lights) { m_scene->SetLight(light.first, light.second); }

    // Update mesh constants
    float rotX = m_appState.modelConstants[0].rotXYZ.x;
    float rotY = m_appState.modelConstants[0].rotXYZ.y;
    float rotZ = m_appState.modelConstants[0].rotXYZ.z;
    XMMATRIX meshRotation = XMMatrixMultiply(XMMatrixMultiply(XMMatrixRotationAxis({ 1.0f, 0.0f, 0.0f }, rotX), XMMatrixRotationAxis({ 0.0f, 1.0f, 0.0f }, rotY)), XMMatrixRotationAxis({ 0.0f, 0.0f, 1.0f }, rotZ));
    DirectX::XMStoreFloat4x4(&m_appState.modelConstants[0].modelMtx, meshRotation);
    DirectX::XMStoreFloat4x4(&m_appState.modelConstants[1].modelMtx, meshRotation);
    DirectX::XMStoreFloat4x4(&m_appState.modelConstants[2].modelMtx, meshRotation);
    DirectX::XMStoreFloat4x4(&m_appState.modelConstants[3].modelMtx, meshRotation);
    DirectX::XMStoreFloat4x4(&m_appState.modelConstants[4].modelMtx, meshRotation);
    DirectX::XMStoreFloat4x4(&m_appState.modelConstants[5].modelMtx, meshRotation);
    
    DirectX::XMStoreFloat4x4(&m_appState.modelConstants[0].nodeTransformMtx, meshRotation);
    DirectX::XMStoreFloat4x4(&m_appState.modelConstants[1].nodeTransformMtx, meshRotation);
    DirectX::XMStoreFloat4x4(&m_appState.modelConstants[2].nodeTransformMtx, meshRotation);
    DirectX::XMStoreFloat4x4(&m_appState.modelConstants[3].nodeTransformMtx, meshRotation);
    DirectX::XMStoreFloat4x4(&m_appState.modelConstants[4].nodeTransformMtx, meshRotation);
    DirectX::XMStoreFloat4x4(&m_appState.modelConstants[5].nodeTransformMtx, meshRotation);

    for (auto meshConstants : m_appState.modelConstants) { m_scene->SetMeshConstants(meshConstants.first, meshConstants.second); }
    m_scene->SetRenderMode(m_appState.currentRenderModeMask);
    
    // Update SkyBox
    m_skyBox->SetCamera(*m_camera);
    m_skyBox->SetSkyBoxConstants({ DXUtil::IdentityMtx() });

    // Update Grid
    m_grid->SetCamera(*m_camera);
    m_grid->SetGridConstants({ DXUtil::IdentityMtx() });
}

void ViewerApp::OnUpdate()
{
    if (m_appState.isExitTriggered) { DestroyWindow(m_hWnd); }
    
    if(m_appState.doRecompileShader) 
    {
        std::string errorMsg;
        m_scene->CompileVertexShader(L"shaders/vs_mesh.hlsl.tmp", errorMsg); m_gui->SetVsCompileErrorMsg(errorMsg); errorMsg.clear();
        m_scene->CompileGeometryShader(L"shaders/gs_mesh.hlsl.tmp", errorMsg); m_gui->SetGsCompileErrorMsg(errorMsg); errorMsg.clear();
        m_scene->CompilePixelShader(L"shaders/ps_mesh.hlsl.tmp", errorMsg); m_gui->SetPsCompileErrorMsg(errorMsg);
        m_appState.doRecompileShader = false;
    }
    
    // Transition from fullscreen to widow 
    if (m_appState.isAppFullscreen && m_appState.isAppWindowed)
    {
        m_renderer->SetFullScreen(true);
        m_appState.isAppWindowed = false;
    }
    
    // Transition from fullscreen to widow 
    if (!m_appState.isAppFullscreen && !m_appState.isAppWindowed)
    {
        m_renderer->SetFullScreen(false);
        m_appState.isAppWindowed = true;
    }

    // Check if a new display mode has been specified
    if(m_appState.screenWidth != m_clientWidth && m_appState.screenHeight != m_clientHeight && !m_appState.isResizingWindow)
    {
        if(!m_appState.isAppFullscreen)
            ::SetWindowPos(m_hWnd, 0, 0, 0, m_appState.screenWidth, m_appState.screenHeight, SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
        //else OnResize(m_appState.currentScreenWidth, m_appState.currentScreenHeight);
    }

    // Check if a new model loading has been triggered from the menu
    if(m_appState.isOpenGLTFPressed) 
    {
        m_gltfLoader = std::make_unique<GLTFSceneLoader>(m_renderer->GetDevice(), m_renderer->GetCommandQueue());
        m_gltfLoader->Load(m_appState.gltfFileLoaded);
        m_gltfLoader->GetScene(0, m_scene);
        m_scene->SetCubeMapTexture(m_cubeMapTexture);
        m_camera->setPosition({ -m_scene->GetSceneRadius() * 1.5f , -m_scene->GetSceneRadius() * 1.5f , -m_scene->GetSceneRadius() * 1.5f });
        m_camera->lookAt(XMFLOAT3( m_scene->GetSceneRadius() * 1.5f , m_scene->GetSceneRadius() * 1.5f , -m_scene->GetSceneRadius() * 1.5f ), { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });
        m_cameraStep = m_scene->GetSceneRadius() / 10.0f;
        m_appState.isOpenGLTFPressed = false;
    }
    UpdateScene();
}

void ViewerApp::OnDraw()
{
    m_renderer->BeginDraw();
    if(m_appState.showSkyBox) m_renderer->Draw(*m_skyBox);
    m_renderer->Draw(*m_grid);
    m_renderer->Draw(*m_scene, m_appState.currentRenderModeMask == 1);
    m_gui->Draw();
    m_renderer->EndDraw();
}

void ViewerApp::OnDestroy()
{
    DEBUG_LOG("Clearing application resources ...");

    m_renderer->FlushCommandQueue();    
    m_gui->ShutDown();

    // Assigning nullptr to ComPtr smar pointer release the interface and set the holded pointer to null
    /*
    
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