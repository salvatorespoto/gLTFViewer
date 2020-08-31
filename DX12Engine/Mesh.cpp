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
	{"NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	{"TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 3, 0, D3D12_INPUT_CLASSIFICATION::D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
};

Mesh::Mesh()
{
	constants.modelMtx = DXUtil::IdentityMtx();
}

void Mesh::SetId(unsigned int id)
{
	m_id = id;
}

unsigned int Mesh::GetId() const
{
	return m_id;
}

void Mesh::SetModelMtx(const DirectX::XMFLOAT4X4& modelMtx)
{
	constants.modelMtx = modelMtx;
}

void Mesh::SetNodeMtx(const DirectX::XMFLOAT4X4& nodeMtx)
{
	constants.nodeTransformMtx = nodeMtx;
}

void Mesh::AddSubMesh(const SubMesh& subMesh)
{
	m_subMeshes.push_back(subMesh);
}