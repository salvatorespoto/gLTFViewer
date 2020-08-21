#include "Mesh.h"
#include "DXUtil.h"

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
	//{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}//,
	{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
};

Mesh::Mesh(std::shared_ptr<AssetsManager> assetsManager)
{
	m_assetsManager = assetsManager;
}

void Mesh::SetId(unsigned int id)
{
	m_id = id;
}

void Mesh::AddSubMesh(const SubMesh& subMesh)
{
	m_subMeshes.push_back(subMesh);
}

void Mesh::Draw(ID3D12GraphicsCommandList* commandList)
{
	for (SubMesh subMesh : m_subMeshes)
	{
		D3D12_VERTEX_BUFFER_VIEW vbView;
		vbView.BufferLocation = m_assetsManager->m_buffersGPU[subMesh.verticesBufferView.bufferId]->GetGPUVirtualAddress() + subMesh.verticesBufferView.byteOffset;
		vbView.StrideInBytes = sizeof(DirectX::XMFLOAT3);
		vbView.SizeInBytes = subMesh.verticesBufferView.byteLength;

		D3D12_VERTEX_BUFFER_VIEW tbView;
		tbView.BufferLocation = m_assetsManager->m_buffersGPU[subMesh.texCoord0BufferView.bufferId]->GetGPUVirtualAddress() + subMesh.texCoord0BufferView.byteOffset;
		tbView.StrideInBytes = sizeof(DirectX::XMFLOAT2);
		tbView.SizeInBytes = subMesh.texCoord0BufferView.byteLength;;

		D3D12_VERTEX_BUFFER_VIEW vertexBuffers[2] = { vbView, tbView };
		commandList->IASetVertexBuffers(0, 2, vertexBuffers);
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		D3D12_INDEX_BUFFER_VIEW ibView;
		ibView.BufferLocation = m_assetsManager->m_buffersGPU[subMesh.indicesBufferView.bufferId]->GetGPUVirtualAddress() + subMesh.indicesBufferView.byteOffset;
		ibView.Format = DXGI_FORMAT_R16_UINT;
		ibView.SizeInBytes = subMesh.indicesBufferView.byteLength;

		D3D12_INDEX_BUFFER_VIEW indexBuffers[1] = { ibView };
		commandList->IASetIndexBuffer(indexBuffers);

		commandList->DrawIndexedInstanced(subMesh.indicesBufferView.count, 1, 0, 0, 0);
	}
}
