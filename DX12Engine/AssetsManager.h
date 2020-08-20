#pragma once

#include "DXUtil.h"
#include "Renderer.h"
#include <string>
#include <vector>
#include <wrl.h>

/*
struct SubMesh
{
	unsigned int bufferId;
	unsigned int byteOffset;
	unsigned int byteLenght;
};

struct Mesh 
{
	std::string id;
	std::vector<SubMesh> subMeshes;
};

class AssetsManager
{

public:
	
	// Buffers that holds all rendering resources
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_assetsBufferGPU;

	AssetsManager() {}
	AssetsManager(Microsoft::WRL::ComPtr<Renderer> renderer)
	{
		
	}


	void UploadBuffer() 
	{
		m_assetsBufferGPU.push_back(createDefaultHeapBuffer(m_renderer->GetDevice().Get(), commandList, m_positions, m_pbByteSize, m_positionBufferUploader));

	}
	
	// Meshes


	// Vertex buffers
	
	// Normal buffers
	
	// Texture coordinate buffers

	// Materials

	// Textures

private:
	Microsoft::WRL::ComPtr<Renderer> m_renderer;
};

*/