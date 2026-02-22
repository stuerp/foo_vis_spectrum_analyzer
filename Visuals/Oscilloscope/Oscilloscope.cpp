
/** $VER: Oscilloscope.cpp (2026.02.22) P. Stuer - Implements an oscilloscope. **/

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
    _XAxisTextStyle = nullptr;
    _YAxisTextStyle = nullptr;

    _ChunkDuration = 0.;

    Reset();
}

/// <summary>
/// Destroys this instance.
/// </summary>
oscilloscope_t::~oscilloscope_t() noexcept
{
    DeleteDeviceSpecificResources();
}

/// <summary>
/// Initializes this instance.
/// </summary>
void oscilloscope_t::Initialize(state_t * state, const graph_description_t * settings, const analysis_t * analysis) noexcept
{
    _State = state;
    _Settings = settings;
    _Analysis = analysis;

    DeleteDeviceSpecificResources();

    // Create the labels.
    {
        for (double Amplitude = _Settings->_AmplitudeLo; Amplitude <= _Settings->_AmplitudeHi; Amplitude -= _Settings->_AmplitudeStep)
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
    SetRect(rect);
}

/// <summary>
/// Resets this instance.
/// </summary>
void oscilloscope_t::Reset() noexcept
{
    if (!_IsResized || (GetWidth() == 0.f) || (GetHeight() == 0.f))
        return;

    _IsResized = true;
}

/// <summary>
/// Recalculates parameters that are render target and size-sensitive.
/// </summary>
void oscilloscope_t::Resize() noexcept
{
    if (!_IsResized || (GetWidth() == 0.f) || (GetHeight() == 0.f))
        return;

    oscilloscope_base_t::Resize();

    // Release resources that are size dependent.
    if (_XAxisTextStyle)
    {
        _XAxisTextStyle->DeleteDeviceSpecificResources();
        _XAxisTextStyle = nullptr;
    }

    if (_YAxisTextStyle)
    {
        _YAxisTextStyle->DeleteDeviceSpecificResources();
        _YAxisTextStyle = nullptr;
    }

    _AxesCommandList.Release();

    _IsResized = false;
}

/// <summary>
/// Renders this instance.
/// </summary>
void oscilloscope_t::Render(ID2D1DeviceContext * deviceContext) noexcept
{
    const size_t FrameCount     = _Analysis->_Chunk.get_sample_count();     // get_sample_count() actually returns the number of frames.
    const uint32_t ChannelCount = _Analysis->_Chunk.get_channel_count();

    // Bail out if no audio is playing. We need the channel count and configuration to draw the axes.
    if ((FrameCount == 0) || (ChannelCount == 0))
        return;

    if (_ChunkDuration != _Analysis->_Chunk.get_duration())
        _AxesCommandList.Release();

    HRESULT hr = CreateDeviceSpecificResources(deviceContext);

    if (!SUCCEEDED(hr))
        return;

    const FLOAT YAxisWidth = _YAxisTextStyle->_Width;

    FLOAT XOffset = 0.f;
    FLOAT YAxisCount = 0.f;

    if (_Settings->HasYAxis() && _Settings->_YAxisLeft)
    {
        XOffset = YAxisWidth;

        ++YAxisCount;
    }

    if (_Settings->HasYAxis() && _Settings->_YAxisRight)
        ++YAxisCount;

    const D2D1_SIZE_F SignalSize = { _Size.width - (YAxisWidth * YAxisCount), _Size.height };

    CComPtr<ID2D1PathGeometry> Geometry;

    hr = CreateSignalGeometry(SignalSize, Geometry);

    if (SUCCEEDED(hr))
    {
        _DeviceContext->SetTarget(_BackBuffer);
        _DeviceContext->BeginDraw();

        const D2D1_MATRIX_3X2_F Translate = D2D1::Matrix3x2F::Translation(XOffset, 0.0f);

        _DeviceContext->SetTransform(Translate);

        // Set a clip region to prevent the anti-aliasing from spilling into the axis rectangle.
        _DeviceContext->PushAxisAlignedClip({ 0.f, 0.f, SignalSize.width, SignalSize.height }, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

        _DeviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

        _DeviceContext->DrawGeometry(Geometry, _SignalLineStyle->_Brush, _SignalLineStyle->_Thickness, _SignalStrokeStyle);

        _DeviceContext->PopAxisAlignedClip();

        _DeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());

        hr = _DeviceContext->EndDraw();
    }

    if (SUCCEEDED(hr))
    {
        // Draw the axes to the window.
        {
            deviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

            deviceContext->SetTransform(D2D1::Matrix3x2F::Identity());

            deviceContext->DrawImage(_AxesCommandList);
        }

        // Draw the back buffer to the window.
        {
            deviceContext->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_ADD);

            deviceContext->DrawBitmap(_BackBuffer);

            deviceContext->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_SOURCE_OVER);
        }

        // Add the phosphor afterglow effect before the next pass.
        {
            _DeviceContext->SetTarget(_FrontBuffer);
            _DeviceContext->BeginDraw();

            if (_State->_PhosphorDecay)
            {
                _GaussBlurEffect->SetInput(0, _BackBuffer);

                _DeviceContext->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_ADD);
                _DeviceContext->DrawImage(_GaussBlurEffect);

                _ColorMatrixEffect->SetInput(0, _BackBuffer);

                _DeviceContext->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_SOURCE_OVER);
                _DeviceContext->DrawImage(_ColorMatrixEffect);
            }
            else
                _DeviceContext->Clear(); // Required for alpha transparency.

            hr = _DeviceContext->EndDraw();
        }

        std::swap(_FrontBuffer, _BackBuffer);
    }

    deviceContext->SetTransform(D2D1::Matrix3x2F::Identity());
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
    HRESULT hr = S_OK;

    if (SUCCEEDED(hr))
        Resize();

    if (SUCCEEDED(hr))
        hr = oscilloscope_base_t::CreateDeviceSpecificResources(deviceContext);

    // The font style is created prescaled to counter the Scale transform in the command list.
    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::XAxisText, _DeviceContext, _Size, L"-999", 1.f, &_XAxisTextStyle);

    // The font style is created prescaled to counter the Scale transform in the command list.
    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::YAxisText, _DeviceContext, _Size, L"-999", 1.f, &_YAxisTextStyle);

    if (SUCCEEDED(hr) && (_AxesCommandList == nullptr))
        hr = CreateAxesCommandList();

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void oscilloscope_t::DeleteDeviceSpecificResources() noexcept
{
    _AxesCommandList.Release();

    if (_YAxisTextStyle)
    {
        _YAxisTextStyle->DeleteDeviceSpecificResources();
        _YAxisTextStyle = nullptr;
    }

    if (_XAxisTextStyle)
    {
        _XAxisTextStyle->DeleteDeviceSpecificResources();
        _XAxisTextStyle = nullptr;
    }

    oscilloscope_base_t::DeleteDeviceSpecificResources();
}

/// <summary>
/// Creates the path geometry for the signal.
/// </summary>
HRESULT oscilloscope_t::CreateSignalGeometry(const D2D1_SIZE_F & clientSize, CComPtr<ID2D1PathGeometry> & Geometry) noexcept
{
    amplitude_scaler_t Scaler;

    switch (_Settings->_YAxisMode)
    {
        case YAxisMode::None:
            Scaler.SetNormalizedMode();
            break;

        case YAxisMode::Decibels:
            Scaler.SetDecibelMode(_Settings->_AmplitudeLo, _Settings->_AmplitudeHi);
            break;

        case YAxisMode::Linear:
            Scaler.SetLinearMode(_Settings->_AmplitudeLo, _Settings->_AmplitudeHi, _Settings->_Gamma, _Settings->_UseAbsolute);
            break;
    }

    const size_t FrameCount     = _Analysis->_Chunk.get_sample_count();    // get_sample_count() actually returns the number of frames.
    const uint32_t ChannelCount = _Analysis->_Chunk.get_channel_count();

    const audio_sample * Samples = _Analysis->_Chunk.get_data();

    uint32_t ChunkChannels    = _Analysis->_Chunk.get_channel_config(); // Mask containing the channels in the audio chunk.
    uint32_t SelectedChannels = _Settings->_SelectedChannels;      // Mask containing the channels selected by the user.

    const size_t SelectedChannelCount = (size_t) std::popcount(ChunkChannels & _Settings->_SelectedChannels);
    const FLOAT ChannelHeight = clientSize.height / (FLOAT) SelectedChannelCount; // Height available to one channel.
    const FLOAT ChannelMax = ChannelHeight * (_Settings->HasYAxis() ? 1.0f : 0.5f);

    HRESULT hr = _Direct2D.Factory->CreatePathGeometry(&Geometry);

    if (SUCCEEDED(hr))
    {
        CComPtr<ID2D1GeometrySink> Sink;

        hr = Geometry->Open(&Sink);

        if (FAILED(hr))
            return hr;

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
                    const FLOAT dx = clientSize.width / (FLOAT) FrameCount;

                    FLOAT x = 0.f;
                    FLOAT y = ChannelBaseline - (std::clamp((FLOAT) (Scaler(Samples[ChannelOffset]) * _State->_YGain), -1.f, 1.f) * ChannelMax);

                    Sink->BeginFigure(D2D1::Point2F(x, y), D2D1_FIGURE_BEGIN_HOLLOW);

                    for (size_t j = ChannelCount + ChannelOffset; j < SampleCount; j += ChannelCount)
                    {
                        x += dx;
                        y = ChannelBaseline - (std::clamp((FLOAT) (Scaler(Samples[j]) * _State->_YGain), -1.f, 1.f) * ChannelMax);

                        Sink->AddLine(D2D1::Point2F(x, y));
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

    return hr;
}

/// <summary>
/// Creates a command list to render the grid and the X and Y axis labels.
/// </summary>
HRESULT oscilloscope_t::CreateAxesCommandList() noexcept
{
    const size_t SelectedChannelCount = (size_t) std::popcount(_Analysis->_Chunk.get_channel_config() & _Settings->_SelectedChannels);
    const FLOAT ChannelHeight = _Size.height / (FLOAT) SelectedChannelCount; // Height available to one channel.
    const FLOAT YAxisWidth = _YAxisTextStyle->_Width;

    const FLOAT x1 = 0.f         + ((_Settings->HasYAxis() && _Settings->_YAxisLeft)  ? YAxisWidth : 0.f);
    const FLOAT x2 = _Size.width - ((_Settings->HasYAxis() && _Settings->_YAxisRight) ? YAxisWidth : 0.f);

    // Create a command list that will store the grid pattern and the axes.
    HRESULT hr = _DeviceContext->CreateCommandList(&_AxesCommandList);

    if (SUCCEEDED(hr))
    {
        _DeviceContext->SetTarget(_AxesCommandList);
        _DeviceContext->BeginDraw();

        _DeviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED); // Prevent line blurring

        // Y-axis
        if (_Settings->HasYAxis())
        {
            _YAxisTextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
            _YAxisTextStyle->SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

            FLOAT y1 = 0.f;
            FLOAT y2 = ChannelHeight;

            D2D1_RECT_F TextRect = { };

            for (uint32_t i = 0; i < SelectedChannelCount; ++i)
            {
                if (_Settings->_YAxisLeft)
                    _DeviceContext->DrawLine(D2D1::Point2F(YAxisWidth, y1), D2D1::Point2F(YAxisWidth, y2), _YAxisLineStyle->_Brush, _YAxisLineStyle->_Thickness, nullptr);

                if (_Settings->_YAxisRight)
                    _DeviceContext->DrawLine(D2D1::Point2F(_Size.width - (YAxisWidth - 1.f), y1), D2D1::Point2F(_Size.width - (YAxisWidth - 1.f), y2), _YAxisLineStyle->_Brush, _YAxisLineStyle->_Thickness, nullptr);

                if (_YAxisTextStyle->IsEnabled())
                {
                    for (const label_t & Label : _Labels)
                    {
                        const FLOAT y = msc::Map(_Settings->ScaleAmplitude(ToMagnitude(Label.Amplitude)), 0., 1., y2, y1);

                        if (_HorizontalGridLineStyle->IsEnabled())
                            _DeviceContext->DrawLine(D2D1::Point2F(x1, y), D2D1::Point2F(x2, y), _HorizontalGridLineStyle->_Brush, _HorizontalGridLineStyle->_Thickness, _AxisStrokeStyle);

                        TextRect.top    = Label.IsMin ? y - _YAxisTextStyle->_Height : (Label.IsMax ? y : y - (_YAxisTextStyle->_Height / 2.f));
                        TextRect.bottom = TextRect.top + _YAxisTextStyle->_Height;

                        if (_Settings->_YAxisLeft)
                        {
                            TextRect.left  = 0.f;
                            TextRect.right = YAxisWidth - 2.f;

                            _DeviceContext->DrawText(Label.Text.c_str(), (UINT) Label.Text.size(), _YAxisTextStyle->_TextFormat, TextRect, _YAxisTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
                        }

                        if (_Settings->_YAxisRight)
                        {
                            TextRect.left  = x2 + 2.f;
                            TextRect.right = _Size.width - 1.f;

                            _DeviceContext->DrawText(Label.Text.c_str(), (UINT) Label.Text.size(), _YAxisTextStyle->_TextFormat, TextRect, _YAxisTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
                        }
                    }
                }

                y1  = y2;
                y2 += ChannelHeight;
            }
        }

        // X-axis
        if (_Settings->HasXAxis())
        {
            _XAxisTextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
            _XAxisTextStyle->SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);

            FLOAT y = ChannelHeight * (_Settings->HasYAxis() ? 1.0f : 0.5f);

            FLOAT y1 = 0;
            FLOAT y2 = ChannelHeight;

            D2D1_RECT_F TextRect = { 0.f, 0.f, x2, 0.f };

            _ChunkDuration = _Analysis->_Chunk.get_duration();

            const int dt = (int) (_ChunkDuration * 100.); // Convert to 10-milliseconds units

            for (uint32_t i = 0; i < SelectedChannelCount; ++i)
            {
                int Time = dt;

                if (_XAxisLineStyle->IsEnabled())
                    _DeviceContext->DrawLine(D2D1::Point2F(x1, y), D2D1::Point2F(x2, y), _XAxisLineStyle->_Brush, _XAxisLineStyle->_Thickness, _AxisStrokeStyle);

                if (_XAxisTextStyle->IsEnabled())
                {
                    TextRect.bottom = y;

                    const FLOAT dx = (x2 - x1) / 10.f;

                    for (TextRect.left = x1 + dx; TextRect.left < x2; TextRect.left += dx)
                    {
                        if (_VerticalGridLineStyle->IsEnabled())
                            _DeviceContext->DrawLine(D2D1::Point2F(TextRect.left, y1), D2D1::Point2F(TextRect.left, y2), _VerticalGridLineStyle->_Brush, _VerticalGridLineStyle->_Thickness, _AxisStrokeStyle);

                        WCHAR Text[8] = { };

                        ::swprintf_s(Text, _countof(Text), L"%3d ms", Time);

                        _DeviceContext->DrawText(Text, (UINT32) ::wcslen(Text), _XAxisTextStyle->_TextFormat, TextRect, _XAxisTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

                        Time += dt;
                    }
                }

                y += ChannelHeight;

                y1 += ChannelHeight;
                y2 += ChannelHeight;
            }
        }

        hr = _DeviceContext->EndDraw();
    }

    if (SUCCEEDED(hr))
        hr = _AxesCommandList->Close();

    return hr;
}
