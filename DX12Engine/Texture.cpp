#include "Texture.h"
#include <ResourceUploadBatch.h>
#include <DDSTextureLoader.h>
#include <WICTextureLoader.h>

void CreateTextureFromMemory(ID3D12Device* device, ID3D12CommandQueue* commandQueue, const uint8_t* textureData, size_t textureDataSize, ID3D12Resource** texture)
{
	DirectX::ResourceUploadBatch resourceUploadBatch(device);
	resourceUploadBatch.Begin();
	DXUtil::ThrowIfFailed(DirectX::CreateWICTextureFromMemory(device, resourceUploadBatch, textureData, textureDataSize, texture, true),
		"Cannot create texture");
	auto finish = resourceUploadBatch.End(commandQueue);
	finish.wait();
	//auto desc = (*texture)->GetDesc();
}

void CreateTextureFromDDSFile(ID3D12Device* device, ID3D12CommandQueue* commandQueue, std::string fileName, ID3D12Resource** texture)
{
	DirectX::ResourceUploadBatch resourceUploadBatch(device);
	resourceUploadBatch.Begin();
	DXUtil::ThrowIfFailed(DirectX::CreateDDSTextureFromFile(device, resourceUploadBatch, std::wstring(fileName.begin(), fileName.end()).c_str(), texture, true),
		"Cannot create DDS texture from " + fileName);
	auto finish = resourceUploadBatch.End(commandQueue);
	finish.wait();
}