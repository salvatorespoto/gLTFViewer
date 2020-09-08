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
//#pragma comment(lib, "shlwapi.lib")

//#pragma comment(lib, "runtimeobject .lib")


// DirectX 12 debug layer

#define ENABLE_DX12_DEBUG_LAYER
#ifdef ENABLE_DX12_DEBUG_LAYER
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif

#include "glTF/tiny_gltf.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx12.h"
#include "imgui/imfilebrowser.h"


// Define virtual key codes
#define VK_KEY_W 0x57
#define VK_KEY_A 0x41
#define VK_KEY_S 0x53
#define VK_KEY_D 0x44

namespace DXUtil
{
    extern ImGuiTextBuffer gLogs;

    /* Transform any std string or string view into any of the 4 the std string types */
    template<typename T, typename F> inline T
    transform_to(F str) noexcept
    {
        if (str.empty()) return {};
        return { std::begin(str), std::end(str) };
    };

    /** Log debug message to the Visual Studio output window */
    #define DEBUG_LOG(msg) { std::wstringstream wss; wss << msg << std::endl; \
        OutputDebugString(wss.str().c_str()); DXUtil::gLogs.append(DXUtil::transform_to<std::string>(wss.str()).c_str()); }
    
    /** Throw exception on failure with a given message */
    inline void ThrowIfFailed(HRESULT hr, std::string errorMsg)
    {
        if (FAILED(hr))
        {
            errorMsg = "Error:" + errorMsg + "[file: " + __FILE__ + " line: " + std::to_string(__LINE__) + " ]";
                throw std::exception(errorMsg.c_str());
        }
    }

    /** Output the message in an ID3DBlob error blog object */
    inline std::string GetErrorBlobMsg(Microsoft::WRL::ComPtr<ID3DBlob> errorBlob)
    {
        if (errorBlob == nullptr) return "";
        std::string errorMsg;
        errorMsg.assign((char*)errorBlob->GetBufferPointer());
        return errorMsg;
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
    UINT PadByteSizeTo256Mul(UINT byteSize);

    /* Store an indentity matmrix in a XMFLOAT4X4 */
    DirectX::XMFLOAT4X4 IdentityMtx();

    /* Store an indentity matmrix in a XMFLOAT4X4 */
    DirectX::XMFLOAT4X4 RightToLeftHandedMtx();
}