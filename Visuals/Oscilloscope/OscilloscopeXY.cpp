
/** $VER: OscilloscopeXY.cpp (2025.10.14) P. Stuer - Implements an oscilloscope in X-Y mode. **/

#include <pch.h>

#include "OscilloscopeXY.h"
#include "AmplitudeScaler.h"

#include "Support.h"
#include "Log.h"

#include "DirectWrite.h"

#pragma hdrstop

/// <summary>
/// Initializes a new instance.
/// </summary>
oscilloscope_xy_t::oscilloscope_xy_t()
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
oscilloscope_xy_t::~oscilloscope_xy_t()
{
    DeleteDeviceSpecificResources();
}

/// <summary>
/// Initializes this instance.
/// </summary>
void oscilloscope_xy_t::Initialize(state_t * state, const graph_settings_t * settings, const analysis_t * analysis) noexcept
{
    _State = state;
    _GraphSettings = settings;
    _Analysis = analysis;

    DeleteDeviceSpecificResources();

    CreateDeviceIndependentResources();
}

/// <summary>
/// Moves this instance on the canvas.
/// </summary>
void oscilloscope_xy_t::Move(const D2D1_RECT_F & rect) noexcept
{
    SetBounds(rect);
}

/// <summary>
/// Resets this instance.
/// </summary>
void oscilloscope_xy_t::Reset() noexcept
{
    _IsResized = true;
}

/// <summary>
/// Recalculates parameters that are render target and size-sensitive.
/// </summary>
void oscilloscope_xy_t::Resize() noexcept
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
void oscilloscope_xy_t::Render(ID2D1DeviceContext * deviceContext) noexcept
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

    if (SUCCEEDED(hr) && (TransformedGeometry != nullptr))
    {
        _DeviceContext->SetTarget(_BackBuffer);

        _DeviceContext->BeginDraw();

        _DeviceContext->DrawGeometry(TransformedGeometry, _SignalLineStyle->_Brush, _SignalLineStyle->_Thickness, _SignalStrokeStyle);

        hr = _DeviceContext->EndDraw();
    }

    if (SUCCEEDED(hr))
    {
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

            deviceContext->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_SOURCE_OVER);
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
HRESULT oscilloscope_xy_t::CreateDeviceIndependentResources() noexcept
{
    HRESULT hr = S_OK;

    // Create a brush stroke style for the signal.
    if (SUCCEEDED(hr))
    {
        const D2D1_STROKE_STYLE_PROPERTIES Properties = D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT, D2D1_LINE_JOIN_BEVEL);

        hr = _Direct2D.Factory->CreateStrokeStyle(Properties, nullptr, 0, &_SignalStrokeStyle);
    }

    // Create a brush stroke style for the grid that remains fixed during the scaling transformation.
    if (SUCCEEDED(hr))
    {
        D2D1_STROKE_STYLE_PROPERTIES1 Properties = D2D1::StrokeStyleProperties1();

        Properties.transformType = D2D1_STROKE_TRANSFORM_TYPE_FIXED; // Prevent stroke scaling

        hr = _Direct2D.Factory->CreateStrokeStyle(Properties, nullptr, 0, &_GridStrokeStyle);
    }

    return hr;
}

/// <summary>
/// Releases the device independent resources.
/// </summary>
void oscilloscope_xy_t::DeleteDeviceIndependentResources() noexcept
{
    _GridStrokeStyle.Release();

    _SignalStrokeStyle.Release();
}

/// <summary>
/// Creates resources which are bound to a particular D3D device.
/// </summary>
HRESULT oscilloscope_xy_t::CreateDeviceSpecificResources(ID2D1DeviceContext * deviceContext) noexcept
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
        hr = CreateGridCommandList();

#ifdef _DEBUG
    if (SUCCEEDED(hr) && (_DebugBrush == nullptr))
        deviceContext->CreateSolidColorBrush(D2D1::ColorF(1.f,0.f,0.f), &_DebugBrush);
#endif

    return hr;
}

/// <summary>
/// Deletes the device specific resources.
/// </summary>
void oscilloscope_xy_t::DeleteDeviceSpecificResources() noexcept
{
    if (_SignalLineStyle)
    {
        _SignalLineStyle->DeleteDeviceSpecificResources();
        _SignalLineStyle = nullptr;
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
/// Creates a command list to render the grid and the X and Y axis labels.
/// </summary>
HRESULT oscilloscope_xy_t::CreateGridCommandList() noexcept
{
    HRESULT hr = S_OK;

    // Create a command list that will store the grid pattern.
    if (SUCCEEDED(hr))
        hr = _DeviceContext->CreateCommandList(&_GridCommandList);

    const FLOAT ScaleFactor = std::min(_Size.width / 2.f, _Size.height  / 2.f);

    const auto Scale = D2D1::Matrix3x2F::Scale(D2D1::SizeF(ScaleFactor, ScaleFactor));

    // Create a pre-scaled fonts to counter the scaling transformation.
    style_t * XAxisTextStyle = nullptr;

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::XAxisText, _DeviceContext, _Size, L"+0.0", ScaleFactor, &XAxisTextStyle);

    style_t * YAxisTextStyle = nullptr;

    if (SUCCEEDED(hr))
        hr = _State->_StyleManager.GetInitializedStyle(VisualElement::YAxisText, _DeviceContext, _Size, L"+0.0", ScaleFactor, &YAxisTextStyle);

    if (SUCCEEDED(hr))
    {
        WCHAR Text[6] = { };

        _DeviceContext->SetTarget(_GridCommandList);
        _DeviceContext->BeginDraw();

        _DeviceContext->SetTransform(Scale);

        // Draw the X-axis, Y-axis and the center label.
        {
            const D2D1_RECT_F TextRect = { 0 - XAxisTextStyle->_Width, 0.f, 0 + XAxisTextStyle->_Width, XAxisTextStyle->_Height };

            XAxisTextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            XAxisTextStyle->SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);

            _DeviceContext->DrawText(L"0.0", 3, XAxisTextStyle->_TextFormat, TextRect, XAxisTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

            _DeviceContext->DrawLine(D2D1::Point2F(-1.f,  0.f), D2D1::Point2F(1.f, 0.f), _XAxisLineStyle->_Brush, 1.f, _GridStrokeStyle);
            _DeviceContext->DrawLine(D2D1::Point2F( 0.f, -1.f), D2D1::Point2F(0.f, 1.f), _YAxisLineStyle->_Brush, 1.f, _GridStrokeStyle);
        }

        for (FLOAT x = .2f; x < 1.01f; x += .2f)
        {
            // Draw the vertical grid line.
            _DeviceContext->DrawLine(D2D1::Point2F(x, -1.f), D2D1::Point2F(x, 1.f), _VerticalGridLineStyle->_Brush, 1.f, _GridStrokeStyle);

            // Draw the negative X label.
            ::StringCchPrintfW(Text, _countof(Text), L"%-.1f", -x);

            D2D1_RECT_F TextRect = { -x - XAxisTextStyle->_Width, 0.f, -x + XAxisTextStyle->_Width, XAxisTextStyle->_Height };

            if (SUCCEEDED(hr))
                _DeviceContext->DrawText(Text, (UINT32) ::wcslen(Text), XAxisTextStyle->_TextFormat, TextRect, XAxisTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

            // Draw the vertical grid line.
            _DeviceContext->DrawLine(D2D1::Point2F(-x, -1.f), D2D1::Point2F(-x, 1.f), _VerticalGridLineStyle->_Brush, 1.f, _GridStrokeStyle);

            // Draw the positive X label.
            ::StringCchPrintfW(Text, _countof(Text), L"%+.1f", x);

            TextRect = { x - XAxisTextStyle->_Width, 0.f, x + XAxisTextStyle->_Width, XAxisTextStyle->_Height };

            if (SUCCEEDED(hr))
                _DeviceContext->DrawText(Text, (UINT32) ::wcslen(Text), XAxisTextStyle->_TextFormat, TextRect, XAxisTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }

        YAxisTextStyle->SetHorizontalAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        YAxisTextStyle->SetVerticalAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

        for (FLOAT y = .2f; y < 1.01f; y += .2f)
        {
            // Draw the horizontal grid line.
            _DeviceContext->DrawLine(D2D1::Point2F(-1.f,  y), D2D1::Point2F(1.f,  y), _HorizontalGridLineStyle->_Brush, 1.f, _GridStrokeStyle);

            // Draw the negative y label.
            ::StringCchPrintfW(Text, _countof(Text), L"%+.1f", y);

            D2D1_RECT_F TextRect = { 0.f, -(y - YAxisTextStyle->_Height), YAxisTextStyle->_Width, -(y + YAxisTextStyle->_Height) };

            if (SUCCEEDED(hr))
                _DeviceContext->DrawText(Text, (UINT32) ::wcslen(Text), YAxisTextStyle->_TextFormat, TextRect, YAxisTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);

            // Draw the horizontal grid line.
            _DeviceContext->DrawLine(D2D1::Point2F(-1.f, -y), D2D1::Point2F(1.f, -y), _HorizontalGridLineStyle->_Brush, 1.f, _GridStrokeStyle);

            // Draw the positive y label.
            ::StringCchPrintfW(Text, _countof(Text), L"%+.1f", -y);

            TextRect = { 0.f, y - YAxisTextStyle->_Height, YAxisTextStyle->_Width, y + YAxisTextStyle->_Height };

            if (SUCCEEDED(hr))
                _DeviceContext->DrawText(Text, (UINT32) ::wcslen(Text), YAxisTextStyle->_TextFormat, TextRect, YAxisTextStyle->_Brush, D2D1_DRAW_TEXT_OPTIONS_CLIP);
        }

        _DeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());

        hr = _DeviceContext->EndDraw();
    }

    if (YAxisTextStyle != nullptr)
        YAxisTextStyle->DeleteDeviceSpecificResources();

    if (XAxisTextStyle != nullptr)
        XAxisTextStyle->DeleteDeviceSpecificResources();

    if (SUCCEEDED(hr))
        hr = _GridCommandList->Close();

    return hr;
}
