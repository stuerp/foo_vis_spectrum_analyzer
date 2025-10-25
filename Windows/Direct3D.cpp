
/** $VER: Direct3D.cpp (2025.10.25) P. Stuer **/

#include "pch.h"

#include "Direct3D.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

HRESULT CompileShader(const char * shaderSource, const char * entryPoint, const char * shaderModel, ID3DBlob ** shader) noexcept
{
    ID3DBlob * ErrorMessages = nullptr;

    HRESULT hr = ::D3DCompile(shaderSource, strlen(shaderSource), nullptr, nullptr, nullptr, entryPoint, shaderModel, 0, 0, shader, &ErrorMessages);

    if (FAILED(hr))
    {
        if (ErrorMessages)
        {
            ::OutputDebugStringA(static_cast<char *>(ErrorMessages->GetBufferPointer()));

            ErrorMessages->Release();
        }
    }

    return hr;
}
