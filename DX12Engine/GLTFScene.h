#pragma once

#include "DXUtil.h"
#include "Buffers.h"
#include "Mesh.h"

class GLTFScene
{
public:
	GLTFScene() = default;
	void LoadScene(Microsoft::WRL::ComPtr<ID3D12Device> device, ID3D12GraphicsCommandList* cmdList, tinygltf::Model model, unsigned int sceneId);
	void ParseSubTree(tinygltf::Node node);
	void BuildMesh(tinygltf::Mesh gltfMesh);

protected:
	Microsoft::WRL::ComPtr<ID3D12Device> m_device;
	ID3D12GraphicsCommandList* m_cmdList;
	std::vector<Mesh> m_meshes;
	tinygltf::Model m_model;
	

};