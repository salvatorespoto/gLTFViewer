#pragma once

#include "DXUtil.h"

/** 
 * Upload data to a default heap buffer and return a pointer to it 
 */
Microsoft::WRL::ComPtr<ID3D12Resource> createDefaultHeapBuffer(
	ID3D12Device* device,
	ID3D12GraphicsCommandList* cmdList,
	const void* initData,
	UINT64 byteSize,
	Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);

class GPUHeapUploader
{
public:
	GPUHeapUploader(Microsoft::WRL::ComPtr<ID3D12Device> device, Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue)
	{
		m_device = device;
		m_commandQueue = commandQueue;

		DXUtil::ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandListAlloc)), "Cannot create command allocator");
		DXUtil::ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandListAlloc.Get(), nullptr, IID_PPV_ARGS(&m_commandList)), "Cannot the command list");
		m_commandList->Close();
		DXUtil::ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)), "Cannot create fence");
	}

	Microsoft::WRL::ComPtr<ID3D12Resource> Upload(const void* initData, UINT64 byteSize)
	{	
		DXUtil::ThrowIfFailed(m_commandListAlloc->Reset(), "Cannot reset allocator");
		DXUtil::ThrowIfFailed(m_commandList->Reset(m_commandListAlloc.Get(), nullptr), "Cannot reset command list");
		Microsoft::WRL::ComPtr<ID3D12Resource> bufferGPU = createDefaultHeapBuffer(m_device.Get(), m_commandList.Get(), initData, byteSize, m_bufferUpload);
		m_commandList->Close();

		ID3D12CommandList* commandLists[] = { m_commandList.Get() };
		m_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
		FlushCommandQueue();
		return bufferGPU;
	}

private:
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandListAlloc;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList;
	Microsoft::WRL::ComPtr<ID3D12Device> m_device;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_commandQueue;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
	UINT m_currentFenceValue = 0;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_bufferUpload = nullptr;

	void FlushCommandQueue()
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
};


/**
 * This is an helper class to work with upload buffers
 */
template <class T>
class UploadBuffer
{
public:

	/**
	 * Constructor
	 * Build an upload buffer of "count" elements on the device
	 */
	UploadBuffer(ID3D12Device* device, UINT count, bool isConstant);

	UploadBuffer(const UploadBuffer&) = delete;

	UploadBuffer& operator=(const UploadBuffer&) = delete;

	/** Destructor */
	~UploadBuffer();

	/**
	 * Get a pointer to the buffer D3D12Resource
	 */
	ID3D12Resource* getResource() const;

	void release();


	/**
	 * Copy data to the buffer
	 */
	void copyData(int index, const T& data);


protected:

	/** Create the buffer */
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


/* *
 * This is an helper class to work with constant buffers
 * /
template <typename T>
class ConstantBuffer : public UploadBuffer<T>
{
public:
	ConstantBuffer(ID3D12Device* device, UINT count);

protected:
	virtual void createBuffer(ID3D12Device* device, UINT count);
};
*/