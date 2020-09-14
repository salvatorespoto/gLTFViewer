#pragma once

#include "DXUtil.h"

bool CompileShaders(std::wstring fileName, std::string& errorMsg, Microsoft::WRL::ComPtr<ID3DBlob>& vertexShader, Microsoft::WRL::ComPtr<ID3DBlob>& pixelShader);

bool CompileVertexShader(std::wstring vsFileName, std::string& errorMsg, Microsoft::WRL::ComPtr<ID3DBlob>& vertexShader);
bool CompileGeometryShader(std::wstring vsFileName, std::string& errorMsg, Microsoft::WRL::ComPtr<ID3DBlob>& geometryShader);
bool CompilePixelShader(std::wstring psFileName, std::string& errorMsg, Microsoft::WRL::ComPtr<ID3DBlob>& pixelShader);