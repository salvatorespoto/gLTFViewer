#pragma once

#include "DXUtil.h"
#include "Renderer.h"
#include "FrameContext.h"

struct Position
{
	DirectX::XMFLOAT3 position; 
};

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

class Mesh
{

public:

	Mesh();
	Mesh(std::shared_ptr<Renderer> renderer, void* positions, unsigned int posByteSize, void* indices, unsigned int indByteSize);
	~Mesh();

	void release();
	void Draw();

	void LoadGLTF(std::string fileName);


	//MeshConstants constants;

	void setDirty(bool isDirty) { m_isDirty = isDirty; }
	bool isDirty() { return m_isDirty; };
	int getIndex() { return m_idx; }

	void* m_positions;
	void* m_indices;

private:

	int m_idx = 0;	// Unique mesh index used to arrange constant buffers
	bool m_isDirty; // This mesh attributes (world matrix) has changed

	UINT64 m_vbByteSize;
	UINT64 m_pbByteSize;
	UINT64 m_cbByteSize;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBufferUploader = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBufferGPU = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_positionBufferUploader = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_positionBufferGPU = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_colorBufferUploader = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_colorBufferGPU = nullptr;

	UINT64 m_ibByteSize;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBufferUploader = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_indexBufferGPU = nullptr;

	std::vector<SubMesh> m_subMeshes;
	
	std::shared_ptr<Renderer> m_renderer;
};