
/** $VER: PeakMeter.cpp (2024.04.28) P. Stuer - Represents a peak meter. **/

#include "pch.h"

#include "PeakMeter.h"

#include "Support.h"
#include "Log.h"

#include "DirectWrite.h"

#pragma hdrstop

PeakMeter::PeakMeter()
{
    _Bounds = { };
    _Size = { };

    Reset();
}

/// <summary>
/// Initializes this instance.
/// </summary>
void PeakMeter::Initialize(state_t * state, const GraphSettings * settings, const Analysis * analysis)
{
    _Gauges.Initialize(state, settings, analysis);
    _GaugeScales.Initialize(state, settings, analysis);
    _GaugeNames.Initialize(state, settings, analysis);
    _RMSReadOut.Initialize(state, settings, analysis);
    _PeakReadOut.Initialize(state, settings, analysis);

    _State = state;
    _GraphSettings = settings;
    _Analysis = analysis;

    ReleaseDeviceSpecificResources();
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void PeakMeter::Move(const D2D1_RECT_F & rect)
{
    SetBounds(rect);

    _Gauges.Move(rect);
    _GaugeNames.Move(rect);
    _GaugeScales.Move(rect);
    _RMSReadOut.Move(rect);
    _PeakReadOut.Move(rect);
}

/// <summary>
/// Resets this instance.
/// </summary>
void PeakMeter::Reset()
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
void PeakMeter::Resize() noexcept
{
    if (!_IsResized || (GetWidth() == 0.f) || (GetHeight() == 0.f))
        return;

    D2D1_RECT_F Rect = _Bounds;

    // Gauge names metrics
    {
        if (_State->_HorizontalPeakMeter)
        {
            if (_GraphSettings->_YAxisLeft)
                Rect.bottom -= (FLOAT) _GaugeScales.GetTextHeight();

            if (_GraphSettings->_YAxisRight)
                Rect.bottom -= (FLOAT) _GaugeScales.GetTextHeight();
        }
        else
        {
            if (_GraphSettings->_YAxisLeft)
                Rect.right  -= (FLOAT) _GaugeScales.GetTextWidth();

            if (_GraphSettings->_YAxisRight)
                Rect.right  -= (FLOAT) _GaugeScales.GetTextWidth();
        }

        _GaugeNames.SetBounds(Rect);
        _GaugeNames.Resize();
    }

    // RMS read out metrics
    {
        if (_State->_HorizontalPeakMeter)
        {
            Rect.left  = 0.f;
            Rect.right = _RMSReadOut.GetTextWidth();
        }

        _RMSReadOut.SetBounds(Rect);
        _RMSReadOut.Resize();
    }

    // Peak read out metrics
    {
        if (_State->_HorizontalPeakMeter)
        {
            Rect.left  = 0.f;
            Rect.right = _PeakReadOut.GetTextWidth();
        }

        _PeakReadOut.SetBounds(Rect);
        _PeakReadOut.Resize();
    }

    // Gauge scales metrics
    {
        Rect = _Bounds;

        if (_State->_HorizontalPeakMeter)
        {
            if (_GraphSettings->_XAxisBottom)
                Rect.right -= _GaugeNames.GetTextWidth();

            if (_RMSReadOut.IsVisible())
                Rect.right -= _RMSReadOut.GetTextWidth();

            if (_PeakReadOut.IsVisible())
                Rect.right -= _PeakReadOut.GetTextWidth();

            if (_GraphSettings->_XAxisBottom || _RMSReadOut.IsVisible() || _PeakReadOut.IsVisible())
                Rect.right -= 4.f;

            if (_GraphSettings->_XAxisTop)
                Rect.right -= _GaugeNames.GetTextWidth();
        }
        else
        {
            if (_GraphSettings->_XAxisBottom)
                Rect.bottom -= _GaugeNames.GetTextHeight();

            if (_RMSReadOut.IsVisible())
                Rect.bottom -= _RMSReadOut.GetTextHeight();

            if (_PeakReadOut.IsVisible())
                Rect.bottom -= _PeakReadOut.GetTextHeight();

            if (_GraphSettings->_XAxisTop)
                Rect.bottom -= _GaugeNames.GetTextHeight();
        }

        _GaugeScales.SetBounds(Rect);
        _GaugeScales.Resize();
    }

    // Gauge metrics
    {
        Rect = _Bounds;

        if (_State->_HorizontalPeakMeter)
        {
            if (_GraphSettings->_XAxisBottom)
                Rect.right -= _GaugeNames.GetTextWidth();

            if (_RMSReadOut.IsVisible())
                Rect.right -= _RMSReadOut.GetTextWidth();

            if (_PeakReadOut.IsVisible())
                Rect.right -= _PeakReadOut.GetTextWidth();

            if (_GraphSettings->_XAxisBottom || _RMSReadOut.IsVisible() || _PeakReadOut.IsVisible())
                Rect.right -= 4.f;

            if (_GraphSettings->_XAxisTop)
                Rect.right -= _GaugeNames.GetTextWidth();

            if (_GraphSettings->_YAxisRight)
                Rect.bottom -= _GaugeScales.GetTextHeight();

            if (_GraphSettings->_YAxisLeft)
                Rect.bottom -= _GaugeScales.GetTextHeight();
        }
        else
        {
            if (_GraphSettings->_XAxisBottom)
                Rect.bottom -= _GaugeNames.GetTextHeight();

            if (_RMSReadOut.IsVisible())
                Rect.bottom -= _RMSReadOut.GetTextHeight();

            if (_PeakReadOut.IsVisible())
                Rect.bottom -= _PeakReadOut.GetTextHeight();

            if (_GraphSettings->_XAxisTop)
                Rect.bottom -= _GaugeNames.GetTextHeight();

            if (_GraphSettings->_YAxisRight)
                Rect.right -= _GaugeScales.GetTextWidth();

            if (_GraphSettings->_YAxisLeft)
                Rect.right -= _GaugeScales.GetTextWidth();
        }

        _Gauges.SetBounds(Rect);
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
            if (_GraphSettings->_YAxisLeft)
                dy += (_GraphSettings->_FlipVertically) ? (_GaugeScales.GetTextHeight() - _GaugeMetrics._Offset) : (_GaugeScales.GetTextHeight() + _GaugeMetrics._Offset);
        }
        else
        {
            if (_GraphSettings->_YAxisLeft)
                dx += (_GraphSettings->_FlipHorizontally) ? (_GaugeScales.GetTextWidth() - _GaugeMetrics._Offset) : (_GaugeScales.GetTextWidth() + _GaugeMetrics._Offset);
        }

        _GaugeNamesTransform = D2D1::Matrix3x2F::Translation(dx, dy);
    }

    // RMS read out transform
    {
        FLOAT dx = 0.f;
        FLOAT dy = 0.f;

        if (_State->_HorizontalPeakMeter)
        {
            if (_GraphSettings->_FlipHorizontally)
                dx = GetWidth() - _RMSReadOut.GetTextWidth();

            if (_GraphSettings->_XAxisBottom)
                dx += _GraphSettings->_FlipHorizontally ? -_GaugeNames.GetTextWidth() : _GaugeNames.GetTextWidth();

            if (_GraphSettings->_YAxisLeft)
                dy += (_GraphSettings->_FlipVertically) ? (_GaugeScales.GetTextHeight() - _GaugeMetrics._Offset) : (_GaugeScales.GetTextHeight() + _GaugeMetrics._Offset);
        }
        else
        {
            if (_GraphSettings->_YAxisLeft)
                dx += _GraphSettings->_FlipHorizontally ? (_GaugeScales.GetTextWidth() - _GaugeMetrics._Offset) : (_GaugeScales.GetTextWidth() + _GaugeMetrics._Offset);

            if (_GraphSettings->_XAxisBottom)
                dy += _GraphSettings->_FlipVertically ? _GaugeNames.GetTextHeight() : -_GaugeNames.GetTextHeight();
        }

        _RMSReadOutTransform = D2D1::Matrix3x2F::Translation(dx, dy);
    }

    // Peak read out transform
    {
        FLOAT dx = 0.f;
        FLOAT dy = 0.f;

        if (_State->_HorizontalPeakMeter)
        {
            if (_GraphSettings->_FlipHorizontally)
                dx = GetWidth() - _PeakReadOut.GetTextWidth();

            if (_GraphSettings->_XAxisBottom)
                dx += _GraphSettings->_FlipHorizontally ? -_GaugeNames.GetTextWidth() : _GaugeNames.GetTextWidth();

            if (_RMSReadOut.IsVisible())
                dx += _GraphSettings->_FlipHorizontally ? -_RMSReadOut.GetTextWidth() : _RMSReadOut.GetTextWidth();

            if (_GraphSettings->_YAxisLeft)
                dy += (_GraphSettings->_FlipVertically) ? (_GaugeScales.GetTextHeight() - _GaugeMetrics._Offset) : (_GaugeScales.GetTextHeight() + _GaugeMetrics._Offset);
        }
        else
        {
            if (_GraphSettings->_YAxisLeft)
                dx += _GraphSettings->_FlipHorizontally ? (_GaugeScales.GetTextWidth() - _GaugeMetrics._Offset) : (_GaugeScales.GetTextWidth() + _GaugeMetrics._Offset);

            if (_GraphSettings->_XAxisBottom)
                dy += _GraphSettings->_FlipVertically ? _GaugeNames.GetTextHeight() : -_GaugeNames.GetTextHeight();

            if (_RMSReadOut.IsVisible())
                dy += _GraphSettings->_FlipVertically ? _RMSReadOut.GetTextHeight() : -_RMSReadOut.GetTextHeight();
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
            if (_GraphSettings->_FlipHorizontally)
            {
                if (_GraphSettings->_XAxisTop)
                    dx += _GaugeNames.GetTextWidth();
            }
            else
            {
                if (_GraphSettings->_XAxisBottom)
                    dx += _GaugeNames.GetTextWidth();
    
                if (_RMSReadOut.IsVisible())
                    dx += _RMSReadOut.GetTextWidth();
    
                if (_PeakReadOut.IsVisible())
                    dx += _PeakReadOut.GetTextWidth();

                if (_GraphSettings->_XAxisBottom || _RMSReadOut.IsVisible() || _PeakReadOut.IsVisible())
                    dx += 4.f;
            }
        }
        else
        {
            if (_GraphSettings->_FlipVertically)
            {
                if (_GraphSettings->_XAxisBottom)
                    dy += _GaugeNames.GetTextHeight();

                if (_RMSReadOut.IsVisible())
                    dy += _RMSReadOut.GetTextHeight();
    
                if (_PeakReadOut.IsVisible())
                    dy += _PeakReadOut.GetTextHeight();
            }
            else
            {
                if (_GraphSettings->_XAxisTop)
                    dy += _GaugeNames.GetTextHeight();
            }
        }

        _GaugeScalesTransform = D2D1::Matrix3x2F::Translation(dx, dy);
    }

    // Gauge transform
    {
        // Translate from client coordinates to element coordinates.
        D2D1::Matrix3x2F Transform = (_GraphSettings->_FlipHorizontally) ? D2D1::Matrix3x2F(-1.f, 0.f, 0.f, 1.f, GetWidth(), 0.f) : D2D1::Matrix3x2F::Identity();

        FLOAT dx = 0.f;
        FLOAT dy = 0.f;

        if (_State->_HorizontalPeakMeter)
        {
            if (_GraphSettings->_FlipVertically)
                Transform = Transform * D2D1::Matrix3x2F(1.f, 0.f, 0.f, -1.f, 0.f, GetHeight());

            if (_GraphSettings->_XAxisBottom)
                dx += (_GraphSettings->_FlipHorizontally) ? -_GaugeNames.GetTextWidth() : _GaugeNames.GetTextWidth();

            if (_RMSReadOut.IsVisible())
                dx += (_GraphSettings->_FlipHorizontally) ? -_RMSReadOut.GetTextWidth() : _RMSReadOut.GetTextWidth();

            if (_PeakReadOut.IsVisible())
                dx += (_GraphSettings->_FlipHorizontally) ? -_PeakReadOut.GetTextWidth() : _PeakReadOut.GetTextWidth();

            if (_GraphSettings->_XAxisBottom || _RMSReadOut.IsVisible() || _PeakReadOut.IsVisible())
                dx += 4.f;

            if (_GraphSettings->_YAxisLeft)
                dy += (_GraphSettings->_FlipVertically) ? -(_GaugeScales.GetTextHeight() + _GaugeMetrics._Offset) : (_GaugeScales.GetTextHeight() + _GaugeMetrics._Offset);
        }
        else
        {
            if (!_GraphSettings->_FlipVertically)
                Transform = Transform * D2D1::Matrix3x2F(1.f, 0.f, 0.f, -1.f, 0.f, GetHeight());

            if (_GraphSettings->_XAxisBottom)
                dy -= (_GraphSettings->_FlipVertically) ? -_GaugeNames.GetTextHeight() : _GaugeNames.GetTextHeight();

            if (_RMSReadOut.IsVisible())
                dy -= (_GraphSettings->_FlipVertically) ? -_RMSReadOut.GetTextHeight() : _RMSReadOut.GetTextHeight();

            if (_PeakReadOut.IsVisible())
                dy -= (_GraphSettings->_FlipVertically) ? -_PeakReadOut.GetTextHeight() : _PeakReadOut.GetTextHeight();

            if (_GraphSettings->_YAxisLeft)
                dx += (_GraphSettings->_FlipHorizontally) ? -(_GaugeScales.GetTextWidth() + _GaugeMetrics._Offset) : (_GaugeScales.GetTextWidth() + _GaugeMetrics._Offset);
        }

        D2D1::Matrix3x2F Translate = D2D1::Matrix3x2F::Translation(dx, dy);

        _GaugesTransform = Transform * Translate;
    }

    _IsResized = false;
}

/// <summary>
/// Renders this instance.
/// </summary>
void PeakMeter::Render(ID2D1RenderTarget * renderTarget)
{
    HRESULT hr = CreateDeviceSpecificResources(renderTarget);

    if (!SUCCEEDED(hr))
        return;

    // Render the gauge scales.
    {
        renderTarget->SetTransform(_GaugeScalesTransform);

        _GaugeScales.Render(renderTarget);
    }

    // Render the gauges.
    {
        renderTarget->SetTransform(_GaugesTransform);

        _Gauges.Render(renderTarget, _GaugeMetrics);
    }

    // Render the gauge names.
    {
        renderTarget->SetTransform(_GaugeNamesTransform);

        _GaugeNames.Render(renderTarget, _GaugeMetrics);
    }

    // Render the RMS read out.
    {
        renderTarget->SetTransform(_RMSReadOutTransform);

        _RMSReadOut.Render(renderTarget, _GaugeMetrics);
    }

    // Render the peak read out.
    {
        renderTarget->SetTransform(_PeakReadOutTransform);

        _PeakReadOut.Render(renderTarget, _GaugeMetrics);
    }

    ResetTransform(renderTarget);
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT PeakMeter::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept
{
    HRESULT hr = _GaugeNames.CreateDeviceSpecificResources(renderTarget);

    if (SUCCEEDED(hr))
        hr = _RMSReadOut.CreateDeviceSpecificResources(renderTarget);

    if (SUCCEEDED(hr))
        hr = _PeakReadOut.CreateDeviceSpecificResources(renderTarget);

    if (SUCCEEDED(hr))
        hr = _GaugeScales.CreateDeviceSpecificResources(renderTarget);

    if (SUCCEEDED(hr))
        hr = _Gauges.CreateDeviceSpecificResources(renderTarget);

    _IsResized = !_IsResized && _Gauges.GetMetrics(_GaugeMetrics);

    if (SUCCEEDED(hr))
        Resize();

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void PeakMeter::ReleaseDeviceSpecificResources() noexcept
{
    _Gauges.ReleaseDeviceSpecificResources();

    _GaugeScales.ReleaseDeviceSpecificResources();

    _PeakReadOut.ReleaseDeviceSpecificResources();

    _RMSReadOut.ReleaseDeviceSpecificResources();

    _GaugeNames.ReleaseDeviceSpecificResources();
}
