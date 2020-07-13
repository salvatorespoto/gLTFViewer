#pragma once

#include <Windows.h>
#include <windowsx.h>           // Windows helper functions
#include <wrl.h>                // ComPtr smart pointers for COM interfaces

#include <tchar.h>
#include <string>
#include <sstream>
#include <vector>
#include <exception>


// DirectX includes

#include <d3d12.h>				// Direct3D 12 objects			
#include <dxgi1_6.h>			// Microsoft DirectX Graphics Infrastructure (DXGI)
#include <d3dcompiler.h>		// Direct3D 12 objects
#include <DirectXmath.h>		// Math class and functions for Direct3D
#include <directxcollision.h>   // Collision detection functions
#include "d3dx12.h"				// Helper structures and functions for Direct3D 12


// Statically link DirectX libraries

#pragma comment(lib, "D3d12.lib")
#pragma comment(lib, "dxgi.lib")       
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")


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


// Get device capacities 

    /** Enumerate all available adapters */
    std::vector<Microsoft::WRL::ComPtr<IDXGIAdapter1>> enumerateAdapters();

    /** Enumerate all output devices connected to a given adapter */
    std::vector<Microsoft::WRL::ComPtr<IDXGIOutput>>  enumerateAdaptersOutputs(IDXGIAdapter* adapter);

    /** Enumerate all the display modes supported from an output device */
    std::vector<DXGI_MODE_DESC> enumerateAdapterOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

    /** Print out adapter info */
    void printAdaptersInfo();
}