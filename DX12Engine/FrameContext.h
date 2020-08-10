#pragma once

#include "DXUtil.h"

/**
 * Constants relative to frame rendering
 */
struct PassConstants
{
	DirectX::XMFLOAT4X4 viewMtx;
	DirectX::XMFLOAT4X4 projMtx;
	DirectX::XMFLOAT4X4 projViewMtx;
};

/**
 * Constants relative to mesh rendering
 */
struct MeshConstants
{
	DirectX::XMFLOAT4X4 modelMtx;
};