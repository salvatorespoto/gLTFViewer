#pragma once

#include "DXUtil.h"
#include "Renderer.h"
#include "DrawableAsset.h"
#include "Buffers.h"
#include "Camera.h"

D3D12_INPUT_ELEMENT_DESC gridVertexElementsDesc[];

struct GridConstants
{
	DirectX::XMFLOAT4X4 modelMtx;
};

class Grid : public DrawableAsset
{
public:
	~Grid();
	void Init(Microsoft::WRL::ComPtr<ID3D12Device> device, Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue, float halfSize);
	Microsoft::WRL::ComPtr<ID3DBlob> GetVertexShader();
	Microsoft::WRL::ComPtr<ID3DBlob> GetPixelShader();
	Microsoft::WRL::ComPtr<ID3D12RootSignature> GetRootSignature();
	void SetCamera(const Camera& camera);
	void SetFrameConstants(FrameConstants frameConstants);
	void SetGridConstants(GridConstants meshConstants);
	void Draw(ID3D12GraphicsCommandList* commandList);

protected:
	void GenerateGrid(float side);
	void SetUpRootSignature(ID3D12GraphicsCommandList* commandList);

	BufferView m_verticesBufferView;
	BufferView m_indicesBufferView;

	UINT m_CBVSRVDescriptorSize = 0;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_CBVSRVDescriptorHeap;

	FrameConstants m_frameConstants;
	std::unique_ptr<UploadBuffer<FrameConstants>> m_frameConstantsBuffer;
	GridConstants m_gridConstants;
	std::unique_ptr<UploadBuffer<GridConstants>> m_gridConstantsBuffer;
};