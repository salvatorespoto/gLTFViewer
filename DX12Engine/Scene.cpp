#include "Scene.h"
#include "Mesh.h"
#include "Camera.h"
#include "SkyBox.h"

#include "UsingDirectives.h"

Scene::~Scene() {}

Scene::Scene(Microsoft::WRL::ComPtr<ID3D12Device> device)
{
	DEBUG_LOG("Initializing Scene object")
	m_device = device;
	m_CBVSRVDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_samplersDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	// Create CBV_SRV_UAV descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC CBVSRVHeapDesc = {};
	CBVSRVHeapDesc = {};
	CBVSRVHeapDesc.NumDescriptors = MATERIALS_N_DESCRIPTORS + TEXTURES_N_DESCRIPTORS + 6; // +6 is the cube map
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
	m_meshConstantsBuffer[meshId] = std::make_unique<UploadBuffer<MeshConstants>>(m_device.Get(), MAX_MESH_INSTANCES, false);

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(m_CBVSRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	hDescriptor.Offset(meshId, m_CBVSRVDescriptorSize); // Texture descriptors starts after material descrisptors, in the CBV_SRV_UAV descriptor heap layout of the Scene class

	D3D12_CONSTANT_BUFFER_VIEW_DESC meshConstantsDesc;
	D3D12_GPU_VIRTUAL_ADDRESS bufferAddress = m_meshConstantsBuffer[meshId]->getResource()->GetGPUVirtualAddress();
	meshConstantsDesc.BufferLocation = bufferAddress;
	meshConstantsDesc.SizeInBytes = DXUtil::PadByteSizeTo256Mul(sizeof(MeshConstants));
	m_device->CreateConstantBufferView(&meshConstantsDesc, hDescriptor);
}

void Scene::SetCubeMapTexture(Microsoft::WRL::ComPtr<ID3D12Resource> cubeMapTexture)
{
	m_cubeMapTexture = cubeMapTexture;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = m_cubeMapTexture->GetDesc().MipLevels;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	srvDesc.Format = m_cubeMapTexture->GetDesc().Format;

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(m_CBVSRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	hDescriptor.Offset(MATERIALS_N_DESCRIPTORS + TEXTURES_N_DESCRIPTORS, m_CBVSRVDescriptorSize);
	m_device->CreateShaderResourceView(m_cubeMapTexture.Get(), &srvDesc, hDescriptor);
}

float Scene::GetSceneRadius()
{
	return sqrt(m_sceneRadius.x*m_sceneRadius.x + m_sceneRadius.y * m_sceneRadius.y + m_sceneRadius.z * m_sceneRadius.z);
}

ComPtr<ID3D12RootSignature> Scene::CreateRootSignature()
{
	if (m_rootSignature) return m_rootSignature;

	CD3DX12_ROOT_PARAMETER rootParameters[4] = {};

	rootParameters[0].InitAsConstantBufferView(0, 0);	// Parameter 1: Root descriptor that will holds the pass constants PassConstants
	rootParameters[1].InitAsShaderResourceView(0, 0);	// Parameter 2: Root descriptor for mesh constants

	CD3DX12_DESCRIPTOR_RANGE descriptorRangesCBVSRV[3] = {};	// Parameter 3: Descriptor table with different ranges
	// Descriptor range for materials
	descriptorRangesCBVSRV[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, MATERIALS_N_DESCRIPTORS, 0, 1, 0);
	// Descriptor range for textures
	descriptorRangesCBVSRV[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, TEXTURES_N_DESCRIPTORS, 0, 1, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);		
	// Descriptor range for cubemap
	descriptorRangesCBVSRV[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 6, 0, 2, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);
	rootParameters[2].InitAsDescriptorTable(3, descriptorRangesCBVSRV);

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
	commandList->SetGraphicsRootShaderResourceView(1, m_meshConstantsBuffer[meshId]->getResource()->GetGPUVirtualAddress());

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
	XMStoreFloat4x4(&meshConstants.nodeTransformMtx, XMMatrixTranspose(XMLoadFloat4x4(&meshConstants.nodeTransformMtx)));	// Transpose the matrix due HLSL use a column-major memory layout by default
	m_meshes[meshId].SetModelMtx(meshConstants.modelMtx);

	int i = 0;
	for(MeshConstants meshConstants : m_meshInstances[meshId]) 
	{
		XMStoreFloat4x4(&meshConstants.nodeTransformMtx, XMMatrixTranspose(XMLoadFloat4x4(&meshConstants.nodeTransformMtx)));	// Transpose the matrix due HLSL use a column-major memory layout by default
		m_meshConstantsBuffer[meshId]->copyData(i++, meshConstants);
	}
	
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
	m_meshInstances.clear();
	if (m_isInitialized)
	{
		SetupNode(m_sceneRoot.get(), DXUtil::IdentityMtx());
		DrawNode(m_sceneRoot.get(), commandList, DXUtil::IdentityMtx());
	}
}

void Scene::SetupNode(SceneNode* node, DirectX::XMFLOAT4X4 parentMtx)
{
	DirectX::XMFLOAT4X4 M;
	DirectX::XMStoreFloat4x4(&M, DirectX::XMMatrixMultiply(DirectX::XMLoadFloat4x4(&node->transformMtx), DirectX::XMLoadFloat4x4(&parentMtx)));

	if (node->meshId != -1)
	{
		m_meshes[node->meshId].SetNodeMtx(M);
		m_meshInstances[node->meshId].push_back(m_meshes[node->meshId].constants);
		SetMeshConstants(node->meshId, m_meshes[node->meshId].constants);		
	}

	for (const std::unique_ptr<SceneNode>& child : node->children)
	{
		SetupNode(child.get(), M);
	}
}

void Scene::DrawNode(SceneNode* node, ID3D12GraphicsCommandList* commandList, DirectX::XMFLOAT4X4 parentMtx)
{
	DirectX::XMFLOAT4X4 M;
	DirectX::XMStoreFloat4x4(&M, DirectX::XMMatrixMultiply(DirectX::XMLoadFloat4x4(&node->transformMtx), DirectX::XMLoadFloat4x4(&parentMtx)));

	if(node->meshId != -1) 
	{
		//m_meshes[node->meshId].SetNodeMtx(M);
		//m_meshInstances[node->meshId].push_back(m_meshes[node->meshId].constants);
		SetMeshConstants(node->meshId, m_meshes[node->meshId].constants);
		SetRootSignature(commandList, node->meshId);
		DrawMesh(m_meshes[node->meshId], commandList);
	}

	for (const std::unique_ptr<SceneNode>& child : node->children)
	{
		DrawNode(child.get(), commandList, M);
	}
}

void Scene::DrawMesh(const Mesh& mesh, ID3D12GraphicsCommandList* commandList) 
{
	int nextIdBuf = 0;
	for (SubMesh subMesh : mesh.m_subMeshes)
	{
		D3D12_VERTEX_BUFFER_VIEW vbView; // Vertices buffer view
		if(subMesh.verticesBufferView.bufferId != -1) 
		{
			vbView.BufferLocation = m_buffersGPU[subMesh.verticesBufferView.bufferId]->GetGPUVirtualAddress() + subMesh.verticesBufferView.byteOffset;
			vbView.StrideInBytes = (subMesh.verticesBufferView.byteStride == 0) ? sizeof(DirectX::XMFLOAT3) : subMesh.verticesBufferView.byteStride;
			vbView.SizeInBytes = static_cast<UINT>(subMesh.verticesBufferView.byteLength);
			commandList->IASetVertexBuffers(nextIdBuf++, 1, &vbView);
		}
		
		D3D12_VERTEX_BUFFER_VIEW nbView; // Normals buffer view
		if (subMesh.normalsBufferView.bufferId != -1)
		{
			nbView.BufferLocation = m_buffersGPU[subMesh.normalsBufferView.bufferId]->GetGPUVirtualAddress() + subMesh.normalsBufferView.byteOffset;
			nbView.StrideInBytes = (subMesh.normalsBufferView.byteStride == 0) ? sizeof(DirectX::XMFLOAT3) : subMesh.normalsBufferView.byteStride;
			nbView.SizeInBytes = static_cast<UINT>(subMesh.normalsBufferView.byteLength);
			commandList->IASetVertexBuffers(nextIdBuf++, 1, &nbView);
		}
		//else nbView = vbView;

		D3D12_VERTEX_BUFFER_VIEW tbView; // Tangents buffer view
		if (subMesh.tangentsBufferView.bufferId != -1)
		{
			tbView.BufferLocation = m_buffersGPU[subMesh.tangentsBufferView.bufferId]->GetGPUVirtualAddress() + subMesh.tangentsBufferView.byteOffset;
			tbView.StrideInBytes = (subMesh.tangentsBufferView.byteStride == 0) ? sizeof(DirectX::XMFLOAT4) : subMesh.tangentsBufferView.byteStride;
			tbView.SizeInBytes = static_cast<UINT>(subMesh.tangentsBufferView.byteLength);
			commandList->IASetVertexBuffers(nextIdBuf++, 1, &tbView);
		}
		//else tbView = vbView;

		D3D12_VERTEX_BUFFER_VIEW tc0bView; // Texture coordinates buffer view
		if (subMesh.texCoord0BufferView.bufferId != -1)
		{
			tc0bView.BufferLocation = m_buffersGPU[subMesh.texCoord0BufferView.bufferId]->GetGPUVirtualAddress() + subMesh.texCoord0BufferView.byteOffset;
			tc0bView.StrideInBytes = (subMesh.texCoord0BufferView.byteStride == 0) ? sizeof(DirectX::XMFLOAT2) : subMesh.texCoord0BufferView.byteStride;
			tc0bView.SizeInBytes = static_cast<UINT>(subMesh.texCoord0BufferView.byteLength);
			commandList->IASetVertexBuffers(nextIdBuf++, 1, &tc0bView);
		}
		//else tc0bView = vbView;

		D3D12_VERTEX_BUFFER_VIEW tc1bView; // Texture coordinates buffer view
		if (subMesh.texCoord1BufferView.bufferId != -1)
		{
			tc1bView.BufferLocation = m_buffersGPU[subMesh.texCoord1BufferView.bufferId]->GetGPUVirtualAddress() + subMesh.texCoord0BufferView.byteOffset;
			tc1bView.StrideInBytes = (subMesh.texCoord1BufferView.byteStride == 0) ? sizeof(DirectX::XMFLOAT2) : subMesh.texCoord1BufferView.byteStride;
			tc1bView.SizeInBytes = static_cast<UINT>(subMesh.texCoord0BufferView.byteLength);
			commandList->IASetVertexBuffers(nextIdBuf++, 1, &tc1bView);
		}
	
		commandList->IASetPrimitiveTopology(subMesh.topology);

		D3D12_INDEX_BUFFER_VIEW ibView;
		if (subMesh.indicesBufferView.bufferId != -1)
		{
			ibView.BufferLocation = m_buffersGPU[subMesh.indicesBufferView.bufferId]->GetGPUVirtualAddress() + subMesh.indicesBufferView.byteOffset;
			ibView.Format = DXGI_FORMAT_R16_UINT;
			ibView.SizeInBytes = static_cast<UINT>(subMesh.indicesBufferView.byteLength);
			D3D12_INDEX_BUFFER_VIEW indexBuffers[1] = { ibView };

			commandList->IASetIndexBuffer(indexBuffers);
			commandList->DrawIndexedInstanced(subMesh.indicesBufferView.count, m_meshInstances[mesh.GetId()].size(), 0, 0, 0);
		}
		else
		{
			// No indices, it's a vertices list
			commandList->DrawInstanced(subMesh.verticesBufferView.count, m_meshInstances[mesh.GetId()].size(), 0, 0);
		}
	}
}
