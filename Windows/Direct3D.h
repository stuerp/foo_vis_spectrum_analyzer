
/** $VER: Direct3D.h (2026.10.07) P. Stuer **/

#pragma once

#include <d3d11.h>

class Direct3D
{
public:
    static HRESULT CompileShader(const char * sourceCode, const char * sourceName, const char * entryPoint, const char * shaderModel, ID3DBlob ** shader, ID3DBlob ** errorMessages) noexcept;

private:
    Direct3D() = default;
};
