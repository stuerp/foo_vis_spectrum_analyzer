
/** $VER: CorrelationMeter.cpp (2024.04.22) P. Stuer - Represents a correlation meter. **/

#include "framework.h"

#include "CorrelationMeter.h"

#include "Support.h"
#include "Log.h"

#include "DirectWrite.h"

#pragma hdrstop

CorrelationMeter::CorrelationMeter()
{
    _Bounds = { };
    _Size = { };

    Reset();
}

/// <summary>
/// Initializes this instance.
/// </summary>
void CorrelationMeter::Initialize(State * state, const GraphSettings * settings, const Analysis * analysis)
{
//  _Gauges.Initialize(state, settings, analysis);

    _State = state;
    _GraphSettings = settings;
    _Analysis = analysis;

    ReleaseDeviceSpecificResources();
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void CorrelationMeter::Move(const D2D1_RECT_F & rect)
{
    SetBounds(rect);

//  _Gauges.Move(rect);
}

/// <summary>
/// Resets this instance.
/// </summary>
void CorrelationMeter::Reset()
{
    _IsResized = true;

//  _Gauges.Reset();
}

/// <summary>
/// Recalculates parameters that are render target and size-sensitive.
/// </summary>
void CorrelationMeter::Resize() noexcept
{
    if (!_IsResized || (GetWidth() == 0.f) || (GetHeight() == 0.f))
        return;

    D2D1_RECT_F Rect = _Bounds;

//  _Gauges.SetBounds(Rect);
//  _Gauges.Resize();

    _IsResized = false;
}

/// <summary>
/// Renders this instance.
/// </summary>
void CorrelationMeter::Render(ID2D1RenderTarget * renderTarget)
{
    HRESULT hr = CreateDeviceSpecificResources(renderTarget);

    if (!SUCCEEDED(hr))
        return;

    // Render the gauges.
    {
//      renderTarget->SetTransform(_GaugesTransform);

//      _Gauges.Render(renderTarget, _GaugeMetrics);
    }

    FLOAT x = (GetWidth() / 2.f) + ((FLOAT) _Analysis->_Balance * GetWidth());

    renderTarget->DrawLine({ x, 0.f }, { x, GetHeight() / 2.f }, _DebugBrush, 4.f);

    x = (GetWidth() / 2.f) + ((FLOAT) _Analysis->_Phase * GetWidth());

    renderTarget->DrawLine({ x, GetHeight() / 2.f }, { x, GetHeight() }, _DebugBrush, 4.f);

    ResetTransform(renderTarget);
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT CorrelationMeter::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept
{
    HRESULT hr = S_OK;

//    if (SUCCEEDED(hr))
//        hr = _Gauges.CreateDeviceSpecificResources(renderTarget);

#ifdef _DEBUG
    if (SUCCEEDED(hr) && (_DebugBrush == nullptr))
        renderTarget->CreateSolidColorBrush(D2D1::ColorF(1.f,0.f,0.f), &_DebugBrush);
#endif

    if (SUCCEEDED(hr))
        Resize();

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void CorrelationMeter::ReleaseDeviceSpecificResources() noexcept
{
//    _Gauges.ReleaseDeviceSpecificResources();
    _DebugBrush.Release();
}
