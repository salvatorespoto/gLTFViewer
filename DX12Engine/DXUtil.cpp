#include "DXUtil.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "glTF/tiny_gltf.h"

using Microsoft::WRL::ComPtr;	// Using Com smart pointers

namespace DXUtil
{
	ImGuiTextBuffer gLogs;

	std::vector<ComPtr<IDXGIAdapter1>> enumerateAdapters()
	{
		UINT i = 0;
		UINT createFactoryFlags = 0;				// = DXGI_CREATE_FACTORY_DEBUG can be used here
		ComPtr<IDXGIFactory6> dxgiFactory;			// Com smart pointer to IDXGIFactory5
		ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)), "Cannot create DXGIFactory");

		ComPtr<IDXGIAdapter1> adapter;
		std::vector<ComPtr<IDXGIAdapter1>> adapterList;
		while (dxgiFactory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND)		// Get the i-th adapter
		{
			adapterList.push_back(adapter);
			++i;
		}

		return adapterList;
	}

	std::vector<ComPtr<IDXGIOutput>>  enumerateAdaptersOutputs(IDXGIAdapter* adapter)
	{
		UINT i = 0;
		ComPtr <IDXGIOutput> output = nullptr;
		std::vector<ComPtr<IDXGIOutput>> outputList;
		while (adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
		{
			outputList.push_back(output);
			++i;
		}
		return outputList;
	}

	std::vector<DXGI_MODE_DESC> enumerateAdapterOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
	{
		UINT count = 0;
		UINT flags = 0;		// This flag specifies modes to include in the list, 0 = 

		output->GetDisplayModeList(format, flags, &count, nullptr);	// Get display modes count
		std::vector<DXGI_MODE_DESC> modeList(count);
		output->GetDisplayModeList(format, flags, &count, &modeList[0]); // Get display modes list

		return modeList;
	}

	void printAdaptersInfo()
	{
		// Iterate over ADAPTERS and print info
		for (ComPtr<IDXGIAdapter1> adapter : enumerateAdapters())
		{
			DXGI_ADAPTER_DESC aDesc;	// Struct that holds the adapter descrdiption
			adapter->GetDesc(&aDesc);	// Get the adapter description
			DEBUG_LOG("--- Adapter: " << aDesc.Description)

				// Iterate over ADAPTER OUTPUTS and print info
				for (ComPtr<IDXGIOutput> output : enumerateAdaptersOutputs(adapter.Get()))
				{
					DXGI_OUTPUT_DESC oDesc;		// Struct that holds the output descrdiption
					output->GetDesc(&oDesc);	// Get the output descrdiption
					DEBUG_LOG("  Output: " << " Device name " << oDesc.DeviceName)
						// Iterate over ADAPTER OUTPUT DISPLAY MODES and print info
						for (DXGI_MODE_DESC displayMode : enumerateAdapterOutputDisplayModes(output.Get(), DXGI_FORMAT_B8G8R8A8_UNORM))
						{
							DEBUG_LOG("    Display mode: " << " Width " << displayMode.Width << " Height " << displayMode.Height)
						}
				}
		}
	}


	UINT PadByteSizeTo256Mul(UINT byteSize)
	{
		return (byteSize + 255) & ~255;
	}

	DirectX::XMFLOAT4X4 IdentityMtx()
	{
		static DirectX::XMFLOAT4X4 I(
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f);

		return I;
	}

	DirectX::XMFLOAT4X4 RightToLeftHandedMtx()
	{
		static DirectX::XMFLOAT4X4 I(
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, -1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f);

		return I;
	}
}