#include "World.h"
#include "Renderer.h"
#include "Mesh.h"
#include "GLTFScene.h"

using Microsoft::WRL::ComPtr;

void World::Init(std::shared_ptr<Renderer> renderer)
{
	m_renderer = renderer;
}


void World::addMesh(Mesh mesh)
{
    m_meshes.push_back(mesh);
}

World::~World()
{
}	

void World::release() 
{
	for (Mesh mesh : m_meshes) mesh.release();
}

std::vector<Mesh> World::getMeshes()
{
	return m_meshes;
}

void World::draw()
{

	for (Mesh mesh : m_meshes)
	{
		mesh.Draw();
	}
}

void World::LoadGLTF(std::string fileName)
{
	/*
	std::string err;
	std::string warn;

	bool ret = m_loader.LoadBinaryFromFile(&m_model, &err, &warn, fileName.c_str());
	if (!warn.empty()) { printf("Warn: %s\n", warn.c_str()); }
	if (!err.empty()) { printf("Err: %s\n", err.c_str()); }
	if (!ret) { printf("Failed to parse glTF\n"); return; }

	m_scene.LoadScene( , m_cmdList, m_model, 0);
	*/
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
}
