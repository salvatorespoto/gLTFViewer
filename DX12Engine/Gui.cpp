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
        ImGui::SliderAngle("Rotation X", &rotX);
        ImGui::SliderAngle("Rotation Y", &rotY);
        ImGui::SliderAngle("Rotation Z", &rotZ);
        XMMATRIX meshRotation = XMMatrixMultiply(XMMatrixMultiply(XMMatrixRotationAxis({ 1.0f, 0.0f, 0.0f }, rotX), XMMatrixRotationAxis({ 0.0f, 1.0f, 0.0f }, rotY)), XMMatrixRotationAxis({ 0.0f, 0.0f, 1.0f }, rotZ));
        DirectX::XMStoreFloat4x4(&appState->meshConstants[0].modelMtx, meshRotation);
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

    colors[ImGuiCol_Text] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.500f, 0.500f, 0.500f, 1.000f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.280f, 0.280f, 0.280f, 0.000f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);
    colors[ImGuiCol_Border] = ImVec4(0.266f, 0.266f, 0.266f, 1.000f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.000f, 0.000f, 0.000f, 0.000f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.200f, 0.200f, 0.200f, 1.000f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.280f, 0.280f, 0.280f, 1.000f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.277f, 0.277f, 0.277f, 1.000f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.300f, 0.300f, 0.300f, 1.000f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_CheckMark] = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_Button] = ImVec4(1.000f, 1.000f, 1.000f, 0.000f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
    colors[ImGuiCol_ButtonActive] = ImVec4(1.000f, 1.000f, 1.000f, 0.391f);
    colors[ImGuiCol_Header] = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
    colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(1.000f, 1.000f, 1.000f, 0.250f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.000f, 1.000f, 1.000f, 0.670f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_Tab] = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.352f, 0.352f, 0.352f, 1.000f);
    colors[ImGuiCol_TabActive] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
    //colors[ImGuiCol_DockingPreview] = ImVec4(1.000f, 0.391f, 0.000f, 0.781f);
    //colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.586f, 0.586f, 0.586f, 1.000f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_NavHighlight] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);

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

   // m_io.Fonts->AddFontFromFileTTF("imgui/Fonts/Ruda-Bold.ttf", 12);
   // m_io.Fonts->AddFontFromFileTTF("imgui/Fonts/Ruda-Bold.ttf", 10);
   m_io.Fonts->AddFontFromFileTTF("imgui/Fonts/Ruda-Bold.ttf", 14);
    //m_io.Fonts->AddFontFromFileTTF("imgui/Fonts/Ruda-Bold.ttf", 18);
}