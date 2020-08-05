#include "Buffers.h"

using Microsoft::WRL::ComPtr;

ComPtr<ID3D12Resource> createDefaultHeapBuffer(
	ID3D12Device* device,
	ID3D12GraphicsCommandList* cmdList,
	const void* initData,
	UINT64 byteSize,
	ComPtr<ID3D12Resource>& uploadBuffer)
{
	// Create the default heap buffer
	ComPtr<ID3D12Resource> defaultHeapBuffer;
	DXUtil::ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),           // A description of the resource, here only specify the bytesize 
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(defaultHeapBuffer.GetAddressOf())), "Cannot create default buffer");

	// Create the upload buffer
	DXUtil::ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),   // Upload type
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),           // A description of the resource, here only specify the bytesize 
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

template <class T>
UploadBuffer<T>::UploadBuffer(ID3D12Device* device, UINT count)
{
	createBuffer(device, count);
}

template <class T>
UploadBuffer<T>::~UploadBuffer()
{
	if (m_buffer != nullptr) m_buffer->Unmap(0, nullptr);
	m_data = nullptr;
}

template <class T>
void UploadBuffer<T>::createBuffer(ID3D12Device* device, UINT count)
{
	m_elementByteSize = sizeof(T);

	DXUtil::ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(m_elementByteSize * count),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,													// Clear value
		IID_PPV_ARGS(&m_buffer)), "Error creating upload buffer");

	DXUtil::ThrowIfFailed(m_buffer->Map(0, nullptr, reinterpret_cast<void**>(&m_data)), "Error creating upload buffer");
}

template <class T>
ID3D12Resource* UploadBuffer<T>::getResource() const
{
	return m_buffer.Get();
}

template <class T>
void UploadBuffer<T>::copyData(int index, const T& data)
{
	memcpy(&m_data[index * m_elementByteSize], &data, sizeof(T));
}

template <class T>
ConstantBuffer<T>::ConstantBuffer(ID3D12Device* device, UINT count) : UploadBuffer<T>(device, count) {}

template <class T>
void ConstantBuffer<T>::createBuffer(ID3D12Device* device, UINT count)
{
	// Hardware can only access cosntant data at offset of 256 bytes
	// so pad the size of the element T to a 255 byte multiple
	this->m_elementByteSize = (sizeof(T) + 255) & ~255;

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(this->m_elementByteSize * count),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,													// Clear value
		IID_PPV_ARGS(&this->m_buffer)), "Error creating upload buffer");

	ThrowIfFailed(this->m_buffer->Map(0, nullptr, reinterpret_cast<void**>(&this->m_data)), "Error creating upload buffer");
}