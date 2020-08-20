#pragma once

#include "DXUtil.h"

struct Material
{
	DirectX::XMFLOAT4 baseColorFactor;
	float metallicFactor;
	float roughnessFactor;
	unsigned int baseColorTextureId;
	unsigned int metallicRoughnessTextureId;
};
