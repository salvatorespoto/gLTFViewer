#pragma once

#include "DXUtil.h"
#include "Renderer.h"
#include <ResourceUploadBatch.h>
#include <WICTextureLoader.h>

void CreateTextureFromMemory(ID3D12Device* device, ID3D12CommandQueue* commandQueue, const uint8_t* textureData, size_t textureDataSize, ID3D12Resource** texture);
