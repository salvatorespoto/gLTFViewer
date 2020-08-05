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
	UploadBuffer(ID3D12Device* device, UINT count);

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
	virtual void createBuffer(ID3D12Device* device, UINT count);

	Microsoft::WRL::ComPtr<ID3D12Resource> m_buffer;	/*< Com pointer to the buffer */
	std::size_t m_elementByteSize;						/*< The byte size of an element in the buffer */
	BYTE* m_data = nullptr;								/*< CPU pointer to mapped data */
};


/**
 * This is an helper class to work with constant buffers
 */
template <typename T>
class ConstantBuffer : public UploadBuffer<T>
{
public:
	ConstantBuffer(ID3D12Device* device, UINT count);

protected:
	virtual void createBuffer(ID3D12Device* device, UINT count);
};