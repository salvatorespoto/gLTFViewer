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
    void Draw(AppState* state);
    void ReSize(unsigned int widht, unsigned int height);
    void ShutDown();

private:
    void SetStyle();
    void LoadShaderSource();
    void SaveShaderSource();
    void FileMenu();

    bool m_isInitialized = false;
    std::shared_ptr<Renderer> m_renderer;
    AppState* m_appState;   // AppState holds main app state infos

    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandListAlloc;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srvDescriptorHeap;
    
    ImGuiIO m_io;
    char m_shaderText[10000];
    bool m_compilationSuccess = true;
    std::string m_errorMsg;
    // create a file browser instance
    ImGui::FileBrowser m_fileDialog;

};