#include "Gui.h"
#include "ViewerApp.h"

#include <iostream>
#include <iomanip>
#include <fstream>

using DXUtil::ThrowIfFailed;

void Gui::Init(std::shared_ptr<Renderer> renderer)
{
    m_renderer = renderer;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    m_io = ImGui::GetIO();
    (void)m_io;
    
    ImGui::StyleColorsLight();

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

    m_isInitialized = true;
}

void Gui::Draw(AppState* appState)
{
    if (!m_isInitialized) throw std::exception("Cannot call GUI::Draw on uninitialized class.");

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    
    // Draw the GUI here
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {

                if (ImGui::MenuItem("Exit"))
                    // Exit...
                    ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Edit"))
            {
                //...
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Window"))
            {
                //...
                ImGui::EndMenu();
            }

            if (ImGui::MenuItem("VIP")) // !!!
                // Very important command...

                if (ImGui::BeginMenu("Help"))
                {
                    //...
                    ImGui::EndMenu();
                }

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

        ImGui::ShowDemoWindow();

        ImGui::Begin("Mesh");
        ImGui::SliderAngle("Rotation X", &appState->meshRotationX);
        ImGui::SliderAngle("Rotation Y", &appState->meshRotationY);
        ImGui::SliderAngle("Rotation Z", &appState->meshRotationZ);
        ImGui::End();

        ImGui::Begin("Light");
        ImGui::SliderFloat("Position X", &appState->lightPositionX, -10.0f, 10.0f, "x = %.3f");
        ImGui::SliderFloat("Position Y", &appState->lightPositionY, -10.0f, 10.0f, "x = %.3f");
        ImGui::SliderFloat("Position Z", &appState->lightPositionZ, -10.0f, 10.0f, "x = %.3f");
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
