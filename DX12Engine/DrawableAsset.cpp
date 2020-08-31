#include "DrawableAsset.h"

void DrawableAsset::AddGPUBuffer(Microsoft::WRL::ComPtr<ID3D12Resource> buffer)
{
	m_buffersGPU.push_back(buffer);
}