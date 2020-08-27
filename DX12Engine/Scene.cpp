#include "Scene.h"
#include "Mesh.h"
#include "Camera.h"

using Microsoft::WRL::ComPtr;
using DirectX::XMStoreFloat4x4;
using DirectX::XMMatrixMultiply;
using DirectX::XMLoadFloat4x4;
using DirectX::XMConvertToRadians;
using DXUtil::ThrowIfFailed;
using DXUtil::ThrowIfFailed;

Scene::Scene(Microsoft::WRL::ComPtr<ID3D12Device> device)
{
	DEBUG_LOG("Initializing Scene object")
	m_device = device;
	m_CBVSRVDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_samplersDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	// Create CBV_SRV_UAV descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC CBVSRVHeapDesc = {};
	CBVSRVHeapDesc = {};
	CBVSRVHeapDesc.NumDescriptors = MESH_CONSTANTS_N_DESCRIPTORS + MATERIALS_N_DESCRIPTORS + TEXTURES_N_DESCRIPTORS;
	CBVSRVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	CBVSRVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(m_device->CreateDescriptorHeap(&CBVSRVHeapDesc, IID_PPV_ARGS(&m_CBVSRVDescriptorHeap)),
		"Cannot create CBV SRV UAV descriptor heap");

	// Create samplers descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC samplersHeapDesc = {};
	samplersHeapDesc.NumDescriptors = SAMPLERS_N_DESCRIPTORS;
	samplersHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	samplersHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(m_device->CreateDescriptorHeap(&samplersHeapDesc, IID_PPV_ARGS(&m_samplersDescriptorHeap)),
		"Cannot create sampler descriptor heap");

	AddFrameConstantsBuffer();

	DEBUG_LOG("Created CBV_SRV_UAV and SAMPLER descriptor heaps")
}

void Scene::AddFrameConstantsBuffer() 
{
	// Each scene contains a buffer that holds the common constants (view matrix, projection matrix etc..)
	m_frameConstantsBuffer = std::make_unique<UploadBuffer<FrameConstants>>(m_device.Get(), 1, true);
}

void Scene::AddGPUBuffer(Microsoft::WRL::ComPtr<ID3D12Resource> buffer)
{
	m_buffersGPU.push_back(buffer);
}

void Scene::AddMaterial(unsigned int materialId, RoughMetallicMaterial material)
{
	m_materials[materialId] = material;
	m_materialsBuffer[materialId] = std::make_unique<UploadBuffer<RoughMetallicMaterial>>(m_device.Get(), 1, true);
	m_materialsBuffer[materialId]->copyData(0, m_materials[materialId]);

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(m_CBVSRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	hDescriptor.Offset(materialId, m_CBVSRVDescriptorSize); // Material descriptors starts after mesh constants descrisptors, in the CBV_SRV_UAV descriptor heap layout of the Scene class

	D3D12_CONSTANT_BUFFER_VIEW_DESC materialViewDesc;
	materialViewDesc.BufferLocation = m_materialsBuffer[materialId]->getResource()->GetGPUVirtualAddress();
	materialViewDesc.SizeInBytes = DXUtil::PadByteSizeTo256Mul(sizeof(RoughMetallicMaterial));
	m_device->CreateConstantBufferView(&materialViewDesc, hDescriptor); 
}

void Scene::AddTexture(unsigned int textureId, Microsoft::WRL::ComPtr<ID3D12Resource> texture)
{
	m_textures[textureId] = texture;

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(m_CBVSRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	hDescriptor.Offset(MATERIALS_N_DESCRIPTORS + textureId, m_CBVSRVDescriptorSize); // Texture descriptors starts after material and mesh constants descrisptors, in the CBV_SRV_UAV descriptor heap layout of the Scene class

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = texture->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = texture->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	m_device->CreateShaderResourceView(texture.Get(), &srvDesc, hDescriptor);
}

void Scene::AddSampler(unsigned int samplerId, D3D12_SAMPLER_DESC samplerDesc)
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(m_samplersDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	hDescriptor.Offset(samplerId, m_samplersDescriptorSize);
	m_device->CreateSampler(&samplerDesc, m_samplersDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}

void Scene::AddLight(unsigned int lightId, Light light)
{
	m_lights[lightId] = light;
}

void Scene::AddMesh(Mesh mesh)
{
	unsigned int meshId = mesh.GetId();
	m_meshes[meshId] = mesh;
	m_meshConstantsBuffer[meshId] = std::make_unique<UploadBuffer<MeshConstants>>(m_device.Get(), 1, true);

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(m_CBVSRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	hDescriptor.Offset(meshId, m_CBVSRVDescriptorSize); // Texture descriptors starts after material descrisptors, in the CBV_SRV_UAV descriptor heap layout of the Scene class

	D3D12_CONSTANT_BUFFER_VIEW_DESC meshConstantsDesc;
	D3D12_GPU_VIRTUAL_ADDRESS bufferAddress = m_meshConstantsBuffer[meshId]->getResource()->GetGPUVirtualAddress();
	meshConstantsDesc.BufferLocation = bufferAddress;
	meshConstantsDesc.SizeInBytes = DXUtil::PadByteSizeTo256Mul(sizeof(MeshConstants));
	m_device->CreateConstantBufferView(&meshConstantsDesc, hDescriptor);
}

ComPtr<ID3D12RootSignature> Scene::CreateRootSignature()
{
	if (m_rootSignature) return m_rootSignature;

	CD3DX12_ROOT_PARAMETER rootParameters[4] = {};

	rootParameters[0].InitAsConstantBufferView(0, 0);	// Parameter 1: Root descriptor that will holds the pass constants PassConstants
	rootParameters[1].InitAsConstantBufferView(1, 0);	// Parameter 2: Root descriptor for mesh constants

	CD3DX12_DESCRIPTOR_RANGE descriptorRangesCBVSRV[2] = {};	// Parameter 3: Descriptor table with different ranges
	// Descriptor range for materials
	descriptorRangesCBVSRV[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, MATERIALS_N_DESCRIPTORS, 0, 1, 0);
	// Descriptor range for textures
	descriptorRangesCBVSRV[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, TEXTURES_N_DESCRIPTORS, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);		
	rootParameters[2].InitAsDescriptorTable(2, descriptorRangesCBVSRV);

	CD3DX12_DESCRIPTOR_RANGE descriptorRangeSamplers[1] = {};	// Parameter 4: Descriptor table for samplers
	descriptorRangeSamplers[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, SAMPLERS_N_DESCRIPTORS, 0);
	rootParameters[3].InitAsDescriptorTable(1, descriptorRangeSamplers);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, rootParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	ThrowIfFailed(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf()), 
		(errorBlob == nullptr) ? "Cannot serialize root signature" : DXUtil::GetErrorBlobMsg(errorBlob));
	ThrowIfFailed(m_device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)), 
		(errorBlob == nullptr) ? "Cannot create root signature" : DXUtil::GetErrorBlobMsg(errorBlob));

	return m_rootSignature;
}

void Scene::SetRootSignature(ID3D12GraphicsCommandList* commandList, unsigned int meshId)
{
	ID3D12DescriptorHeap* descriptorHeaps[] = { m_CBVSRVDescriptorHeap.Get(), m_samplersDescriptorHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	commandList->SetGraphicsRootSignature(m_rootSignature.Get());

	// Set the frame constants root parameter
	commandList->SetGraphicsRootConstantBufferView(0, m_frameConstantsBuffer->getResource()->GetGPUVirtualAddress());

	// Set the mesh constants root parameter
	commandList->SetGraphicsRootConstantBufferView(1, m_meshConstantsBuffer[meshId]->getResource()->GetGPUVirtualAddress());

	// Set the descriptors table parameter for mesh constants, materials, textures
	commandList->SetGraphicsRootDescriptorTable(2, m_CBVSRVDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	// Set the descriptors table parameter for samplers
	commandList->SetGraphicsRootDescriptorTable(3, m_samplersDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
}

void Scene::SetCamera(const Camera& camera) 
{
	m_frameConstants.projMtx = camera.getProjMtx();
	m_frameConstants.viewMtx = camera.getViewMtx();
	XMStoreFloat4x4(&m_frameConstants.projViewMtx, XMMatrixTranspose(XMMatrixMultiply(XMLoadFloat4x4(&m_frameConstants.viewMtx), XMLoadFloat4x4(&m_frameConstants.projMtx))));
	m_frameConstants.eyePosition = DirectX::XMFLOAT4(camera.GetPosition().x, camera.GetPosition().y, camera.GetPosition().z, 1.0f);
	m_frameConstantsBuffer->copyData(0, m_frameConstants);
}

void Scene::SetMeshConstants(unsigned int meshId, MeshConstants meshConstants)
{
	if (m_meshes.find(meshId) == m_meshes.end()) return;
	m_meshes[meshId].SetModelMtx(meshConstants.modelMtx);
	m_meshConstantsBuffer[meshId]->copyData(0, meshConstants);
}

void Scene::SetLight(unsigned int lightId, Light light)
{
	if (m_lights.find(lightId) == m_lights.end()) return;
	m_lights[lightId] = light;
	m_frameConstants.lights[lightId] = light;
}

void Scene::UpdateConstants(FrameConstants frameConstants)
{
	m_frameConstantsBuffer->copyData(0, frameConstants);
}

void Scene::Draw(ID3D12GraphicsCommandList* commandList)
{
	for (const auto& mesh : m_meshes) 
	{
		SetRootSignature(commandList, mesh.second.GetId());
		DrawMesh(mesh.second, commandList); 
	}
}

void Scene::DrawMesh(const Mesh& mesh, ID3D12GraphicsCommandList* commandList) 
{
	for (SubMesh subMesh : mesh.m_subMeshes)
	{
		D3D12_VERTEX_BUFFER_VIEW vbView; // Vertices buffer view
		vbView.BufferLocation = m_buffersGPU[subMesh.verticesBufferView.bufferId]->GetGPUVirtualAddress() + subMesh.verticesBufferView.byteOffset;
		vbView.StrideInBytes = sizeof(DirectX::XMFLOAT3);
		vbView.SizeInBytes = subMesh.verticesBufferView.byteLength;

		D3D12_VERTEX_BUFFER_VIEW nbView; // Normals buffer view
		nbView.BufferLocation = m_buffersGPU[subMesh.normalsBufferView.bufferId]->GetGPUVirtualAddress() + subMesh.normalsBufferView.byteOffset;
		nbView.StrideInBytes = sizeof(DirectX::XMFLOAT3);
		nbView.SizeInBytes = subMesh.verticesBufferView.byteLength;

		D3D12_VERTEX_BUFFER_VIEW tbView; // Tangents buffer view
		tbView.BufferLocation = m_buffersGPU[subMesh.tangentsBufferView.bufferId]->GetGPUVirtualAddress() + subMesh.tangentsBufferView.byteOffset;
		tbView.StrideInBytes = sizeof(DirectX::XMFLOAT4);
		tbView.SizeInBytes = subMesh.tangentsBufferView.byteLength;

		D3D12_VERTEX_BUFFER_VIEW tcbView; // Texture coordinates buffer view
		tcbView.BufferLocation = m_buffersGPU[subMesh.texCoord0BufferView.bufferId]->GetGPUVirtualAddress() + subMesh.texCoord0BufferView.byteOffset;
		tcbView.StrideInBytes = sizeof(DirectX::XMFLOAT2);
		tcbView.SizeInBytes = subMesh.texCoord0BufferView.byteLength;;

		D3D12_VERTEX_BUFFER_VIEW vertexBuffers[4] = { vbView, nbView, tbView, tcbView };
		commandList->IASetVertexBuffers(0, 4, vertexBuffers);
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		D3D12_INDEX_BUFFER_VIEW ibView; // Index buffer view
		ibView.BufferLocation = m_buffersGPU[subMesh.indicesBufferView.bufferId]->GetGPUVirtualAddress() + subMesh.indicesBufferView.byteOffset;
		ibView.Format = DXGI_FORMAT_R16_UINT;
		ibView.SizeInBytes = subMesh.indicesBufferView.byteLength;

		D3D12_INDEX_BUFFER_VIEW indexBuffers[1] = { ibView };
		commandList->IASetIndexBuffer(indexBuffers);

		commandList->DrawIndexedInstanced(subMesh.indicesBufferView.count, 1, 0, 0, 0);
	}
}
