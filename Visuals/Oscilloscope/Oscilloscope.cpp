
/** $VER: Oscilloscope.cpp (2026.09.25) P. Stuer - Implements an oscilloscope. **/

#include <pch.h>

#include "Oscilloscope.h"

#include <Analyzers/AmplitudeScaler.h>

#include "Support.h"

#include "Direct2D.h"

#pragma hdrstop

/// <summary>
/// Destroys this instance.
/// </summary>
oscilloscope_t::~oscilloscope_t() noexcept
{
    DeleteDeviceSpecificResources();
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void oscilloscope_t::Move(const D2D1_RECT_F & rect) noexcept
{
//  Log.Write("*** " __FUNCTION__ );

    InitializeMetrics(rect);
}

/// <summary>
/// Initializes this instance.
/// </summary>
void oscilloscope_t::Configure(state_t * state, graph_options_t * graphOptions, analysis_t * analysis, bool isFirst, bool isLast, ID3D11Device * d3dDevice, ID3D11DeviceContext * d3dDeviceContext) noexcept
{
    _State = state;
    _GraphOptions = graphOptions;
    _Analysis = analysis;

    _SquareBitmaps = false;

    DeleteDeviceSpecificResources();

    // Create the labels.
    {
        _Labels.clear();

        WCHAR Text[16] = { };

        for (double Amplitude = _GraphOptions->_AmplitudeLo; Amplitude <= _GraphOptions->_AmplitudeHi; Amplitude -= _GraphOptions->_AmplitudeStep)
        {
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
/// Renders this instance.
/// </summary>
void oscilloscope_t::Render(ID2D1DeviceContext * deviceContext, IDXGISwapChain1 * swapChain) noexcept
{
    {
        const size_t FrameCount     = _Analysis->_Chunk.get_sample_count();     // get_sample_count() actually returns the number of frames.
        const uint32_t ChannelCount = _Analysis->_Chunk.get_channel_count();

        // Bail out if no audio is playing. We need the channel count and configuration to draw the axes.
        if ((FrameCount == 0) || (ChannelCount == 0))
            return;

        if (_GraphOptions->HasXAxis() && (_ChunkDuration != _Analysis->_Chunk.get_duration()))
        {
            _StaticContext.Reset();
            _AxesCount = 0;
        }
    }

    HRESULT hr = CreateDeviceSpecificResources(deviceContext);

    if (FAILED(hr))
        return;

    const FLOAT YAxisWidth = _YAxisTextStyle._Width;

    FLOAT XOffset = 0.f;
    FLOAT YAxisCount = 0.f;

    if (_GraphOptions->HasYAxis() && _GraphOptions->_YAxisLeft)
    {
        XOffset = YAxisWidth;

        ++YAxisCount;
    }

    if (_GraphOptions->HasYAxis() && _GraphOptions->_YAxisRight)
        ++YAxisCount;

    ComPtr<ID2D1PathGeometry> Geometry;

    // Create the signal.
    if (!_State->_IsPaused || (_State->_IsPaused && _State->_VisualizeDuringPause))
    {
        // Create the signal geometry.
        const D2D1_SIZE_F SignalSize = { _Size.width - (YAxisWidth * YAxisCount), _Size.height };

        {
            audio_chunk_impl DstChunk;

            const double Ratio = (double) _Analysis->_Chunk.get_sample_count() / (double) SignalSize.width;

            _Downsampler.Process(_Analysis->_Chunk, DstChunk, Ratio);

            hr = CreateSignalGeometry(DstChunk, SignalSize, Geometry);

            if (FAILED(hr))
                return;
        }
    }
    else
        Geometry.Reset();

    {
        const FLOAT Opacity = (_State->_Afterglow != 0.f) ? std::expf(-(1000.f / (FLOAT) _State->_RefreshRateLimit) / _State->_Afterglow) : 0.f;

        _OpacityEffect->SetValue(D2D1_OPACITY_PROP_OPACITY, Opacity);

        _BlurEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION, _State->_BlurSigma);
    }

    const size_t BitmapIndex = 1 - _PrevBitmapIndex;

    // Draw the signal in the composite buffer.
    {
        _DeviceContext->SetTarget(_Bitmaps[BitmapIndex].Get());

        _DeviceContext->BeginDraw();

        _DeviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

        _DeviceContext->Clear();

        if (_State->_HasPhosphorDecay)
        {
            // Draw a faded and blurred version of the previous bitmap.
            {
                _OpacityEffect->SetInput(0, _Bitmaps[_PrevBitmapIndex].Get());

                _DeviceContext->DrawImage(_BlurEffect.Get());
            }

            // Draw a faded and wide version of the signal.
            if (Geometry)
            {
                FLOAT OldOpacity = _SignalLineStyle._Brush->GetOpacity();

                _SignalLineStyle._Brush->SetOpacity(OldOpacity * .25f);

                _DeviceContext->DrawGeometry(Geometry.Get(), _SignalLineStyle._Brush.Get(), _SignalLineStyle._Thickness * 3.f, _SignalStrokeStyle.Get());

                _SignalLineStyle._Brush->SetOpacity(OldOpacity);
            }
        }

        // Draw a normal version of the signal.
        if (Geometry)
        {
            _DeviceContext->DrawGeometry(Geometry.Get(), _SignalLineStyle._Brush.Get(), _SignalLineStyle._Thickness, _SignalStrokeStyle.Get());
        }

        hr = _DeviceContext->EndDraw();
    }

    {
        // Draw the static content.
        {
            deviceContext->DrawImage(_StaticContext.Get());
        }

        // Draw the composite buffer to the window.
        {
            const D2D1_MATRIX_3X2_F Translate = D2D1::Matrix3x2F::Translation(XOffset, 0.f);

            deviceContext->SetTransform(Translate);

            deviceContext->DrawBitmap(_Bitmaps[BitmapIndex].Get());
        }

        deviceContext->SetTransform(D2D1::Matrix3x2F::Identity());
    }

    _PrevBitmapIndex = BitmapIndex;
}

/// <summary>
/// Creates resources which are not bound to any D3D device. Their lifetime effectively extends for the duration of the app.
/// </summary>
HRESULT oscilloscope_t::CreateDeviceIndependentResources() noexcept
{
    HRESULT hr = oscilloscope_base_t::CreateDeviceIndependentResources();

    return hr;
}

/// <summary>
/// Releases the device independent resources.
/// </summary>
void oscilloscope_t::DeleteDeviceIndependentResources() noexcept
{
    oscilloscope_base_t::DeleteDeviceIndependentResources();
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT oscilloscope_t::CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept
{
    if (_State->_ResizeResources)
        DeleteDeviceSpecificResources();

    HRESULT hr = oscilloscope_base_t::CreateDeviceSpecificResources(deviceContext);

    // The font style is created prescaled to counter the Scale transform in the command list.
    if (_XAxisTextStyle._Brush == nullptr)
    {
        _XAxisTextStyle = *_State->_StyleManager.GetStyle(VisualElement::XAxisText);

        _XAxisTextStyle.SetColor(_State->_ArtworkDominantColor, _State->_ArtworkGradientStops, _State->_UserInterfaceColors);

        hr = _XAxisTextStyle.CreateDeviceSpecificResources(deviceContext, _Size, L"-999", 1.f);

        if (FAILED(hr))
            return hr;
    }

    // The font style is created prescaled to counter the Scale transform in the command list.
    if (_YAxisTextStyle._Brush == nullptr)
    {
        _YAxisTextStyle = *_State->_StyleManager.GetStyle(VisualElement::YAxisText);

        _YAxisTextStyle.SetColor(_State->_ArtworkDominantColor, _State->_ArtworkGradientStops, _State->_UserInterfaceColors);

        hr = _YAxisTextStyle.CreateDeviceSpecificResources(deviceContext, _Size, L"-999", 1.f);

        if (FAILED(hr))
            return hr;
    }

    const uint32_t AxesCount = (size_t) _State->_Downmix ? 1u : std::popcount(_Analysis->_Chunk.get_channel_config() & _GraphOptions->_ActiveChannelMask);

    if ((_StaticContext == nullptr) || (_AxesCount != AxesCount))
        hr = CreateStaticContent(AxesCount);

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void oscilloscope_t::DeleteDeviceSpecificResources() noexcept
{
    _StaticContext.Reset();
    _AxesCount = 0;

    _YAxisTextStyle.DeleteDeviceSpecificResources();
    _XAxisTextStyle.DeleteDeviceSpecificResources();

    oscilloscope_base_t::DeleteDeviceSpecificResources();
}

/// <summary>
/// Creates the path geometry for the signal.
/// </summary>
HRESULT oscilloscope_t::CreateSignalGeometry(const audio_chunk_impl & chunk, const D2D1_SIZE_F & clientSize, ComPtr<ID2D1PathGeometry> & geometry) noexcept
{
    size_t FrameCount = chunk.get_sample_count();                   // get_sample_count() actually returns the number of frames.

    const uint32_t ChannelCount = chunk.get_channel_count();

    uint32_t AvailableChannelMask = chunk.get_channel_config();         // Mask containing the channels in the audio chunk.
    uint32_t ActiveChannelMask    = _GraphOptions->_ActiveChannelMask;  // Mask containing the channels selected by the user.

    const size_t ActiveChannelCount = (size_t) std::popcount(AvailableChannelMask & ActiveChannelMask);

    const FLOAT ChannelHeight = clientSize.height / (FLOAT) ActiveChannelCount; // Height available to one channel.
    const FLOAT ChannelMax    = ChannelHeight * (_GraphOptions->HasYAxis() ? 1.0f : 0.5f);

    const audio_sample * Frames = chunk.get_data();

    if (_State->_ZeroCrossingTrigger && (FrameCount >= 4))
    {
        FrameCount /= 2;

        const size_t CrossIndex = FindZeroCrossing(Frames, FrameCount, ChannelCount);
        
        Frames += CrossIndex * ChannelCount;
    }

    // Create the signal geometry.
    {
        amplitude_scaler_t Scaler;

        switch (_GraphOptions->_YAxisMode)
        {
            case YAxisMode::None:
                Scaler.SetNormalizedMode();
                break;

            case YAxisMode::Decibels:
                Scaler.SetDecibelMode(_GraphOptions->_AmplitudeLo, _GraphOptions->_AmplitudeHi);
                break;

            case YAxisMode::Linear:
                Scaler.SetLinearMode(_GraphOptions->_AmplitudeLo, _GraphOptions->_AmplitudeHi, _GraphOptions->_Gamma, _GraphOptions->_UseAbsolute);
                break;
        }

        HRESULT hr = Direct2DFactory::Get()->CreatePathGeometry(geometry.GetAddressOf());

        if (FAILED(hr))
            return hr;

        ComPtr<ID2D1GeometrySink> Sink;

        hr = geometry->Open(Sink.GetAddressOf());

        if (FAILED(hr))
            return hr;

        FLOAT ChannelBaseline = ChannelMax;
        size_t ChannelOffset = 0;
            
        while ((AvailableChannelMask != 0) && (ActiveChannelMask != 0))
        {
            // Render the signal if the channel is in the chunk and if it has been selected.
            if (AvailableChannelMask & 1)
            {
                if (ActiveChannelMask & 1)
                {
                    const size_t SampleCount = FrameCount * ChannelCount;
                    const FLOAT dx = clientSize.width / (FLOAT) FrameCount;

                    FLOAT x = 0.f;
                    FLOAT y = ChannelBaseline - (std::clamp((FLOAT) (Scaler(Frames[ChannelOffset]) * _State->_YInputGain), -1.f, 1.f) * ChannelMax);

                    Sink->BeginFigure(D2D1::Point2F(x, y), D2D1_FIGURE_BEGIN_HOLLOW);

                    for (size_t i = ChannelCount + ChannelOffset; i < SampleCount; i += ChannelCount)
                    {
                        x += dx;
                        y = ChannelBaseline - (std::clamp((FLOAT) (Scaler(Frames[i]) * _State->_YInputGain), -1.f, 1.f) * ChannelMax);

                        Sink->AddLine(D2D1::Point2F(x, y));
                    }

                    Sink->EndFigure(D2D1_FIGURE_END_OPEN);


                    ChannelBaseline += ChannelHeight;
                }

                ChannelOffset++;
            }

            AvailableChannelMask >>= 1;
            ActiveChannelMask >>= 1;
        }

        hr = Sink->Close();

        return hr;
    }
}

/// <summary>
/// Creates a command list to render the grid and the X and Y axis labels.
/// </summary>
HRESULT oscilloscope_t::CreateStaticContent(uint32_t axesCount) noexcept
{
    _StaticContext.Reset();

    const FLOAT ChannelHeight = _Size.height / (FLOAT) axesCount; // Height available to one channel.
    const FLOAT YAxisWidth = _YAxisTextStyle._Width;

    const FLOAT x1 = 0.f         + ((_GraphOptions->HasYAxis() && _GraphOptions->_YAxisLeft)  ? YAxisWidth : 0.f);
    const FLOAT x2 = _Size.width - ((_GraphOptions->HasYAxis() && _GraphOptions->_YAxisRight) ? YAxisWidth : 0.f);

    // Create a command list that will store the grid pattern and the axes.
    HRESULT hr = _DeviceContext->CreateCommandList(_StaticContext.GetAddressOf());

    if (FAILED(hr))
        return hr;

    _DeviceContext->SetTarget(_StaticContext.Get());

    _DeviceContext->BeginDraw();

    _DeviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED); // Prevent line blurring

    // Y-axis
    if (_GraphOptions->HasYAxis())
    {
        _YAxisTextStyle.SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
        _YAxisTextStyle.SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        FLOAT y1 = 0.f;
        FLOAT y2 = ChannelHeight;

        D2D1_RECT_F TextRect = { };

        for (uint32_t i = 0; i < axesCount; ++i)
        {
            if (_GraphOptions->_YAxisLeft)
                _DeviceContext->DrawLine(D2D1::Point2F(YAxisWidth, y1), D2D1::Point2F(YAxisWidth, y2), _YAxisLineStyle._Brush.Get(), _YAxisLineStyle._Thickness, nullptr);

            if (_GraphOptions->_YAxisRight)
                _DeviceContext->DrawLine(D2D1::Point2F(_Size.width - (YAxisWidth - 1.f), y1), D2D1::Point2F(_Size.width - (YAxisWidth - 1.f), y2), _YAxisLineStyle._Brush.Get(), _YAxisLineStyle._Thickness, nullptr);

            if (_YAxisTextStyle.IsEnabled())
            {
                for (const label_t & Label : _Labels)
                {
                    const FLOAT y = msc::Map(_GraphOptions->ScaleAmplitude(ToMagnitude(Label.Amplitude)), 0., 1., y2, y1);

                    if (_HorizontalGridLineStyle.IsEnabled())
                        _DeviceContext->DrawLine(D2D1::Point2F(x1, y), D2D1::Point2F(x2, y), _HorizontalGridLineStyle._Brush.Get(), _HorizontalGridLineStyle._Thickness, _StaticStrokeStyle.Get());

                    TextRect.top    = Label.IsMin ? y - _YAxisTextStyle._Height : (Label.IsMax ? y : y - (_YAxisTextStyle._Height / 2.f));
                    TextRect.bottom = TextRect.top + _YAxisTextStyle._Height;

                    if (_GraphOptions->_YAxisLeft)
                    {
                        TextRect.left  = 0.f;
                        TextRect.right = YAxisWidth - 2.f;

                        _DeviceContext->DrawText(Label.Text.c_str(), (UINT) Label.Text.size(), _YAxisTextStyle._TextFormat.Get(), TextRect, _YAxisTextStyle._Brush.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
                    }

                    if (_GraphOptions->_YAxisRight)
                    {
                        TextRect.left  = x2 + 2.f;
                        TextRect.right = _Size.width - 1.f;

                        _DeviceContext->DrawText(Label.Text.c_str(), (UINT) Label.Text.size(), _YAxisTextStyle._TextFormat.Get(), TextRect, _YAxisTextStyle._Brush.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
                    }
                }
            }

            y1  = y2;
            y2 += ChannelHeight;
        }
    }

    // X-axis
    if (_GraphOptions->HasXAxis())
    {
        _XAxisTextStyle.SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        _XAxisTextStyle.SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);

        FLOAT y = ChannelHeight * (_GraphOptions->HasYAxis() ? 1.0f : 0.5f);

        FLOAT y1 = 0;
        FLOAT y2 = ChannelHeight;

        D2D1_RECT_F TextRect = { 0.f, 0.f, x2, 0.f };

        _ChunkDuration = _Analysis->_Chunk.get_duration();

        const int dt = (int) (_ChunkDuration * 100.); // Convert to 10-milliseconds units

        for (uint32_t i = 0; i < axesCount; ++i)
        {
            int Time = dt;

            if (_XAxisLineStyle.IsEnabled())
                _DeviceContext->DrawLine(D2D1::Point2F(x1, y), D2D1::Point2F(x2, y), _XAxisLineStyle._Brush.Get(), _XAxisLineStyle._Thickness, _StaticStrokeStyle.Get());

            if (_XAxisTextStyle.IsEnabled())
            {
                TextRect.bottom = y;

                const FLOAT dx = (x2 - x1) / 10.f;

                for (TextRect.left = x1 + dx; TextRect.left < x2; TextRect.left += dx)
                {
                    if (_VerticalGridLineStyle.IsEnabled())
                        _DeviceContext->DrawLine(D2D1::Point2F(TextRect.left, y1), D2D1::Point2F(TextRect.left, y2), _VerticalGridLineStyle._Brush.Get(), _VerticalGridLineStyle._Thickness, _StaticStrokeStyle.Get());

                    WCHAR Text[8] = { };

                    ::StringCchPrintfW(Text, _countof(Text), L"%3d ms", Time);

                    _DeviceContext->DrawText(Text, (UINT32) ::wcslen(Text), _XAxisTextStyle._TextFormat.Get(), TextRect, _XAxisTextStyle._Brush.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);

                    Time += dt;
                }
            }

            y += ChannelHeight;

            y1 += ChannelHeight;
            y2 += ChannelHeight;
        }
    }

    (void) _DeviceContext->EndDraw();

    hr = _StaticContext->Close();

    _AxesCount = axesCount;

    return hr;
}
