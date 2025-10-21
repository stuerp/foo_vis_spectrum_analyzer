
/** $VER: DirectX.cpp (2025.10.21) P. Stuer **/

#include "pch.h"

#include "DirectX.h"
#include "Direct2D.h"
#include "DirectWrite.h"
#include "WIC.h"

namespace DirectX
{
int32_t _Count;

/// <summary>
/// Initializes DirectX once per component.
/// </summary>
HRESULT Initialize() noexcept
{
    ++_Count;

    if (_Count > 1)
        return S_OK;

    HRESULT hr = _Direct2D.Initialize();

    if (SUCCEEDED(hr))
        hr = _DirectWrite.Initialize();

    if (SUCCEEDED(hr))
        hr = _WIC.Initialize();

    return hr;
}

/// <summary>
/// Terminates DirectX once per component.
/// </summary>
void Terminate() noexcept
{
    --_Count;

    if (_Count > 0)
        return;

    _WIC.Terminate();

    _DirectWrite.Terminate();

    _Direct2D.Terminate();
}

}
