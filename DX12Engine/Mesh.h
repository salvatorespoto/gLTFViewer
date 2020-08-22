#pragma once

#include "DXUtil.h"
#include "AssetsManager.h"

#define DESCRIPTORS_HEAP_SIZE 50

/** Mesh vertex descriptor */
D3D12_INPUT_ELEMENT_DESC vertexElementsDesc[];

/** A submes is a part of a Mesh */
struct SubMesh
{
	BufferView verticesBufferView;
	BufferView indicesBufferView;
	BufferView normalsBufferView;
	BufferView texCoord0BufferView;
	BufferView texCoord1BufferView;
	unsigned int materialId;
	unsigned int renderMode;
};

class AssetsManager;

/** A 3D mesh */
class Mesh
{
public:
	Mesh() = delete;
	Mesh(std::shared_ptr<AssetsManager> assetsManager);
	Mesh(const Mesh& mesh) = default;
	Mesh& operator=(const Mesh& mesh) = default;
	
	void SetId(unsigned int id);
	void SetModelMtx(const DirectX::XMFLOAT4X4& modelMtx);
	void AddSubMesh(const SubMesh& subMesh);
	void Draw(ID3D12GraphicsCommandList* commandList);

public:
	unsigned int m_id;
	std::vector<SubMesh> m_subMeshes;
	DirectX::XMFLOAT4X4 m_modelMtx = DXUtil::IdentityMtx();
	std::shared_ptr<AssetsManager> m_assetsManager;
};