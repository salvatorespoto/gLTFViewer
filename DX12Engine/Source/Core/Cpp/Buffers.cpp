#include "Buffers.h"

using Microsoft::WRL::ComPtr;
using DXUtil::ThrowIfFailed;

ComPtr<ID3D12Resource> createDefaultHeapBuffer(
	ID3D12Device* device,
	ID3D12GraphicsCommandList* cmdList,
	const void* initData,
	const UINT64 byteSize,
	ComPtr<ID3D12Resource>& uploadBuffer)
{
	// Create the default heap buffer
	ComPtr<ID3D12Resource> defaultHeapBuffer;
	DXUtil::ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),	// Default heap type
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),           
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(defaultHeapBuffer.GetAddressOf())), "Cannot create default buffer");

	// Create the upload buffer
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),   // Upload heap type
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),           
		D3D12_RESOURCE_STATE_GENERIC_READ,                  // State required to start an upload heap
		nullptr,
		IID_PPV_ARGS(uploadBuffer.GetAddressOf())), "Cannot create upload buffer");

	// Indicate what data have to be copied to the upload buffer
	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = initData;
	subResourceData.RowPitch = byteSize;
	subResourceData.SlicePitch = subResourceData.RowPitch;

	// Add the command to the command list to set the transition for the default heap resource from common to a copy destination
	cmdList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(defaultHeapBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_COPY_DEST));

	// Move resource from upload buffer to default heap buffer
	UpdateSubresources<1>(cmdList, defaultHeapBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);

	// Add the command to set the transition for the default heap resource from copy dest to a read 
	cmdList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(defaultHeapBuffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_GENERIC_READ));

	return defaultHeapBuffer;
}

GPUHeapUploader::GPUHeapUploader(ComPtr<ID3D12Device> device, ComPtr<ID3D12CommandQueue> commandQueue)
{
	m_device = device;
	m_commandQueue = commandQueue;

	ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandListAlloc)), "Cannot create command allocator");
	ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandListAlloc.Get(), nullptr, IID_PPV_ARGS(&m_commandList)), "Cannot the command list");
	m_commandList->Close();
	ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)), "Cannot create fence");
}

ComPtr<ID3D12Resource> GPUHeapUploader::Upload(const void* initData, const UINT64 byteSize)
{
	ThrowIfFailed(m_commandListAlloc->Reset(), "Cannot reset allocator");
	ThrowIfFailed(m_commandList->Reset(m_commandListAlloc.Get(), nullptr), "Cannot reset command list");
	ComPtr<ID3D12Resource> bufferGPU = createDefaultHeapBuffer(m_device.Get(), m_commandList.Get(), initData, byteSize, m_bufferUpload);
	m_commandList->Close();

	ID3D12CommandList* commandLists[] = { m_commandList.Get() };
	m_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
	FlushCommandQueue();
	return bufferGPU;
}

void GPUHeapUploader::FlushCommandQueue()
{
	m_currentFenceValue++;
	m_commandQueue->Signal(m_fence.Get(), m_currentFenceValue);

	if (m_fence->GetCompletedValue() < m_currentFenceValue)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
		DXUtil::ThrowIfFailed(m_fence->SetEventOnCompletion(m_currentFenceValue, eventHandle), "Cannot set fence event on completion");
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}
