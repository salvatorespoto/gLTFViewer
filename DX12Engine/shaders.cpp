#include "shaders.h"


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