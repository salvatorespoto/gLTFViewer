#pragma once

#include "DXUtil.h"
#include "Scene.h"

#define DESCRIPTORS_HEAP_SIZE 50

/** Mesh vertex descriptor */
D3D12_INPUT_ELEMENT_DESC vertexElementsDesc[];

/** A submes is a part of a Mesh */
struct SubMesh
{
	BufferView verticesBufferView;
	BufferView colorsBufferView;
	BufferView normalsBufferView;
	BufferView tangentsBufferView;
	BufferView texCoord0BufferView;
	BufferView texCoord1BufferView;
	BufferView indicesBufferView;
	unsigned int materialId;
	D3D_PRIMITIVE_TOPOLOGY topology;
};

/** MeshConstants contains all mesh constants used from shaders */
struct MeshConstants
{
	DirectX::XMFLOAT4X4 modelMtx;
	DirectX::XMFLOAT4X4 nodeTransformMtx;
	DirectX::XMFLOAT4 rotXYZ; // Rotations about the X, Y and Z axis. The fourth component is not used
};

/** A 3D mesh */
class Mesh
{
public:
	Mesh();
	Mesh(const Mesh& mesh) = default;
	Mesh& operator=(const Mesh& mesh) = default;
	
	void SetId(unsigned int id);
	unsigned int GetId() const;
	void SetModelMtx(const DirectX::XMFLOAT4X4& modelMtx);
	void SetNodeMtx(const DirectX::XMFLOAT4X4& nodeMtx); // A model transformation defined as the default position of the mesh in the world
	void AddSubMesh(const SubMesh& subMesh);

	friend void Scene::DrawMesh(const Mesh& mesh, ID3D12GraphicsCommandList* commandList);
	MeshConstants constants;
protected:
	unsigned int m_id;
	
	std::vector<SubMesh> m_subMeshes;
};