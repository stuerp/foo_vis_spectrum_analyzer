
/** $VER: Oscilloscope.cpp (2025.10.12) P. Stuer - Implements an oscilloscope. **/

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

    _BackBuffer.Release();
    _FrontBuffer.Release();

    _GridCommandList.Release();

    _IsResized = false;
}

/// <summary>
/// Renders this instance.
/// </summary>
void oscilloscope_t::Render(ID2D1DeviceContext * deviceContext) noexcept
{
    if (_State->_XYMode)
        RenderXY(deviceContext);
    else
        RenderAmplitude(deviceContext);
}

/// <summary>
/// Renders the signal.
/// </summary>
void oscilloscope_t::RenderAmplitude(ID2D1DeviceContext * deviceContext) noexcept
{
    const size_t FrameCount     = _Analysis->_Chunk.get_sample_count();    // get_sample_count() actually returns the number of frames.
    const uint32_t ChannelCount = _Analysis->_Chunk.get_channel_count();

    if ((FrameCount == 0) || (ChannelCount == 0))
        return;

    const audio_sample * Samples = _Analysis->_Chunk.get_data();

    HRESULT hr = CreateDeviceSpecificResources(deviceContext);

    if (!SUCCEEDED(hr))
        return;

    const int SelectedChannelCount = std::popcount(_Analysis->_Chunk.get_channel_config() & _GraphSettings->_SelectedChannels);

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
    const FLOAT x1 =                ((_GraphSettings->_YAxisMode != YAxisMode::None) && _GraphSettings->_YAxisLeft)  ? YAxisWidth : 0.f;
    const FLOAT x2 = _Size.width - (((_GraphSettings->_YAxisMode != YAxisMode::None) && _GraphSettings->_YAxisRight) ? YAxisWidth : 0.f);

    // Y-axis
    if (_GraphSettings->_YAxisMode != YAxisMode::None)
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

    // X-axis. Draw them last to prevent them from being overdrawn by the horizontal grid lines.
    if (_XAxisLineStyle->IsEnabled())
    {
        FLOAT ChannelBaseline = ChannelHeight * ((_GraphSettings->_YAxisMode == YAxisMode::None) ? 0.5f : 1.0f);

        for (uint32_t i = 0; i < ChannelCount; ++i)
        {
            deviceContext->DrawLine(D2D1::Point2F(x1, ChannelBaseline), D2D1::Point2F(x2, ChannelBaseline), _XAxisLineStyle->_Brush, _XAxisLineStyle->_Thickness, nullptr);

            ChannelBaseline += ChannelHeight;
        }
    }

    // Draw the signal.
    deviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

    CComPtr<ID2D1PathGeometry> Path;

    hr = _Direct2D.Factory->CreatePathGeometry(&Path);

    if (SUCCEEDED(hr))
    {
        CComPtr<ID2D1GeometrySink> Sink;

        hr = Path->Open(&Sink);

        if (SUCCEEDED(hr))
        {
            const FLOAT ZoomFactor = (FLOAT) 1.f;

            uint32_t ChunkChannels    = _Analysis->_Chunk.get_channel_config(); // Mask containing the channels in the audio chunk.
            uint32_t SelectedChannels = _GraphSettings->_SelectedChannels;      // Mask containing the channels selected by the user.

            FLOAT ChannelBaseline = ChannelHeight * ((_GraphSettings->_YAxisMode == YAxisMode::None) ? 0.5f : 1.0f);

            for (uint32_t i = 0; i < ChannelCount; ++i)
            {
                if ((ChunkChannels & 1) == (SelectedChannels & 1))
                {
                    const FLOAT dx = (_Size.width - (YAxisWidth * YAxisCount)) / (FLOAT) FrameCount;
                    FLOAT x = _GraphSettings->_YAxisLeft ? YAxisWidth : 0.f;

                    for (t_uint32 j = 0; j < FrameCount; ++j)
                    {
                        const FLOAT Value = (FLOAT) Scaler(Samples[(j * ChannelCount) + i]);

                        const FLOAT y = ChannelBaseline - (Value * ZoomFactor * ChannelHeight);

                        if (j == 0)
                            Sink->BeginFigure(D2D1::Point2F(x, y), D2D1_FIGURE_BEGIN_HOLLOW);
                        else
                            Sink->AddLine(D2D1::Point2F(x, y));

                        x += dx;
                    }

                    Sink->EndFigure(D2D1_FIGURE_END_OPEN);

                    ChannelBaseline += ChannelHeight;
                }

                ChunkChannels    >>= 1;
                SelectedChannels >>= 1;
            }

            hr = Sink->Close();
        }

        if (SUCCEEDED(hr))
            deviceContext->DrawGeometry(Path, _SignalLineStyle->_Brush, _SignalLineStyle->_Thickness, _SignalStrokeStyle);
    }
}

/// <summary>
/// Renders the signal in X-Y mode.
/// </summary>
void oscilloscope_t::RenderXY(ID2D1DeviceContext * deviceContext) noexcept
{
    HRESULT hr = CreateDeviceSpecificResources(deviceContext);

    if (!SUCCEEDED(hr))
        return;

    const FLOAT ScaleFactor = std::min(_Size.width / 2.f, _Size.height  / 2.f);

    const auto Translate = D2D1::Matrix3x2F::Translation(_Size.width  / 2.f, _Size.height / 2.f);
    const auto Scale     = D2D1::Matrix3x2F::Scale(D2D1::SizeF(ScaleFactor, ScaleFactor));

    CComPtr<ID2D1TransformedGeometry> TransformedGeometry;

    const size_t FrameCount     = _Analysis->_Chunk.get_sample_count();    // get_sample_count() actually returns the number of frames.
    const uint32_t ChannelCount = _Analysis->_Chunk.get_channel_count();

    if ((FrameCount >= 2) || (ChannelCount == 2))
    {
        const audio_sample * Samples = _Analysis->_Chunk.get_data();

        deviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

        CComPtr<ID2D1PathGeometry> Geometry;

        hr = _Direct2D.Factory->CreatePathGeometry(&Geometry);

        if (SUCCEEDED(hr))
        {
            CComPtr<ID2D1GeometrySink> Sink;

            hr = Geometry->Open(&Sink);

            FLOAT x = (FLOAT) Samples[0];
            FLOAT y = (FLOAT) Samples[1];

            Sink->BeginFigure(D2D1::Point2F(x, y), D2D1_FIGURE_BEGIN_HOLLOW);

            for (size_t i = 2; i < FrameCount; i += 2)
            {
                x = (FLOAT) Samples[i    ];
                y = (FLOAT) Samples[i + 1];

                Sink->AddLine(D2D1::Point2F(x, y));
            }

            Sink->EndFigure(D2D1_FIGURE_END_OPEN);

            hr = Sink->Close();
        }

        if (SUCCEEDED(hr))
            hr = _Direct2D.Factory->CreateTransformedGeometry(Geometry, Scale * Translate, &TransformedGeometry);
    }

    if (SUCCEEDED(hr))
    {
        if (TransformedGeometry != nullptr)
        {
            _DeviceContext->SetTarget(_BackBuffer);

            _DeviceContext->BeginDraw();

            _DeviceContext->DrawGeometry(TransformedGeometry, _SignalLineStyle->_Brush, _SignalLineStyle->_Thickness, _SignalStrokeStyle);

            hr = _DeviceContext->EndDraw();
        }
    }

    if (SUCCEEDED(hr))
    {
        deviceContext->Clear(D2D1::ColorF(0));

        // Draw the grid.
        {
            deviceContext->SetTransform(Translate);

            deviceContext->DrawImage(_GridCommandList);

            deviceContext->SetTransform(D2D1::Matrix3x2F::Identity());
        }

        // Draw the back buffer to the window.
        {
            deviceContext->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_ADD);

            deviceContext->DrawBitmap(_BackBuffer);
        }

        // Add the phosphor afterglow effect before the next pass.
        {
            _DeviceContext->SetTarget(_FrontBuffer);
            _DeviceContext->BeginDraw();

            _GaussBlurEffect->SetInput(0, _BackBuffer);

            _DeviceContext->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_ADD);
            _DeviceContext->DrawImage(_GaussBlurEffect);

            _ColorMatrixEffect->SetInput(0, _BackBuffer);

            _DeviceContext->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_SOURCE_OVER);
            _DeviceContext->DrawImage(_ColorMatrixEffect);

            hr = _DeviceContext->EndDraw();
        }

        std::swap(_FrontBuffer, _BackBuffer);
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
void oscilloscope_t::ReleaseDeviceIndependentResources() noexcept
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
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::XAxisLine, deviceContext, _Size, L"", 1.f, &_XAxisLineStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::YAxisText, deviceContext, _Size, L"+999", 1.f, &_YAxisTextStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::YAxisLine, deviceContext, _Size, L"", 1.f, &_YAxisLineStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::HorizontalGridLine, deviceContext, _Size, L"", 1.f, &_HorizontalGridLineStyle);

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::VerticalGridLine, deviceContext, _Size, L"", 1.f, &_VerticalGridLineStyle);

    if (_State->_XYMode)
    {
        if (SUCCEEDED(hr) && (_DeviceContext == nullptr))
            hr = _Direct2D.Device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_ENABLE_MULTITHREADED_OPTIMIZATIONS, &_DeviceContext);

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

                    _DeviceContext->Clear(D2D1::ColorF(0));

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

                    _DeviceContext->Clear(D2D1::ColorF(0));

                    hr = _DeviceContext->EndDraw();

                    _DeviceContext->SetTarget(nullptr);
                }
            }
        }

        if (SUCCEEDED(hr) && (_GaussBlurEffect == nullptr))
        {
            hr = deviceContext->CreateEffect(CLSID_D2D1GaussianBlur, &_GaussBlurEffect);

            if (SUCCEEDED(hr))
            {
                const float BLUR_SIGMA = 3.0f;

                _GaussBlurEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION, BLUR_SIGMA);
                _GaussBlurEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_OPTIMIZATION, D2D1_DIRECTIONALBLUR_OPTIMIZATION_QUALITY);
                _GaussBlurEffect->SetValue(D2D1_GAUSSIANBLUR_PROP_BORDER_MODE, D2D1_BORDER_MODE_HARD);
            }
        }

        if (SUCCEEDED(hr) && (_ColorMatrixEffect == nullptr))
        {
            hr = deviceContext->CreateEffect(CLSID_D2D1ColorMatrix, &_ColorMatrixEffect);

            if (SUCCEEDED(hr))
            {
                const float DECAY_FACTOR = 0.92f; // Higher values for longer persistence

                // Color matrix for uniform decay
                #pragma warning(disable: 5246) // 'anonymous struct or union': the initialization of a subobject should be wrapped in braces
                const D2D1_MATRIX_5X4_F DecayMatrix =
                {
                    DECAY_FACTOR, 0, 0, 0,
                    0, DECAY_FACTOR, 0, 0,
                    0, 0, DECAY_FACTOR, 0,
                    0, 0, 0, 1,
                    0, 0, 0, 0
                };

                _ColorMatrixEffect->SetValue(D2D1_COLORMATRIX_PROP_COLOR_MATRIX, DecayMatrix);
            }
        }

        if (SUCCEEDED(hr) && (_GridCommandList == nullptr))
            hr = CreateGrid();
    }

#ifdef _DEBUG
    if (SUCCEEDED(hr) && (_DebugBrush == nullptr))
        deviceContext->CreateSolidColorBrush(D2D1::ColorF(1.f,0.f,0.f), &_DebugBrush);
#endif

    return hr;
}

/// <summary>
/// Releases the device specific resources.
/// </summary>
void oscilloscope_t::ReleaseDeviceSpecificResources() noexcept
{
    if (_SignalLineStyle)
    {
        _SignalLineStyle->ReleaseDeviceSpecificResources();
        _SignalLineStyle = nullptr;
    }

    if (_XAxisLineStyle)
    {
        _XAxisLineStyle->ReleaseDeviceSpecificResources();
        _XAxisLineStyle = nullptr;
    }

    if (_YAxisTextStyle)
    {
        _YAxisTextStyle->ReleaseDeviceSpecificResources();
        _YAxisTextStyle = nullptr;
    }

    if (_YAxisLineStyle)
    {
        _YAxisLineStyle->ReleaseDeviceSpecificResources();
        _YAxisLineStyle = nullptr;
    }

    if (_HorizontalGridLineStyle)
    {
        _HorizontalGridLineStyle->ReleaseDeviceSpecificResources();
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
/// 
/// </summary>
HRESULT oscilloscope_t::CreateGrid() noexcept
{
    HRESULT hr = S_OK;

    // Create a command list that will store the grid pattern.
    if (SUCCEEDED(hr))
        hr = _DeviceContext->CreateCommandList(&_GridCommandList);

    if (SUCCEEDED(hr))
    {
        CComPtr<ID2D1SolidColorBrush> Brush;

        hr = _DeviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF(1.f, 1.f, 1.f, 0.4f)), &Brush);

        CComPtr<ID2D1StrokeStyle1> StrokeStyle;

        D2D1_STROKE_STYLE_PROPERTIES1 Properties = D2D1::StrokeStyleProperties1();

        Properties.transformType = D2D1_STROKE_TRANSFORM_TYPE_FIXED; // Prevent stroke scaling

        _Direct2D.Factory->CreateStrokeStyle(Properties, nullptr, 0, &StrokeStyle);

        {
            const FLOAT ScaleFactor = std::min(_Size.width / 2.f, _Size.height  / 2.f);

            const auto Scale = D2D1::Matrix3x2F::Scale(D2D1::SizeF(ScaleFactor, ScaleFactor));

            style_t * TextStyle = nullptr;

            hr = _State->_StyleManager.GetInitializedStyle(VisualElement::XAxisText, _DeviceContext, _Size, L"+0.0", ScaleFactor, &TextStyle);

            TextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            TextStyle->SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

            _DeviceContext->SetTarget(_GridCommandList);
            _DeviceContext->BeginDraw();

            _DeviceContext->SetTransform(Scale);

            for (FLOAT x = 0.f; x <= 1.f; x += .2f)
            {
                _DeviceContext->DrawLine(D2D1::Point2F( x, -1.f), D2D1::Point2F( x, 1.f), Brush, 1.f, StrokeStyle);
                _DeviceContext->DrawLine(D2D1::Point2F(-x, -1.f), D2D1::Point2F(-x, 1.f), Brush, 1.f, StrokeStyle);

                {
                    WCHAR Text[6] = { };

                    D2D1_RECT_F r = { x - TextStyle->_Width, 0.f, x + TextStyle->_Width, TextStyle->_Height };

                    ::StringCchPrintfW(Text, _countof(Text), L"%+.1f", x);

                    _DeviceContext->DrawText(Text, (UINT32) ::wcslen(Text), TextStyle->_TextFormat, r, TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

                    if (x != 0.f)
                    {
                        r = { -x - TextStyle->_Width, 0.f, -x + TextStyle->_Width, TextStyle->_Height };

                        ::StringCchPrintfW(Text, _countof(Text), L"%+.1f", -x);

                        _DeviceContext->DrawText(Text, (UINT32) ::wcslen(Text), TextStyle->_TextFormat, r, TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
                    }
                }
            }

            TextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
            TextStyle->SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

            for (FLOAT y = 0.f; y < 1.01f; y += .2f)
            {
                _DeviceContext->DrawLine(D2D1::Point2F(-1.f,  y), D2D1::Point2F(1.f,  y), Brush, 1.f, StrokeStyle);
                _DeviceContext->DrawLine(D2D1::Point2F(-1.f, -y), D2D1::Point2F(1.f, -y), Brush, 1.f, StrokeStyle);

                if (y != 0.f)
                {
                    WCHAR Text[6] = { };

                    D2D1_RECT_F r = { 0.f, -(y - TextStyle->_Height), TextStyle->_Width, -(y + TextStyle->_Height) };

                    ::StringCchPrintfW(Text, _countof(Text), L"%+.1f", y);

                    _DeviceContext->DrawText(Text, (UINT32) ::wcslen(Text), TextStyle->_TextFormat, r, TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

                    r = { 0.f, (y - TextStyle->_Height), TextStyle->_Width, (y + TextStyle->_Height) };

                    ::StringCchPrintfW(Text, _countof(Text), L"%+.1f", -y);

                    _DeviceContext->DrawText(Text, (UINT32) ::wcslen(Text), TextStyle->_TextFormat, r, TextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
                }
            }

            _DeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());

            hr = _DeviceContext->EndDraw();

            TextStyle->ReleaseDeviceSpecificResources();
        }
    }

    if (SUCCEEDED(hr))
        hr = _GridCommandList->Close();

    return hr;
}
