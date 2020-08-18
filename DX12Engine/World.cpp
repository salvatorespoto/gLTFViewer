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
	return m_scene.getMeshes();
}

void World::draw()
{

	for (Mesh mesh : m_scene.getMeshes())
	{
		mesh.Draw();
	}
}

void World::LoadGLTF(std::string fileName)
{
	
	std::string err;
	std::string warn;

	bool ret = m_loader.LoadBinaryFromFile(&m_model, &err, &warn, fileName.c_str());
	if (!warn.empty()) { printf("Warn: %s\n", warn.c_str()); }
	if (!err.empty()) { printf("Err: %s\n", err.c_str()); }
	if (!ret) { printf("Failed to parse glTF\n"); return; }

	m_scene.LoadScene( m_renderer, m_model, 0);
}
