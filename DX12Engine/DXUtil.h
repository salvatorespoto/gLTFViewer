#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <Windows.h>
#include <windowsx.h>           // Windows helper functions
#include <wrl.h>                // ComPtr smart pointers for COM interfaces

#include <tchar.h>
#include <string>
#include <sstream>
#include <vector>
#include <exception>

#include <d3d12.h>				// Direct3D 12 objects			
#include <dxgi1_6.h>			// Microsoft DirectX Graphics Infrastructure (DXGI)
#include <DirectXColors.h>
#include <d3dcompiler.h>		// Direct3D 12 objects
#include <DirectXmath.h>		// Math class and functions for Direct3D
#include <directxcollision.h>   // Collision detection functions
#include "d3dx12.h"				// Helper structures and functions for Direct3D 12

// Statically link DirectX libraries

#pragma comment(lib, "D3d12.lib")
#pragma comment(lib, "dxgi.lib")       
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")
//#pragma comment(lib, "runtimeobject .lib")


// DirectX 12 debug layer

#define ENABLE_DX12_DEBUG_LAYER
#ifdef ENABLE_DX12_DEBUG_LAYER
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

#include "glTF/tiny_gltf.h"


// Define virtual key codes
#define VK_KEY_W 0x57
#define VK_KEY_A 0x41
#define VK_KEY_S 0x53
#define VK_KEY_D 0x44

namespace DXUtil
{

    // Error and exception handling 

    /** Log debug message to the Visual Studio output window */
    #define DEBUG_LOG(msg) { std::wstringstream wss; wss << msg << std::endl; OutputDebugString(wss.str().c_str()); }

    /** Throw exception on failure with a given message */
    inline void ThrowIfFailed(HRESULT hr, std::string errorMsg)
    {
        if (FAILED(hr))
        {
            errorMsg = "Error:" + errorMsg + "[file: " + __FILE__ + " line: " + std::to_string(__LINE__) + " ]";
                throw std::exception(errorMsg.c_str());
        }
    }

    /** Throw exception on failure with a given message */
    inline void ThrowException(std::string errorMsg)
    {
        errorMsg = "Error:" + errorMsg + "[file: " + __FILE__ + " line: " + std::to_string(__LINE__) + " ]";
        throw std::exception(errorMsg.c_str());
    }

    /** Enumerate all available adapters */
    std::vector<Microsoft::WRL::ComPtr<IDXGIAdapter1>> enumerateAdapters();

    /** Enumerate all output devices connected to a given adapter */
    std::vector<Microsoft::WRL::ComPtr<IDXGIOutput>>  enumerateAdaptersOutputs(IDXGIAdapter* adapter);

    /** Enumerate all the display modes supported from an output device */
    std::vector<DXGI_MODE_DESC> enumerateAdapterOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

    /** Print out adapter info */
    void printAdaptersInfo();

    /** Compute the byte size of a buffer after padding it to a 256 byte multiple */
    UINT padByteSizeTo256Mul(UINT byteSize);

    /* Store an indentity matmrix in a XMFLOAT4X4 */
    DirectX::XMFLOAT4X4 IdentityMtx();
}