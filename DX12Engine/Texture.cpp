#include "Texture.h"

void CreateTextureFromMemory(ID3D12Device* device, ID3D12CommandQueue* commandQueue, const uint8_t* textureData, size_t textureDataSize, ID3D12Resource** texture)
{
	DirectX::ResourceUploadBatch resourceUploadBatch(device);
	resourceUploadBatch.Begin();
	DXUtil::ThrowIfFailed(DirectX::CreateWICTextureFromMemory(device, resourceUploadBatch, textureData, textureDataSize, texture, true),
		"Cannot create texture");
	auto finish = resourceUploadBatch.End(commandQueue);
	finish.wait();
	auto desc = (*texture)->GetDesc();
}