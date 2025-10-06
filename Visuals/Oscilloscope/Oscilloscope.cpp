
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
    const audio_sample * Samples = _Analysis->_Chunk.get_data();
    const size_t SampleCount     = _Analysis->_Chunk.get_sample_count();
    const uint32_t ChannelCount  = _Analysis->_Chunk.get_channel_count();

    if ((SampleCount == 0) || (ChannelCount == 0))
        return;

    HRESULT hr = CreateDeviceSpecificResources(renderTarget);

    if (!SUCCEEDED(hr))
        return;

    renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

    FLOAT ChannelHeight = _Size.height / 2.f / (FLOAT) ChannelCount; // Height available to one channel.

    if (_GraphSettings->_YAxisMode == YAxisMode::Decibels)
        ChannelHeight = _Size.height / (FLOAT) ChannelCount; // Height available to one channel.

    // Draw the horizontal grid lines.
    if (_HorizontalGridLineStyle->IsEnabled())
    {
        for (uint32_t i = 0; i < ChannelCount; ++i)
        {
            FLOAT y = (FLOAT) _Size.height * (((FLOAT) i + 0.5f) / (FLOAT) ChannelCount);

            if (_GraphSettings->_YAxisMode == YAxisMode::Decibels)
                y = (FLOAT) _Size.height * (((FLOAT) i + 1.0f) / (FLOAT) ChannelCount);

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
            const FLOAT ZoomFactor = (FLOAT) 1.f;

            for (uint32_t i = 0; i < ChannelCount; ++i)
            {
                FLOAT Baseline = (FLOAT) _Size.height * (((FLOAT) i + 0.5f) / (FLOAT) ChannelCount);

                if (_GraphSettings->_YAxisMode == YAxisMode::Decibels)
                    Baseline = (FLOAT) _Size.height * (((FLOAT) i + 1.0f) / (FLOAT) ChannelCount);

                for (t_uint32 j = 0; j < SampleCount; ++j)
                {
                    audio_sample Amplitude = Samples[(j * ChannelCount) + i];

                    if (_GraphSettings->_YAxisMode == YAxisMode::Decibels)
                    {
                        const double dBMax   = _GraphSettings->_AmplitudeHi;
                        const double dBMin   = _GraphSettings->_AmplitudeLo;
                        const double dBRange = dBMax - dBMin;

                        double dB = (Amplitude == 0.) ? dBMin : 20.0 * std::log10(std::abs(Amplitude));
                        dB = std::clamp(dB, dBMin, dBMax);
                        Amplitude = ((dB - dBMin) / dBRange);
                    }

                    const FLOAT x = (_Size.width * (FLOAT) j) / (FLOAT) (SampleCount - 1);
                    const FLOAT y = Baseline - (FLOAT) (Amplitude * ZoomFactor * ChannelHeight);

                    if (j == 0)
                        Sink->BeginFigure(D2D1::Point2F(x, y), D2D1_FIGURE_BEGIN_HOLLOW);
                    else
                        Sink->AddLine(D2D1::Point2F(x, y));
                }

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
