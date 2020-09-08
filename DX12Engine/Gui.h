#pragma once

#include "DXUtil.h"
#include "Renderer.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx12.h"
#include "imgui/imfilebrowser.h"

// Forward declaration of imgui message handler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

struct AppState;

class Gui final
{
public:
    Gui() = default;
	Gui(const Gui& g) = delete;
	Gui(const Gui&& g) = delete;
	Gui& operator=(const Gui& g) = delete;
	Gui& operator=(const Gui&& g) = delete;
	~Gui() = default;

    void Init(std::shared_ptr<Renderer> renderer, AppState* appState);
    void Draw();
    void ReSize(unsigned int widht, unsigned int height);
    bool WantCaptureMouse();
    bool WantCaptureKeyboard();
    void ShutDown();

private:
    static constexpr int FRAME_RATE_SERIES_SIZE = 15;
    void SetStyle();
    void LoadShaderSource();
    void SaveShaderSource();
    void DrawMenuBar();
    void DrawBarFileMenu();
    void DrawStatistics();
    void DrawControls();
    void DrawColorPicker(DirectX::XMFLOAT4& color, int width, std::string id);
    void DrawEditor();
    void DrawVertexShaderEditor();
    void DrawGeometryShaderEditor();
    void DrawPixelShaderEditor();
    void DrawHeaderShaderEditor();
    void DrawLogs();

    bool m_isInitialized = false;
    std::shared_ptr<Renderer> m_renderer;
    AppState* m_appState;   // AppState holds the window app state infos

    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandListAlloc;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvDescriptorHeap;
    
    char m_vertexShaderText[50000];
    char m_geometryShaderText[50000];
    char m_pixelShaderText[50000];
    char m_headerShaderText[50000];
    
    float m_frameRateSeries[FRAME_RATE_SERIES_SIZE];
    bool m_compilationSuccess = true;
    std::string m_errorMsg;
    // create a file browser instance
    ImGui::FileBrowser m_fileDialog;

};