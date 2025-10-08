
/** $VER: Oscilloscope.cpp (2025.10.08) P. Stuer - Implements an oscilloscope. **/

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

    _SignalLineStyle =  nullptr;
    _HorizontalGridLineStyle = nullptr;
    _VerticalGridLineStyle = nullptr;

    _LineStyle = nullptr;
    _TextStyle = nullptr;

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
    const size_t FrameCount      = _Analysis->_Chunk.get_sample_count();    // get_sample_count() actually returns the number of frames.
    const uint32_t ChannelCount  = _Analysis->_Chunk.get_channel_count();

    if ((FrameCount == 0) || (ChannelCount == 0))
        return;

    const audio_sample * Samples = _Analysis->_Chunk.get_data();

    HRESULT hr = CreateDeviceSpecificResources(renderTarget);

    if (!SUCCEEDED(hr))
        return;

    const FLOAT ChannelHeight = _Size.height / (FLOAT) ChannelCount; // Height available to one channel.
    FLOAT YAxisWidth = _TextStyle->_Width;

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

    renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

    _TextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING); // Right-align horizontally, also for the right axis.
    _TextStyle->SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

    // Draw the axis.
    for (uint32_t i = 0; i < ChannelCount; ++i)
    {
        const FLOAT Baseline = ChannelHeight * ((FLOAT) i + ((_GraphSettings->_YAxisMode == YAxisMode::None) ? 0.5f : 1.0f));

        const FLOAT x1 =                ((_GraphSettings->_YAxisMode != YAxisMode::None) && _GraphSettings->_YAxisLeft)  ? YAxisWidth : 0.f;
        const FLOAT x2 = _Size.width - (((_GraphSettings->_YAxisMode != YAxisMode::None) && _GraphSettings->_YAxisRight) ? YAxisWidth : 0.f);

        // X-axis
        if (_HorizontalGridLineStyle->IsEnabled())
            renderTarget->DrawLine(D2D1::Point2F(x1, Baseline), D2D1::Point2F(x2, Baseline), _HorizontalGridLineStyle->_Brush, _HorizontalGridLineStyle->_Thickness, nullptr);

        // Y-axis
        if (_GraphSettings->_YAxisMode != YAxisMode::None)
        {
            const FLOAT y1 = ChannelHeight * (FLOAT) i;
            const FLOAT y2 = y1 + ChannelHeight;

            if (_VerticalGridLineStyle->IsEnabled())
            {
                if (_GraphSettings->_YAxisLeft)
                    renderTarget->DrawLine(D2D1::Point2F(YAxisWidth, y1), D2D1::Point2F(YAxisWidth, y2), _VerticalGridLineStyle->_Brush, _VerticalGridLineStyle->_Thickness, nullptr);

                if (_GraphSettings->_YAxisRight)
                    renderTarget->DrawLine(D2D1::Point2F(x2 + 1.f, y1), D2D1::Point2F(x2 + 1.f, y2), _VerticalGridLineStyle->_Brush, _VerticalGridLineStyle->_Thickness, nullptr);
            }

            if (_TextStyle->IsEnabled())
            {
                D2D1_RECT_F r = {  };

                for (const label_t & Label : _Labels)
                {
                    const FLOAT y = msc::Map(_GraphSettings->ScaleA(ToMagnitude(Label.Amplitude)), 0., 1., y2, y1);

                    // Draw the label.
                    r.top    = Label.IsMin ? y - _TextStyle->_Height : (Label.IsMax ? y : y - (_TextStyle->_Height / 2.f));
                    r.bottom = r.top + _TextStyle->_Height;

                    if (_HorizontalGridLineStyle->IsEnabled())
                        renderTarget->DrawLine(D2D1::Point2F(x1, y), D2D1::Point2F(x2, y), _HorizontalGridLineStyle->_Brush, _HorizontalGridLineStyle->_Thickness, nullptr);

                    if (_GraphSettings->_YAxisLeft)
                    {
                        r.left  = 0.f;
                        r.right = x1 - 2.f;

                        renderTarget->DrawText(Label.Text.c_str(), (UINT) Label.Text.size(), _TextStyle->_TextFormat, r, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
                    }

                    if (_GraphSettings->_YAxisRight)
                    {
                        r.right = _Size.width - 1;
                        r.left  = x2 + 2.f;

                        renderTarget->DrawText(Label.Text.c_str(), (UINT) Label.Text.size(), _TextStyle->_TextFormat, r, _TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
                    }
                }
            }
        }
    }

    // Draw the signal.
    renderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

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
                const FLOAT Baseline = ChannelHeight * ((FLOAT) i + ((_GraphSettings->_YAxisMode == YAxisMode::None) ? 0.5f : 1.0f));

                const FLOAT dx = (_Size.width - (YAxisWidth * YAxisCount)) / (FLOAT) FrameCount;
                FLOAT x = _GraphSettings->_YAxisLeft ? YAxisWidth : 0.f;

                for (t_uint32 j = 0; j < FrameCount; ++j)
                {
                    const FLOAT Value = (FLOAT) Scaler(Samples[(j * ChannelCount) + i]);

                    const FLOAT y = Baseline - (Value * ZoomFactor * ChannelHeight);

                    if (j == 0)
                        Sink->BeginFigure(D2D1::Point2F(x, y), D2D1_FIGURE_BEGIN_HOLLOW);
                    else
                        Sink->AddLine(D2D1::Point2F(x, y));

                    x += dx;
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
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::VerticalGridLine, renderTarget, Size, L"", &_VerticalGridLineStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::HorizontalGridLine, renderTarget, _Size, L"", &_LineStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::YAxisText, renderTarget, _Size, L"+999", &_TextStyle);

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
void oscilloscope_t::ReleaseDeviceSpecificResources() noexcept
{
#ifdef _DEBUG
    _DebugBrush.Release();
#endif

    if (_VerticalGridLineStyle)
    {
        _VerticalGridLineStyle->ReleaseDeviceSpecificResources();
        _VerticalGridLineStyle = nullptr;
    }

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

    if (_TextStyle)
    {
        _TextStyle->ReleaseDeviceSpecificResources();
        _TextStyle = nullptr;
    }

    if (_LineStyle)
    {
        _LineStyle->ReleaseDeviceSpecificResources();
        _LineStyle = nullptr;
    }
}
