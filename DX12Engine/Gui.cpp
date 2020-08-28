#include "Gui.h"
#include "ViewerApp.h"
#include "Mesh.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include "imgui/imfilebrowser.h"

using DXUtil::ThrowIfFailed;
using DirectX::XMMATRIX;
using DirectX::XMMatrixRotationAxis;
using DirectX::XMMatrixMultiply;

void Gui::Init(std::shared_ptr<Renderer> renderer, AppState* appState)
{
    DEBUG_LOG("Initializing Gui object")
    m_renderer = renderer;
    m_appState = appState;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    m_io = ImGui::GetIO();
    (void)m_io;

    SetStyle();

    D3D12_DESCRIPTOR_HEAP_DESC dhDesc = {};
    dhDesc.NumDescriptors = 1;
    dhDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    dhDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(m_renderer->GetDevice()->CreateDescriptorHeap(&dhDesc, IID_PPV_ARGS(&m_srvDescriptorHeap)), "Cannot create SRV descriptor heap for ImGui");

    ImGui_ImplWin32_Init(m_renderer->GetWindowHandle());
    ImGui_ImplDX12_Init(m_renderer->GetDevice().Get(), 1, DXGI_FORMAT_R8G8B8A8_UNORM, m_srvDescriptorHeap.Get(), m_srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

    ThrowIfFailed(m_renderer->GetDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandListAlloc)), "Cannot create command allocator");
    ThrowIfFailed(m_renderer->GetDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandListAlloc.Get(), nullptr, IID_PPV_ARGS(&m_commandList)), "Cannot create the command list");
    m_commandList->Close();

    LoadShaderSource();

    m_fileDialog.SetTitle("title");
    m_fileDialog.SetTypeFilters({ ".glb", ".gltf" });

    m_isInitialized = true;


}

void Gui::Draw(AppState* appState)
{
    if (!m_isInitialized) throw std::exception("Cannot call GUI::Draw on uninitialized class.");

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    
    // Draw the GUI
    {
        //ImGui::ShowDemoWindow();

        if (ImGui::BeginMainMenuBar())
        {
            FileMenu();
            ImGui::EndMainMenuBar();
        }

        ImGui::Begin("Statistics");
        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();

        static const char* current_item = NULL;
        ImGui::Begin("Controls");
      
        ImGui::Checkbox("Fullscreen", &appState->isAppFullscreen);
        
        std::vector<DXGI_MODE_DESC> displayModes = m_renderer->GetDisplayModes();
        static int item_current_idx = 0;
        std::string combo_label = (std::to_string(appState->currentScreenWidth) + "x" + std::to_string(appState->currentScreenHeight)).c_str();
        ImGui::SameLine();
        if (ImGui::BeginCombo("##combo_display_modes", combo_label.c_str(), 0))
        {
            std::string previousModeDescription = ""; // Used to show modes with the same resolution only once
            for(DXGI_MODE_DESC mode : displayModes)
            {
                std::string modeDescription = std::to_string(mode.Width) + "x" + std::to_string(mode.Height);
                if (modeDescription.compare(previousModeDescription) == 0) continue;
                item_current_idx++;
                const bool is_selected = (mode.Width == appState->currentScreenWidth && mode.Height == appState->currentScreenHeight);
                if (ImGui::Selectable(modeDescription.c_str(), is_selected))
                {
                    ImGui::SetItemDefaultFocus();
                    appState->currentScreenWidth = mode.Width;
                    appState->currentScreenHeight = mode.Height;
                }
                //if (is_selected) ImGui::SetItemDefaultFocus();
                previousModeDescription = modeDescription;
            }
            ImGui::EndCombo();
        }
        ImGui::End();

        ImGui::Begin("Mesh");
        float rotX = 0.0f, rotY = 0.0f, rotZ = 0.0f;
        ImGui::SliderAngle("Rotation X", &appState->meshConstants[0].rotXYZ.x);
        ImGui::SliderAngle("Rotation Y", &appState->meshConstants[0].rotXYZ.y);
        ImGui::SliderAngle("Rotation Z", &appState->meshConstants[0].rotXYZ.z);
        ImGui::End();

        ImGui::Begin("Light");
        ImGui::SliderFloat("Position X", &appState->lights[0].position.x, -10.0f, 10.0f, "x = %.3f");
        ImGui::SliderFloat("Position Y", &appState->lights[0].position.y, -10.0f, 10.0f, "x = %.3f");
        ImGui::SliderFloat("Position Z", &appState->lights[0].position.z, -10.0f, 10.0f, "x = %.3f");
        ImGui::SliderFloat("R", &appState->lights[0].color.x, 0.0f, 1.0f, "x = %.3f");
        ImGui::SliderFloat("G", &appState->lights[0].color.y, 0.0f, 1.0f, "x = %.3f");
        ImGui::SliderFloat("B", &appState->lights[0].color.z, 0.0f, 1.0f, "x = %.3f");
        ImGui::End();

        //std::vector<DXGI_MODE_DESC> displayModes = m_renderer->GetDisplayModes();

        ImGui::Begin("Shader source");
        
        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(1.0f, 0.5f, 1.0f));
        if (ImGui::Button("Compile"))
        {
            SaveShaderSource();
            m_compilationSuccess = m_renderer->CompileShaders(L"shaders.hlsl.tmp", m_errorMsg);
        }
        ImGui::PopStyleColor(1);

        if(!m_compilationSuccess)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor::HSV(1.0f, 0.5f, 1.0f));
            ImGui::TextWrapped(m_errorMsg.c_str());
            ImGui::PopStyleColor(1);
            ImGui::Spacing();
        }
        else 
        {
            ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor::HSV(0.33f, 0.5f, 1.0f));
            ImGui::TextWrapped("Compilation success");
            ImGui::PopStyleColor(1);
            ImGui::Spacing();
        }

        ImGui::SetNextItemWidth(ImGui::GetWindowWidth());
        ImGui::SetNextItemWidth(ImGui::GetWindowHeight());

        ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput;

        ImGui::InputTextMultiline("##source", m_shaderText, 10000, ImVec2(-FLT_MIN, -FLT_MIN), flags);
        ImGui::End();
    }

    ImGui::Render();
 
    ID3D12DescriptorHeap* descriptorHeaps[] = { m_srvDescriptorHeap.Get() };
    m_renderer->GetCommandList()->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_renderer->GetCommandList().Get());
}

void Gui::ReSize(unsigned int width, unsigned int height)
{
    if (!m_isInitialized) return;
    //ImGui_ImplDX12_InvalidateDeviceObjects();
    //ImGui_ImplDX12_CreateDeviceObjects();
   // m_io.DisplaySize = { float(width), float(height) };
}

bool Gui::WantCaptureMouse()
{
    return ImGui::GetIO().WantCaptureMouse;
}

bool Gui::WantCaptureKeyboard()
{
    return ImGui::GetIO().WantCaptureKeyboard;
}

void Gui::ShutDown()
{
    if (!m_isInitialized) return;

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void Gui::LoadShaderSource() 
{
    std::ifstream inFile;
    inFile.open("shaders.hlsl");
    std::string text((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
    memcpy(m_shaderText, text.c_str(), text.length());
    
    return ;
}

void Gui::SaveShaderSource() 
{
    std::ofstream out("shaders.hlsl.tmp");
    if (!out)
    {
        return;
    }
    out.write((char*)m_shaderText, strlen(m_shaderText));
    out.close();
}

void Gui::FileMenu() 
{
    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::Button("Open glTF ...")) { m_fileDialog.Open(); }
        m_fileDialog.Display();
        if (m_fileDialog.HasSelected())
        {
            std::cout << "Selected filename" << m_fileDialog.GetSelected().string() << std::endl;
            m_appState->isOpenGLTFPressed = true;
            m_appState->gltfFileLoaded = m_fileDialog.GetSelected().string();
            m_fileDialog.Close();
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::Button("Exit"))
        {
            m_appState->isExitTriggered = true;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndMenu();
    }
}

void Gui::SetStyle() 
{
    ImGuiStyle* style = &ImGui::GetStyle();
    ImVec4* colors = style->Colors;
    style->Colors[ImGuiCol_Text] = ImVec4(0.73f, 0.73f, 0.73f, 1.00f);
    style->Colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    style->Colors[ImGuiCol_WindowBg] = ImVec4(0.26f, 0.26f, 0.26f, 0.95f);
    style->Colors[ImGuiCol_ChildBg] = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    style->Colors[ImGuiCol_PopupBg] = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
    style->Colors[ImGuiCol_Border] = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
    style->Colors[ImGuiCol_BorderShadow] = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
    style->Colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    style->Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    style->Colors[ImGuiCol_FrameBgActive] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    style->Colors[ImGuiCol_TitleBg] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
    style->Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
    style->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
    style->Colors[ImGuiCol_MenuBarBg] = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
    style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
    style->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
    style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
    style->Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
    style->Colors[ImGuiCol_CheckMark] = ImVec4(0.78f, 0.78f, 0.78f, 1.00f);
    style->Colors[ImGuiCol_SliderGrab] = ImVec4(0.74f, 0.74f, 0.74f, 1.00f);
    style->Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.74f, 0.74f, 0.74f, 1.00f);
    style->Colors[ImGuiCol_Button] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
    style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.43f, 0.43f, 0.43f, 1.00f);
    style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
    style->Colors[ImGuiCol_Header] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
    style->Colors[ImGuiCol_HeaderHovered] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
    style->Colors[ImGuiCol_HeaderActive] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
    style->Colors[ImGuiCol_ResizeGrip] = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
    style->Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style->Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style->Colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    style->Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    style->Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    style->Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    style->Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.32f, 0.52f, 0.65f, 1.00f);
    style->Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.20f, 0.20f, 0.20f, 0.50f);

    style->ChildRounding = 4.0f;
    style->FrameBorderSize = 1.0f;
    style->FrameRounding = 2.0f;
    style->GrabMinSize = 7.0f;
    style->PopupRounding = 2.0f;
    style->ScrollbarRounding = 12.0f;
    style->ScrollbarSize = 13.0f;
    style->TabBorderSize = 1.0f;
    style->TabRounding = 0.0f;
    style->WindowRounding = 4.0f;
   
    m_io.Fonts->AddFontFromFileTTF("imgui/Fonts/Ruda-Bold.ttf", 14);
}