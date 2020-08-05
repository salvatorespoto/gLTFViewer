#pragma once

#include "DXUtil.h"

/**
 * Constants relative to frame rendering
 */
struct FrameConstants
{
	DirectX::XMFLOAT4X4 viewMtx;
	DirectX::XMFLOAT4X4 projMtx;
	double totalTime;
	double deltaTime;
};

/**
 * Constants relative to mesh rendering
 */
struct MeshConstants
{
	DirectX::XMFLOAT4X4 modelMtx;
};

/**
 * CPU submit multiple frame to render to GPU without waiting for each frame completion.
 * This is done to keep both the CPU and GPU busy and don't make them wait for each other.
 * Each frame submitted needs has its own allocator and frame and mesh constants. 
 */
struct FrameContext2
{
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_cmdListAlloc;	// Each frame context has its own allocator
	UINT64 m_fence = 0;	// Used to check if this frame resources are still used from the GPU
};




/**
 * Holds the resources needed from a frame.
 */
struct FrameResource
{
public:
	
};