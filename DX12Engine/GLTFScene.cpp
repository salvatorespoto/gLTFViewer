#include "GLTFScene.h"
#include "Texture.h"
#include "Material.h"

using Microsoft::WRL::ComPtr;

void GLTFScene::LoadScene(std::shared_ptr<Renderer> renderer, tinygltf::Model model, unsigned int sceneId)
{
	m_renderer = renderer;
	m_model = model;
	if (sceneId >= model.scenes.size()) DXUtil::ThrowException("Scene index out of range");
	
	// Add materials

	// Add textures

	// Add samplers


	tinygltf::Image image = m_model.images[m_model.textures[3].source];
	tinygltf::BufferView imageBufferView = m_model.bufferViews[image.bufferView];
	tinygltf::Buffer imageBuffer = m_model.buffers[imageBufferView.buffer];
	uint8_t* imageBufferBegin = imageBuffer.data.data() + imageBufferView.byteOffset;
	ID3D12Resource* texture; 
	CreateTextureFromMemory(m_renderer.get(), imageBufferBegin, imageBufferView.byteLength, &texture);
	m_renderer->AddTexture(texture);

	tinygltf::Material gltfMat = m_model.materials[0];
	Material mat;
	mat.baseColorFactor = { 
		static_cast<float>(gltfMat.pbrMetallicRoughness.baseColorFactor[0]), 
		static_cast<float>(gltfMat.pbrMetallicRoughness.baseColorFactor[1]), 
		static_cast<float>(gltfMat.pbrMetallicRoughness.baseColorFactor[2]), 
		static_cast<float>(gltfMat.pbrMetallicRoughness.baseColorFactor[3])
	};
	mat.roughnessFactor = static_cast<float>(gltfMat.pbrMetallicRoughness.roughnessFactor);
	mat.metallicFactor = static_cast<float>(gltfMat.pbrMetallicRoughness.metallicFactor);
	mat.baseColorTextureId = gltfMat.pbrMetallicRoughness.baseColorTexture.index;
	mat.metallicRoughnessTextureId = gltfMat.pbrMetallicRoughness.metallicRoughnessTexture.index;
	


	// Add sampler
	D3D12_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.MipLODBias = 0.0f;
	samplerDesc.MaxAnisotropy = 1;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	
	m_renderer->AddSample(samplerDesc);

	// Load meshes
	for (tinygltf::Node node : model.nodes)
	{
		if (node.mesh != -1)
		{
			BuildMesh(m_model.meshes[node.mesh]);
			ParseSubTree(node);
		}
		else if (node.camera != -1) continue;
	}

	
}

void GLTFScene::ParseSubTree(tinygltf::Node node)
{
	for (int childId : node.children)
	{
		tinygltf::Node node = m_model.nodes[childId];
		if (node.mesh != -1)
		{
			BuildMesh(m_model.meshes[node.mesh]);
		}
		else if (node.camera != -1) continue;
	}
}

void GLTFScene::BuildMesh(tinygltf::Mesh gltfMesh)
{
	for (tinygltf::Primitive primitive : gltfMesh.primitives)
	{
		int acessorPositionsId = primitive.attributes["POSITION"];
		int accessorNormalsId = primitive.attributes["NORMAL"];
		int accessorIndexesId = primitive.indices;

		// Get primitive buffers BufferView (primitive->attributes["buffer type"]->accessor->bufferView
		tinygltf::BufferView positionsBufferView = m_model.bufferViews[m_model.accessors[primitive.attributes["POSITION"]].bufferView];
		tinygltf::BufferView normalsBufferView = m_model.bufferViews[m_model.accessors[primitive.attributes["NORMAL"]].bufferView];
		tinygltf::BufferView textCoordBufferView = m_model.bufferViews[m_model.accessors[primitive.attributes["TEXCOORD_0"]].bufferView];
		tinygltf::BufferView indexesBufferView = m_model.bufferViews[m_model.accessors[primitive.indices].bufferView];
		

		tinygltf::Buffer positionsBuffer = m_model.buffers[positionsBufferView.buffer];
		void* positionsBufferBegin = positionsBuffer.data.data() + positionsBufferView.byteOffset;

		tinygltf::Buffer indexesBuffer = m_model.buffers[indexesBufferView.buffer];
		void* indexesBufferBegin = indexesBuffer.data.data() + indexesBufferView.byteOffset;

		tinygltf::Buffer textCoordBuffer = m_model.buffers[textCoordBufferView.buffer];
		void* textCoordBufferBegin = textCoordBuffer.data.data() + textCoordBufferView.byteOffset;

		m_renderer->ResetCommandList();
		m_meshes.emplace_back(m_renderer, positionsBufferBegin, positionsBufferView.byteLength, 
			indexesBufferBegin, indexesBufferView.byteLength, textCoordBufferBegin, textCoordBufferView.byteLength);
		m_renderer->GetCommandList()->Close();
		m_renderer->ExecuteCommandList(m_renderer->GetCommandList().Get());
		m_renderer->FlushCommandQueue();

	}
	/*
	std::vector<DirectX::XMFLOAT3> positions =
	{
		{ DirectX::XMFLOAT3(+1.0f, +1.0f, -1.0f) },
		{ DirectX::XMFLOAT3(+1.0f, -1.0f, -1.0f) },
		{ DirectX::XMFLOAT3(+1.0f, +1.0f, +1.0f) },
		{ DirectX::XMFLOAT3(+1.0f, -1.0f, +1.0f) },
		{ DirectX::XMFLOAT3(-1.0f, +1.0f, -1.0f) },
		{ DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f) },
		{ DirectX::XMFLOAT3(-1.0f, +1.0f, +1.0f) },
		{ DirectX::XMFLOAT3(-1.0f, -1.0f, +1.0f) }
	};
	UINT64 m_pbByteSize = 8 * sizeof(DirectX::XMFLOAT3);

	std::vector<std::uint16_t> indices =
	{
		4, 2, 0,
		2, 7, 3,
		6, 5, 7,
		1, 7, 5,
		0, 3, 1,
		4, 1, 5,
		4, 6, 2,
		2, 6, 7,
		6, 4, 5,
		1, 3, 7,
		0, 2, 3,
		4, 0, 1
	};
	UINT64 ibByteSize = indices.size() * sizeof(std::uint16_t);

	m_renderer->ResetCommandList();
	m_meshes.emplace_back(m_renderer, positions.data(), m_pbByteSize, indices.data(), ibByteSize);
	m_renderer->GetCommandList()->Close();
	m_renderer->ExecuteCommandList(m_renderer->GetCommandList().Get());
	m_renderer->FlushCommandQueue();
		*/
}

std::vector<Mesh> GLTFScene::getMeshes()
{
	return m_meshes;
}