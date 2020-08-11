#include "World.h"
#include "Mesh.h"

using Microsoft::WRL::ComPtr;

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

void World::draw(ComPtr<ID3D12GraphicsCommandList> cmdList)
{

	for (Mesh mesh : m_meshes)
	{
		mesh.addDrawCommands(cmdList);
	}
}
