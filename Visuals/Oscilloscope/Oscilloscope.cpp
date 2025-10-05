
/** $VER: Oscilloscope.cpp (2025.10.04) P. Stuer - Implements an oscilloscope. **/

#include <pch.h>

#include "Oscilloscope.h"

#include "Support.h"
#include "Log.h"

#include "DirectWrite.h"

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
oscilloscope_t::oscilloscope_t()
{
    _Bounds = { };
    _Size = { };

    _SignalLineStyle =  nullptr;

    Reset();
}

/// <summary>
/// Destroys this instance.
/// </summary>
oscilloscope_t::~oscilloscope_t()
{
    ReleaseDeviceSpecificResources();
}

/// <summary>
/// Initializes this instance.
/// </summary>
void oscilloscope_t::Initialize(state_t * state, const graph_settings_t * settings, const analysis_t * analysis) noexcept
{
    _State = state;
    _GraphSettings = settings;
    _Analysis = analysis;

    ReleaseDeviceSpecificResources();

    // Non-device specific resources
    D2D1_STROKE_STYLE_PROPERTIES StrokeStyleProperties = D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT, D2D1_LINE_JOIN_BEVEL);

    _Direct2D.Factory->CreateStrokeStyle(StrokeStyleProperties, nullptr, 0, &_SignalStrokeStyle);
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void oscilloscope_t::Move(const D2D1_RECT_F & rect) noexcept
{
    SetBounds(rect);
}

/// <summary>
/// Resets this instance.
/// </summary>
void oscilloscope_t::Reset() noexcept
{
    _IsResized = true;
}

/// <summary>
/// Recalculates parameters that are render target and size-sensitive.
/// </summary>
void oscilloscope_t::Resize() noexcept
{
    if (!_IsResized || (GetWidth() == 0.f) || (GetHeight() == 0.f))
        return;

    _IsResized = false;
}

/// <summary>
/// Renders this instance.
/// </summary>
void oscilloscope_t::Render(ID2D1RenderTarget * renderTarget) noexcept
{
    HRESULT hr = CreateDeviceSpecificResources(renderTarget);

    if (!SUCCEEDED(hr))
        return;

    renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

    // Draw the horizontal grid lines.
    if (_HorizontalGridLineStyle->IsEnabled())
    {
        for (uint32_t i = 0; i < _Analysis->_ChannelCount; ++i)
        {
            const FLOAT y = (FLOAT) _Size.height * (((FLOAT) i + 0.5f) / (FLOAT) _Analysis->_ChannelCount);

            renderTarget->DrawLine(D2D1::Point2F(0.f, y), D2D1::Point2F(_Size.width, y), _HorizontalGridLineStyle->_Brush, _HorizontalGridLineStyle->_Thickness, nullptr);
        }
    }

    // Draw the signal.
    CComPtr<ID2D1PathGeometry> Path;

    hr = _Direct2D.Factory->CreatePathGeometry(&Path);

    if (SUCCEEDED(hr))
    {
        CComPtr<ID2D1GeometrySink> Sink;

        hr = Path->Open(&Sink);

        if (SUCCEEDED(hr))
        {
            const audio_sample * Samples = _Analysis->_Chunk.get_data();
            const size_t SampleCount     = _Analysis->_Chunk.get_sample_count();

            const FLOAT ZoomFactor = (FLOAT) 1.f;

            for (uint32_t i = 0; i < _Analysis->_ChannelCount; ++i)
            {
                const FLOAT Baseline = (FLOAT) _Size.height * (((FLOAT) i + 0.5f) / (FLOAT) _Analysis->_ChannelCount);

                for (t_uint32 j = 0; j < SampleCount; ++j)
                {
                    const audio_sample Sample = Samples[(j * _Analysis->_ChannelCount) + i];

                    const FLOAT x = (_Size.width * (FLOAT) j) / (FLOAT)(SampleCount - 1);
                    const FLOAT y = Baseline - (FLOAT)(Sample * ZoomFactor * _Size.height / 2.f / _Analysis->_ChannelCount) + 0.5f;

                    if (j == 0)
                        Sink->BeginFigure(D2D1::Point2F(x, y), D2D1_FIGURE_BEGIN_HOLLOW);
                    else
                        Sink->AddLine(D2D1::Point2F(x, y));
                }

                if ((_Analysis->_ChannelCount > 0) && (SampleCount > 0))
                    Sink->EndFigure(D2D1_FIGURE_END_OPEN);
            }

            hr = Sink->Close();
        }

        if (SUCCEEDED(hr))
            renderTarget->DrawGeometry(Path, _SignalLineStyle->_Brush, _SignalLineStyle->_Thickness, _SignalStrokeStyle);
    }
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT oscilloscope_t::CreateDeviceSpecificResources(ID2D1RenderTarget * renderTarget) noexcept
{
    HRESULT hr = S_OK;

    D2D1_SIZE_F Size = renderTarget->GetSize();

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::SignalLine, renderTarget, Size, L"", &_SignalLineStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::HorizontalGridLine, renderTarget, Size, L"", &_HorizontalGridLineStyle);

    if (SUCCEEDED(hr))
        Resize();

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void oscilloscope_t::ReleaseDeviceSpecificResources() noexcept
{
    if (_HorizontalGridLineStyle)
    {
        _HorizontalGridLineStyle->ReleaseDeviceSpecificResources();
        _HorizontalGridLineStyle = nullptr;
    }

    if (_SignalLineStyle)
    {
        _SignalLineStyle->ReleaseDeviceSpecificResources();
        _SignalLineStyle = nullptr;
    }
}
