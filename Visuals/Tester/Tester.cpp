
/** $VER: Tester.cpp (2026.06.24) P. Stuer - Implements a minimal visualization for testing purposes. **/

#include <pch.h>

#include <cmath>

#include "Tester.h"

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
void tester_t::Initialize(state_t * state, graph_options_t * graphOptions, const analysis_t * analysis, bool isFirst, bool isLast) noexcept
{
    _State = state;
    _GraphOptions = graphOptions;
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

    _Angle = 0.f;

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

    deviceContext->PushAxisAlignedClip(_Rect, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

    const D2D1_MATRIX_3X2_F Translate = D2D1::Matrix3x2F::Translation(_Rect.left + (_Size.width / 2.f), _Rect.top + (_Size.height / 2.f));

    deviceContext->SetTransform(Translate);

    float Sin;
    float Cos;

    ::D2D1SinCos(_Angle, &Sin, &Cos);

    const auto r = std::sqrt(_Size.width * _Size.width / 4.f + _Size.height * _Size.height / 4.f);

    const auto p1 = D2D1::Point2F(Sin * r, Cos * r);
    const auto p2 = D2D1::Point2F(-p1.x, -p1.y);

    deviceContext->DrawLine(p1, p2, _DebugBrush);

    _Angle = msc::Wrap(_Angle - (FLOAT) (M_PI / 180.), 359.f);

    deviceContext->SetTransform(D2D1::Matrix3x2F::Identity());

    deviceContext->PopAxisAlignedClip();
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
    if (_DebugBrush == nullptr)
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
