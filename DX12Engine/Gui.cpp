#include "Gui.h"
#include "WindowApp.h"

using DXUtil::ThrowIfFailed;

void Gui::Init(std::shared_ptr<Renderer> renderer)
{
    m_renderer = renderer;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    m_io = ImGui::GetIO();
    (void)m_io;

    ImGui::StyleColorsDark();

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
        ImGui::Begin("Statistics");
        ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();

        ImGui::Begin("Controls");
        ImGui::Checkbox("Fullscreen", &appState->m_isAppFullscreen);
        ImGui::End();

        //ImGui::ShowDemoWindow();
    }

    ImGui::Render();
 
    ID3D12DescriptorHeap* descriptorHeaps[] = { m_srvDescriptorHeap.Get() };
    m_renderer->GetCommandList()->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_renderer->GetCommandList().Get());
}

void Gui::RecreateDeviceObjects()
{
    if (!m_isInitialized) return;

    ImGui_ImplDX12_InvalidateDeviceObjects();
    ImGui_ImplDX12_CreateDeviceObjects();
}

void Gui::ShutDown()
{
    if (!m_isInitialized) return;

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}
