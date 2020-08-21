#pragma once

#include "DXUtil.h"
#include "Mesh.h"
#include "GLTFScene.h"
#include "AssetsManager.h"

class World
{

public:

	World() = default;
	~World();

	void Init(std::shared_ptr<Renderer> renderer);
	void LoadGLTF(std::string fileName);

	void release();
	//void addMesh(Mesh mesh);

	/** Render a frame basing on the current frame resources */
	void draw();

	/** Return the vector of meshes in the world */
	std::vector<Mesh> getMeshes();
	tinygltf::Model m_model;
	tinygltf::TinyGLTF m_loader;

	ID3D12GraphicsCommandList* m_cmdList;
	//std::vector<Mesh> m_meshes;
	GLTFScene m_scene;

private:
	std::shared_ptr<Renderer> m_renderer;
	std::shared_ptr<AssetsManager> m_assetsManager;
};

