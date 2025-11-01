
/** $VER: Tester.cpp (2025.10.21) P. Stuer - Implements a minimal visualization for testing purposes. **/

#include <pch.h>

#include "Tester.h"

#include "Support.h"
#include "Log.h"

#include "DirectWrite.h"

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
tester_t::tester_t()
{
    _Rect = { };
    _Size = { };

    Reset();
}

/// <summary>
/// Destroys this instance.
/// </summary>
tester_t::~tester_t()
{
    DeleteDeviceSpecificResources();
}

/// <summary>
/// Initializes this instance.
/// </summary>
void tester_t::Initialize(state_t * state, const graph_description_t * settings, const analysis_t * analysis) noexcept
{
    _State = state;
    _GraphDescription = settings;
    _Analysis = analysis;

    CreateDeviceIndependentResources();
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void tester_t::Move(const D2D1_RECT_F & rect) noexcept
{
    SetRect(rect);
}

/// <summary>
/// Resets this instance.
/// </summary>
void tester_t::Reset() noexcept
{
    if (!_IsResized || (GetWidth() == 0.f) || (GetHeight() == 0.f))
        return;

    _IsResized = true;
}

/// <summary>
/// Terminates this instance.
/// </summary>
void tester_t::Release() noexcept
{
    DeleteDeviceSpecificResources();
}

/// <summary>
/// Recalculates parameters that are render target and size-sensitive.
/// </summary>
void tester_t::Resize() noexcept
{
    if (!_IsResized || (GetWidth() == 0.f) || (GetHeight() == 0.f))
        return;

    _p1 = D2D1::Point2F();
    _p2 = D2D1::Point2F(_Size.width, _Size.height);
    _d = 2.f;

    _IsResized = false;
}

/// <summary>
/// Renders this instance.
/// </summary>
void tester_t::Render(ID2D1DeviceContext * deviceContext) noexcept
{
    HRESULT hr = CreateDeviceSpecificResources(deviceContext);

    if (!SUCCEEDED(hr))
        return;

    deviceContext->DrawLine(_p1, _p2, _DebugBrush);

    _p1.x += _d;
    _p2.x -= _d;

    if (_p1.x < 0.f || _p1.x > _Size.width)
        _d = -_d;
}

/// <summary>
/// Creates resources which are not bound to any D3D device. Their lifetime effectively extends for the duration of the app.
/// </summary>
HRESULT tester_t::CreateDeviceIndependentResources() noexcept
{
    HRESULT hr = S_OK;

    return hr;
}

/// <summary>
/// Releases the device independent resources.
/// </summary>
void tester_t::DeleteDeviceIndependentResources() noexcept
{
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT tester_t::CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept
{
    if ((_Size.width == 0.f) || _Size.height == 0.f)
        return E_FAIL;

    Resize();

    HRESULT hr = S_OK;

#ifdef _DEBUG
    if (SUCCEEDED(hr) && (_DebugBrush == nullptr))
        hr = deviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &_DebugBrush);
#endif

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void tester_t::DeleteDeviceSpecificResources() noexcept
{
#ifdef _DEBUG
    _DebugBrush.Release();
#endif
}
