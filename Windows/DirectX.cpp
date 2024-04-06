
/** $VER: DirectX.cpp (2024.01.30) P. Stuer **/

#include "framework.h"
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
void Initialize()
{
    ++_Count;

    if (_Count > 1)
        return;

    HRESULT hr = _Direct2D.Initialize();

    if (SUCCEEDED(hr))
        hr = _DirectWrite.Initialize();

    if (SUCCEEDED(hr))
        hr = _WIC.Initialize();
}

/// <summary>
/// Terminates DirectX once per component.
/// </summary>
void Terminate()
{
    --_Count;

    if (_Count > 0)
        return;

    _WIC.Terminate();

    _DirectWrite.Terminate();

    _Direct2D.Terminate();
}

}
