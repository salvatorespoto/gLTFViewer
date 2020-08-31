#pragma once

#include "DXUtil.h"

bool CompileShaders(std::wstring fileName, std::string& errorMsg, Microsoft::WRL::ComPtr<ID3DBlob>& vertexShader, Microsoft::WRL::ComPtr<ID3DBlob>& pixelShader);
