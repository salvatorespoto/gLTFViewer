#pragma once

#include "DXUtil.h"
#include "Renderer.h"
#include "DrawableAsset.h"
#include "Buffers.h"
#include "Camera.h"

D3D12_INPUT_ELEMENT_DESC skyBoxVertexElementsDesc[];

struct SkyBoxConstants
{
	DirectX::XMFLOAT4X4 modelMtx;
};

class SkyBox : public DrawableAsset
{
public:
	~SkyBox();
	void Init(Microsoft::WRL::ComPtr<ID3D12Device> device, Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue);
	void SetCubeMapTexture(Microsoft::WRL::ComPtr<ID3D12Resource> cubeMapTexture);
	Microsoft::WRL::ComPtr<ID3DBlob> GetVertexShader() const override;
	Microsoft::WRL::ComPtr<ID3DBlob> GetPixelShader() const override;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> GetRootSignature() override;
	void SetCamera(const Camera& camera);
	void SetFrameConstants(FrameConstants frameConstants);
	void SetSkyBoxConstants(SkyBoxConstants meshConstants);
	void Draw(ID3D12GraphicsCommandList* commandList) override;

protected:
	const unsigned int SKYBOX_SPHERE_SUBDIVISION = 1; // Number of subdivision to generate the skybox sphere

	void GenerateSphere();
	void SetUpRootSignature(ID3D12GraphicsCommandList* commandList);
    
	BufferView m_verticesBufferView;
	BufferView m_indicesBufferView;
	BufferView m_normalsBufferView;
	
	UINT m_CBVSRVDescriptorSize = 0;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_CBVSRVDescriptorHeap;

	FrameConstants m_frameConstants;
	std::unique_ptr<UploadBuffer<FrameConstants>> m_frameConstantsBuffer;
	SkyBoxConstants m_skyBoxConstants;
	std::unique_ptr<UploadBuffer<SkyBoxConstants>> m_skyBoxConstantsBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_cubeMapTexture;
	CD3DX12_STATIC_SAMPLER_DESC m_staticSamplerDesc;
};