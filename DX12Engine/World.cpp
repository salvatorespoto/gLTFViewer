#include "World.h"
#include "Renderer.h"
#include "AssetTypes.h"
#include "GLTFScene.h"
#include "AssetsManager.h"

using Microsoft::WRL::ComPtr;

void World::Init(std::shared_ptr<Renderer> renderer)
{
	m_renderer = renderer;
	m_assetsManager = std::make_shared<AssetsManager>(renderer->GetDevice());
}

/*
void World::addMesh(Mesh mesh)
{
    m_meshes.push_back(mesh);
}
*/
World::~World()
{
}	

void World::release() 
{
	//for (Mesh mesh : m_meshes) mesh.release();
}

std::vector<Mesh> World::getMeshes()
{
	return m_scene.getMeshes();
}

void World::draw()
{

	ID3D12GraphicsCommandList* commandList = m_renderer->GetCommandList().Get();

	// Set descriptor heaps
	ID3D12DescriptorHeap* descriptorHeaps[] = { /*m_CBV_SRV_DescriptorHeap.Get(),*/ m_assetsManager->m_texturesDescriptorHeap.Get(), m_assetsManager->m_samplersDescriptorHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	//CD3DX12_GPU_DESCRIPTOR_HANDLE cbv(m_CBV_SRV_DescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	//cbv.Offset(0, m_CBV_SRV_DescriptorSize);
	//commandList->SetGraphicsRootDescriptorTable(1, cbv);

	CD3DX12_GPU_DESCRIPTOR_HANDLE srv(m_assetsManager->m_texturesDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	srv.Offset(0, m_assetsManager->m_SRV_DescriptorSize);
	commandList->SetGraphicsRootDescriptorTable(1, srv);

	// Samplers
	commandList->SetGraphicsRootDescriptorTable(2, m_assetsManager->m_samplersDescriptorHeap->GetGPUDescriptorHandleForHeapStart());



	for (Mesh mesh : m_scene.getMeshes())
	{
		mesh.Draw(m_renderer->GetCommandList().Get());
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

	m_scene.LoadScene( m_renderer, m_assetsManager, m_model, 0);
}
