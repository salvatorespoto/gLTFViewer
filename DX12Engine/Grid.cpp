#include "Grid.h"

#include "shaders.h"
#include "Renderer.h"
#include "Camera.h"

#include "using_directives.h"

D3D12_INPUT_ELEMENT_DESC gridVertexElementsDesc[] =
{
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 } 
};

Grid::~Grid() {}

void Grid::Init(ComPtr<ID3D12Device> device, ComPtr<ID3D12CommandQueue> commandQueue, float halfSize)
{
    m_device = device;
    m_commandQueue = commandQueue;
    GenerateGrid(halfSize);

    std::string errorMsg;
    CompileShaders(L"shaders/grid.hlsl", errorMsg, m_vertexShader, m_pixelShader);

    m_frameConstantsBuffer = std::make_unique<UploadBuffer<FrameConstants>>(m_device.Get(), 1, true);
    m_gridConstantsBuffer = std::make_unique<UploadBuffer<GridConstants>>(m_device.Get(), 1, true);

    m_CBVSRVDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Create CBV_SRV_UAV descriptor heap for the cube map texture descriptor
    D3D12_DESCRIPTOR_HEAP_DESC CBVSRVHeapDesc = {};
    CBVSRVHeapDesc = {};
    CBVSRVHeapDesc.NumDescriptors = 1;
    CBVSRVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    CBVSRVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&CBVSRVHeapDesc, IID_PPV_ARGS(&m_CBVSRVDescriptorHeap)),
        "Cannot create CBV SRV UAV descriptor heap");
}

ComPtr<ID3DBlob> Grid::GetVertexShader()
{
    return m_vertexShader;
}

ComPtr<ID3DBlob> Grid::GetPixelShader()
{
    return m_pixelShader;
}

ComPtr<ID3D12RootSignature> Grid::GetRootSignature()
{
    if (m_rootSignature != nullptr) return m_rootSignature;

    CD3DX12_ROOT_PARAMETER rootParameters[2] = {};

    rootParameters[0].InitAsConstantBufferView(0, 0);	        // Parameter 1: Root descriptor that will holds the pass constants PassConstants
    rootParameters[1].InitAsConstantBufferView(1, 0);	        // Parameter 2: Root descriptor for model matrix (skybox rotation)

    
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    ThrowIfFailed(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf()),
        (errorBlob == nullptr) ? "Cannot serialize root signature" : DXUtil::GetErrorBlobMsg(errorBlob));
    ThrowIfFailed(m_device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)),
        (errorBlob == nullptr) ? "Cannot create root signature" : DXUtil::GetErrorBlobMsg(errorBlob));

    return m_rootSignature;
}

void Grid::SetCamera(const Camera& camera)
{
    m_frameConstants.projMtx = camera.getProjMtx();
    m_frameConstants.viewMtx = camera.getViewMtx();
    XMStoreFloat4x4(&m_frameConstants.projViewMtx, XMMatrixTranspose(XMMatrixMultiply(XMLoadFloat4x4(&m_frameConstants.viewMtx), XMLoadFloat4x4(&m_frameConstants.projMtx))));
    m_frameConstants.eyePosition = DirectX::XMFLOAT4(camera.GetPosition().x, camera.GetPosition().y, camera.GetPosition().z, 1.0f);
    m_frameConstantsBuffer->copyData(0, m_frameConstants);
}

void Grid::SetFrameConstants(FrameConstants frameConstants)
{
    m_frameConstants = frameConstants;
    m_frameConstantsBuffer->copyData(0, frameConstants);
}

void Grid::SetGridConstants(GridConstants gridConstants)
{
    m_gridConstants = gridConstants;
    m_gridConstantsBuffer->copyData(0, gridConstants);
}

void Grid::SetUpRootSignature(ID3D12GraphicsCommandList* commandList)
{

    ID3D12DescriptorHeap* descriptorHeaps[] = { m_CBVSRVDescriptorHeap.Get() };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    commandList->SetGraphicsRootConstantBufferView(0, m_frameConstantsBuffer->getResource()->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(1, m_gridConstantsBuffer->getResource()->GetGPUVirtualAddress());
}

void Grid::Draw(ID3D12GraphicsCommandList* commandList)
{
    SetUpRootSignature(commandList);

    D3D12_VERTEX_BUFFER_VIEW vbView; // Vertices buffer view
    vbView.BufferLocation = m_buffersGPU[m_verticesBufferView.bufferId]->GetGPUVirtualAddress() + m_verticesBufferView.byteOffset;
    vbView.StrideInBytes = sizeof(DirectX::XMFLOAT3);
    vbView.SizeInBytes = m_verticesBufferView.byteLength;

    D3D12_VERTEX_BUFFER_VIEW vertexBuffers[1] = { vbView };
    commandList->IASetVertexBuffers(0, 1, vertexBuffers);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    commandList->DrawInstanced(m_verticesBufferView.count, 1, 0, 0);
}

void Grid::GenerateGrid(float halfSide)
{
    float step = halfSide / 5.0f;

    std::vector<DirectX::XMFLOAT3> vertices;
    for (int i = -5; i < 5; i++)
    {
        vertices.push_back({ i * step, 0.0f, -halfSide});
        vertices.push_back({ i * step, 0.0f, halfSide });
        vertices.push_back({ -halfSide, 0.0f,  i * step });
        vertices.push_back({ halfSide, 0.0f,  i * step });
    } 

    unsigned char* verticesData = reinterpret_cast<unsigned char*> (vertices.data());
    size_t verticesByteSize = vertices.size() * sizeof(DirectX::XMFLOAT3);

    size_t bufferByteSize = verticesByteSize;
    std::vector<unsigned char> buffer(bufferByteSize);
    memcpy(buffer.data(), reinterpret_cast<unsigned char*> (vertices.data()), verticesByteSize);
    
    // Upload data to the GPU
    GPUHeapUploader gpuHeapUploader(m_device.Get(), m_commandQueue.Get());
    AddGPUBuffer(gpuHeapUploader.Upload(buffer.data(), buffer.size()));

    // Set up buffer views
    m_verticesBufferView.bufferId = 0;
    m_verticesBufferView.byteOffset = 0;
    m_verticesBufferView.byteLength = verticesByteSize;
    m_verticesBufferView.count = vertices.size();
}