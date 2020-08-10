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
	if (m_buffer != nullptr) m_buffer->Unmap(0, nullptr);
	m_data = nullptr;
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