#pragma once

#include "DXUtil.h"
#include "Renderer.h"
#include "Buffers.h"
#include "Mesh.h"

class GLTFScene
{
public:
	GLTFScene() = default;
	void LoadScene(std::shared_ptr<Renderer> renderer, tinygltf::Model model, unsigned int sceneId);
	void ParseSubTree(tinygltf::Node node);
	void BuildMesh(tinygltf::Mesh gltfMesh);
	std::vector<Mesh> getMeshes();

protected:
	std::shared_ptr<Renderer> m_renderer;
	ID3D12GraphicsCommandList* m_cmdList;
	std::vector<Mesh> m_meshes;
	tinygltf::Model m_model;
};