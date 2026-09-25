
/** $VER: OscilloscopeXY.cpp (2026.09.23) P. Stuer - Implements an oscilloscope in X-Y mode. **/

#include <pch.h>

#include "OscilloscopeXY.h"

#include "Direct2D.h"

#pragma hdrstop

/// <summary>
/// Destroys this instance.
/// </summary>
oscilloscope_xy_t::~oscilloscope_xy_t() noexcept
{
    DeleteDeviceSpecificResources();
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void oscilloscope_xy_t::Move(const D2D1_RECT_F & rect) noexcept
{
//  Log.Write("*** " __FUNCTION__ );

    InitializeMetrics(rect);
}

/// <summary>
/// Initializes this instance.
/// </summary>
void oscilloscope_xy_t::Configure(state_t * state, graph_options_t * graphOptions, analysis_t * analysis, bool isFirst, bool isLast, ID3D11Device * d3dDevice, ID3D11DeviceContext * d3dDeviceContext) noexcept
{
    _State = state;
    _GraphOptions = graphOptions;
    _Analysis = analysis;

    _SquareBitmaps = true;

    DeleteDeviceSpecificResources();

    CreateDeviceIndependentResources();
}

/// <summary>
/// Renders this instance.
/// </summary>
void oscilloscope_xy_t::Render(ID2D1DeviceContext * deviceContext, IDXGISwapChain1 * swapChain) noexcept
{
    HRESULT hr = CreateDeviceSpecificResources(deviceContext);

    if (FAILED(hr))
        return;

    ComPtr<ID2D1TransformedGeometry> TransformedGeometry;

    // Create the signal.
    if (!_State->_IsPaused || (_State->_IsPaused && _State->_VisualizeDuringPause))
    {
        hr = CreateSignalGeometry(_Analysis->_Chunk, TransformedGeometry);

        if (FAILED(hr))
            return;
    }
    else
        TransformedGeometry.Reset();

    {
        const FLOAT Opacity = (_State->_Afterglow != 0.f) ? std::expf(-(1000.f / (FLOAT) _State->_RefreshRateLimit) / _State->_Afterglow) : 0.f;

        _OpacityEffect->SetValue(D2D1_OPACITY_PROP_OPACITY, Opacity);

        _BlurEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION, _State->_BlurSigma);
    }

    const size_t BitmapIndex = 1 - _PrevBitmapIndex;

    // Draw the signal in the composite buffer. Keep drawing even if no signal data is available to create the blur effect.
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
            if (TransformedGeometry)
            {
                FLOAT OldOpacity = _SignalLineStyle._Brush->GetOpacity();

                _SignalLineStyle._Brush->SetOpacity(OldOpacity * .25f);

                _DeviceContext->DrawGeometry(TransformedGeometry.Get(), _SignalLineStyle._Brush.Get(), _SignalLineStyle._Thickness * 3.f, _SignalStrokeStyle.Get());

                _SignalLineStyle._Brush->SetOpacity(OldOpacity);
            }
        }

        // Draw the new content.
        if (TransformedGeometry)
        {
            _DeviceContext->DrawGeometry(TransformedGeometry.Get(), _SignalLineStyle._Brush.Get(), _SignalLineStyle._Thickness, _SignalStrokeStyle.Get());
        }

        hr = _DeviceContext->EndDraw();

        if (FAILED(hr))
            return;
    }

    {
        // Draw the static content.
        {
            const auto Translate = D2D1::Matrix3x2F::Translation(_Rect.left + (_Size.width / 2.f), _Rect.top + (_Size.height / 2.f));
            const auto Rotate    = D2D1::Matrix3x2F::Rotation(_State->_Rotation, D2D1::Point2F(0.f, 0.f));

            deviceContext->SetTransform(Rotate * Translate);

            deviceContext->DrawImage(_StaticContent.Get());
        }

        // Draw the composite buffer to the window.
        {
            const auto Translate = D2D1::Matrix3x2F::Translation(_Rect.left + ((_Size.width - _Side) / 2.f), _Rect.top + ((_Size.height - _Side) / 2.f));

            deviceContext->SetTransform(Translate);

            deviceContext->DrawBitmap(_Bitmaps[BitmapIndex].Get());
        }

        deviceContext->SetTransform(D2D1::Matrix3x2F::Identity());
    }

    _PrevBitmapIndex = BitmapIndex;
}

/// <summary>
/// Creates the path geometry for the signal.
/// </summary>
HRESULT oscilloscope_xy_t::CreateSignalGeometry(const audio_chunk_impl & chunk, ComPtr<ID2D1TransformedGeometry> & transformedGeometry) noexcept
{
    size_t FrameCount = chunk.get_sample_count();                                               // get_sample_count() actually returns the number of frames.

    const uint32_t ChannelCount         = chunk.get_channel_count();
    const uint32_t AvailableChannelMask = chunk.get_channel_config();                           // Mask containing the channels in the audio chunk.
    const uint32_t ActiveChannelMask    = _GraphOptions->_ActiveChannelMask;                                // Mask containing the channels selected by the user.
    const uint32_t PairedChannelMask    = analysis_t::ChannelPairs[(size_t) _GraphOptions->_ChannelPair];   // Mask containing the channels selected by the user as a channel pair.

    const uint32_t ChannelMask = AvailableChannelMask & ActiveChannelMask & PairedChannelMask;

    if ((ChannelCount < 2) || (ChannelMask == 0))
        return S_FALSE;

    const audio_sample * Frames = chunk.get_data();

    if (_State->_ZeroCrossingTrigger && (FrameCount >= 4))
    {
        FrameCount /= 2;

        const size_t CrossIndex = FindZeroCrossing(Frames, FrameCount, ChannelCount);
        
        Frames += CrossIndex * ChannelCount;
    }

    if (FrameCount < 2)
        return S_FALSE;

    size_t Channel1 = (size_t) std::countr_zero(ChannelMask);         // Index of the channel 1 sample in the audio chunk.
    size_t Channel2 = (size_t) (31 - std::countl_zero(ChannelMask));  // Index of the channel 2 sample in the audio chunk.

    if (_GraphOptions->_SwapChannels)
        std::swap(Channel1, Channel2);

    // Create the signal geometry.
    {
        ComPtr<ID2D1PathGeometry> Geometry;

        HRESULT hr = _Direct2D.Factory->CreatePathGeometry(Geometry.GetAddressOf());

        if (SUCCEEDED(hr))
        {
            ComPtr<ID2D1GeometrySink> Sink;

            hr = Geometry->Open(Sink.GetAddressOf());

            auto x = (FLOAT) std::clamp(Frames[Channel1] * _State->_XInputGain, -1., 1.);
            auto y = (FLOAT) std::clamp(Frames[Channel2] * _State->_YInputGain, -1., 1.);

            Sink->BeginFigure(D2D1::Point2F(x, y), D2D1_FIGURE_BEGIN_HOLLOW);

            for (size_t i = ChannelCount; i < FrameCount; i += ChannelCount)
            {
                x = (FLOAT) std::clamp(Frames[Channel1 + i] * _State->_XInputGain, -1., 1.);
                y = (FLOAT) std::clamp(Frames[Channel2 + i] * _State->_YInputGain, -1., 1.);

                Sink->AddLine(D2D1::Point2F(x, y));
            }

            Sink->EndFigure(D2D1_FIGURE_END_OPEN);

            hr = Sink->Close();
        }

        if (SUCCEEDED(hr))
        {
            const auto Rotate = D2D1::Matrix3x2F::Rotation(_State->_Rotation, D2D1::Point2F(0.f, 0.f));

            hr = _Direct2D.Factory->CreateTransformedGeometry(Geometry.Get(), Rotate * _ScaleTransform * _TranslateTransform, transformedGeometry.GetAddressOf());
        }

        return hr;
    }
}

/// <summary>
/// Creates resources which are not bound to any D3D device. Their lifetime effectively extends for the duration of the app.
/// </summary>
HRESULT oscilloscope_xy_t::CreateDeviceIndependentResources() noexcept
{
    HRESULT hr = oscilloscope_base_t::CreateDeviceIndependentResources();

    return hr;
}

/// <summary>
/// Releases the device independent resources.
/// </summary>
void oscilloscope_xy_t::DeleteDeviceIndependentResources() noexcept
{
    oscilloscope_base_t::DeleteDeviceIndependentResources();
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT oscilloscope_xy_t::CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept
{
    if (_State->_ResizeResources)
        DeleteDeviceSpecificResources();

    oscilloscope_base_t::CreateDeviceSpecificResources(deviceContext);

    HRESULT hr = S_OK;

    if (_XAxisTextStyle._Brush == nullptr)
    {
        _XAxisTextStyle = *_State->_StyleManager.GetStyle(VisualElement::XAxisText);

        _XAxisTextStyle.SetColor(_State->_ArtworkDominantColor, _State->_ArtworkGradientStops, _State->_UserInterfaceColors);

        // The font style is created prescaled to counter the Scale transform in the command list.
        hr = _XAxisTextStyle.CreateDeviceSpecificResources(deviceContext, _Size, L"+0.0", _HalfSide);

        if (FAILED(hr))
            return hr;
    }

    if (_YAxisTextStyle._Brush == nullptr)
    {
        _YAxisTextStyle = *_State->_StyleManager.GetStyle(VisualElement::YAxisText);

        _YAxisTextStyle.SetColor(_State->_ArtworkDominantColor, _State->_ArtworkGradientStops, _State->_UserInterfaceColors);

        // The font style is created prescaled to counter the Scale transform in the command list.
        hr = _YAxisTextStyle.CreateDeviceSpecificResources(deviceContext, _Size, L"+0.0", _HalfSide);

        if (FAILED(hr))
            return hr;
    }

    if (_StaticContent == nullptr)
        hr = CreateStaticContent();

    return hr;
}

/// <summary>
/// Deletes the device specific resources.
/// </summary>
void oscilloscope_xy_t::DeleteDeviceSpecificResources() noexcept
{
    _StaticContent.Reset();

    _YAxisTextStyle.DeleteDeviceSpecificResources();
    _XAxisTextStyle.DeleteDeviceSpecificResources();

    oscilloscope_base_t::DeleteDeviceSpecificResources();
}

/// <summary>
/// Creates a command list to render the static content (grid, the X and Y axis labels).
/// This is created in a [-1, 1] axis setup and scaled up as necessary.
/// </summary>
HRESULT oscilloscope_xy_t::CreateStaticContent() noexcept
{
    HRESULT hr = S_OK;

    _TranslateTransform = D2D1::Matrix3x2F::Translation(_HalfSide, _HalfSide);
    _ScaleTransform     = D2D1::Matrix3x2F::Scale(D2D1::SizeF(_HalfSide, _HalfSide));

    // Create a command list that will store the grid pattern and the axes.
    if (SUCCEEDED(hr))
        hr = _DeviceContext->CreateCommandList(_StaticContent.GetAddressOf());

    if (SUCCEEDED(hr))
    {
        WCHAR Text[6] = { };

        _DeviceContext->SetTarget(_StaticContent.Get());

        _DeviceContext->BeginDraw();

        _DeviceContext->SetTransform(_ScaleTransform);
        _DeviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED); // Prevent line blurring

        // Draw the X-axis, Y-axis and the center label.
        {
            D2D1_RECT_F TextRect = { -1.f, 0.01f, 1.f, 1.f };

            if (_GraphOptions->HasXAxis() || _GraphOptions->HasYAxis())
            {
                _XAxisTextStyle.SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
                _XAxisTextStyle.SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

                _DeviceContext->DrawText(L"0.0", 3, _XAxisTextStyle._TextFormat.Get(), TextRect, _XAxisTextStyle._Brush.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }

            if (_GraphOptions->HasXAxis())
                _DeviceContext->DrawLine(D2D1::Point2F(-1.f,  0.f), D2D1::Point2F(1.f, 0.f), _XAxisLineStyle._Brush.Get(), 1.f, _StaticStrokeStyle.Get());

            if (_GraphOptions->HasYAxis())
                _DeviceContext->DrawLine(D2D1::Point2F( 0.f, -1.f), D2D1::Point2F(0.f, 1.f), _YAxisLineStyle._Brush.Get(), 1.f, _StaticStrokeStyle.Get());
        }

        _XAxisTextStyle.SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

        for (FLOAT x = .2f; x < 1.01f; x += .2f)
        {
            // Draw the vertical grid line.
            if (_VerticalGridLineStyle.IsEnabled())
            {
                _DeviceContext->DrawLine(D2D1::Point2F( x, -1.f), D2D1::Point2F( x, 1.f), _VerticalGridLineStyle._Brush.Get(), 1.f, _StaticStrokeStyle.Get());
                _DeviceContext->DrawLine(D2D1::Point2F(-x, -1.f), D2D1::Point2F(-x, 1.f), _VerticalGridLineStyle._Brush.Get(), 1.f, _StaticStrokeStyle.Get());
            }

            if (_GraphOptions->HasXAxis())
            {
                // Draw the negative X label.
                _XAxisTextStyle.SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);

                D2D1_RECT_F TextRect = { -x + 0.01f, 0.01f, 0.f, 1.f };

                ::StringCchPrintfW(Text, _countof(Text), L"%-.1f", -x);
                _DeviceContext->DrawText(Text, (UINT32) ::wcslen(Text), _XAxisTextStyle._TextFormat.Get(), TextRect, _XAxisTextStyle._Brush.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);

                // Draw the positive X label.
                _XAxisTextStyle.SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);

                TextRect = { 0.f, 0.01f, x - 0.01f, 1.f };

                ::StringCchPrintfW(Text, _countof(Text), L"%+.1f", x);
                _DeviceContext->DrawText(Text, (UINT32) ::wcslen(Text), _XAxisTextStyle._TextFormat.Get(), TextRect, _XAxisTextStyle._Brush.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }
        }

        if (!_GraphOptions->HasXAxis())
            _DeviceContext->DrawLine(D2D1::Point2F(0.f, -1.f), D2D1::Point2F(0.f, 1.f), _VerticalGridLineStyle._Brush.Get(), 1.f, _StaticStrokeStyle.Get());

        _YAxisTextStyle.SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);

        for (FLOAT y = .2f; y < 1.01f; y += .2f)
        {
            // Draw the horizontal grid line.
            if (_HorizontalGridLineStyle.IsEnabled())
            {
                _DeviceContext->DrawLine(D2D1::Point2F(-1.f,  y), D2D1::Point2F(1.f,  y), _HorizontalGridLineStyle._Brush.Get(), 1.f, _StaticStrokeStyle.Get());
                _DeviceContext->DrawLine(D2D1::Point2F(-1.f, -y), D2D1::Point2F(1.f, -y), _HorizontalGridLineStyle._Brush.Get(), 1.f, _StaticStrokeStyle.Get());
            }

            if (_GraphOptions->HasYAxis())
            {
                // Draw the negative y label.
                _YAxisTextStyle.SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);

                D2D1_RECT_F TextRect = { 0.01f, y - 0.01f, 1.f, -1.f };

                ::StringCchPrintfW(Text, _countof(Text), L"%-.1f", -y);
                _DeviceContext->DrawText(Text, (UINT32) ::wcslen(Text), _YAxisTextStyle._TextFormat.Get(), TextRect, _YAxisTextStyle._Brush.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);

                // Draw the positive y label.
                _YAxisTextStyle.SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

                TextRect = { 0.01f, -y + 0.01f, 1.f, 0.f };

                ::StringCchPrintfW(Text, _countof(Text), L"%+.1f", y);
                _DeviceContext->DrawText(Text, (UINT32) ::wcslen(Text), _YAxisTextStyle._TextFormat.Get(), TextRect, _YAxisTextStyle._Brush.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
            }
        }

        if (!_GraphOptions->HasYAxis())
            _DeviceContext->DrawLine(D2D1::Point2F(-1.f, 0.f), D2D1::Point2F(1.f, 0.f), _HorizontalGridLineStyle._Brush.Get(), 1.f, _StaticStrokeStyle.Get());

        _DeviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

        _DeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());

        hr = _DeviceContext->EndDraw();
    }

    if (SUCCEEDED(hr))
        hr = _StaticContent->Close();

    return hr;
}
