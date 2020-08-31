#pragma once

#include "DXUtil.h"

/** A DrawableAsset is an asset that could be renderer from the Renderer object */
class DrawableAsset
{
public:
	virtual void AddGPUBuffer(Microsoft::WRL::ComPtr<ID3D12Resource> buffer);
	virtual Microsoft::WRL::ComPtr<ID3DBlob> GetVertexShader() = 0;
	virtual Microsoft::WRL::ComPtr<ID3DBlob> GetPixelShader() = 0;
	virtual	Microsoft::WRL::ComPtr<ID3D12RootSignature> GetRootSignature() = 0;
	virtual void Draw(ID3D12GraphicsCommandList* commandList) = 0;

protected:
	virtual void SetUpRootSignature(ID3D12GraphicsCommandList* commandList) = 0;

	Microsoft::WRL::ComPtr<ID3DBlob> m_vertexShader;
	Microsoft::WRL::ComPtr<ID3DBlob> m_pixelShader;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_buffersGPU;	// Buffers that stores asset data in the GPU default heap
	Microsoft::WRL::ComPtr<ID3D12Device> m_device;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
};

