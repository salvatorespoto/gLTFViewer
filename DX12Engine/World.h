#pragma once

#include "DXUtil.h"
#include "Mesh.h"

/**
 * This class contains all the entities rendered in the world:
 * - meshes
 */
class World
{

public:

	/** Constructor */
	World() {}
	
	/** Add a 3D mesh to the world */
	void addMesh(Mesh mesh);

	/** Render a frame basing on the current frame resources */
	void draw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> cmdList);

	/** Return the vector of meshes in the world */
	std::vector<Mesh> getMeshes();

	std::vector<Mesh> m_meshes;				
};

