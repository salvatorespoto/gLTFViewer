#pragma once

#include "DXUtil.h"

void CreateTextureFromMemory(ID3D12Device* device, ID3D12CommandQueue* commandQueue, const uint8_t* textureData, size_t textureDataSize, ID3D12Resource** texture);
void CreateTextureFromDDSFile(ID3D12Device* device, ID3D12CommandQueue* commandQueue, std::string fileName, ID3D12Resource** texture);
