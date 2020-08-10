#pragma once

#include "DXUtil.h"
#include "FrameContext.h"

/** A mesh vertex */
struct Vertex
{
	DirectX::XMFLOAT3 position; 
	//DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT4 color;
	//DirectX::XMFLOAT2 texCoord;
};

/** Array of input element descriptors */
D3D12_INPUT_ELEMENT_DESC vertexElementsDesc[];

/** A 3D submesh idexes into the vertex and index arrays of a mesh */
struct SubMesh
{
	UINT indexCount;		
	UINT startIndex;
	INT baseVertexIndex;
};

/*
struct MeshConstants
{
	DirectX::XMFLOAT4X4 m_worldMtx = IdentityMtx();
};
*/

/**
 * A 3D mesh owns its vertex and buffer arrays and is composed from severeral SubMeshes
 */
class Mesh
{

public:

	Mesh();

	Mesh(Microsoft::WRL::ComPtr<ID3D12Device> device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList);

	/** Draw the 3D mesh */
	void addDrawCommands(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList);

	/** The external function loadMeshFromObj has access to the Mesh class private member */
	//friend Mesh loadMeshFromObj(std::string objFileName);

	MeshConstants constants;

	void setDirty(bool isDirty) { m_isDirty = isDirty; }
	bool isDirty() { return m_isDirty; };
	int getIndex() { return m_idx; }

	std::vector<Vertex> m_vertices;
	std::vector<std::uint16_t> m_indices;

private:

	UINT64 m_vbByteSize;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBufferUploader = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBufferGPU = nullptr;
	
	UINT64 m_ibByteSize;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBufferUploader = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBufferGPU = nullptr;

	std::vector<SubMesh> m_subMeshes;
	
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_cmdList;

	int m_idx = 0;	/*< Unique mesh index used to arrange constant buffers */
	bool m_isDirty; /*< This mesh attributes (world matrix) has changed */
};