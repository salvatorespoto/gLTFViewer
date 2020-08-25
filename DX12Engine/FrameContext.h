#pragma once

#include "DXUtil.h"

/** All the common constants used for a single frame rendering */
struct PassConstants
{
	DirectX::XMFLOAT4X4 viewMtx;
	DirectX::XMFLOAT4X4 projMtx;
	DirectX::XMFLOAT4X4 projViewMtx;
	DirectX::XMFLOAT3 eyePosition; float _pad0;
	DirectX::XMFLOAT3 lightPosition; float _pad1;
};

/**
 * Constants relative to mesh rendering
 */
struct MeshConstants
{
	DirectX::XMFLOAT4X4 modelMtx;
};