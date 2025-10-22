
/** $VER: PeakMeter.cpp (2025.09.24) P. Stuer - Represents a peak meter. **/

#include "pch.h"

#include "PeakMeter.h"

#include "Support.h"
#include "Log.h"

#include "DirectWrite.h"

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
peak_meter_t::peak_meter_t()
{
    _Rect = { };
    _Size = { };

    Reset();
}

/// <summary>
/// Destroys this instance.
/// </summary>
peak_meter_t::~peak_meter_t()
{
    DeleteDeviceSpecificResources();
}

/// <summary>
/// Initializes this instance.
/// </summary>
void peak_meter_t::Initialize(state_t * state, const graph_description_t * settings, const analysis_t * analysis) noexcept
{
    _Gauges.Initialize(state, settings, analysis);
    _GaugeScales.Initialize(state, settings, analysis);
    _GaugeNames.Initialize(state, settings, analysis);
    _RMSReadOut.Initialize(state, settings, analysis);
    _PeakReadOut.Initialize(state, settings, analysis);

    _State = state;
    _GraphDescription = settings;
    _Analysis = analysis;

    DeleteDeviceSpecificResources();
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void peak_meter_t::Move(const D2D1_RECT_F & rect) noexcept
{
    SetRect(rect);

    _Gauges.Move(rect);
    _GaugeNames.Move(rect);
    _GaugeScales.Move(rect);
    _RMSReadOut.Move(rect);
    _PeakReadOut.Move(rect);
}

/// <summary>
/// Resets this instance.
/// </summary>
void peak_meter_t::Reset() noexcept
{
    _IsResized = true;

    _Gauges.Reset();
    _GaugeNames.Reset();
    _GaugeScales.Reset();
    _RMSReadOut.Reset();
    _PeakReadOut.Reset();
}

/// <summary>
/// Recalculates parameters that are render target and size-sensitive.
/// </summary>
void peak_meter_t::Resize() noexcept
{
    if (!_IsResized || (GetWidth() == 0.f) || (GetHeight() == 0.f))
        return;

    D2D1_RECT_F Rect = _Rect;

    // Gauge names metrics
    {
        if (_State->_HorizontalPeakMeter)
        {
            if (_GraphDescription->_YAxisLeft)
                Rect.bottom -= (FLOAT) _GaugeScales.GetTextHeight();

            if (_GraphDescription->_YAxisRight)
                Rect.bottom -= (FLOAT) _GaugeScales.GetTextHeight();
        }
        else
        {
            if (_GraphDescription->_YAxisLeft)
                Rect.right  -= (FLOAT) _GaugeScales.GetTextWidth();

            if (_GraphDescription->_YAxisRight)
                Rect.right  -= (FLOAT) _GaugeScales.GetTextWidth();
        }

        _GaugeNames.SetRect(Rect);
        _GaugeNames.Resize();
    }

    // RMS read out metrics
    {
        if (_State->_HorizontalPeakMeter)
        {
            Rect.left  = 0.f;
            Rect.right = _RMSReadOut.GetTextWidth();
        }

        _RMSReadOut.SetRect(Rect);
        _RMSReadOut.Resize();
    }

    // Peak read out metrics
    {
        if (_State->_HorizontalPeakMeter)
        {
            Rect.left  = 0.f;
            Rect.right = _PeakReadOut.GetTextWidth();
        }

        _PeakReadOut.SetRect(Rect);
        _PeakReadOut.Resize();
    }

    // Gauge scales metrics
    {
        Rect = _Rect;

        if (_State->_HorizontalPeakMeter)
        {
            if (_GraphDescription->_XAxisBottom)
                Rect.right -= _GaugeNames.GetTextWidth();

            if (_RMSReadOut.IsVisible())
                Rect.right -= _RMSReadOut.GetTextWidth();

            if (_PeakReadOut.IsVisible())
                Rect.right -= _PeakReadOut.GetTextWidth();

            if (_GraphDescription->_XAxisBottom || _RMSReadOut.IsVisible() || _PeakReadOut.IsVisible())
                Rect.right -= 4.f;

            if (_GraphDescription->_XAxisTop)
                Rect.right -= _GaugeNames.GetTextWidth();
        }
        else
        {
            if (_GraphDescription->_XAxisBottom)
                Rect.bottom -= _GaugeNames.GetTextHeight();

            if (_RMSReadOut.IsVisible())
                Rect.bottom -= _RMSReadOut.GetTextHeight();

            if (_PeakReadOut.IsVisible())
                Rect.bottom -= _PeakReadOut.GetTextHeight();

            if (_GraphDescription->_XAxisTop)
                Rect.bottom -= _GaugeNames.GetTextHeight();
        }

        _GaugeScales.SetRect(Rect);
        _GaugeScales.Resize();
    }

    // Gauge metrics
    {
        Rect = _Rect;

        if (_State->_HorizontalPeakMeter)
        {
            if (_GraphDescription->_XAxisBottom)
                Rect.right -= _GaugeNames.GetTextWidth();

            if (_RMSReadOut.IsVisible())
                Rect.right -= _RMSReadOut.GetTextWidth();

            if (_PeakReadOut.IsVisible())
                Rect.right -= _PeakReadOut.GetTextWidth();

            if (_GraphDescription->_XAxisBottom || _RMSReadOut.IsVisible() || _PeakReadOut.IsVisible())
                Rect.right -= 4.f;

            if (_GraphDescription->_XAxisTop)
                Rect.right -= _GaugeNames.GetTextWidth();

            if (_GraphDescription->_YAxisRight)
                Rect.bottom -= _GaugeScales.GetTextHeight();

            if (_GraphDescription->_YAxisLeft)
                Rect.bottom -= _GaugeScales.GetTextHeight();
        }
        else
        {
            if (_GraphDescription->_XAxisBottom)
                Rect.bottom -= _GaugeNames.GetTextHeight();

            if (_RMSReadOut.IsVisible())
                Rect.bottom -= _RMSReadOut.GetTextHeight();

            if (_PeakReadOut.IsVisible())
                Rect.bottom -= _PeakReadOut.GetTextHeight();

            if (_GraphDescription->_XAxisTop)
                Rect.bottom -= _GaugeNames.GetTextHeight();

            if (_GraphDescription->_YAxisRight)
                Rect.right -= _GaugeScales.GetTextWidth();

            if (_GraphDescription->_YAxisLeft)
                Rect.right -= _GaugeScales.GetTextWidth();
        }

        _Gauges.SetRect(Rect);
        _Gauges.Resize();
    }

    // Don't continue the resize operation until the gauge metrics have been calculated.
    if (!_Gauges.GetMetrics(_GaugeMetrics))
        return;

    // Gauge names transform
    {
        FLOAT dx = 0.f;
        FLOAT dy = 0.f;

        if (_State->_HorizontalPeakMeter)
        {
            if (_GraphDescription->_YAxisLeft)
                dy += (_GraphDescription->_FlipVertically) ? (_GaugeScales.GetTextHeight() - _GaugeMetrics._Offset) : (_GaugeScales.GetTextHeight() + _GaugeMetrics._Offset);
        }
        else
        {
            if (_GraphDescription->_YAxisLeft)
                dx += (_GraphDescription->_FlipHorizontally) ? (_GaugeScales.GetTextWidth() - _GaugeMetrics._Offset) : (_GaugeScales.GetTextWidth() + _GaugeMetrics._Offset);
        }

        _GaugeNamesTransform = D2D1::Matrix3x2F::Translation(dx, dy);
    }

    // RMS read out transform
    {
        FLOAT dx = 0.f;
        FLOAT dy = 0.f;

        if (_State->_HorizontalPeakMeter)
        {
            if (_GraphDescription->_FlipHorizontally)
                dx = GetWidth() - _RMSReadOut.GetTextWidth();

            if (_GraphDescription->_XAxisBottom)
                dx += _GraphDescription->_FlipHorizontally ? -_GaugeNames.GetTextWidth() : _GaugeNames.GetTextWidth();

            if (_GraphDescription->_YAxisLeft)
                dy += (_GraphDescription->_FlipVertically) ? (_GaugeScales.GetTextHeight() - _GaugeMetrics._Offset) : (_GaugeScales.GetTextHeight() + _GaugeMetrics._Offset);
        }
        else
        {
            if (_GraphDescription->_YAxisLeft)
                dx += _GraphDescription->_FlipHorizontally ? (_GaugeScales.GetTextWidth() - _GaugeMetrics._Offset) : (_GaugeScales.GetTextWidth() + _GaugeMetrics._Offset);

            if (_GraphDescription->_XAxisBottom)
                dy += _GraphDescription->_FlipVertically ? _GaugeNames.GetTextHeight() : -_GaugeNames.GetTextHeight();
        }

        _RMSReadOutTransform = D2D1::Matrix3x2F::Translation(dx, dy);
    }

    // Peak read out transform
    {
        FLOAT dx = 0.f;
        FLOAT dy = 0.f;

        if (_State->_HorizontalPeakMeter)
        {
            if (_GraphDescription->_FlipHorizontally)
                dx = GetWidth() - _PeakReadOut.GetTextWidth();

            if (_GraphDescription->_XAxisBottom)
                dx += _GraphDescription->_FlipHorizontally ? -_GaugeNames.GetTextWidth() : _GaugeNames.GetTextWidth();

            if (_RMSReadOut.IsVisible())
                dx += _GraphDescription->_FlipHorizontally ? -_RMSReadOut.GetTextWidth() : _RMSReadOut.GetTextWidth();

            if (_GraphDescription->_YAxisLeft)
                dy += (_GraphDescription->_FlipVertically) ? (_GaugeScales.GetTextHeight() - _GaugeMetrics._Offset) : (_GaugeScales.GetTextHeight() + _GaugeMetrics._Offset);
        }
        else
        {
            if (_GraphDescription->_YAxisLeft)
                dx += _GraphDescription->_FlipHorizontally ? (_GaugeScales.GetTextWidth() - _GaugeMetrics._Offset) : (_GaugeScales.GetTextWidth() + _GaugeMetrics._Offset);

            if (_GraphDescription->_XAxisBottom)
                dy += _GraphDescription->_FlipVertically ? _GaugeNames.GetTextHeight() : -_GaugeNames.GetTextHeight();

            if (_RMSReadOut.IsVisible())
                dy += _GraphDescription->_FlipVertically ? _RMSReadOut.GetTextHeight() : -_RMSReadOut.GetTextHeight();
        }

        _PeakReadOutTransform = D2D1::Matrix3x2F::Translation(dx, dy);
    }

    // Gauge scales transform
    {
        // Translate from client coordinates to element coordinates.
        FLOAT dx = 0.f;
        FLOAT dy = 0.f;

        if (_State->_HorizontalPeakMeter)
        {
            if (_GraphDescription->_FlipHorizontally)
            {
                if (_GraphDescription->_XAxisTop)
                    dx += _GaugeNames.GetTextWidth();
            }
            else
            {
                if (_GraphDescription->_XAxisBottom)
                    dx += _GaugeNames.GetTextWidth();
    
                if (_RMSReadOut.IsVisible())
                    dx += _RMSReadOut.GetTextWidth();
    
                if (_PeakReadOut.IsVisible())
                    dx += _PeakReadOut.GetTextWidth();

                if (_GraphDescription->_XAxisBottom || _RMSReadOut.IsVisible() || _PeakReadOut.IsVisible())
                    dx += 4.f;
            }
        }
        else
        {
            if (_GraphDescription->_FlipVertically)
            {
                if (_GraphDescription->_XAxisBottom)
                    dy += _GaugeNames.GetTextHeight();

                if (_RMSReadOut.IsVisible())
                    dy += _RMSReadOut.GetTextHeight();
    
                if (_PeakReadOut.IsVisible())
                    dy += _PeakReadOut.GetTextHeight();
            }
            else
            {
                if (_GraphDescription->_XAxisTop)
                    dy += _GaugeNames.GetTextHeight();
            }
        }

        _GaugeScalesTransform = D2D1::Matrix3x2F::Translation(dx, dy);
    }

    // Gauge transform
    {
        // Translate from client coordinates to element coordinates.
        D2D1::Matrix3x2F Transform = (_GraphDescription->_FlipHorizontally) ? D2D1::Matrix3x2F(-1.f, 0.f, 0.f, 1.f, GetWidth(), 0.f) : D2D1::Matrix3x2F::Identity();

        FLOAT dx = 0.f;
        FLOAT dy = 0.f;

        if (_State->_HorizontalPeakMeter)
        {
            if (_GraphDescription->_FlipVertically)
                Transform = Transform * D2D1::Matrix3x2F(1.f, 0.f, 0.f, -1.f, 0.f, GetHeight());

            if (_GraphDescription->_XAxisBottom)
                dx += (_GraphDescription->_FlipHorizontally) ? -_GaugeNames.GetTextWidth() : _GaugeNames.GetTextWidth();

            if (_RMSReadOut.IsVisible())
                dx += (_GraphDescription->_FlipHorizontally) ? -_RMSReadOut.GetTextWidth() : _RMSReadOut.GetTextWidth();

            if (_PeakReadOut.IsVisible())
                dx += (_GraphDescription->_FlipHorizontally) ? -_PeakReadOut.GetTextWidth() : _PeakReadOut.GetTextWidth();

            if (_GraphDescription->_XAxisBottom || _RMSReadOut.IsVisible() || _PeakReadOut.IsVisible())
                dx += 4.f;

            if (_GraphDescription->_YAxisLeft)
                dy += (_GraphDescription->_FlipVertically) ? -(_GaugeScales.GetTextHeight() + _GaugeMetrics._Offset) : (_GaugeScales.GetTextHeight() + _GaugeMetrics._Offset);
        }
        else
        {
            if (!_GraphDescription->_FlipVertically)
                Transform = Transform * D2D1::Matrix3x2F(1.f, 0.f, 0.f, -1.f, 0.f, GetHeight());

            if (_GraphDescription->_XAxisBottom)
                dy -= (_GraphDescription->_FlipVertically) ? -_GaugeNames.GetTextHeight() : _GaugeNames.GetTextHeight();

            if (_RMSReadOut.IsVisible())
                dy -= (_GraphDescription->_FlipVertically) ? -_RMSReadOut.GetTextHeight() : _RMSReadOut.GetTextHeight();

            if (_PeakReadOut.IsVisible())
                dy -= (_GraphDescription->_FlipVertically) ? -_PeakReadOut.GetTextHeight() : _PeakReadOut.GetTextHeight();

            if (_GraphDescription->_YAxisLeft)
                dx += (_GraphDescription->_FlipHorizontally) ? -(_GaugeScales.GetTextWidth() + _GaugeMetrics._Offset) : (_GaugeScales.GetTextWidth() + _GaugeMetrics._Offset);
        }

        D2D1::Matrix3x2F Translate = D2D1::Matrix3x2F::Translation(dx, dy);

        _GaugesTransform = Transform * Translate;
    }

    _IsResized = false;
}

/// <summary>
/// Renders this instance.
/// </summary>
void peak_meter_t::Render(ID2D1DeviceContext * deviceContext) noexcept
{
    HRESULT hr = CreateDeviceSpecificResources(deviceContext);

    if (!SUCCEEDED(hr))
        return;

    // Render the gauge scales.
    {
        deviceContext->SetTransform(_GaugeScalesTransform);

        _GaugeScales.Render(deviceContext);
    }

    // Render the gauges.
    {
        deviceContext->SetTransform(_GaugesTransform);

        _Gauges.Render(deviceContext, _GaugeMetrics);
    }

    // Render the gauge names.
    {
        deviceContext->SetTransform(_GaugeNamesTransform);

        _GaugeNames.Render(deviceContext, _GaugeMetrics);
    }

    // Render the RMS read out.
    {
        deviceContext->SetTransform(_RMSReadOutTransform);

        _RMSReadOut.Render(deviceContext, _GaugeMetrics);
    }

    // Render the peak read out.
    {
        deviceContext->SetTransform(_PeakReadOutTransform);

        _PeakReadOut.Render(deviceContext, _GaugeMetrics);
    }

    ResetTransform(deviceContext);
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT peak_meter_t::CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept
{
    HRESULT hr = _GaugeNames.CreateDeviceSpecificResources(deviceContext);

    if (SUCCEEDED(hr))
        hr = _RMSReadOut.CreateDeviceSpecificResources(deviceContext);

    if (SUCCEEDED(hr))
        hr = _PeakReadOut.CreateDeviceSpecificResources(deviceContext);

    if (SUCCEEDED(hr))
        hr = _GaugeScales.CreateDeviceSpecificResources(deviceContext);

    if (SUCCEEDED(hr))
        hr = _Gauges.CreateDeviceSpecificResources(deviceContext);

    _IsResized = !_IsResized && _Gauges.GetMetrics(_GaugeMetrics);

    if (SUCCEEDED(hr))
        Resize();

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void peak_meter_t::DeleteDeviceSpecificResources() noexcept
{
    _Gauges.DeleteDeviceSpecificResources();

    _GaugeScales.DeleteDeviceSpecificResources();

    _PeakReadOut.DeleteDeviceSpecificResources();

    _RMSReadOut.DeleteDeviceSpecificResources();

    _GaugeNames.DeleteDeviceSpecificResources();
}
