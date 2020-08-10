#include "Mesh.h"
#include "Buffers.h"

using Microsoft::WRL::ComPtr;
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
	},
	{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}//,
	//{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
};

Mesh::Mesh(){}

Mesh::Mesh(ComPtr<ID3D12Device> device, ComPtr<ID3D12GraphicsCommandList> cmdList)
{
	m_cmdList = cmdList;
	m_vertices =
	{
		{ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(DirectX::Colors::White) },
		{ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(DirectX::Colors::Black) },
		{ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(DirectX::Colors::Red) },
		{ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(DirectX::Colors::Green) },
		{ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(DirectX::Colors::Blue) },
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(DirectX::Colors::Yellow) },
		{ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(DirectX::Colors::Cyan) },
		{ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(DirectX::Colors::Magenta) }
	};
	m_vbByteSize = 8 * sizeof(Vertex);
	m_vertexBufferGPU = createDefaultHeapBuffer(device.Get(), m_cmdList.Get(), m_vertices.data(), m_vbByteSize, m_vertexBufferUploader);

	 
	m_indices =
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

	m_ibByteSize = 36 * sizeof(std::uint16_t);
	m_indexBufferGPU = createDefaultHeapBuffer(device.Get(), m_cmdList.Get(), m_indices.data(), m_ibByteSize, m_indexBufferUploader);
	
}

void Mesh::addDrawCommands(ComPtr<ID3D12GraphicsCommandList> cmdList)
{


	// Define the vertex buffer view to the GPU vertex buffer resource
	D3D12_VERTEX_BUFFER_VIEW vbView;
	vbView.BufferLocation = m_vertexBufferGPU->GetGPUVirtualAddress();
	vbView.StrideInBytes = sizeof(Vertex);
	vbView.SizeInBytes = m_vbByteSize;

	// Bind the vertex buffer to the pipeline
	D3D12_VERTEX_BUFFER_VIEW vertexBuffers[1] = { vbView };
	cmdList->IASetVertexBuffers(0, 1, vertexBuffers);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Create the index buffer view
	D3D12_INDEX_BUFFER_VIEW ibView;
	ibView.BufferLocation = m_indexBufferGPU->GetGPUVirtualAddress();
	ibView.Format = DXGI_FORMAT_R16_UINT;
	ibView.SizeInBytes = m_ibByteSize;
	D3D12_INDEX_BUFFER_VIEW indexBuffers[1] = { ibView };
	cmdList->IASetIndexBuffer(indexBuffers);

	// Draw all submeshes
	//for (SubMesh subMesh : m_subMeshes)
	//{
		cmdList->DrawIndexedInstanced(36, 1, 0, 0, 0);
	//}
}