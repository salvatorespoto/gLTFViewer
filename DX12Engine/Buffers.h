#pragma once

#include "DXUtil.h"

constexpr uint8_t BUFFER_ELEM_VEC2 = 2;
constexpr uint8_t BUFFER_ELEM_VEC3 = 3;
constexpr uint8_t BUFFER_ELEM_VEC4 = 4;
constexpr uint8_t BUFFER_ELEM_TYPE_UNSIGNED_CHAR = 1;
constexpr uint8_t BUFFER_ELEM_TYPE_UNSIGNED_SHORT = 2;
constexpr uint8_t BUFFER_ELEM_TYPE_UNSIGNED_INT = 3;
constexpr uint8_t BUFFER_ELEM_TYPE_FLOAT = 4;

/** BufferView is used to address resources in larger memory buffers */
struct BufferView
{
	int8_t bufferId = -1;
	size_t byteOffset = 0;
	size_t byteLength = 0;
	size_t byteStride = 0;
	size_t count = 0;
	uint8_t elemType = BUFFER_ELEM_VEC3;
	uint8_t componentType = BUFFER_ELEM_TYPE_FLOAT;
};

/** Upload data to a default heap buffer and return a com pointer to it */
Microsoft::WRL::ComPtr<ID3D12Resource> createDefaultHeapBuffer(
	ID3D12Device* device,
	ID3D12GraphicsCommandList* cmdList,
	const void* initData,
	UINT64 byteSize,
	Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);

/** Helper class to upload data to GPU DEFAULT HEAP synchronously */
class GPUHeapUploader
{
public:
	GPUHeapUploader(Microsoft::WRL::ComPtr<ID3D12Device> device, Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue);
	Microsoft::WRL::ComPtr<ID3D12Resource> Upload(const void* initData, UINT64 byteSize);

private:
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandListAlloc;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
	Microsoft::WRL::ComPtr<ID3D12Device> m_device;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
	UINT m_currentFenceValue = 0;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_bufferUpload = nullptr;

	void FlushCommandQueue();
};

/** Helper class to upload data to the GPU UPLOAD HEAP */
template <class T>
class UploadBuffer
{
public:
	UploadBuffer(ID3D12Device* device, UINT count, bool isConstant);
	UploadBuffer(const UploadBuffer&) = delete;
	UploadBuffer& operator=(const UploadBuffer&) = delete;
	~UploadBuffer();

	ID3D12Resource* getResource() const;
	void release();
	void copyData(int index, const T& data);

protected:
	virtual void createBuffer(ID3D12Device* device, UINT count, bool isConstant);

	Microsoft::WRL::ComPtr<ID3D12Resource> m_buffer;	/*< Com pointer to the buffer */
	std::size_t m_elementByteSize;						/*< The byte size of an element in the buffer */
	BYTE* m_data = nullptr;								/*< CPU pointer to mapped data */
};

template <class T>
UploadBuffer<T>::UploadBuffer(ID3D12Device* device, UINT count, bool isConstant)
{
	createBuffer(device, count, isConstant);
}

template <class T>
UploadBuffer<T>::~UploadBuffer()
{
	if (m_buffer != nullptr)
	{
		m_buffer->Unmap(0, nullptr);
	}
}

template <class T>
void UploadBuffer<T>::release()
{
	if (m_buffer != nullptr)
	{
		m_buffer->Unmap(0, nullptr);
		m_buffer = nullptr;
	}
}

template <class T>
void UploadBuffer<T>::createBuffer(ID3D12Device* device, UINT count, bool isConstant)
{
	if (isConstant)
	{
		// Hardware can only access cosntant data at offset of 256 bytes
		// so pad the size of the element T to a 255 byte multiple
		this->m_elementByteSize = (sizeof(T) + 255) & ~255;
	}
	else m_elementByteSize = sizeof(T);

	DXUtil::ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(m_elementByteSize * count),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,													
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