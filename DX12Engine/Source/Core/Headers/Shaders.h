// SPDX-FileCopyrightText: 2022 Salvatore Spoto <salvatore.spoto@gmail.com> 
// SPDX-License-Identifier: MIT

#pragma once

#include "DXUtil.h"

bool CompileShaders(const std::wstring fileName, std::string& errorMsg, Microsoft::WRL::ComPtr<ID3DBlob>& vertexShader, Microsoft::WRL::ComPtr<ID3DBlob>& pixelShader);

bool CompileVertexShader(const std::wstring vsFileName, std::string& errorMsg, Microsoft::WRL::ComPtr<ID3DBlob>& vertexShader);
bool CompileGeometryShader(const std::wstring vsFileName, std::string& errorMsg, Microsoft::WRL::ComPtr<ID3DBlob>& geometryShader);
bool CompilePixelShader(const std::wstring psFileName, std::string& errorMsg, Microsoft::WRL::ComPtr<ID3DBlob>& pixelShader);