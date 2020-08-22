#pragma once

#include "DXUtil.h"

struct TextureAccessor 
{
	uint32_t textureId;
	uint32_t texCoordId;
	DirectX::XMFLOAT2 _pad;
};

struct RoughMetallicMaterial
{
	DirectX::XMFLOAT4 baseColorFactor;
	float metallicFactor; DirectX::XMFLOAT3 _pad0;
	float roughnessFactor; DirectX::XMFLOAT3 _pad1;
	TextureAccessor baseColorTA;
	TextureAccessor roughMetallicTA;
	TextureAccessor normalTA;
	TextureAccessor emissiveTA;
	TextureAccessor occlusionTA;
};
