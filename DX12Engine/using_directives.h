#pragma once

#include "DXUtil.h"

using Microsoft::WRL::ComPtr;
using DXUtil::ThrowIfFailed;
using std::string;

// DirectX data types and functions
using 
DirectX::XMFLOAT2, 
DirectX::XMFLOAT3, 
DirectX::XMFLOAT4, 
DirectX::XMFLOAT3X3, 
DirectX::XMFLOAT4X4, 
DirectX::XMVECTOR, 
DirectX::XMMATRIX,
DirectX::XMVectorGetX, 
DirectX::XMVectorGetY, 
DirectX::XMVectorGetZ, 
DirectX::XMVectorSetX, 
DirectX::XMVectorSetY, 
DirectX::XMVectorSetZ,
DirectX::XMLoadFloat4, 
DirectX::XMStoreFloat4, 
DirectX::XMVector3Normalize, 
DirectX::XMVectorSubtract, 
DirectX::XMVectorAdd, 
DirectX::XMVectorScale,
DirectX::XMVectorMultiply, 
DirectX::XMVector3Dot, 
DirectX::XMVector3Cross,
DirectX::XMMatrixTranslation, 
DirectX::XMMatrixRotationQuaternion, 
DirectX::XMMatrixScaling, 
DirectX::XMMatrixMultiply,
DirectX::XMConvertToRadians,
DirectX::XMMatrixRotationAxis;