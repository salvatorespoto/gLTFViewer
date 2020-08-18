#include "Mesh.h"
#include "DXUtil.h"
#include "Buffers.h"

using Microsoft::WRL::ComPtr;
using DXUtil::ThrowIfFailed;
using DirectX::XMFLOAT3;
using DirectX::XMFLOAT4;


D3D12_INPUT_ELEMENT_DESC vertexElementsDesc[] =
{
	{
		"POSITION",						// Semantic name: this will be used from the vertex shader to indentify the input field 
		0,								// Semantin index: a descriptor could have the same name, but different index (e.g. "POSITION0", "POSITION1")
		DXGI_FORMAT_R32G32B32_FLOAT,	// A DXGI_FORMAT that describe the data type
		0,								// Input slot 0
		0,								// Byte offset from the beginning of the struct, 0 because is the first struct field
		D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,	// Input slot class
		0								// Instaced data step rate
	}//,
	//{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}//,
	//{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
};

Mesh::Mesh(){}

Mesh::Mesh(std::shared_ptr<Renderer> renderer, void* positions, unsigned int posByteSize, void* indices, unsigned int indByteSize)
{
	m_renderer = renderer;
	ID3D12GraphicsCommandList* commandList = m_renderer->GetCommandList().Get();

	m_positions = positions;
	m_pbByteSize = posByteSize;
	m_indices = indices;
	m_ibByteSize = indByteSize;
	
	// Upload mesh data to the default heap buffer
	m_positionBufferGPU = createDefaultHeapBuffer(m_renderer->GetDevice().Get(), commandList, m_positions, m_pbByteSize, m_positionBufferUploader);
	m_indexBufferGPU = createDefaultHeapBuffer(m_renderer->GetDevice().Get(), commandList, m_indices, m_ibByteSize, m_indexBufferUploader);
}

Mesh::~Mesh()
{}

void Mesh::release()
{
	if (m_vertexBufferGPU) { m_vertexBufferGPU = nullptr; }
	if (m_vertexBufferUploader) { m_vertexBufferUploader = nullptr; }
	if (m_indexBufferGPU) { m_indexBufferGPU = nullptr; }
	if (m_indexBufferUploader) { m_indexBufferUploader = nullptr; }
}

void Mesh::Draw()
{
	// Define the vertex buffer view to the GPU vertex buffer resource
	//D3D12_VERTEX_BUFFER_VIEW vbView;
	//vbView.BufferLocation = m_vBufferGPU->GetGPUVirtualAddress();
	//vbView.StrideInBytes = sizeof(Vertex);
	//vbView.SizeInBytes = m_vbByteSize;

	ID3D12GraphicsCommandList* commandList = m_renderer->GetCommandList().Get();

	D3D12_VERTEX_BUFFER_VIEW pbView;
	pbView.BufferLocation = m_positionBufferGPU->GetGPUVirtualAddress();
	pbView.StrideInBytes = sizeof(Position);
	pbView.SizeInBytes = m_pbByteSize;
	
	//D3D12_VERTEX_BUFFER_VIEW cbView;
	//cbView.BufferLocation = m_colorBufferGPU->GetGPUVirtualAddress();
	//cbView.StrideInBytes = sizeof(Color);
	//cbView.SizeInBytes = m_cbByteSize;

	// Bind the vertex buffer to the pipeline
	D3D12_VERTEX_BUFFER_VIEW vertexBuffers[1] = { pbView };
	commandList->IASetVertexBuffers(0, 1, vertexBuffers);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Create the index buffer view
	D3D12_INDEX_BUFFER_VIEW ibView;
	ibView.BufferLocation = m_indexBufferGPU->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = m_ibByteSize;
	D3D12_INDEX_BUFFER_VIEW indexBuffers[1] = { ibView };
	commandList->IASetIndexBuffer(indexBuffers);

	commandList->DrawIndexedInstanced(46356, 1, 0, 0, 0);
}

void Mesh::LoadGLTF(std::string fileName) 
{
	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;

	bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, fileName.c_str());
	
	if (!warn.empty()) { printf("Warn: %s\n", warn.c_str()); }

	if (!err.empty()) { printf("Err: %s\n", err.c_str()); }

	if (!ret) { printf("Failed to parse glTF\n"); return; }
}
