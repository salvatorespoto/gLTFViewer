#include "Texture.h"
#include "Renderer.h"


void CreateTextureFromMemory(Renderer* renderer, const uint8_t* textureData, size_t textureDataSize, ID3D12Resource** texture)
{
	DirectX::ResourceUploadBatch resourceUploadBatch(renderer->GetDevice().Get());
	resourceUploadBatch.Begin();
	DXUtil::ThrowIfFailed(DirectX::CreateWICTextureFromMemory(renderer->GetDevice().Get(), resourceUploadBatch, textureData, textureDataSize, texture, true),
		"Cannot create texture");
	auto finish = resourceUploadBatch.End(renderer->GetCommandQueue().Get());
	finish.wait();
	auto desc = (*texture)->GetDesc();
}