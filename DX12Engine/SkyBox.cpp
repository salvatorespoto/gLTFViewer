#include "SkyBox.h"
#include "shaders.h"
#include "Texture.h"
#include "Renderer.h"
#include "Camera.h"

D3D12_INPUT_ELEMENT_DESC skyBoxVertexElementsDesc[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "NORMAL",   0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
};

using Microsoft::WRL::ComPtr;
using DirectX::XMLoadFloat3;
using DirectX::XMStoreFloat3;
using DirectX::XMVector3Normalize;
using DirectX::XMVectorAdd;
using DXUtil::ThrowIfFailed;

SkyBox::~SkyBox() {}

void SkyBox::Init(ComPtr<ID3D12Device> device, ComPtr<ID3D12CommandQueue> commandQueue)
{
    m_device = device;
    m_commandQueue = commandQueue;
    GenerateSphere();

    std::string errorMsg;
    CompileShaders(L"shaders/skybox.hlsl", errorMsg, m_vertexShader, m_pixelShader);

    m_frameConstantsBuffer = std::make_unique<UploadBuffer<FrameConstants>>(m_device.Get(), 1, true);
    m_skyBoxConstantsBuffer = std::make_unique<UploadBuffer<SkyBoxConstants>>(m_device.Get(), 1, true);

    m_CBVSRVDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Create CBV_SRV_UAV descriptor heap for the cube map texture descriptor
    D3D12_DESCRIPTOR_HEAP_DESC CBVSRVHeapDesc = {};
    CBVSRVHeapDesc = {};
    CBVSRVHeapDesc.NumDescriptors = 1;
    CBVSRVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    CBVSRVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&CBVSRVHeapDesc, IID_PPV_ARGS(&m_CBVSRVDescriptorHeap)),
        "Cannot create CBV SRV UAV descriptor heap");

    D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    samplerDesc.ShaderRegister = 0;
    samplerDesc.RegisterSpace = 0;
    m_staticSamplerDesc = CD3DX12_STATIC_SAMPLER_DESC(samplerDesc);
}

void SkyBox::SetCubeMapTexture(Microsoft::WRL::ComPtr<ID3D12Resource> cubeMapTexture)
{
    m_cubeMapTexture = cubeMapTexture;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.TextureCube.MostDetailedMip = 0;
    srvDesc.TextureCube.MipLevels = m_cubeMapTexture->GetDesc().MipLevels;
    srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
    srvDesc.Format = m_cubeMapTexture->GetDesc().Format;

    m_device->CreateShaderResourceView(m_cubeMapTexture.Get(), &srvDesc, m_CBVSRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}

ComPtr<ID3DBlob> SkyBox::GetVertexShader() const
{
    return m_vertexShader;
}

ComPtr<ID3DBlob> SkyBox::GetPixelShader() const
{
    return m_pixelShader;
}

ComPtr<ID3D12RootSignature> SkyBox::GetRootSignature()
{
    if(m_rootSignature != nullptr) return m_rootSignature;

    CD3DX12_ROOT_PARAMETER rootParameters[3] = {};

    rootParameters[0].InitAsConstantBufferView(0, 0);	        // Parameter 1: Root descriptor that will holds the pass constants PassConstants
    rootParameters[1].InitAsConstantBufferView(1, 0);	        // Parameter 2: Root descriptor for model matrix (skybox rotation)
    
    CD3DX12_DESCRIPTOR_RANGE descriptorRangesCBVSRV[1] = {};	// Parameter 3: Descriptor table for the cube map descriptor heap
    descriptorRangesCBVSRV[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, 0);
    rootParameters[2].InitAsDescriptorTable(1, descriptorRangesCBVSRV);

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(3, rootParameters, 1, &m_staticSamplerDesc, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    ThrowIfFailed(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf()),
        (errorBlob == nullptr) ? "Cannot serialize root signature" : DXUtil::GetErrorBlobMsg(errorBlob));
    ThrowIfFailed(m_device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)),
        (errorBlob == nullptr) ? "Cannot create root signature" : DXUtil::GetErrorBlobMsg(errorBlob));

    return m_rootSignature;
}

void SkyBox::SetCamera(const Camera& camera)
{
    m_frameConstants.projMtx = camera.getProjMtx();
    m_frameConstants.viewMtx = camera.getViewMtx();
    XMStoreFloat4x4(&m_frameConstants.projViewMtx, XMMatrixTranspose(XMMatrixMultiply(XMLoadFloat4x4(&m_frameConstants.viewMtx), XMLoadFloat4x4(&m_frameConstants.projMtx))));
    m_frameConstants.eyePosition = DirectX::XMFLOAT4(camera.GetPosition().x, camera.GetPosition().y, camera.GetPosition().z, 1.0f);
    m_frameConstantsBuffer->copyData(0, m_frameConstants);
}

void SkyBox::SetFrameConstants(FrameConstants frameConstants)
{
    m_frameConstants = frameConstants;
    m_frameConstantsBuffer->copyData(0, frameConstants);
}

void SkyBox::SetSkyBoxConstants(SkyBoxConstants skyBoxConstants)
{
    m_skyBoxConstants = skyBoxConstants;
    m_skyBoxConstantsBuffer->copyData(0, skyBoxConstants);
}

void SkyBox::SetUpRootSignature(ID3D12GraphicsCommandList* commandList)
{

    ID3D12DescriptorHeap* descriptorHeaps[] = { m_CBVSRVDescriptorHeap.Get() };
    commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    commandList->SetGraphicsRootSignature(m_rootSignature.Get());

    commandList->SetGraphicsRootConstantBufferView(0, m_frameConstantsBuffer->getResource()->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(1, m_skyBoxConstantsBuffer->getResource()->GetGPUVirtualAddress());
    commandList->SetGraphicsRootDescriptorTable(2, m_CBVSRVDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
}

void SkyBox::Draw(ID3D12GraphicsCommandList* commandList)
{
    SetUpRootSignature(commandList);

    D3D12_VERTEX_BUFFER_VIEW vbView; // Vertices buffer view
    vbView.BufferLocation = m_buffersGPU[m_verticesBufferView.bufferId]->GetGPUVirtualAddress() + m_verticesBufferView.byteOffset;
    vbView.StrideInBytes = sizeof(DirectX::XMFLOAT3);
    vbView.SizeInBytes = static_cast<UINT>(m_verticesBufferView.byteLength);

    D3D12_VERTEX_BUFFER_VIEW nbView; // Normals buffer view
    nbView.BufferLocation = m_buffersGPU[m_normalsBufferView.bufferId]->GetGPUVirtualAddress() + m_normalsBufferView.byteOffset;
    nbView.StrideInBytes = sizeof(DirectX::XMFLOAT3);
    nbView.SizeInBytes = static_cast<UINT>(m_normalsBufferView.byteLength);

    D3D12_VERTEX_BUFFER_VIEW vertexBuffers[2] = { vbView, nbView };
    commandList->IASetVertexBuffers(0, 2, vertexBuffers);
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    D3D12_INDEX_BUFFER_VIEW ibView; // Index buffer view
    ibView.BufferLocation = m_buffersGPU[m_indicesBufferView.bufferId]->GetGPUVirtualAddress() + m_indicesBufferView.byteOffset;
    ibView.Format = DXGI_FORMAT_R16_UINT;
    ibView.SizeInBytes = static_cast<UINT>(m_indicesBufferView.byteLength);

    D3D12_INDEX_BUFFER_VIEW indexBuffers[1] = { ibView };
    commandList->IASetIndexBuffer(indexBuffers);

    commandList->DrawIndexedInstanced(static_cast<UINT>(m_indicesBufferView.count), 1, 0, 0, 0);
}

void SkyBox::GenerateSphere()
{
    float X = 0.525731112119133606f;
    float Z = 0.850650808352039932f;

    std::vector<DirectX::XMFLOAT3> vertices =
    {
        {-X, 0.0, Z},
        {X, 0.0, Z},
        {-X, 0.0, -Z},
        {X, 0.0, -Z},
        {0.0, Z, X},
        {0.0, Z, -X},
        {0.0, -Z, X},
        {0.0, -Z, -X},
        {Z, X, 0.0},
        {-Z, X, 0.0},
        {Z, -X, 0.0},
        {-Z, -X, 0.0}
    };

    std::vector<DirectX::XMFLOAT3> normals;

    // Counter clock wise order
    std::vector<uint16_t> triangles =
    {
        0,1,4,
        0,4,9,
        9,4,5,
        4,8,5,
        4,1,8,
        8,1,10,
        8,10,3,
        5,8,3,
        5,3,2,
        2,3,7,
        7,3,10,
        7,10,6,
        7,6,11,
        11,6,0,
        0,6,1,
        6,10,1,
        9,11,0,
        9,2,11,
        9,5,2,
        7,11,2
    };

    // Subdivide to generate a more detailed sphere
    for (size_t n = 0; n < SKYBOX_SPHERE_SUBDIVISION; n++)
    {
        std::vector<DirectX::XMFLOAT3> subVertices;
        std::vector<uint16_t> subTriangles;
        for (size_t i = 0; i < triangles.size(); i += 3)
        {
            DirectX::XMFLOAT3 v1, v2, v3, v12, v23, v31;
            v1 = vertices[triangles[i]];
            v2 = vertices[triangles[i + 1]];
            v3 = vertices[triangles[i + 2]];
            XMStoreFloat3(&v12, XMVector3Normalize(XMVectorAdd(XMLoadFloat3(&v1), XMLoadFloat3(&v2))));
            XMStoreFloat3(&v23, XMVector3Normalize(XMVectorAdd(XMLoadFloat3(&v2), XMLoadFloat3(&v3))));
            XMStoreFloat3(&v31, XMVector3Normalize(XMVectorAdd(XMLoadFloat3(&v3), XMLoadFloat3(&v1))));

            subVertices.push_back(v1);
            subVertices.push_back(v2);
            subVertices.push_back(v3);
            subVertices.push_back(v12);
            subVertices.push_back(v23);
            subVertices.push_back(v31);

            uint16_t idx = static_cast<uint16_t>(i) * 2; // Now there are 3 more vertices and 3 more triangles for each old one
            subTriangles.push_back(idx); subTriangles.push_back(idx + 3); subTriangles.push_back(idx + 5); // v1,v12,v31
            subTriangles.push_back(idx + 1); subTriangles.push_back(idx + 4); subTriangles.push_back(idx + 3); // v2,v23,v12
            subTriangles.push_back(idx + 2); subTriangles.push_back(idx + 5); subTriangles.push_back(idx + 4); // v3,v31,v23
            subTriangles.push_back(idx + 3); subTriangles.push_back(idx + 4); subTriangles.push_back(idx + 5); // v12,v23,v31
        }

        vertices = subVertices;
        triangles = subTriangles;
    }

    // Vertex normals is the same direction of the position vector
    for (DirectX::XMFLOAT3& v : vertices)
    {
        DirectX::XMFLOAT3 normal;
        XMStoreFloat3(&normal, XMVector3Normalize(XMLoadFloat3(&v)));
        normals.push_back(normal);

        v = { 100.0f * v.x, 100.0f * v.y, 100.0f * v.z };
    }

    unsigned char* verticesData = reinterpret_cast<unsigned char*> (vertices.data());
    unsigned char* normalsData = reinterpret_cast<unsigned char*> (normals.data());
    unsigned char* indicesData = reinterpret_cast<unsigned char*> (triangles.data());
    size_t verticesByteSize = vertices.size() * sizeof(DirectX::XMFLOAT3);
    size_t normalsByteSize = normals.size() * sizeof(DirectX::XMFLOAT3);
    size_t indicesByteSize = triangles.size() * sizeof(uint16_t);

    size_t bufferByteSize = verticesByteSize + normalsByteSize + indicesByteSize;
    std::vector<unsigned char> buffer(bufferByteSize);
    memcpy(buffer.data(), reinterpret_cast<unsigned char*> (vertices.data()), verticesByteSize);
    memcpy(buffer.data() + verticesByteSize, reinterpret_cast<unsigned char*> (normals.data()), normalsByteSize);
    memcpy(buffer.data() + verticesByteSize + normalsByteSize, reinterpret_cast<unsigned char*> (triangles.data()), indicesByteSize);

    // Upload data to the GPU
    GPUHeapUploader gpuHeapUploader(m_device.Get(), m_commandQueue.Get());
    AddGPUBuffer(gpuHeapUploader.Upload(buffer.data(), buffer.size()));

    // Set up buffer views
    m_verticesBufferView.bufferId = 0;
    m_verticesBufferView.byteOffset = 0;
    m_verticesBufferView.byteLength = verticesByteSize;
    m_verticesBufferView.count = vertices.size();
    m_normalsBufferView.bufferId = 0;
    m_normalsBufferView.byteOffset = verticesByteSize;
    m_normalsBufferView.byteLength = normalsByteSize;
    m_normalsBufferView.count = normals.size();
    m_indicesBufferView.bufferId = 0;
    m_indicesBufferView.byteOffset = verticesByteSize + normalsByteSize;
    m_indicesBufferView.byteLength = indicesByteSize;
    m_indicesBufferView.count = triangles.size();
}