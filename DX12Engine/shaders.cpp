#include "shaders.h"
#include <Pathcch.h>
#include <atlstr.h>

#include "using_directives.h"

bool CompileShaders(std::wstring fileName, std::string& errorMsg, Microsoft::WRL::ComPtr<ID3DBlob>& vertexShader, Microsoft::WRL::ComPtr<ID3DBlob>& pixelShader)
{

#if defined(_DEBUG)
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compileFlags = 0;
#endif

    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
    if (FAILED(D3DCompileFromFile(fileName.c_str(), nullptr, nullptr, "VSMain", "vs_5_1", compileFlags, 0, &vertexShader, &errorBlob)))
    {
        errorMsg = DXUtil::GetErrorBlobMsg(errorBlob);
        DEBUG_LOG(errorMsg.c_str())
            return false;
    }

    if (FAILED(D3DCompileFromFile(fileName.c_str(), nullptr, nullptr, "PSMain", "ps_5_1", compileFlags, 0, &pixelShader, &errorBlob)))
    {
        errorMsg = DXUtil::GetErrorBlobMsg(errorBlob);
        DEBUG_LOG(errorMsg.c_str())
            return false;
    }

    DEBUG_LOG("Compiled shaders")
        return true;
}

bool CompileVertexShader(std::wstring vsFileName, std::string& errorMsg, ComPtr<ID3DBlob>& vertexShader)
{
#if defined(_DEBUG)
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compileFlags = 0;
#endif

    TCHAR filePath[200];
    _tcscpy_s(filePath, CA2T(DXUtil::transform_to<string>(vsFileName).c_str()));
    PathCchRemoveFileSpec(filePath, 200);

    TCHAR currentPath[200];
    GetCurrentDirectory(200, currentPath);
    SetCurrentDirectory(filePath);

    ComPtr<ID3DBlob> errorBlob;
    if (FAILED(D3DCompileFromFile(vsFileName.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_1", compileFlags, 0, &vertexShader, &errorBlob)))
    {
        errorMsg = DXUtil::GetErrorBlobMsg(errorBlob);
        DEBUG_LOG(errorMsg.c_str())
            return false;
    }

    SetCurrentDirectory(currentPath);

    DEBUG_LOG("Compiled vertex shader")
    return true;
}

bool CompileGeometryShader(std::wstring vsFileName, std::string& errorMsg, ComPtr<ID3DBlob>& geometryShader)
{
    return true;
}

bool CompilePixelShader(std::wstring psFileName, std::string& errorMsg, ComPtr<ID3DBlob>& pixelShader)
{
#if defined(_DEBUG)
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compileFlags = 0;
#endif
    
    TCHAR filePath[200];
    _tcscpy_s(filePath, CA2T(DXUtil::transform_to<string>(psFileName).c_str()));
    PathCchRemoveFileSpec(filePath, 200);

    TCHAR currentPath[200];
    GetCurrentDirectory(200, currentPath);
    SetCurrentDirectory(filePath);

    ComPtr<ID3DBlob> errorBlob;
    if (FAILED(D3DCompileFromFile(psFileName.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_1", compileFlags, 0, &pixelShader, &errorBlob)))
    {
        errorMsg = DXUtil::GetErrorBlobMsg(errorBlob);
        DEBUG_LOG(errorMsg.c_str())
            return false;
    }
    SetCurrentDirectory(currentPath);

    DEBUG_LOG("Compiled pixel shader")
    return true;
}