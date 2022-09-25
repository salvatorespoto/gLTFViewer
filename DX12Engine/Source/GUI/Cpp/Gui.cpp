// SPDX-FileCopyrightText: 2022 Salvatore Spoto <salvatore.spoto@gmail.com> 
// SPDX-License-Identifier: MIT

#include "Gui.h"
#include "shaders.h"
#include "Mesh.h"
#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>

#include "imgui/imfilebrowser.h"

#include "using_directives.h"

void Gui::Init(std::shared_ptr<Renderer> renderer, AppState* appState)
{
    DEBUG_LOG("Initializing Gui object")
    m_renderer = renderer;
    m_appState = appState;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    
    m_fileDialog.SetTitle("title");
    m_fileDialog.SetTypeFilters({ ".glb", ".gltf" });

    SetStyle();

    D3D12_DESCRIPTOR_HEAP_DESC dhDesc = {};
    dhDesc.NumDescriptors = 1;
    dhDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    dhDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(m_renderer->GetDevice()->CreateDescriptorHeap(&dhDesc, IID_PPV_ARGS(&m_srvDescriptorHeap)), 
        "Cannot create SRV descriptor heap for ImGui");

    ImGui_ImplWin32_Init(m_renderer->GetWindowHandle());
    ImGui_ImplDX12_Init(m_renderer->GetDevice().Get(), 1, DXGI_FORMAT_R8G8B8A8_UNORM, m_srvDescriptorHeap.Get(), m_srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

    ThrowIfFailed(m_renderer->GetDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandListAlloc)), "Cannot create command allocator");
    ThrowIfFailed(m_renderer->GetDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandListAlloc.Get(), nullptr, IID_PPV_ARGS(&m_commandList)), "Cannot create the command list");
    m_commandList->Close();

    for (int i = 0; i < FRAME_RATE_SERIES_SIZE; i++) m_frameRateSeries[i] = ImGui::GetIO().Framerate;

    LoadShaderSource();
    m_isInitialized = true;
}

void Gui::Draw()
{
    if (!m_isInitialized) throw std::exception("Cannot call GUI::Draw on uninitialized class.");

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    
    // Draw the GUI
    {
        //ImGui::ShowDemoWindow();
        DrawMenuBar();
        DrawStatistics();
        DrawControls();
        DrawEditor();
        DrawLogs();
    }

    ImGui::Render();
 
    ID3D12DescriptorHeap* descriptorHeaps[] = { m_srvDescriptorHeap.Get() };
    m_renderer->GetCommandList()->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_renderer->GetCommandList().Get());
}

void Gui::SetVsCompileErrorMsg(const std::string& errorMsg) noexcept
{
    m_vsCompileErrorMsg = errorMsg;
}

void Gui::SetGsCompileErrorMsg(const std::string& errorMsg) noexcept
{
    m_gsCompileErrorMsg = errorMsg;
}

void Gui::SetPsCompileErrorMsg(const std::string& errorMsg) noexcept
{
    m_psCompileErrorMsg = errorMsg;
}

void Gui::ReSize(const unsigned int width, const unsigned int height)
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
    
    inFile.open("Source/Shaders/vs_mesh.hlsl");
    std::string text((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
    memcpy(m_vertexShaderText, text.c_str(), text.length());
    inFile.close();

    inFile.open("Source/Shaders/gs_mesh.hlsl");
    text.assign((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
    memcpy(m_geometryShaderText, text.c_str(), text.length());
    inFile.close();

    inFile.open("Source/Shaders/ps_mesh.hlsl");
    text.assign((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
    memcpy(m_pixelShaderText, text.c_str(), text.length());
    inFile.close();

    inFile.open("Source/Shaders/mesh_common.hlsli");
    text.assign((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
    memcpy(m_headerShaderText, text.c_str(), text.length());
    inFile.close();
    
    return ;
}

void Gui::SaveShaderSource() 
{
    std::ofstream out("Source/Shaders/vs_mesh.hlsl.tmp");
    if (!out)
    {
        return;
    }
    out.write((char*)m_vertexShaderText, strlen(m_vertexShaderText));
    out.close();

    out.open("Source/Shaders/gs_mesh.hlsl.tmp");
    if (!out)
    {
        return;
    }
    out.write((char*)m_geometryShaderText, strlen(m_geometryShaderText));
    out.close();

    out.open("Source/Shaders/ps_mesh.hlsl.tmp");
    if (!out)
    {
        return;
    }
    out.write((char*)m_pixelShaderText, strlen(m_pixelShaderText));
    out.close();

    out.open("Source/Shaders/mesh_common.hlsli.tmp");
    if (!out)
    {
        return;
    }
    out.write((char*)m_headerShaderText, strlen(m_headerShaderText));
    out.close();
}

void Gui::DrawMenuBar() 
{
    if (ImGui::BeginMainMenuBar())
    {
        DrawBarFileMenu();
        ImGui::EndMainMenuBar();
    }
}

void Gui::DrawStatistics()
{
    ImGui::Begin("Statistics");
    ImGui::PushItemWidth(ImGui::GetWindowWidth());
    for(int i=1; i<FRAME_RATE_SERIES_SIZE; i++) m_frameRateSeries[i-1] = m_frameRateSeries[i];
    m_frameRateSeries[FRAME_RATE_SERIES_SIZE-1] = ImGui::GetIO().Framerate;
    
    char overlay[50];
    sprintf(overlay, "%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::PlotHistogram("", m_frameRateSeries, IM_ARRAYSIZE(m_frameRateSeries), 0, overlay, 0.0f, 100.0f, ImVec2(0, 80.0f));
    ImGui::PopItemWidth();
    ImGui::End();
}

void Gui::DrawControls()
{
    static const char* current_item = NULL;
    static bool allItemOpen = true;

    ImGui::Begin("Controls");

    if (allItemOpen) ImGui::SetNextItemOpen(true);
    if (ImGui::CollapsingHeader("Window settings"))
    {
        ImGui::Checkbox("Fullscreen", &m_appState->isAppFullscreen);
        std::vector<DXGI_MODE_DESC> displayModes = m_renderer->GetDisplayModes();

        static int item_current_idx = 0;
        std::string combo_label = (std::to_string(m_appState->screenWidth) + "x" + std::to_string(m_appState->screenHeight)).c_str();
        if (ImGui::BeginCombo("##combo_display_modes", combo_label.c_str(), 0))
        {
            std::string previousModeDescription = ""; // Used to show modes with the same resolution only once
            for (const DXGI_MODE_DESC& mode : displayModes)
            {
                std::string modeDescription = std::to_string(mode.Width) + "x" + std::to_string(mode.Height);
                if (modeDescription.compare(previousModeDescription) == 0) continue;
                item_current_idx++;
                const bool is_selected = (mode.Width == m_appState->screenWidth && mode.Height == m_appState->screenHeight);
                if (ImGui::Selectable(modeDescription.c_str(), is_selected))
                {
                    ImGui::SetItemDefaultFocus();
                    m_appState->screenWidth = mode.Width;
                    m_appState->screenHeight = mode.Height;
                }
                //if (is_selected) ImGui::SetItemDefaultFocus();
                previousModeDescription = modeDescription;
            }
            ImGui::EndCombo();
        }
    }

    if (allItemOpen) ImGui::SetNextItemOpen(true);
    if (ImGui::CollapsingHeader("Visualize"))
    {
        static int selected = 0;
        if (ImGui::Selectable("Rendering",     selected == 0)) { selected = 0; m_appState->currentRenderModeMask = 0x0; }
        if (ImGui::Selectable("Wireframe",     selected == 1)) { selected = 1; m_appState->currentRenderModeMask = 0x1 << 0; }
        if (ImGui::Selectable("Base Color",    selected == 2)) { selected = 2; m_appState->currentRenderModeMask = 0x1 << 1; }
        if (ImGui::Selectable("Normal map",    selected == 3)) { selected = 3; m_appState->currentRenderModeMask = 0x1 << 2; }
        if (ImGui::Selectable("Roughness map", selected == 4)) { selected = 4; m_appState->currentRenderModeMask = 0x1 << 3; }
        if (ImGui::Selectable("Occlusion map", selected == 5)) { selected = 5; m_appState->currentRenderModeMask = 0x1 << 4; }
        if (ImGui::Selectable("Emissive map",  selected == 6)) { selected = 6; m_appState->currentRenderModeMask = 0x1 << 5; }
    }

    if (allItemOpen) ImGui::SetNextItemOpen(true);
    if (ImGui::CollapsingHeader("SkyBox"))
    {
        ImGui::Checkbox("Show sky box", &m_appState->showSkyBox);

        // Load cubemap
        // Visualize skybox
    }

    if (allItemOpen) ImGui::SetNextItemOpen(true);
    if (ImGui::CollapsingHeader("GlTF Model"))
    {
        ImGui::BeginGroup();
        ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.80f);
        ImGui::Text("Rot X"); ImGui::SameLine(); ImGui::SliderAngle("##x", &m_appState->modelConstants[0].rotXYZ.x);
        ImGui::Text("Rot Y"); ImGui::SameLine(); ImGui::SliderAngle("##y", &m_appState->modelConstants[0].rotXYZ.y);
        ImGui::Text("Rot Z"); ImGui::SameLine(); ImGui::SliderAngle("##z", &m_appState->modelConstants[0].rotXYZ.z);
        ImGui::PopItemWidth();
        ImGui::EndGroup();
        ImGui::Separator();
    }

    if (allItemOpen) ImGui::SetNextItemOpen(true);
    if (ImGui::CollapsingHeader("Ambient light"))
    {
        DrawColorPicker(m_appState->lights[0].color, static_cast<int>(ImGui::GetWindowWidth()), "##ambientlight");
        ImGui::Separator();
    }

    if(allItemOpen) ImGui::SetNextItemOpen(true);
    if (ImGui::CollapsingHeader("Point light"))
    {
        DrawColorPicker(m_appState->lights[1].color, 80, "##light1");
        ImGui::BeginGroup();
        ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.65f);
        ImGui::SliderFloat("##PosXLight1", &m_appState->lights[1].position.x, -10.0f, 10.0f, "Pos X = %.3f");
        ImGui::SliderFloat("##PosYLight1", &m_appState->lights[1].position.y, -10.0f, 10.0f, "Pos Y= %.3f");
        ImGui::SliderFloat("##PosZLight1", &m_appState->lights[1].position.z, -10.0f, 10.0f, "Pos Z = %.3f");
        ImGui::PopItemWidth();
        ImGui::EndGroup();
        ImGui::Separator();
    }

    ImGui::End();
    
    allItemOpen = false;

    //std::vector<DXGI_MODE_DESC> displayModes = m_renderer->GetDisplayModes();
}

void Gui::DrawBarFileMenu()
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

void Gui::DrawEditor() 
{
    ImGui::Begin("Shader Editor");

    std::string errorVertexMsg, errorGeometryMsg, errorPixelMsg;
    static bool vsOk = true;
    static bool gsOk = true;
    static bool psOk = true;

    if (ImGui::Button("Compile"))
    {
        SaveShaderSource();
        m_appState->doRecompileShader = true;
    }

    if(!m_vsCompileErrorMsg.empty()) 
    {
        ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor::HSV(1.0f, 0.5f, 1.0f));
        ImGui::TextWrapped("VERTEX SHADER ERRORS:");
        ImGui::TextWrapped(m_vsCompileErrorMsg.c_str());
        ImGui::PopStyleColor(1);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor::HSV(0.33f, 0.5f, 1.0f)); 
        ImGui::TextWrapped("VERTEX SHADER OK");
        ImGui::PopStyleColor(1);
    }

    if (!m_gsCompileErrorMsg.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor::HSV(1.0f, 0.5f, 1.0f));
        ImGui::TextWrapped("GEOMETRY SHADER ERRORS:");
        ImGui::TextWrapped(m_gsCompileErrorMsg.c_str());
        ImGui::PopStyleColor(1);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor::HSV(0.33f, 0.5f, 1.0f)); 
        ImGui::TextWrapped("GEOMETRY SHADER OK");
        ImGui::PopStyleColor(1);
    }

    if (!m_psCompileErrorMsg.empty()) 
    {
        ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor::HSV(1.0f, 0.5f, 1.0f));
        ImGui::TextWrapped("PIXEL SHADER ERRORS:");
        ImGui::TextWrapped(m_psCompileErrorMsg.c_str());
        ImGui::PopStyleColor(1);
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor::HSV(0.33f, 0.5f, 1.0f));
        ImGui::TextWrapped("PIXEL SHADER OK");
        ImGui::PopStyleColor(1);
    }

    ImGui::Spacing();
    
    char tabNames[4][20] = { "Vertex Shader", "Geometry Shader", "Pixel Shader", "Header" };
    if (ImGui::BeginTabBar("Editor"))
    {
        for (int n = 0; n < 4; n++)
        {
            bool open = true;
            if (ImGui::BeginTabItem(tabNames[n], &open, ImGuiTabItemFlags_None))
            {
                if (n == 0) DrawVertexShaderEditor();
                if (n == 1) DrawGeometryShaderEditor();
                if (n == 2) DrawPixelShaderEditor();
                if (n == 3) DrawHeaderShaderEditor();
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }

    ImGui::End();

}

void Gui::DrawVertexShaderEditor() 
{
    ImGui::BeginChild("VertexEditor");
    ImGui::Spacing();
    ImGui::SetNextItemWidth(ImGui::GetWindowWidth());
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput;
    ImGui::InputTextMultiline("##sourceVertexShader", m_vertexShaderText, 50000, ImVec2(-FLT_MIN, -FLT_MIN), flags);
    ImGui::EndChild();
}

void Gui::DrawGeometryShaderEditor() 
{
    ImGui::BeginChild("GeometryEditor");
    ImGui::Spacing();
    ImGui::SetNextItemWidth(ImGui::GetWindowWidth());
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput;
    ImGui::InputTextMultiline("##sourceGeometryShader", m_geometryShaderText, 50000, ImVec2(-FLT_MIN, -FLT_MIN), flags);
    ImGui::EndChild();
}

void Gui::DrawPixelShaderEditor() 
{
    ImGui::BeginChild("PixelEditor");
    ImGui::Spacing();
    ImGui::SetNextItemWidth(ImGui::GetWindowWidth());
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput;
    ImGui::InputTextMultiline("##sourcePixelShader", m_pixelShaderText, 50000, ImVec2(-FLT_MIN, -FLT_MIN), flags);
    ImGui::EndChild();
}

void Gui::DrawHeaderShaderEditor() 
{
    ImGui::BeginChild("HeaderEditor");
    ImGui::Spacing();
    ImGui::SetNextItemWidth(ImGui::GetWindowWidth());
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput;
    ImGui::InputTextMultiline("##sourceHeaderShader", m_headerShaderText, 50000, ImVec2(-FLT_MIN, -FLT_MIN), flags);
    ImGui::EndChild();
}

void Gui::DrawLogs() 
{   
    ImGui::Begin("Logs");
    ImGui::BeginChild("Log");
    ImGui::TextUnformatted(DXUtil::gLogs.begin(), DXUtil::gLogs.end()); 
    ImGui::EndChild();
    ImGui::End();
}

void Gui::DrawColorPicker(DirectX::XMFLOAT4& color, int width, std::string id) 
{
    static int pickerId = 0; 

    ImVec4 c = ImVec4(color.x, color.y, color.z, color.w);
    ImVec4 backup_color;

    ImGuiColorEditFlags misc_flags = ImGuiColorEditFlags__OptionsDefault;
    bool open_popup = ImGui::ColorButton(id.append("Color##3c").c_str(), *(ImVec4*)&color, misc_flags | (1 ? ImGuiColorEditFlags_NoBorder : 0), ImVec2(static_cast<float>(width), 80));
    
    ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
    
    if (open_popup)
    {
        ImGui::OpenPopup(id.append("ColorPicker").c_str());
        backup_color = c;
    }
    
    if (ImGui::BeginPopup(id.append("ColorPicker").c_str()))
    {
        pickerId++;
        ImGui::Text("Color");
        ImGui::Separator();
        ImGui::ColorPicker4(id.append("Picker").c_str(), (float*)&c, misc_flags | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview);
        ImGui::SameLine();

        ImGui::BeginGroup();
        ImGui::Text("Current");
        ImGui::ColorButton(id.append("Current").c_str(), c, ImGuiColorEditFlags_NoPicker, ImVec2(60, 40));
        ImGui::Text("Previous");
        if (ImGui::ColorButton(id.append("Previous").c_str(), backup_color, ImGuiColorEditFlags_NoPicker, ImVec2(60, 40))) c = backup_color;
        ImGui::Separator();
       
        ImGui::EndGroup();
        ImGui::EndPopup();

        color.x = c.x;
        color.y = c.y;
        color.z = c.z;
        color.w = c.w;
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
   
    ImGui::GetIO().Fonts->AddFontFromFileTTF("External/imgui/Fonts/Ruda-Bold.ttf", 14);
}