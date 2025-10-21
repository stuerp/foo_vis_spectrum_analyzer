
/** $VER: Oscilloscope.cpp (2025.10.17) P. Stuer - Implements an oscilloscope. **/

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

    _BackBuffer.Release();
    _FrontBuffer.Release();

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

    HRESULT hr = CreateDeviceSpecificResources(deviceContext);

    if (!SUCCEEDED(hr))
        return;

    const FLOAT YAxisWidth = _YAxisTextStyle->_Width;

    FLOAT XOffset = 0.f;
    FLOAT YAxisCount = 0.f;

    if (_GraphSettings->HasYAxis())
    {
        XOffset = YAxisWidth;

        ++YAxisCount;
    }

    if (_GraphSettings->HasYAxis())
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
        // Draw the axes.
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
                _DeviceContext->Clear(D2D1::ColorF(D2D1::ColorF(0.f, 0.f, 0.f, 0.f)));

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
    HRESULT hr = S_OK;

    const D2D1_STROKE_STYLE_PROPERTIES StrokeStyleProperties = D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT, D2D1_LINE_JOIN_BEVEL);

    if (_SignalStrokeStyle == nullptr)
        hr = _Direct2D.Factory->CreateStrokeStyle(StrokeStyleProperties, nullptr, 0, &_SignalStrokeStyle);

    // Create a brush stroke style for the grid that remains fixed during the scaling transformation.
    if (SUCCEEDED(hr))
    {
        D2D1_STROKE_STYLE_PROPERTIES1 Properties = D2D1::StrokeStyleProperties1();

        Properties.transformType = D2D1_STROKE_TRANSFORM_TYPE_FIXED; // Prevent stroke scaling

        hr = _Direct2D.Factory->CreateStrokeStyle(Properties, nullptr, 0, &_AxisStrokeStyle);
    }

    return hr;
}

/// <summary>
/// Releases the device independent resources.
/// </summary>
void oscilloscope_t::DeleteDeviceIndependentResources() noexcept
{
    _AxisStrokeStyle.Release();

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
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::XAxisLine, deviceContext, _Size, L"", 1.f, &_XAxisLineStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::YAxisLine, deviceContext, _Size, L"", 1.f, &_YAxisLineStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::HorizontalGridLine, deviceContext, _Size, L"", 1.f, &_HorizontalGridLineStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::VerticalGridLine, deviceContext, _Size, L"", 1.f, &_VerticalGridLineStyle);

    if (SUCCEEDED(hr) && (_DeviceContext == nullptr))
    {
        CComPtr<ID2D1Device> D2DDevice;

        deviceContext->GetDevice(&D2DDevice);

        hr = D2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_ENABLE_MULTITHREADED_OPTIMIZATIONS, &_DeviceContext);
    }

    if (SUCCEEDED(hr))
    {
        const D2D1_BITMAP_PROPERTIES1 BitmapProperties = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE));

        if (_FrontBuffer == nullptr)
        {
            hr = deviceContext->CreateBitmap(D2D1::SizeU((UINT32) _Size.width, (UINT32) _Size.height), nullptr, 0, &BitmapProperties, &_FrontBuffer);

            if (SUCCEEDED(hr))
            {
                _DeviceContext->SetTarget(_FrontBuffer);

                _DeviceContext->BeginDraw();

                _DeviceContext->Clear(D2D1::ColorF(D2D1::ColorF::Black));

                hr = _DeviceContext->EndDraw();

                _DeviceContext->SetTarget(nullptr);
            }
        }

        if (_BackBuffer == nullptr)
        {
            hr = _DeviceContext->CreateBitmap(D2D1::SizeU((UINT32) _Size.width, (UINT32) _Size.height), nullptr, 0, &BitmapProperties, &_BackBuffer);

            if (SUCCEEDED(hr))
            {
                _DeviceContext->SetTarget(_BackBuffer);

                _DeviceContext->BeginDraw();

                _DeviceContext->Clear(D2D1::ColorF(D2D1::ColorF::Black));

                hr = _DeviceContext->EndDraw();

                _DeviceContext->SetTarget(nullptr);
            }
        }
    }

    // The font style is created prescaled to counter the Scale transform in the command list.
    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::XAxisText, _DeviceContext, _Size, L"-999", 1.f, &_XAxisTextStyle);

    // The font style is created prescaled to counter the Scale transform in the command list.
    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::YAxisText, _DeviceContext, _Size, L"-999", 1.f, &_YAxisTextStyle);

    if (SUCCEEDED(hr) && (_GaussBlurEffect == nullptr))
    {
        hr = _DeviceContext->CreateEffect(CLSID_D2D1GaussianBlur, &_GaussBlurEffect);

        if (SUCCEEDED(hr))
        {
            _GaussBlurEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION, _State->_BlurSigma);
            _GaussBlurEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_OPTIMIZATION, D2D1_DIRECTIONALBLUR_OPTIMIZATION_QUALITY);
            _GaussBlurEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_BORDER_MODE, D2D1_BORDER_MODE_HARD);
        }
    }

    if (SUCCEEDED(hr) && (_ColorMatrixEffect == nullptr))
    {
        hr = _DeviceContext->CreateEffect(CLSID_D2D1ColorMatrix, &_ColorMatrixEffect);

        if (SUCCEEDED(hr))
        {
            // Color matrix for uniform decay
            #pragma warning(disable: 5246) // 'anonymous struct or union': the initialization of a subobject should be wrapped in braces
            const D2D1_MATRIX_5X4_F DecayMatrix =
            {
                _State->_DecayFactor, 0, 0, 0,
                0, _State->_DecayFactor, 0, 0,
                0, 0, _State->_DecayFactor, 0,
                0, 0, 0, 1,
                0, 0, 0, 0
            };

            _ColorMatrixEffect->SetValue(D2D1_COLORMATRIX_PROP_COLOR_MATRIX, DecayMatrix);
        }
    }

    if (SUCCEEDED(hr) && (_AxesCommandList == nullptr))
        hr = CreateAxesCommandList();

#ifdef _DEBUG
    if (SUCCEEDED(hr) && (_DebugBrush == nullptr))
        deviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), &_DebugBrush);
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

    _ColorMatrixEffect.Release();

    _GaussBlurEffect.Release();

    _BackBuffer.Release();

    _FrontBuffer.Release();

    _DeviceContext.Release();
}

/// <summary>
/// Creates the path geometry for the signal.
/// </summary>
HRESULT oscilloscope_t::CreateSignalGeometry(const D2D1_SIZE_F & clientSize, CComPtr<ID2D1PathGeometry> & Geometry) noexcept
{
    amplitude_scaler_t Scaler;

    switch (_GraphSettings->_YAxisMode)
    {
        case YAxisMode::None:
            Scaler.SetNormalizedMode();
            break;

        case YAxisMode::Decibels:
            Scaler.SetDecibelMode(_GraphSettings->_AmplitudeLo, _GraphSettings->_AmplitudeHi);
            break;

        case YAxisMode::Linear:
            Scaler.SetLinearMode(_GraphSettings->_AmplitudeLo, _GraphSettings->_AmplitudeHi, _GraphSettings->_Gamma, _GraphSettings->_UseAbsolute);
            break;
    }

    const size_t FrameCount     = _Analysis->_Chunk.get_sample_count();    // get_sample_count() actually returns the number of frames.
    const uint32_t ChannelCount = _Analysis->_Chunk.get_channel_count();

    const audio_sample * Samples = _Analysis->_Chunk.get_data();

    uint32_t ChunkChannels    = _Analysis->_Chunk.get_channel_config(); // Mask containing the channels in the audio chunk.
    uint32_t SelectedChannels = _GraphSettings->_SelectedChannels;      // Mask containing the channels selected by the user.

    const size_t SelectedChannelCount = (size_t) std::popcount(ChunkChannels & _GraphSettings->_SelectedChannels);
    const FLOAT ChannelHeight = clientSize.height / (FLOAT) SelectedChannelCount; // Height available to one channel.
    const FLOAT ChannelMax = ChannelHeight * (_GraphSettings->HasYAxis() ? 1.0f : 0.5f);

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
    const size_t SelectedChannelCount = (size_t) std::popcount(_Analysis->_Chunk.get_channel_config() & _GraphSettings->_SelectedChannels);
    const FLOAT ChannelHeight = _Size.height / (FLOAT) SelectedChannelCount; // Height available to one channel.
    const FLOAT YAxisWidth = _YAxisTextStyle->_Width;

    const FLOAT x1 = 0.f         + ((_GraphSettings->HasYAxis() && _GraphSettings->_YAxisLeft)  ? YAxisWidth : 0.f);
    const FLOAT x2 = _Size.width - ((_GraphSettings->HasYAxis() && _GraphSettings->_YAxisRight) ? YAxisWidth : 0.f);

    // Create a command list that will store the grid pattern and the axes.
    HRESULT hr = _DeviceContext->CreateCommandList(&_AxesCommandList);

    if (SUCCEEDED(hr))
    {
        _DeviceContext->SetTarget(_AxesCommandList);
        _DeviceContext->BeginDraw();

        _DeviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED); // Prevent line blurring

        // Y-axis
        if (_GraphSettings->HasYAxis())
        {
            _YAxisTextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
            _YAxisTextStyle->SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

            FLOAT y1 = 0.f;
            FLOAT y2 = ChannelHeight;

            D2D1_RECT_F TextRect = { };

            for (uint32_t i = 0; i < SelectedChannelCount; ++i)
            {
                if (_GraphSettings->_YAxisLeft)
                    _DeviceContext->DrawLine(D2D1::Point2F(YAxisWidth, y1), D2D1::Point2F(YAxisWidth, y2), _YAxisLineStyle->_Brush, _YAxisLineStyle->_Thickness, nullptr);

                if (_GraphSettings->_YAxisRight)
                    _DeviceContext->DrawLine(D2D1::Point2F(_Size.width - (YAxisWidth - 1.f), y1), D2D1::Point2F(_Size.width - (YAxisWidth - 1.f), y2), _YAxisLineStyle->_Brush, _YAxisLineStyle->_Thickness, nullptr);

                if (_YAxisTextStyle->IsEnabled())
                {
                    for (const label_t & Label : _Labels)
                    {
                        const FLOAT y = msc::Map(_GraphSettings->ScaleAmplitude(ToMagnitude(Label.Amplitude)), 0., 1., y2, y1);

                        if (_HorizontalGridLineStyle->IsEnabled())
                            _DeviceContext->DrawLine(D2D1::Point2F(x1, y), D2D1::Point2F(x2, y), _HorizontalGridLineStyle->_Brush, _HorizontalGridLineStyle->_Thickness, _AxisStrokeStyle);

                        TextRect.top    = Label.IsMin ? y - _YAxisTextStyle->_Height : (Label.IsMax ? y : y - (_YAxisTextStyle->_Height / 2.f));
                        TextRect.bottom = TextRect.top + _YAxisTextStyle->_Height;

                        if (_GraphSettings->_YAxisLeft)
                        {
                            TextRect.left  = 0.f;
                            TextRect.right = YAxisWidth - 2.f;

                            _DeviceContext->DrawText(Label.Text.c_str(), (UINT) Label.Text.size(), _YAxisTextStyle->_TextFormat, TextRect, _YAxisTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
                        }

                        if (_GraphSettings->_YAxisRight)
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
        if (_GraphSettings->HasXAxis())
        {
            _XAxisTextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
            _XAxisTextStyle->SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_FAR);

            FLOAT y = ChannelHeight * (_GraphSettings->HasYAxis() ? 1.0f : 0.5f);

            D2D1_RECT_F TextRect = { 0.f, 0.f, x2, 0.f };

            const int ChunkDuration = (int) (_Analysis->_Chunk.get_duration() * 1000.); // Convert to milliseconds

            int Time = ChunkDuration / 10;

            for (uint32_t i = 0; i < SelectedChannelCount; ++i)
            {
                if (_XAxisLineStyle->IsEnabled())
                    _DeviceContext->DrawLine(D2D1::Point2F(x1, y), D2D1::Point2F(x2, y), _XAxisLineStyle->_Brush, _XAxisLineStyle->_Thickness, _AxisStrokeStyle);

                if (_XAxisTextStyle->IsEnabled())
                {
                    TextRect.bottom = y;

                    FLOAT dx = (x2 - x1) / 10.f;

                    for (TextRect.left = x1 + dx; TextRect.left < x2; TextRect.left += dx)
                    {
                        WCHAR Text[8] = { };

                        ::swprintf_s(Text, _countof(Text), L"%3d ms", Time);

                        _DeviceContext->DrawText(Text, (UINT32) ::wcslen(Text), _XAxisTextStyle->_TextFormat, TextRect, _XAxisTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

                        Time += ChunkDuration / 10;
                    }
                }

                y += ChannelHeight;
            }
        }

        hr = _DeviceContext->EndDraw();
    }

    if (SUCCEEDED(hr))
        hr = _AxesCommandList->Close();

    return hr;
}
