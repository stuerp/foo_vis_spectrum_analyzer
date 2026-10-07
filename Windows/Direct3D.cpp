
/** $VER: Direct3D.cpp (2026.10.07) P. Stuer **/

#include "pch.h"

#include "Direct3D.h"

#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

/// <summary>
/// Compiles the specified shader source code.
/// </summary>
HRESULT Direct3D::CompileShader(const char * sourceCode, const char * sourceName, const char * entryPoint, const char * shaderModel, ID3DBlob ** shader, ID3DBlob ** errorMessages) noexcept
{
    if ((shader == nullptr) || (errorMessages == nullptr))
        return E_POINTER;

    *shader = nullptr;

    UINT Flags = D3DCOMPILE_ENABLE_STRICTNESS;

#if defined(_DEBUG)
    Flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    Flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

    return ::D3DCompile(sourceCode, ::strlen(sourceCode), sourceName, nullptr, nullptr, entryPoint, shaderModel, Flags, 0, shader, errorMessages);
}
