#include "GLTFScene.h"

using Microsoft::WRL::ComPtr;

void GLTFScene::LoadScene(ComPtr<ID3D12Device> device, ID3D12GraphicsCommandList* cmdList, tinygltf::Model model, unsigned int sceneId)
{
	m_model = model;
	m_device = device;
	m_cmdList = cmdList;
	if (sceneId >= model.scenes.size()) DXUtil::ThrowException("Scene index out of range");

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
	/*
	for (tinygltf::Primitive primitive : gltfMesh.primitives)
	{
		int acessorPositionsId = primitive.attributes["POSITION"];
		int accessorNormalsId = primitive.attributes["NORMAL"];
		int accessorIndexesId = primitive.indices;

		// Get primitive buffers BufferView (primitive->attributes["buffer type"]->accessor->bufferView
		tinygltf::BufferView positionsBufferView = m_model.bufferViews[m_model.accessors[primitive.attributes["POSITION"]].bufferView];
		tinygltf::BufferView normalsBufferView = m_model.bufferViews[m_model.accessors[primitive.attributes["NORMAL"]].bufferView];
		tinygltf::BufferView indexesBufferView = m_model.bufferViews[m_model.accessors[primitive.indices].bufferView];

		tinygltf::Buffer positionsBuffer = m_model.buffers[positionsBufferView.buffer];
		void* positionsBufferBegin = positionsBuffer.data.data() + positionsBufferView.byteOffset;

		tinygltf::Buffer indexesBuffer = m_model.buffers[indexesBufferView.buffer];
		void* indexesBufferBegin = positionsBuffer.data.data() + positionsBufferView.byteOffset;

		m_meshes.emplace_back(m_device, m_cmdList, positionsBufferBegin, positionsBufferView.byteLength, indexesBufferBegin, indexesBufferView.byteLength);
		//std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>(m_device, m_cmdList, positionsBufferBegin, positionsBufferView.byteLength, indexesBufferBegin, indexesBufferView.byteLength);
		//m_meshes.push_back(mesh);
	}*/

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
	//m_meshes.emplace_back(m_device, m_cmdList, positions.data(), m_pbByteSize, indices.data(), ibByteSize);
}
