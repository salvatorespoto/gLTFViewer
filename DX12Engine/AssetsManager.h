#pragma once

#include "DXUtil.h"
#include "Renderer.h"
#include <string>
#include <vector>
#include <map>
#include <wrl.h>

#define DESCRIPTORS_HEAP_SIZE 50

class AssetsManager
{
public:
	AssetsManager(Microsoft::WRL::ComPtr<ID3D12Device> device);

	void AddGPUBuffer(Microsoft::WRL::ComPtr<ID3D12Resource> buffer);
	void AddTexture(unsigned int textureId, Microsoft::WRL::ComPtr<ID3D12Resource> texture);
	void AddSampler(unsigned int samplerId, D3D12_SAMPLER_DESC samplerDesc);

public:
	Microsoft::WRL::ComPtr<ID3D12Device> m_device;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_buffersGPU;
	std::map<unsigned int, Microsoft::WRL::ComPtr<ID3D12Resource>> m_textures;
	UINT m_SRV_DescriptorSize = 0;
	UINT m_sampler_DescriptorSize = 0;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_samplersDescriptorHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_texturesDescriptorHeap;
};