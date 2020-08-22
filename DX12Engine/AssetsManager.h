#pragma once

#include "DXUtil.h"
#include "Renderer.h"
#include "Material.h"
#include <string>
#include <vector>
#include <map>
#include <wrl.h>

#define DESCRIPTORS_HEAP_SIZE 50

class AssetsManager
{
public:
	AssetsManager(Microsoft::WRL::ComPtr<ID3D12Device> device,
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> texturesDescriptorHeap,
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> samplersDescriptorHeap);

	void AddGPUBuffer(Microsoft::WRL::ComPtr<ID3D12Resource> buffer);
	void AddMaterial(unsigned int materialId, RoughMetallicMaterial material);
	void AddTexture(unsigned int textureId, Microsoft::WRL::ComPtr<ID3D12Resource> texture);
	void AddSampler(unsigned int samplerId, D3D12_SAMPLER_DESC samplerDesc);

public:
	Microsoft::WRL::ComPtr<ID3D12Device> m_device;
	
	UINT m_SRV_DescriptorSize = 0;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_texturesDescriptorHeap;
	UINT m_sampler_DescriptorSize = 0;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_samplersDescriptorHeap;
	
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_buffersGPU;
	std::map<unsigned int, Microsoft::WRL::ComPtr<ID3D12Resource>> m_textures;
	std::map<unsigned int, RoughMetallicMaterial> m_materials;
};