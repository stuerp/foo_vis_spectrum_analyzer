
/** $VER: Direct3D.h (2025.10.25) P. Stuer **/

#pragma once

#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

HRESULT CompileShader(const char * shaderSource, const char * entryPoint, const char * shaderModel, ID3DBlob ** shader) noexcept;
