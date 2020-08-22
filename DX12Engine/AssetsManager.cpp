#include "AssetsManager.h"

AssetsManager::AssetsManager(Microsoft::WRL::ComPtr<ID3D12Device> device, 
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> texturesDescriptorHeap,
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> samplersDescriptorHeap

)
{
	m_device = device;
	m_texturesDescriptorHeap = texturesDescriptorHeap;
	m_samplersDescriptorHeap = samplersDescriptorHeap;
}

void AssetsManager::AddGPUBuffer(Microsoft::WRL::ComPtr<ID3D12Resource> buffer)
{
	m_buffersGPU.push_back(buffer);
}

void AssetsManager::AddMaterial(unsigned int materialId, RoughMetallicMaterial material)
{
	m_materials[materialId] = material;
}

void AssetsManager::AddTexture(unsigned int textureId, Microsoft::WRL::ComPtr<ID3D12Resource> texture)
{
	m_textures[textureId] = texture;

	// Create the view for the texture
	m_SRV_DescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(m_texturesDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	hDescriptor.Offset(textureId+2, m_SRV_DescriptorSize);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = texture->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = texture->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	m_device->CreateShaderResourceView(texture.Get(), &srvDesc, hDescriptor);
}

void AssetsManager::AddSampler(unsigned int samplerId, D3D12_SAMPLER_DESC samplerDesc)
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(m_samplersDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	hDescriptor.Offset(samplerId, m_sampler_DescriptorSize);
	m_device->CreateSampler(&samplerDesc, m_samplersDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}
