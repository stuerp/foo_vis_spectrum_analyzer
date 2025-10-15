
/** $VER: Oscilloscope.cpp (2025.10.15) P. Stuer - Implements an oscilloscope. **/

#include <pch.h>

#include "Oscilloscope.h"
#include "AmplitudeScaler.h"

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

    _SignalLineStyle = nullptr;

    _XAxisTextStyle = nullptr;
    _XAxisLineStyle = nullptr;

    _YAxisTextStyle = nullptr;
    _YAxisLineStyle = nullptr;

    _HorizontalGridLineStyle = nullptr;
    _VerticalGridLineStyle = nullptr;

    Reset();
}

/// <summary>
/// Destroys this instance.
/// </summary>
oscilloscope_t::~oscilloscope_t()
{
    DeleteDeviceSpecificResources();
}

/// <summary>
/// Initializes this instance.
/// </summary>
void oscilloscope_t::Initialize(state_t * state, const graph_settings_t * settings, const analysis_t * analysis) noexcept
{
    _State = state;
    _GraphSettings = settings;
    _Analysis = analysis;

    DeleteDeviceSpecificResources();

    // Create the labels.
    {
        for (double Amplitude = _GraphSettings->_AmplitudeLo; Amplitude <= _GraphSettings->_AmplitudeHi; Amplitude -= _GraphSettings->_AmplitudeStep)
        {
            WCHAR Text[16] = { };

            ::StringCchPrintfW(Text, _countof(Text), L"%+d", (int) Amplitude);

            const label_t lb =
            {
                .Text = Text,
                .Amplitude = Amplitude
            };

            _Labels.push_back(lb);
        }

        if (!_Labels.empty())
        {
            _Labels.front().IsMin = true;
            _Labels.back().IsMax  = true;
        }
    }

    CreateDeviceIndependentResources();
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
void oscilloscope_t::Render(ID2D1DeviceContext * deviceContext) noexcept
{
    const size_t FrameCount     = _Analysis->_Chunk.get_sample_count();    // get_sample_count() actually returns the number of frames.
    const uint32_t ChannelCount = _Analysis->_Chunk.get_channel_count();

    if ((FrameCount == 0) || (ChannelCount == 0))
        return;

    const audio_sample * Samples = _Analysis->_Chunk.get_data();

    HRESULT hr = CreateDeviceSpecificResources(deviceContext);

    if (!SUCCEEDED(hr))
        return;

    const size_t SelectedChannelCount = (size_t) std::popcount(_Analysis->_Chunk.get_channel_config() & _GraphSettings->_SelectedChannels);

    const FLOAT ChannelHeight = _Size.height / (FLOAT) SelectedChannelCount; // Height available to one channel.
    FLOAT YAxisWidth = _YAxisTextStyle->_Width;

    amplitude_scaler_t Scaler;

    switch (_GraphSettings->_YAxisMode)
    {
        case YAxisMode::None:
            Scaler.SetNormalizedMode();

            YAxisWidth = 0.f;
            break;

        case YAxisMode::Decibels:
            Scaler.SetDecibelMode(_GraphSettings->_AmplitudeLo, _GraphSettings->_AmplitudeHi);
            break;

        case YAxisMode::Linear:
            Scaler.SetLinearMode(_GraphSettings->_AmplitudeLo, _GraphSettings->_AmplitudeHi, _GraphSettings->_Gamma, _GraphSettings->_UseAbsolute);
            break;
    }

    FLOAT YAxisCount = 0.f;

    if (_GraphSettings->_YAxisLeft)
        ++YAxisCount;

    if (_GraphSettings->_YAxisRight)
        ++YAxisCount;

    deviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

    _YAxisTextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING); // Right-align horizontally, also for the right axis.
    _YAxisTextStyle->SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // Draw the axis.
    const FLOAT x1 =                (_GraphSettings->HasYAxis() && _GraphSettings->_YAxisLeft)  ? YAxisWidth : 0.f;
    const FLOAT x2 = _Size.width - ((_GraphSettings->HasYAxis() && _GraphSettings->_YAxisRight) ? YAxisWidth : 0.f);

    // Y-axis
    if (_GraphSettings->HasYAxis())
    {
        for (uint32_t i = 0; i < ChannelCount; ++i)
        {
            const FLOAT y1 = ChannelHeight * (FLOAT) i;
            const FLOAT y2 = y1 + ChannelHeight;

            if (_YAxisLineStyle->IsEnabled())
            {
                if (_GraphSettings->_YAxisLeft)
                    deviceContext->DrawLine(D2D1::Point2F(YAxisWidth, y1), D2D1::Point2F(YAxisWidth, y2), _YAxisLineStyle->_Brush, _YAxisLineStyle->_Thickness, nullptr);

                if (_GraphSettings->_YAxisRight)
                    deviceContext->DrawLine(D2D1::Point2F(x2 + 1.f, y1), D2D1::Point2F(x2 + 1.f, y2), _YAxisLineStyle->_Brush, _YAxisLineStyle->_Thickness, nullptr);
            }

            if (_YAxisTextStyle->IsEnabled())
            {
                D2D1_RECT_F r = {  };

                for (const label_t & Label : _Labels)
                {
                    const FLOAT y = msc::Map(_GraphSettings->ScaleAmplitude(ToMagnitude(Label.Amplitude)), 0., 1., y2, y1);

                    // Draw the label.
                    r.top    = Label.IsMin ? y - _YAxisTextStyle->_Height : (Label.IsMax ? y : y - (_YAxisTextStyle->_Height / 2.f));
                    r.bottom = r.top + _YAxisTextStyle->_Height;

                    if (_HorizontalGridLineStyle->IsEnabled())
                        deviceContext->DrawLine(D2D1::Point2F(x1, y), D2D1::Point2F(x2, y), _HorizontalGridLineStyle->_Brush, _HorizontalGridLineStyle->_Thickness, nullptr);

                    if (_GraphSettings->_YAxisLeft)
                    {
                        r.left  = 0.f;
                        r.right = x1 - 2.f;

                        deviceContext->DrawText(Label.Text.c_str(), (UINT) Label.Text.size(), _YAxisTextStyle->_TextFormat, r, _YAxisTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
                    }

                    if (_GraphSettings->_YAxisRight)
                    {
                        r.right = _Size.width - 1.f;
                        r.left  = x2 + 2.f;

                        deviceContext->DrawText(Label.Text.c_str(), (UINT) Label.Text.size(), _YAxisTextStyle->_TextFormat, r, _YAxisTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
                    }
                }
            }
        }
    }

    // X-axis
    if (_GraphSettings->HasXAxis())
    {
        FLOAT ChannelBaseline = ChannelHeight * (_GraphSettings->HasYAxis() ? 1.0f : 0.5f);

        for (uint32_t i = 0; i < SelectedChannelCount; ++i)
        {
            deviceContext->DrawLine(D2D1::Point2F(x1, ChannelBaseline), D2D1::Point2F(x2, ChannelBaseline), _XAxisLineStyle->_Brush, _XAxisLineStyle->_Thickness, nullptr);

            ChannelBaseline += ChannelHeight;
        }
    }

    // Draw the signal.
    deviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

    CComPtr<ID2D1PathGeometry> Geometry;

    hr = _Direct2D.Factory->CreatePathGeometry(&Geometry);

    if (SUCCEEDED(hr))
    {
        CComPtr<ID2D1GeometrySink> Sink;

        hr = Geometry->Open(&Sink);

        if (SUCCEEDED(hr))
        {
            const FLOAT ZoomFactor = (FLOAT) 1.f;

            uint32_t ChunkChannels    = _Analysis->_Chunk.get_channel_config(); // Mask containing the channels in the audio chunk.
            uint32_t SelectedChannels = _GraphSettings->_SelectedChannels;      // Mask containing the channels selected by the user.

            const FLOAT ChannelMax = ChannelHeight * (_GraphSettings->HasYAxis() ? 1.0f : 0.5f);

            FLOAT ChannelBaseline = ChannelMax;
            size_t ChannelOffset = 0;
            
            while ((ChunkChannels != 0) && (SelectedChannels != 0))
            {
                // Render the signal if the channel is in the chunk and if it has been selected.
                if (ChunkChannels & 1)
                {
                    if (SelectedChannels & 1)
                    {
                        const size_t SampleCount = FrameCount * ChannelCount;
                        const FLOAT dx = (_Size.width - (YAxisWidth * YAxisCount)) / (FLOAT) FrameCount;
                        FLOAT x = _GraphSettings->_YAxisLeft ? YAxisWidth : 0.f;

                        FLOAT y = ChannelBaseline - ((FLOAT) Scaler(Samples[ChannelOffset]) * ZoomFactor * ChannelMax);

                        Sink->BeginFigure(D2D1::Point2F(x, y), D2D1_FIGURE_BEGIN_HOLLOW);

                        for (size_t j = ChannelCount + ChannelOffset; j < SampleCount; j += ChannelCount)
                        {
                            y = ChannelBaseline - ((FLOAT) Scaler(Samples[j]) * ZoomFactor * ChannelMax);

                            Sink->AddLine(D2D1::Point2F(x, y));

                            x += dx;
                        }

                        Sink->EndFigure(D2D1_FIGURE_END_OPEN);

                        ChannelBaseline += ChannelHeight;
                    }

                    ChannelOffset++;
                }

                ChunkChannels    >>= 1;
                SelectedChannels >>= 1;
            }

            hr = Sink->Close();
        }

        if (SUCCEEDED(hr))
            deviceContext->DrawGeometry(Geometry, _SignalLineStyle->_Brush, _SignalLineStyle->_Thickness, _SignalStrokeStyle);
    }
}

/// <summary>
/// Creates resources which are not bound to any D3D device. Their lifetime effectively extends for the duration of the app.
/// </summary>
HRESULT oscilloscope_t::CreateDeviceIndependentResources() noexcept
{
    HRESULT hr = S_OK;

    const D2D1_STROKE_STYLE_PROPERTIES StrokeStyleProperties = D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT, D2D1_LINE_JOIN_BEVEL);

    if (_SignalStrokeStyle == nullptr)
        hr = _Direct2D.Factory->CreateStrokeStyle(StrokeStyleProperties, nullptr, 0, &_SignalStrokeStyle);

    return hr;
}

/// <summary>
/// Releases the device independent resources.
/// </summary>
void oscilloscope_t::DeleteDeviceIndependentResources() noexcept
{
    _SignalStrokeStyle.Release();
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT oscilloscope_t::CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr))
        Resize();

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::SignalLine, deviceContext, _Size, L"", 1.f, &_SignalLineStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::XAxisText, deviceContext, _Size, L"999", 1.f, &_XAxisTextStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::XAxisLine, deviceContext, _Size, L"", 1.f, &_XAxisLineStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::YAxisText, deviceContext, _Size, L"+999", 1.f, &_YAxisTextStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::YAxisLine, deviceContext, _Size, L"", 1.f, &_YAxisLineStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::HorizontalGridLine, deviceContext, _Size, L"", 1.f, &_HorizontalGridLineStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::VerticalGridLine, deviceContext, _Size, L"", 1.f, &_VerticalGridLineStyle);

#ifdef _DEBUG
    if (SUCCEEDED(hr) && (_DebugBrush == nullptr))
        deviceContext->CreateSolidColorBrush(D2D1::ColorF(1.f,0.f,0.f), &_DebugBrush);
#endif

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void oscilloscope_t::DeleteDeviceSpecificResources() noexcept
{
    if (_SignalLineStyle)
    {
        _SignalLineStyle->DeleteDeviceSpecificResources();
        _SignalLineStyle = nullptr;
    }

    if (_XAxisTextStyle)
    {
        _XAxisTextStyle->DeleteDeviceSpecificResources();
        _XAxisTextStyle = nullptr;
    }

    if (_XAxisLineStyle)
    {
        _XAxisLineStyle->DeleteDeviceSpecificResources();
        _XAxisLineStyle = nullptr;
    }

    if (_YAxisTextStyle)
    {
        _YAxisTextStyle->DeleteDeviceSpecificResources();
        _YAxisTextStyle = nullptr;
    }

    if (_YAxisLineStyle)
    {
        _YAxisLineStyle->DeleteDeviceSpecificResources();
        _YAxisLineStyle = nullptr;
    }

    if (_HorizontalGridLineStyle)
    {
        _HorizontalGridLineStyle->DeleteDeviceSpecificResources();
        _HorizontalGridLineStyle = nullptr;
    }

#ifdef _DEBUG
    _DebugBrush.Release();
#endif
}
